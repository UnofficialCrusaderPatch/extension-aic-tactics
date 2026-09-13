#include "aic_tactics/runtime.hpp"
#include "aic_tactics/random.hpp"
#include "aic_tactics/shc141_recruitment.hpp"
#include "aic_tactics/shc141_groups.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

CharacterConfiguration configurations[17];
RecruitmentObservation observations[9];
int configurationLocked;
int* legacyWallCounts;
int defenseTypeCounts[9][80];
unsigned int defenseCensusTick;
int defenseCensusValid;
NativeBindings nativeBindings;

int nativeRandom(void*) {
    const int result = *reinterpret_cast<unsigned short*>(nativeBindings.rngValue);
    typedef int (__thiscall *Next)(void*);
    reinterpret_cast<Next>(nativeBindings.rngNext)(reinterpret_cast<void*>(nativeBindings.rngState));
    return result;
}

namespace {
const unsigned int PlayerStride = 0x39F4;
const unsigned int& Units = nativeBindings.units;
const unsigned int& Tribes = nativeBindings.tribes;
template<class T> T& memory(unsigned int address) { return *reinterpret_cast<T*>(address); }
int& playerValue(int player, unsigned int address) { return memory<int>(address + player * PlayerStride); }
int aicValue(void* aic, int character, int offset) {
    return *reinterpret_cast<int*>(static_cast<unsigned char*>(aic) + (character - 1) * 0x2A4 + offset);
}
typedef void (__thiscall *PlayerAction)(void*, int);
typedef int (__thiscall *PlayerQuery)(void*, int);
typedef int (__thiscall *TwoIntQuery)(void*, int, int);
typedef void (__thiscall *TwoIntAction)(void*, int, int);

bool active(int player) {
    if (player < 1 || player > 8) return false;
    const int character = playerValue(player, (nativeBindings.players + 0x2300));
    return character >= 2 && character <= 17
        && configurations[character - 1].recruitment.mode == WeightedRoles;
}

int buildingFor(int player, int unitType) {
    int building, type;
    if (unitType == 29 || unitType == 30) {
        building = playerValue(player, (nativeBindings.players + 0x1FC)); type = 24;
    } else if (unitType == 5) {
        building = playerValue(player, (nativeBindings.players + 0x224)); type = 25;
    } else if (unitType == 37) {
        building = reinterpret_cast<TwoIntQuery>(nativeBindings.findRecruitmentBuilding)(reinterpret_cast<void*>(nativeBindings.buildings), player, 38);
        type = 38;
    } else if (unitType >= 70 && unitType <= 76) {
        building = playerValue(player, (nativeBindings.players + 0x24C)); type = 8;
    } else if (unitType >= 22 && unitType <= 28) {
        building = playerValue(player, (nativeBindings.players + 0x15C)); type = 9;
    } else return 0;
    if (building <= 0 || building >= 2000 || building >= memory<int>((nativeBindings.buildings + 8))) return 0;
    const unsigned int address = (nativeBindings.buildings + 0x14) + building * 0x32C;
    const short state = memory<short>(address + 0xD0);
    // The native building lookup excludes unused/removing records. Acquisition
    // check-only does not inspect its building argument, so validate the cache here.
    if (state == 0 || state == 3 || memory<short>(address + 0xD2) != type
        || memory<short>(address + 0xD6) != player) return 0;
    return building;
}

RecruitmentServices services() {
    RecruitmentServices result;
    result.units = reinterpret_cast<void*>(Units);
    result.european = reinterpret_cast<RecruitFunction>(nativeBindings.recruitEuropean);
    result.nonEuropean = reinterpret_cast<RecruitFunction>(nativeBindings.recruitNonEuropean);
    result.failureReason = reinterpret_cast<int*>(Units + 0x60C);
    result.requiredResource = reinterpret_cast<int*>(Units + 0x610);
    for (int i = 0; i < 8; ++i)
        result.availableHorses[i] = reinterpret_cast<short*>((nativeBindings.players + 0x2252) + (i + 1) * PlayerStride);
    return result;
}

struct Candidate {
    int unitType;
    int building;
    int behaviour;
    int cursor;
};

struct Probe {
    RecruitmentServices native;
    RecruitmentObservation* observation;
    int role;
    int missingResource[4];
    int freeGroups;
    void* aic;
    int character;
    int defenseMaximum;
    int recruitedWalls;
    const int* recruitedTypes;
};

int moatVacancies(void* aic, int character, int player) {
    const int maximum = aicValue(aic, character, 0x15C);
    if (maximum <= 0) return 0;
    const int group = memory<short>((nativeBindings.players + 0x3120) + player * PlayerStride);
    if (group < 0 || group >= 1250) return 0;
    int size = 0;
    if (group && memory<unsigned int>(Tribes + 0x34 + group * nativeBindings.tribeStride)
        == memory<unsigned int>((nativeBindings.players + 0x32C4) + player * PlayerStride)) {
        if (memory<int>(Tribes + 0x2C + group * nativeBindings.tribeStride) != player
            || memory<short>(Tribes + 0x40 + group * nativeBindings.tribeStride) == 0) return 0;
        size = memory<short>(Tribes + 0x5C + group * nativeBindings.tribeStride);
        if (size < 0) return 0;
    }
    if (size >= maximum) return 0;
    // Existing moat registry owner; at most once per recruitment opportunity.
    if (reinterpret_cast<PlayerQuery>(nativeBindings.moatVacancies)(reinterpret_cast<void*>(nativeBindings.moat), player) <= 0) return 0;
    return maximum - size;
}

bool groupRangeAvailable(int player, int first, int count, int freeGroups, bool smallest) {
    if (first < 0 || count < 1 || first + count > 200) return false;
    bool room = false;
    for (int slot = first; slot < first + count; ++slot) {
        const int group = memory<short>((nativeBindings.players + 0x310C) + player * PlayerStride + slot * 2);
        if (group < 0 || group >= 1250) return false;
        if (group == 0 || memory<unsigned int>(Tribes + 0x34 + group * nativeBindings.tribeStride)
                != memory<unsigned int>((nativeBindings.players + 0x329C) + player * PlayerStride + slot * 4))
            return freeGroups > 0; // Native allocates the first missing slot.
        if (memory<int>(Tribes + 0x2C + group * nativeBindings.tribeStride) != player
            || memory<short>(Tribes + 0x40 + group * nativeBindings.tribeStride) == 0) return false;
        const int size = memory<short>(Tribes + 0x5C + group * nativeBindings.tribeStride);
        if (size >= 0 && size < (smallest ? 1000 : 3200)) room = true;
    }
    return room;
}

bool destinationAvailable(const Probe& probe, int player, int unitType, int behaviour) {
    int first = 0, count = 1;
    bool smallest = false;
    if (probe.role == DefenseRole && behaviour == 5) first = 10;
    else if (probe.role == SortieRole) first = behaviour + 160;
    else if (probe.role == AttackRole) {
        if (behaviour < 10 || behaviour > 20) return false;
        first = memory<int>(nativeBindings.attackGroupSlots + (behaviour - 10) * 4);
        int offset = 0, maximum = 1;
        if (first == 15) { offset = 0x264; maximum = 2; }
        if (first == 186) { offset = 0x270; maximum = 3; }
        if (first == 190) { offset = 0x284; maximum = 2; }
        if (first == 192) { offset = 0x29C; maximum = 8; }
        if (offset) count = aicValue(probe.aic, probe.character, offset);
        if (count < 1) count = 1; // Original attack grouping treats <=1 as one.
        if (count > maximum) return false;
        smallest = count > 1;
    } else if (probe.role == RaidRole) {
        int index = 0;
        while (index < 20 && memory<int>(nativeBindings.raidTypes + index * 4) != unitType) ++index;
        if (index == 20) return false;
        first = index < 1 ? 180 : index < 2 ? 181 : index < 7 ? 182
            : index < 12 ? 183 : index < 15 ? 184 : 185;
    } else {
        const int walls = (legacyWallCounts ? legacyWallCounts[player] : playerValue(player, (nativeBindings.players + 0x30E8)))
            + probe.recruitedWalls;
        if (walls >= aicValue(probe.aic, probe.character, 0x180)) {
            first = 170;
            count = aicValue(probe.aic, probe.character, 0x174);
            if (count < 1 || count > 10) return false;
            smallest = true;
        } else {
            int index = 0;
            if (probe.character != 7 && playerValue(player, (nativeBindings.players + 0x3864)) == 1
                && memory<short>((nativeBindings.players + 0x386E) + player * PlayerStride) > 0
                && playerValue(player, (nativeBindings.players + 0x30C0)) > 0
                && (unitType == 22 || unitType == 23 || unitType == 70 || unitType == 72 || unitType == 76)) index = 13;
            else {
                while (index < 20 && memory<int>(nativeBindings.defenseTypes + index * 4) != unitType) ++index;
                if (index == 20) index = 0;
            }
            count = playerValue(player, (nativeBindings.players + 0x308C) + index * 4);
            if (count < 1) {
                first = 1;
                for (int i = 0; i < 7; ++i)
                    if (memory<int>(nativeBindings.specialDefenders + i * 4) == unitType) first = 0;
                count = 1;
            } else {
                first = memory<int>(nativeBindings.defenseSlots + index * 4);
                if (index == 8 || index == 10 || index == 17) {
                    const int maximum = aicValue(probe.aic, probe.character, 0x114);
                    if (count > maximum) count = maximum;
                }
                if (count > (first == 160 ? 6 : 10)) return false;
                smallest = true;
            }
        }
    }
    return groupRangeAvailable(player, first, count, probe.freeGroups, smallest);
}

bool eligible(Probe& probe, int player, int unitType,
    int behaviour, int cursor, Candidate& candidate) {
    if (probe.role == AttackRole && !reserveRoleAvailable(probe.aic, player, behaviour)) return false;
    const int building = buildingFor(player, unitType);
    RecruitmentAvailability result;
    probe.observation->probeTypes[probe.role] = unitType;
    if (!destinationAvailable(probe, player, unitType, behaviour)) {
        probe.observation->probeReasons[probe.role] = -3;
        return false;
    }
    const bool valid = queryRecruitment(probe.native, player, unitType, building, result);
    probe.observation->probeReasons[probe.role] = valid ? result.failureReason : -1;
    if (valid && result.failureReason == 2 && probe.missingResource[probe.role] == 0)
        probe.missingResource[probe.role] = result.requiredResource;
    if (!valid || !result.eligible) return false;
    candidate.unitType = unitType;
    candidate.building = building;
    candidate.behaviour = behaviour;
    candidate.cursor = cursor;
    return true;
}

bool rosterCandidate(Probe& native, void* aic, int character,
    int player, int offset, int cursor, int count, int behaviour, Candidate& result) {
    int length = 0;
    while (length < count && aicValue(aic, character, offset + length * 4) != 0) ++length;
    if (!length) return false;
    if (cursor < 0 || cursor >= length) cursor = 0;
    for (int i = 0; i < length; ++i) {
        const int index = (cursor + i) % length;
        const int unitType = aicValue(aic, character, offset + index * 4);
        if (native.role == DefenseRole && configurations[character - 1].defenseComposition == PreserveSlots) {
            int roster[8];
            for (int slot = 0; slot < 8; ++slot) roster[slot] = aicValue(aic, character, offset + slot * 4);
            if (unitType < 1 || unitType >= 80 || defenseTypeCounts[player][unitType] + native.recruitedTypes[unitType]
                >= defenseTypeQuota(roster, native.defenseMaximum, unitType)) continue;
        }
        if (eligible(native, player, unitType,
            behaviour, index + 1, result)) return true;
    }
    return false;
}

int attackCandidates(Probe& native, void* aic, int character,
    int player, const int* recruited, Candidate* results) {
    // Native attack subroles in their existing priority-table order. Type lookup
    // for these roles does not advance the main-roster cursor or consume RNG.
    const int maximumOffsets[10] = {0x23C, 0x244, 0x250, 0x24C, 0x254,
        0x258, 0x260, 0x26C, 0x278, 0x280};
    int count = 0;
    bool openSubrole = false;
    for (int index = 0; index < 10; ++index) {
        int maximum = aicValue(aic, character, maximumOffsets[index]);
        if (index == 0) {
            const int wave = playerValue(player, (nativeBindings.players + 0x3924));
            const int waveMaximum = wave > 0x1FFFFFFF ? 0x7FFFFFFF : wave > 0 ? wave * 4 : 0;
            if (waveMaximum < maximum) maximum = waveMaximum;
        }
        if (maximum <= static_cast<__int64>(playerValue(player, (nativeBindings.players + 0x2BA8) + index * 4)) + recruited[index]) continue;
        if (index == 1) {
            const int target = playerValue(player, (nativeBindings.players + 0x2BD8));
            if (target < 1 || target > 8 || playerValue(target, (nativeBindings.players + 0x396C)) <= 5) continue;
        }
        openSubrole = true;
        const int behaviour = index + 10;
        const int unitType = reinterpret_cast<TwoIntQuery>(nativeBindings.attackRecruitType)(aic, player, behaviour);
        if (eligible(native, player, unitType, behaviour, -1, results[count])) ++count;
    }
    if ((!openSubrole || static_cast<__int64>(playerValue(player, (nativeBindings.players + 0x2BD0))) + recruited[10] < aicValue(aic, character, 0x298))
        && rosterCandidate(native, aic, character, player, 0x288,
            playerValue(player, (nativeBindings.players + 0x3108)), 4, 20, results[count])) ++count;
    return count;
}

void includeEquipmentType(bool* types, int unitType) {
    if (unitType >= 22 && unitType <= 28) types[unitType - 22] = true;
}

void includeEquipmentRoster(bool* types, void* aic, int character, int offset, int count) {
    for (int slot = 0; slot < count; ++slot) {
        const int type = aicValue(aic, character, offset + slot * 4);
        if (type == 0) break;
        includeEquipmentType(types, type);
    }
}

bool equipmentSurplus(const Probe& probe, int player, bool compositionReady,
    int recruitedDefense, int ranged, int melee, int missingMoat) {
    const int character = probe.character;
    void* const aic = probe.aic;
    int recipe[7][26] = {{0}};
    // Use the acquisition owner's table, including armor and horse requirements.
    // Index zero in this local accounting denotes horses, never a native resource.
    for (int type = 0; type < 7; ++type) {
        const unsigned int row = nativeBindings.equipmentRecipes + type * 16;
        const int offsets[3] = {0, 4, 8};
        for (int item = 0; item < 3; ++item) {
            const int resource = memory<int>(row + offsets[item]);
            if (resource < 0 || resource >= 26) return false;
            if (resource) ++recipe[type][resource];
        }
        if (memory<int>(row + 12) == -1) recipe[type][0] = 1;
        else if (memory<int>(row + 12) != 0) return false;
    }
    __int64 reserve[26] = {0};
    bool defenseTypes[7] = {false}, types[7] = {false};
    includeEquipmentRoster(defenseTypes, aic, character, 0x184, 8);
    const __int64 deficit = static_cast<__int64>(probe.defenseMaximum)
        - playerValue(player, (nativeBindings.players + 0x30E8)) - recruitedDefense;
    if (configurations[character - 1].defenseComposition == PreserveSlots) {
        if (!compositionReady) return false;
        int roster[8];
        for (int slot = 0; slot < 8; ++slot) roster[slot] = aicValue(aic, character, 0x184 + slot * 4);
        for (int type = 0; type < 7; ++type) {
            const __int64 missing = static_cast<__int64>(defenseTypeQuota(roster, probe.defenseMaximum, type + 22))
                - defenseTypeCounts[player][type + 22] - probe.recruitedTypes[type + 22];
            if (missing > 0 && deficit > 0) {
                const __int64 needed = missing < deficit ? missing : deficit;
                for (int resource = 0; resource < 26; ++resource) reserve[resource] += needed * recipe[type][resource];
            }
        }
    } else if (deficit > 0) {
        // Native composition may fill all vacant seats with any available roster
        // type. Reserve the worst per-seat use of each resource, without guessing
        // which unit the native cursor will be able to acquire later.
        for (int resource = 0; resource < 26; ++resource) {
            int largest = 0;
            for (int type = 0; type < 7; ++type)
                if (defenseTypes[type] && recipe[type][resource] > largest) largest = recipe[type][resource];
            reserve[resource] = deficit * largest;
        }
    }
    for (int sortie = 0; sortie < 2; ++sortie) {
        const int type = aicValue(aic, character, sortie ? 0x158 : 0x150);
        includeEquipmentType(types, type);
        const __int64 maximum = static_cast<__int64>(aicValue(aic, character, sortie ? 0x154 : 0x14C));
        const __int64 missing = maximum + (sortie ? 0 : playerValue(player, (nativeBindings.players + 0x3940)) / 2)
            - playerValue(player, sortie ? (nativeBindings.players + 0x3944) : (nativeBindings.players + 0x393C)) - (sortie ? melee : ranged);
        if (type >= 22 && type <= 28 && maximum >= 0 && missing > 0)
            for (int resource = 0; resource < 26; ++resource) reserve[resource] += missing * recipe[type - 22][resource];
    }
    const int moatType = aicValue(aic, character, 0x160);
    if (missingMoat > 0 && moatType >= 22 && moatType <= 28) {
        includeEquipmentType(types, moatType);
        for (int resource = 0; resource < 26; ++resource)
            reserve[resource] += static_cast<__int64>(missingMoat) * recipe[moatType - 22][resource];
    }
    includeEquipmentRoster(types, aic, character, 0x184, 8);
    includeEquipmentRoster(types, aic, character, 0x1AC, 8);
    includeEquipmentRoster(types, aic, character, 0x288, 4);
    for (int role = 10; role < 20; ++role)
        includeEquipmentType(types, reinterpret_cast<TwoIntQuery>(nativeBindings.attackRecruitType)(aic, player, role));
    for (int type = 0; type < 7; ++type) {
        if (!types[type]) continue;
        bool spare = true, consumes = false;
        for (int resource = 1; resource < 26; ++resource) {
            if (recipe[type][resource] == 0) continue;
            consumes = true;
            if (static_cast<__int64>(playerValue(player, (nativeBindings.players + 0x4D0) + resource * 4))
                    < reserve[resource] + recipe[type][resource]) spare = false;
        }
        if (!spare || !consumes) continue;
        RecruitmentAvailability available;
        if (queryRecruitment(probe.native, player, type + 22, buildingFor(player, type + 22), available)
            && available.eligible && (!recipe[type][0] || available.availableHorses > reserve[0])) return true;
    }
    return false;
}

void assign(void* aic, int player, int character, int role, const Candidate& candidate, int unit, int recruitedWalls) {
    if (role == DefenseRole) {
        if (candidate.behaviour == 5) {
            reinterpret_cast<PlayerAction>(nativeBindings.assignMoatDigger)(aic, unit);
            return;
        }
        playerValue(player, (nativeBindings.players + 0x3100)) = candidate.cursor;
        const int walls = (legacyWallCounts ? legacyWallCounts[player] : playerValue(player, (nativeBindings.players + 0x30E8)))
            + recruitedWalls;
        reinterpret_cast<PlayerAction>(walls < aicValue(aic, character, 0x180) ? nativeBindings.wallDefense : nativeBindings.patrolDefense)(aic, unit);
    } else if (role == RaidRole) {
        playerValue(player, (nativeBindings.players + 0x3104)) = candidate.cursor;
        reinterpret_cast<PlayerAction>(nativeBindings.assignRaider)(aic, unit);
    } else if (role == AttackRole) {
        if (candidate.cursor >= 0) playerValue(player, (nativeBindings.players + 0x3108)) = candidate.cursor;
        reinterpret_cast<TwoIntAction>(nativeBindings.assignAttacker)(aic, unit, candidate.behaviour);
    } else {
        memory<short>(nativeBindings.unitRecords + 0x42A + unit * 0x490) = static_cast<short>(candidate.behaviour);
        const int group = reinterpret_cast<TwoIntQuery>(nativeBindings.findSortieGroup)(aic, player, candidate.behaviour + 160);
        reinterpret_cast<TwoIntAction>(nativeBindings.addUnitToTribe)(reinterpret_cast<void*>(Tribes), unit, group);
    }
}
} // namespace

