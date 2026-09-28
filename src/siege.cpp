#include "aic_tactics/runtime.hpp"

namespace AicTactics {
namespace SHC141 {

int safePlacementFallback = 1;
int siegePaymentFallback = 0;
int siegePaymentPolicy[17];

namespace {
typedef int (__thiscall *TileCheck)(void*, int, int, int, int);
typedef int (__thiscall *ResourceCheck)(void*, int, int, int);
typedef int (__thiscall *PlaceTent)(void*, int, int, unsigned int, int);
typedef int (__thiscall *PopUnit)(void*, int);
typedef int (__thiscall *AddUnit)(void*, unsigned int, int);

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
    const int result = reinterpret_cast<PlaceTent>(nativeBindings.siegePlaceTent)(
        troopValue, tribe, command, distance, instruction);
    if (result || tribe <= 0 || tribe >= 1250) return result;
    const unsigned int group = nativeBindings.tribes + tribe * nativeBindings.tribeStride;
    const int player = *reinterpret_cast<const int*>(group + 0x2C);
    if (player < 1 || player > 8) return result;
    const int offset = player * 0x39F4;
    if (!siegePlacementPolicyEnabled(player)
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

int __cdecl siegePlacementPolicyEnabled(int player)
{
    if (player < 1 || player > 8) return false;
    const unsigned int playerAddress = nativeBindings.players + player * 0x39F4;
    const int character = *reinterpret_cast<const int*>(playerAddress + 0x2300);
    if (character < 2 || character > 17) return false;
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
