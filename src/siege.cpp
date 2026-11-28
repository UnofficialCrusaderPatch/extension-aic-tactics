#include "aic_tactics/runtime.hpp"

namespace AicTactics {
namespace SHC141 {

int safePlacementFallback = 1;
int siegePaymentFallback = 0;
int siegePaymentPolicy[17];
int largeSiegeFallback = 0;
int siegeForceMaximumFallback = 0;
int largeSiegePolicy[17];
int siegeForceMaximum[17];

namespace {
typedef int (__thiscall *TileCheck)(void*, int, int, int, int);
typedef int (__thiscall *ResourceCheck)(void*, int, int, int);
typedef int (__thiscall *PlaceTent)(void*, int, int, unsigned int, int);
typedef int (__thiscall *PopUnit)(void*, int);
typedef int (__thiscall *AddUnit)(void*, unsigned int, int);
typedef void (__thiscall *AssaultBatch)(void*, int);
int activeForcePlayer;
int activeForceMaximum;
int activeForceCount;

int siegeCharacter(int player)
{
    if (player < 1 || player > 8) return 0;
    const unsigned int owner = nativeBindings.players + player * 0x39F4;
    const int character = *reinterpret_cast<const int*>(owner + 0x2300);
    return character >= 2 && character <= 17 ? character : 0;
}

bool largerForceEnabled(int player)
{
    const int character = siegeCharacter(player);
    if (!character) return false;
    const int choice = largeSiegePolicy[character - 1];
    return choice == 1 || (choice == 0 && largeSiegeFallback != 0);
}

int configuredForceMaximum(int player)
{
    const int encoded = siegeForceMaximum[siegeCharacter(player) - 1];
    return encoded ? encoded - 1 : siegeForceMaximumFallback;
}

int existingWaveEquipment(void* aic, int player, int wave)
{
    typedef int (__thiscall *CountEngines)(void*, int);
    int count = reinterpret_cast<CountEngines>(nativeBindings.siegeUnitCount)(aic, player);
    const int buildingLimit = *reinterpret_cast<const int*>(nativeBindings.buildings + 8);
    for (int id = 1; id < buildingLimit && id < static_cast<int>(nativeBindings.buildingCapacity); ++id) {
        const unsigned int address = nativeBindings.buildings + 0x14 + id * 0x32C;
        const int type = *reinterpret_cast<const short*>(address + 0xD2);
        if ((type == 29 || type == 54 || (type >= 80 && type <= 84))
            && *reinterpret_cast<const short*>(address + 0xD6) == player
            && *reinterpret_cast<const short*>(address + 0xD0) != 3
            && *reinterpret_cast<const int*>(address + 0x2AC) == wave) ++count;
    }
    return count;
}

int siegeCompositionSize(int character)
{
    const unsigned int aic = nativeBindings.aicRecords + (character - 2) * 676;
    int count = 0;
    while (count < 8 && *reinterpret_cast<const int*>(aic + 0x214 + count * 4) > 0)
        ++count;
    return count;
}

int freePlayerTribes(int player)
{
    int count = 0;
    for (int id = 1250 - player; id > 0; id -= 8) {
        const unsigned int group = nativeBindings.tribes + id * nativeBindings.tribeStride;
        if (*reinterpret_cast<const short*>(group + 0x40) == 0) ++count;
    }
    return count;
}

struct SourceCrew {
    int source;
    unsigned int uid;
    int count;
    int available;
    int units[64];
};

bool captureSourceCrew(int player, SourceCrew& crew)
{
    const unsigned int owner = nativeBindings.players + player * 0x39F4;
    crew.source = *reinterpret_cast<const short*>(owner + 0x3130);
    crew.uid = *reinterpret_cast<const unsigned int*>(owner + 0x32E4);
    if (crew.source <= 0 || crew.source >= 1250) return false;
    const unsigned int group = nativeBindings.tribes + crew.source * nativeBindings.tribeStride;
    if (*reinterpret_cast<const unsigned int*>(group + 0x34) != crew.uid
        || *reinterpret_cast<const int*>(group + 0x2C) != player) return false;
    crew.count = *reinterpret_cast<const short*>(group + 0x5C);
    if (crew.count <= 0 || crew.count > 64) return false;
    crew.available = 0;
    typedef int (__thiscall *IndexedUnit)(void*, int, int);
    for (int index = 0; index < crew.count; ++index) {
        const int unit = reinterpret_cast<IndexedUnit>(nativeBindings.siegeGetUnitForIndex)(
            reinterpret_cast<void*>(nativeBindings.tribes), crew.source, index);
        if (unit <= 0 || unit >= static_cast<int>(nativeBindings.unitCapacity)) return false;
        crew.units[index] = unit;
        const unsigned int record = nativeBindings.unitRecords + unit * 0x490;
        if (*reinterpret_cast<const short*>(record + 0x8C) == 2
            && *reinterpret_cast<const short*>(record + 0x8E) == 30
            && *reinterpret_cast<const short*>(record + 0x96) == player
            && *reinterpret_cast<const short*>(record + 0x42A) == 10
            && *reinterpret_cast<const short*>(record + 0x2A0) == 0
            && *reinterpret_cast<const int*>(record + 0x3C8) > 0
            && *reinterpret_cast<const short*>(record + 0x2D8) == crew.source
            && *reinterpret_cast<const unsigned int*>(record + 0x2E4) == crew.uid)
            ++crew.available;
    }
    return true;
}

void restoreUnselectedCrew(int player, const SourceCrew& crew)
{
    const unsigned int group = nativeBindings.tribes + crew.source * nativeBindings.tribeStride;
    if (*reinterpret_cast<const unsigned int*>(group + 0x34) != crew.uid
        || *reinterpret_cast<const int*>(group + 0x2C) != player) return;
    for (int index = 0; index < crew.count; ++index) {
        const unsigned int unit = nativeBindings.unitRecords + crew.units[index] * 0x490;
        if (*reinterpret_cast<const short*>(unit + 0x8C) != 2
            || *reinterpret_cast<const short*>(unit + 0x8E) != 30
            || *reinterpret_cast<const short*>(unit + 0x96) != player
            || *reinterpret_cast<const short*>(unit + 0x42A) != 10
            || *reinterpret_cast<const short*>(unit + 0x2D8) != 0) continue;
        reinterpret_cast<AddUnit>(nativeBindings.addUnitToTribe)(
            reinterpret_cast<void*>(nativeBindings.tribes), crew.units[index], crew.source);
    }
}

bool siegeConstruction(int command)
{
    return command == 94 || (command >= 190 && command <= 194)
        || command == 210 || command == 211 || command == 358;
}

} // namespace

int __cdecl siegeResourceAdmission(int player, int command)
{
    if (player < 1 || player > 8 || !siegeConstruction(command)) return 1;
    const unsigned int playerAddress = nativeBindings.players + player * 0x39F4;
    const int character = *reinterpret_cast<const int*>(playerAddress + 0x2300);
    if (character < 2 || character > 17) return 1;
    const int choice = siegePaymentPolicy[character - 1];
    if (choice == 2 || (choice == 0 && !siegePaymentFallback)) return 1;
    return reinterpret_cast<ResourceCheck>(nativeBindings.siegeResourceCheck)(
        reinterpret_cast<void*>(nativeBindings.gameState), command, player, 0) != 0;
}

int __cdecl siegePaymentEnabledForGoldOffset(int playerStrideOffset)
{
    if (playerStrideOffset < 0 || playerStrideOffset % 0x39F4 != 0) return 0;
    const int player = playerStrideOffset / 0x39F4;
    if (player < 1 || player > 8) return 0;
    const unsigned int playerAddress = nativeBindings.players + playerStrideOffset;
    const int character = *reinterpret_cast<const int*>(playerAddress + 0x2300);
    if (character < 2 || character > 17) return 0;
    const int choice = siegePaymentPolicy[character - 1];
    return choice == 1 || (choice == 0 && siegePaymentFallback != 0);
}

// The native assault constructor creates a new engineer tribe before finding
// a site. Its caller ignores a failed placement, leaving those engineers in
// a tribe with no order. Return them to the attack engineer tribe through the
// native membership owner; the next attack can use them normally.
int __fastcall placeSiegeTentAndRecoverEngineers(void* troopValue, void*, int tribe,
    int command, unsigned int distance, int instruction)
{
    int result = 0;
    if (tribe > 0 && tribe < 1250) {
        const unsigned int group = nativeBindings.tribes + tribe * nativeBindings.tribeStride;
        const int player = *reinterpret_cast<const int*>(group + 0x2C);
        if (!activeForcePlayer || player != activeForcePlayer
            || activeForceCount < activeForceMaximum) {
            result = reinterpret_cast<PlaceTent>(nativeBindings.siegePlaceTent)(
                troopValue, tribe, command, distance, instruction);
            if (result && player == activeForcePlayer) ++activeForceCount;
        }
    } else {
        result = reinterpret_cast<PlaceTent>(nativeBindings.siegePlaceTent)(
            troopValue, tribe, command, distance, instruction);
    }
    if (result || tribe <= 0 || tribe >= 1250) return result;
    const unsigned int group = nativeBindings.tribes + tribe * nativeBindings.tribeStride;
    const int player = *reinterpret_cast<const int*>(group + 0x2C);
    if (player < 1 || player > 8) return result;
    const int offset = player * 0x39F4;
    if (player != activeForcePlayer && !siegePlacementPolicyEnabled(player)
        && !siegePaymentEnabledForGoldOffset(offset)) return result;
    const unsigned int owner = nativeBindings.players + offset;
    const int source = *reinterpret_cast<const short*>(owner + 0x3130);
    const unsigned int sourceUID = *reinterpret_cast<const unsigned int*>(owner + 0x32E4);
    if (source <= 0 || source >= 1250 || source == tribe) return result;
    const unsigned int sourceGroup = nativeBindings.tribes + source * nativeBindings.tribeStride;
    if (*reinterpret_cast<const unsigned int*>(sourceGroup + 0x34) != sourceUID
        || *reinterpret_cast<const int*>(sourceGroup + 0x2C) != player) return result;
    const int size = *reinterpret_cast<const short*>(group + 0x5C);
    if (size <= 0 || size > 16) return result;
    for (int index = 0; index < size; ++index) {
        const int unit = reinterpret_cast<PopUnit>(nativeBindings.popUnitFromTribe)(
            reinterpret_cast<void*>(nativeBindings.tribes), tribe);
        if (unit <= 0 || unit >= static_cast<int>(nativeBindings.unitCapacity)) break;
        const unsigned int record = nativeBindings.unitRecords + unit * 0x490;
        if (*reinterpret_cast<const short*>(record + 0x8E) != 30
            || *reinterpret_cast<const short*>(record + 0x96) != player
            || *reinterpret_cast<const short*>(record + 0x42A) != 10) {
            reinterpret_cast<AddUnit>(nativeBindings.addUnitToTribe)(
                reinterpret_cast<void*>(nativeBindings.tribes), unit, tribe);
            break;
        }
        reinterpret_cast<AddUnit>(nativeBindings.addUnitToTribe)(
            reinterpret_cast<void*>(nativeBindings.tribes), unit, source);
    }
    return result;
}

// The game's assault batch consumes the eight authored composition entries
// once at the state-2 rally transition. Repeat that same native owner only
// while it constructs equipment and the opted-in AI has wave engineers and
// native tribe capacity left. No array bound, pool or placement owner changes.
void __fastcall buildLargerSiegeForce(void* aic, void*, int player)
{
    const AssaultBatch original = reinterpret_cast<AssaultBatch>(nativeBindings.siegeAssaultBatch);
    if (!largerForceEnabled(player)) { original(aic, player); return; }
    const int configuredMaximum = configuredForceMaximum(player);
    const int character = siegeCharacter(player);
    const int composition = siegeCompositionSize(character);
    if (!composition) { original(aic, player); return; }
    const unsigned int owner = nativeBindings.players + player * 0x39F4;
    const int wave = *reinterpret_cast<const int*>(owner + 0x3924);
    if (wave <= 0) { original(aic, player); return; }
    const int existing = existingWaveEquipment(aic, player, wave);
    if (activeForcePlayer || freePlayerTribes(player) == 0) return;
    SourceCrew crew;
    if (!captureSourceCrew(player, crew)) return;
    const int availableEngineers = crew.available;
    // One engine needs at least one of the currently unassigned attack
    // engineers. The native batch and crew owner decide the actual mix.
    int maximum = existing + availableEngineers;
    if (configuredMaximum > 0 && maximum > configuredMaximum) maximum = configuredMaximum;
    if (existing >= maximum) return;
    activeForcePlayer = player;
    activeForceMaximum = maximum;
    activeForceCount = existing;
    for (int batch = 0; batch < availableEngineers && activeForceCount < maximum; ++batch) {
        // Native placement can use the remaining slots for a partial batch.
        if (freePlayerTribes(player) == 0) break;
        if (batch > 0 && !captureSourceCrew(player, crew)) break;
        const int before = activeForceCount;
        original(aic, player);
        restoreUnselectedCrew(player, crew);
        if (activeForceCount == before) break;
    }
    activeForcePlayer = 0;
    activeForceMaximum = 0;
    activeForceCount = 0;
}

int __cdecl siegePlacementPolicyEnabled(int player)
{
    if (player < 1 || player > 8) return false;
    const unsigned int playerAddress = nativeBindings.players + player * 0x39F4;
    const int character = *reinterpret_cast<const int*>(playerAddress + 0x2300);
    if (character < 2 || character > 17) return false;
    const int index = character - 1;
    if (largerForceEnabled(player)
        || siegePaymentPolicy[index] == 1
        || (siegePaymentPolicy[index] == 0 && siegePaymentFallback)
        || siegeHarassPolicy[index] == 1
        || (siegeHarassPolicy[index] == 0 && siegeHarassFallback)) return true;
    const int choice = configurations[character - 1].safeSiegePlacement;
    return choice == 1 || (choice == 0 && safePlacementFallback != 0);
}

// The attack-angle finder checks path access and tent-point reservations, but
// does not check the 3x3 construction footprint before returning its point.
// Keep its BFS and final native placeBuilding admission; this predicate only
// rejects a candidate that is already blocked by a unit or building.
int __cdecl siegeTentCandidateAllowed(int tribe, int point)
{
    // The hook is shared by every personality once enabled. Unknown native
    // records must keep the original finder's decision.
    if (tribe <= 0 || tribe >= 1250) return 1;
    const unsigned int group = nativeBindings.tribes
        + tribe * nativeBindings.tribeStride;
    const int player = *reinterpret_cast<const int*>(group + 0x2C);
    if (!siegePlacementPolicyEnabled(player)) return 1;
    if (point <= 0 || point >= 216) return 0;

    const unsigned int candidate = nativeBindings.siegeTentPointX + point * 32;
    const int x = *reinterpret_cast<const int*>(candidate);
    const int y = *reinterpret_cast<const int*>(candidate + 4);
    if (x < 1 || x >= 399 || y < 1 || y >= 399) return 0;

    const unsigned char* tileMap = reinterpret_cast<const unsigned char*>(nativeBindings.siegeTileMap);
    for (int dy = -1; dy <= 1; ++dy) {
        const int row = *reinterpret_cast<const int*>(nativeBindings.mapRows + (y + dy) * 12);
        for (int dx = -1; dx <= 1; ++dx) {
            const int tile = row + x + dx;
            if (tile < 0 || tile >= 80400) return 0;
            if (*reinterpret_cast<const short*>(tileMap + nativeBindings.siegeTileOccupancyOffset + tile * 2)
                || *reinterpret_cast<const short*>(tileMap + 0x2029B0 + tile * 2)) return 0;
        }
    }
    return 1;
}

// The attack-angle caller records the reservation before placeBuilding and
// otherwise orders construction even when admission fails. Roll back only its
// writes; the original finder and native construction remain the owners.
int __cdecl failedSiegeTent(int tribe, int point)
{
    if (tribe <= 0 || tribe >= 1250 || point <= 0 || point >= 216) return 0;
    unsigned char* const group = reinterpret_cast<unsigned char*>(nativeBindings.tribes
        + tribe * nativeBindings.tribeStride);
    const int player = *reinterpret_cast<const int*>(group + 0x2C);
    if (!siegePlacementPolicyEnabled(player)
        || !*reinterpret_cast<const int*>(nativeBindings.siegePlacementFail)) return 0;

    const unsigned int uid = *reinterpret_cast<const unsigned int*>(group + 0x34);
    unsigned char* const current = reinterpret_cast<unsigned char*>(
        nativeBindings.siegeTentPointX + point * 32);
    if (*reinterpret_cast<const int*>(current + 0x10) == tribe
        && *reinterpret_cast<const unsigned int*>(current + 0x14) == uid) {
        *reinterpret_cast<int*>(current + 0x10) = 0;
        *reinterpret_cast<unsigned int*>(current + 0x14) = 0;
        *reinterpret_cast<int*>(current + 0x18) = 0;
    }

    const unsigned int index = nativeBindings.siegeTribeIndexOffset;
    const unsigned short previous = *reinterpret_cast<const unsigned short*>(group + index + 2);
    *reinterpret_cast<unsigned short*>(group + index) = previous < 216 ? previous : 0;
    if (previous > 0 && previous < 216) {
        unsigned char* const old = reinterpret_cast<unsigned char*>(
            nativeBindings.siegeTentPointX + previous * 32);
        if (*reinterpret_cast<const int*>(old + 0x10) == 0
            && *reinterpret_cast<const unsigned int*>(old + 0x14) == 0) {
            *reinterpret_cast<int*>(old + 0x10) = tribe;
            *reinterpret_cast<unsigned int*>(old + 0x14) = uid;
        }
    }
    return 1;
}

// Replaces only the native footprint tile check call. All terrain, ownership,
// access and placement decisions still run through the original checker.
int __fastcall checkedSiegeTile(void* tileMap, void*, int tile, int player, int command, int flags)
{
    if (siegeConstruction(command) && tile >= 0 && tile < 160000
        && siegePlacementPolicyEnabled(player)) {
        const short unit = *reinterpret_cast<const short*>(
            reinterpret_cast<const unsigned char*>(tileMap)
                + nativeBindings.siegeTileOccupancyOffset + tile * 2);
        if (unit > 0 && unit < static_cast<int>(nativeBindings.unitCapacity)) {
            const unsigned int record = nativeBindings.unitRecords + unit * 0x490;
            const int owner = *reinterpret_cast<const short*>(record + 0x96);
            const int dying = *reinterpret_cast<const short*>(record + 0x2A0);
            // The native placement path can delete any friendly unit on the
            // footprint, including a waiting engineer or an existing engine.
            if (owner == player && dying == 0) return 1;
        }
    }
    return reinterpret_cast<TileCheck>(nativeBindings.originalSiegeTileCheck)(
        tileMap, tile, player, command, flags);
}

} // namespace SHC141
} // namespace AicTactics