void __cdecl invalidateDefenseCensus() {
    defenseCensusValid = false;
}

void __cdecl resetDefenseCensus() {
    std::memset(defenseTypeCounts, 0, sizeof(defenseTypeCounts));
    defenseCensusTick = memory<unsigned int>(nativeBindings.gameTick);
    defenseCensusValid = true;
}

void __cdecl countDefenseUnit(int player, int unitType) {
    if (player < 1 || player > 8 || unitType < 1 || unitType >= 80 || !active(player)) return;
    const int character = playerValue(player, (nativeBindings.players + 0x2300));
    if (configurations[character - 1].defenseComposition == PreserveSlots)
        ++defenseTypeCounts[player][unitType];
}

void __fastcall rangedSortie(void* aic, void*, int player) {
    if (!active(player)) reinterpret_cast<PlayerAction>(nativeBindings.rangedSortieNative)(aic, player);
}
void __fastcall meleeSortie(void* aic, void*, int player) {
    if (!active(player)) reinterpret_cast<PlayerAction>(nativeBindings.meleeSortieNative)(aic, player);
}

int __cdecl recruitOpportunity(void* aic, int player, int attempts) {
    configurationLocked = 1;
    if (!active(player)) return 0;
    const int character = playerValue(player, (nativeBindings.players + 0x2300));
    const int strength = playerValue(player, (nativeBindings.players + 0x30F4));
    if (strength < 0 || strength > 2 || attempts < 1 || attempts > 4) return 1;
    const CharacterConfiguration& configuration = configurations[character - 1];
    const RecruitmentServices native = services();
    RecruitmentObservation& observation = observations[player];
    int recruited[4] = {0, 0, 0, 0};
    int recruitedAttack[11] = {0};
    int ranged = 0, melee = 0;
    int recruitedWalls = 0;
    int missingMoat = -1;
    int recruitedTypes[80] = {0};
    bool compositionReady = true;
    if (configuration.defenseComposition == PreserveSlots) {
        int censusTotal = 0;
        for (int type = 1; type < 80; ++type) censusTotal += defenseTypeCounts[player][type];
        compositionReady = defenseCensusValid && memory<unsigned int>(nativeBindings.gameTick) - defenseCensusTick <= 1
            && censusTotal == playerValue(player, (nativeBindings.players + 0x30E8));
    }
    for (int attempt = 0; attempt < attempts; ++attempt) {
        ++observation.sequence;
        observation.tick = memory<int>(nativeBindings.gameTick);
        observation.character = character - 1;
        observation.strength = strength;
        observation.attempts = attempts;
        observation.role = NoRole;
        observation.rngSamples = 0;
        observation.eligibleRoles = 0;
        observation.aicAddress = reinterpret_cast<unsigned int>(aic);
        observation.purchaseResource = observation.purchaseAmount = 0;
        Probe probe;
        probe.native = native;
        probe.observation = &observation;
        probe.aic = aic;
        probe.character = character;
        probe.recruitedWalls = recruitedWalls;
        probe.recruitedTypes = recruitedTypes;
        for (int r = 0; r < 4; ++r) {
            probe.missingResource[r] = 0;
            observation.probeReasons[r] = -2;
            observation.probeTypes[r] = 0;
        }
        Candidate candidates[4];
        Candidate defenders[2];
        int defenderCount = 0;
        Candidate attackers[11];
        int attackerCount = 0;
        unsigned int mask = 0;
        __int64 defenseMaximum = aicValue(aic, character, 0x170);
        if (memory<int>(nativeBindings.scenarioMode) == 3 && memory<int>(nativeBindings.scenarioCustom) == 1 && memory<int>(nativeBindings.scenarioMission) == 2)
            defenseMaximum = defenseMaximum * 4 / 3;
        if (playerValue(player, (nativeBindings.players + 0x38F0)) > 0) defenseMaximum *= 4;
        probe.defenseMaximum = defenseMaximum <= 0 ? 0
            : defenseMaximum > 0x7FFFFFFF ? 0x7FFFFFFF : static_cast<int>(defenseMaximum);
        const bool defenseIncomplete = static_cast<__int64>(playerValue(player, (nativeBindings.players + 0x30E8))) + recruited[DefenseRole] < defenseMaximum;
        const bool initialDefense = defenseIncomplete && configuration.initialDefenseTicks > 0
            && memory<unsigned int>(nativeBindings.gameTick) < static_cast<unsigned int>(configuration.initialDefenseTicks);
        unsigned int facts = defenseIncomplete ? DefenseIncomplete : 0;
        if (playerValue(player, (nativeBindings.players + 0x2BA4)) != 0 || committedAttackActive(player)
            || reserveRecruitmentActive(player)) facts |= AttackActive;
        if (qualifyingHomeThreat(player)) facts |= HomeUnderThreat;
        bool needsEquipmentFact = false;
        for (int row = 0; row < configuration.recruitment.conditionCount; ++row)
            if ((configuration.recruitment.conditions[row].requiredFacts
                | configuration.recruitment.conditions[row].forbiddenFacts) & EquipmentSurplus) needsEquipmentFact = true;
        if (needsEquipmentFact && missingMoat == -1) missingMoat = moatVacancies(aic, character, player);
        if (needsEquipmentFact && equipmentSurplus(probe, player, compositionReady, recruited[DefenseRole], ranged, melee, missingMoat))
            facts |= EquipmentSurplus;
        observation.facts = facts;

        RecruitmentPlan requested;
        if (prepareRecruitment(configuration.recruitment, configuration.baseRows[strength], strength,
                facts, 15, requested) != DrawRecruitmentRole) break;
        observation.condition = requested.conditionIndex;

        TribeAvailability groups;
        queryTribeAvailability(reinterpret_cast<unsigned char*>(Tribes + 0x28), 1250, nativeBindings.tribeStride, player, groups);
        observation.freeGroups = groups.freeSlots;
        probe.freeGroups = groups.freeSlots;
        if (playerValue(player, (nativeBindings.players + 0x2B6C)) <= 0 || playerValue(player, (nativeBindings.players + 0x3974)) == 0) break;

        probe.role = DefenseRole;
        if (requested.eligibleWeights.values[DefenseRole] > 0) {
            if (defenseIncomplete && compositionReady && rosterCandidate(probe, aic, character, player, 0x184,
                playerValue(player, (nativeBindings.players + 0x3100)), 8, 1, defenders[defenderCount])) ++defenderCount;
            if (missingMoat == -1) missingMoat = moatVacancies(aic, character, player);
            if (missingMoat > 0 && eligible(probe, player, aicValue(aic, character, 0x160), 5, -1,
                defenders[defenderCount])) ++defenderCount;
            if (defenderCount) mask |= 1U << DefenseRole;
        }
        const int raidMaximum = reinterpret_cast<TwoIntQuery>(nativeBindings.raidMaximum)(aic, character - 1, player);
        probe.role = RaidRole;
        if (!initialDefense && offensiveActionsAllowed(player) && requested.eligibleWeights.values[RaidRole] > 0 && playerValue(player, (nativeBindings.players + 0x30EC)) + recruited[RaidRole] < raidMaximum
            && rosterCandidate(probe, aic, character, player, 0x1AC,
                playerValue(player, (nativeBindings.players + 0x3104)), 8, 2, candidates[RaidRole])) mask |= 1U << RaidRole;
        probe.role = AttackRole;
        if (!initialDefense && requested.eligibleWeights.values[AttackRole] > 0
            && (!(facts & AttackActive) || reserveRecruitmentActive(player))) {
            attackerCount = attackCandidates(probe, aic, character, player, recruitedAttack, attackers);
            if (attackerCount) mask |= 1U << AttackRole;
        }
        const int rangedMaximum = aicValue(aic, character, 0x14C);
        const int meleeMaximum = aicValue(aic, character, 0x154);
        probe.role = SortieRole;
        const int rangedType = aicValue(aic, character, 0x150);
        const int meleeType = aicValue(aic, character, 0x158);
        if (requested.eligibleWeights.values[SortieRole] > 0) {
        if (rangedType != 5 && rangedType != 29 && rangedType != 30 && rangedType != 37
            && rangedMaximum >= 0 && playerValue(player, (nativeBindings.players + 0x393C)) + ranged
            < rangedMaximum + playerValue(player, (nativeBindings.players + 0x3940)) / 2
            && eligible(probe, player, rangedType, 6, -1, candidates[SortieRole]))
            mask |= 1U << SortieRole;
        else if (meleeType != 5 && meleeType != 29 && meleeType != 30 && meleeType != 37
            && meleeMaximum >= 0 && playerValue(player, (nativeBindings.players + 0x3944)) + melee < meleeMaximum
            && eligible(probe, player, meleeType, 7, -1, candidates[SortieRole]))
            mask |= 1U << SortieRole;
        }
        // Preserve native equipment purchasing through its existing request array.
        // Probes remain read-only. Publish at most one request after probing, only
        // for a positive-weight role with an unmet quota, in stable role order.
        for (int demandRole = 0; demandRole < 4; ++demandRole) {
            const int resource = probe.missingResource[demandRole];
            int amount = aicValue(aic, character, 0x9C);
            if (resource <= 0 || resource >= 26 || amount <= 0) continue;
            if (demandRole != SortieRole && (facts & HomeUnderThreat)) amount = 5;
            playerValue(player, (nativeBindings.players + 0x2A70) + resource * 4) = amount;
            observation.purchaseResource = resource;
            observation.purchaseAmount = amount;
            break;
        }
        observation.eligibleRoles = mask;
        RecruitmentPlan plan;
        if (prepareRecruitment(configuration.recruitment, configuration.baseRows[strength], strength,
                facts, mask, plan) != DrawRecruitmentRole) break;
        observation.condition = plan.conditionIndex;
        BoundedDraw draw;
        if (drawBounded(plan.totalWeight, nativeRandom, 0, draw) != DrawSucceeded) break;
        observation.rngSamples = draw.samplesConsumed;
        const int role = selectRecruitmentRole(plan, draw.ticket);
        if (role < 0 || role > 3) break;
        observation.role = role;
        ++observation.decisions[role];
        if (role == DefenseRole) {
            BoundedDraw unitDraw;
            if (drawBounded(defenderCount, nativeRandom, 0, unitDraw) != DrawSucceeded) break;
            observation.rngSamples += unitDraw.samplesConsumed;
            candidates[role] = defenders[unitDraw.ticket];
        }
        if (role == AttackRole) {
            BoundedDraw unitDraw;
            if (drawBounded(attackerCount, nativeRandom, 0, unitDraw) != DrawSucceeded) break;
            observation.rngSamples += unitDraw.samplesConsumed;
            candidates[role] = attackers[unitDraw.ticket];
        }
        const Candidate& candidate = candidates[role];
        const bool european = candidate.unitType >= 22 && candidate.unitType <= 28;
        const int unit = (european ? native.european : native.nonEuropean)(native.units,
            candidate.unitType, candidate.building, player, 0);
        if (!unit) break;
        assign(aic, player, character, role, candidate, unit, recruitedWalls);
        if (role == DefenseRole && candidate.behaviour != 5) {
            if (configuration.defenseComposition == PreserveSlots)
                ++recruitedTypes[candidate.unitType];
            if (memory<short>(nativeBindings.unitRecords + 0x42A + unit * 0x490) == 1) ++recruitedWalls;
        }
        observation.unit = unit;
        observation.unitType = candidate.unitType;
        observation.hireUID = memory<unsigned int>(nativeBindings.unitRecords + 0x98 + unit * 0x490);
        observation.hireTick = observation.tick;
        observation.hireRole = role;
        ++observation.hires[role];
        if (role == DefenseRole && candidate.behaviour == 5) --missingMoat;
        else ++recruited[role];
        if (role == AttackRole) ++recruitedAttack[candidate.behaviour - 10];
        if (role == SortieRole) { if (candidate.behaviour == 6) ++ranged; else ++melee; }
    }
    return 1;
}

} // namespace SHC141
} // namespace AicTactics
