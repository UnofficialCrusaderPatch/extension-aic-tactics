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

namespace {
const unsigned int PlayerStride = 0x39F4;
const unsigned int Units = 0x1387F38;
const unsigned int Tribes = 0x1667F78;
template<class T> T& memory(unsigned int address) { return *reinterpret_cast<T*>(address); }
int& playerValue(int player, unsigned int address) { return memory<int>(address + player * PlayerStride); }
int aicValue(void* aic, int character, int offset) {
    return *reinterpret_cast<int*>(static_cast<unsigned char*>(aic) + (character - 1) * 0x2A4 + offset);
}
typedef void (__thiscall *PlayerAction)(void*, int);
typedef int (__thiscall *TwoIntQuery)(void*, int, int);
typedef void (__thiscall *TwoIntAction)(void*, int, int);

bool active(int player) {
    if (player < 1 || player > 8) return false;
    const int character = playerValue(player, 0x115E0F8);
    return character >= 2 && character <= 17
        && configurations[character - 1].recruitment.mode == WeightedRoles;
}

int takeRandom(void*) {
    const int result = memory<unsigned short>(0x1A279C2);
    typedef int (__thiscall *Next)(void*);
    reinterpret_cast<Next>(0x46A7D0)(reinterpret_cast<void*>(0x1A279C0));
    return result;
}

int buildingFor(int player, int unitType) {
    int building, type;
    if (unitType == 29 || unitType == 30) {
        building = playerValue(player, 0x115BFF4); type = 24;
    } else if (unitType == 5) {
        building = playerValue(player, 0x115C01C); type = 25;
    } else if (unitType == 37) {
        building = reinterpret_cast<TwoIntQuery>(0x40AAD0)(reinterpret_cast<void*>(0xF98520), player, 38);
        type = 38;
    } else if (unitType >= 70 && unitType <= 76) {
        building = playerValue(player, 0x115C044); type = 8;
    } else if (unitType >= 22 && unitType <= 28) {
        building = playerValue(player, 0x115BF54); type = 9;
    } else return 0;
    if (building <= 0 || building >= 2000 || building >= memory<int>(0xF98528)) return 0;
    const unsigned int address = 0xF98534 + building * 0x32C;
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
    result.european = reinterpret_cast<RecruitFunction>(0x52E960);
    result.nonEuropean = reinterpret_cast<RecruitFunction>(0x52EC10);
    result.failureReason = reinterpret_cast<int*>(Units + 0x60C);
    result.requiredResource = reinterpret_cast<int*>(Units + 0x610);
    for (int i = 0; i < 8; ++i)
        result.availableHorses[i] = reinterpret_cast<short*>(0x115E04A + (i + 1) * PlayerStride);
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
};

bool groupRangeAvailable(int player, int first, int count, int freeGroups, bool smallest) {
    if (first < 0 || count < 1 || first + count > 200) return false;
    bool room = false;
    for (int slot = first; slot < first + count; ++slot) {
        const int group = memory<short>(0x115EF04 + player * PlayerStride + slot * 2);
        if (group < 0 || group >= 1250) return false;
        if (group == 0 || memory<unsigned int>(Tribes + 0x34 + group * 0x334)
                != memory<unsigned int>(0x115F094 + player * PlayerStride + slot * 4))
            return freeGroups > 0; // Native allocates the first missing slot.
        if (memory<int>(Tribes + 0x2C + group * 0x334) != player
            || memory<short>(Tribes + 0x40 + group * 0x334) == 0) return false;
        const int size = memory<short>(Tribes + 0x5C + group * 0x334);
        if (size >= 0 && size < (smallest ? 1000 : 3200)) room = true;
    }
    return room;
}

bool destinationAvailable(const Probe& probe, int player, int unitType, int behaviour) {
    int first = 0, count = 1;
    bool smallest = false;
    if (probe.role == SortieRole) first = behaviour + 160;
    else if (probe.role == AttackRole) {
        if (behaviour < 10 || behaviour > 20) return false;
        first = memory<int>(0xB3EC1C + (behaviour - 10) * 4);
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
        while (index < 20 && memory<int>(0xB426C8 + index * 4) != unitType) ++index;
        if (index == 20) return false;
        first = index < 1 ? 180 : index < 2 ? 181 : index < 7 ? 182
            : index < 12 ? 183 : index < 15 ? 184 : 185;
    } else {
        const int walls = legacyWallCounts ? legacyWallCounts[player] : playerValue(player, 0x115EEE0);
        if (walls >= aicValue(probe.aic, probe.character, 0x180)) {
            first = 170;
            count = aicValue(probe.aic, probe.character, 0x174);
            if (count < 1 || count > 10) return false;
            smallest = true;
        } else {
            int index = 0;
            if (probe.character != 7 && playerValue(player, 0x115F65C) == 1
                && memory<short>(0x115F666 + player * PlayerStride) > 0
                && playerValue(player, 0x115EEB8) > 0
                && (unitType == 22 || unitType == 23 || unitType == 70 || unitType == 72 || unitType == 76)) index = 13;
            else {
                while (index < 20 && memory<int>(0xB425E8 + index * 4) != unitType) ++index;
                if (index == 20) index = 0;
            }
            count = playerValue(player, 0x115EE84 + index * 4);
            if (count < 1) {
                first = 1;
                for (int i = 0; i < 7; ++i)
                    if (memory<int>(0xB3EB34 + i * 4) == unitType) first = 0;
                count = 1;
            } else {
                first = memory<int>(0xB42638 + index * 4);
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
        if (eligible(native, player, aicValue(aic, character, offset + index * 4),
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
            const int wave = playerValue(player, 0x115F71C);
            const int waveMaximum = wave > 0x1FFFFFFF ? 0x7FFFFFFF : wave > 0 ? wave * 4 : 0;
            if (waveMaximum < maximum) maximum = waveMaximum;
        }
        if (maximum <= static_cast<__int64>(playerValue(player, 0x115E9A0 + index * 4)) + recruited[index]) continue;
        if (index == 1) {
            const int target = playerValue(player, 0x115E9D0);
            if (target < 1 || target > 8 || playerValue(target, 0x115F764) <= 5) continue;
        }
        openSubrole = true;
        const int behaviour = index + 10;
        const int unitType = reinterpret_cast<TwoIntQuery>(0x4CC250)(aic, player, behaviour);
        if (eligible(native, player, unitType, behaviour, -1, results[count])) ++count;
    }
    if ((!openSubrole || static_cast<__int64>(playerValue(player, 0x115E9C8)) + recruited[10] < aicValue(aic, character, 0x298))
        && rosterCandidate(native, aic, character, player, 0x288,
            playerValue(player, 0x115EF00), 4, 20, results[count])) ++count;
    return count;
}

void assign(void* aic, int player, int character, int role, const Candidate& candidate, int unit) {
    if (role == DefenseRole) {
        playerValue(player, 0x115EEF8) = candidate.cursor;
        const int walls = legacyWallCounts ? legacyWallCounts[player] : playerValue(player, 0x115EEE0);
        reinterpret_cast<PlayerAction>(walls < aicValue(aic, character, 0x180) ? 0x4D2660 : 0x4D2730)(aic, unit);
    } else if (role == RaidRole) {
        playerValue(player, 0x115EEFC) = candidate.cursor;
        reinterpret_cast<PlayerAction>(0x4D2790)(aic, unit);
    } else if (role == AttackRole) {
        if (candidate.cursor >= 0) playerValue(player, 0x115EF00) = candidate.cursor;
        reinterpret_cast<TwoIntAction>(0x4D27E0)(aic, unit, candidate.behaviour);
    } else {
        memory<short>(0x1388976 + unit * 0x490) = static_cast<short>(candidate.behaviour);
        const int group = reinterpret_cast<TwoIntQuery>(0x4CC910)(aic, player, candidate.behaviour + 160);
        reinterpret_cast<TwoIntAction>(0x522590)(reinterpret_cast<void*>(Tribes), unit, group);
    }
}
} // namespace

void __fastcall rangedSortie(void* aic, void*, int player) {
    if (!active(player)) reinterpret_cast<PlayerAction>(0x4CD560)(aic, player);
}
void __fastcall meleeSortie(void* aic, void*, int player) {
    if (!active(player)) reinterpret_cast<PlayerAction>(0x4CD690)(aic, player);
}

int __cdecl recruitOpportunity(void* aic, int player, int attempts) {
    configurationLocked = 1;
    if (!active(player)) return 0;
    const int character = playerValue(player, 0x115E0F8);
    const int strength = playerValue(player, 0x115EEEC);
    if (strength < 0 || strength > 2 || attempts < 1 || attempts > 4) return 1;
    const CharacterConfiguration& configuration = configurations[character - 1];
    const RecruitmentServices native = services();
    RecruitmentObservation& observation = observations[player];
    int recruited[4] = {0, 0, 0, 0};
    int recruitedAttack[11] = {0};
    int ranged = 0, melee = 0;
    for (int attempt = 0; attempt < attempts; ++attempt) {
        ++observation.sequence;
        observation.tick = memory<int>(0x1FE7DA8);
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
        for (int r = 0; r < 4; ++r) {
            probe.missingResource[r] = 0;
            observation.probeReasons[r] = -2;
            observation.probeTypes[r] = 0;
        }
        Candidate candidates[4];
        Candidate attackers[11];
        int attackerCount = 0;
        unsigned int mask = 0;
        __int64 defenseMaximum = aicValue(aic, character, 0x170);
        if (memory<int>(0x1FE7D78) == 3 && memory<int>(0x1FE9CA4) == 1 && memory<int>(0x1FE9CAC) == 2)
            defenseMaximum = defenseMaximum * 4 / 3;
        if (playerValue(player, 0x115F6E8) > 0) defenseMaximum *= 4;
        const bool defenseIncomplete = static_cast<__int64>(playerValue(player, 0x115EEE0)) + recruited[DefenseRole] < defenseMaximum;
        unsigned int facts = defenseIncomplete ? DefenseIncomplete : 0;
        if (playerValue(player, 0x115E99C) != 0) facts |= AttackActive;
        if (playerValue(player, 0x115F6E8) > 0) facts |= HomeUnderThreat;
        observation.facts = facts;

        RecruitmentPlan requested;
        if (prepareRecruitment(configuration.recruitment, configuration.baseRows[strength], strength,
                facts, 15, requested) != DrawRecruitmentRole) break;
        observation.condition = requested.conditionIndex;

        TribeAvailability groups;
        queryTribeAvailability(reinterpret_cast<unsigned char*>(Tribes + 0x28), 1250, player, groups);
        observation.freeGroups = groups.freeSlots;
        probe.freeGroups = groups.freeSlots;
        if (playerValue(player, 0x115E964) <= 0 || playerValue(player, 0x115F76C) == 0) break;

        probe.role = DefenseRole;
        if (requested.eligibleWeights.values[DefenseRole] > 0 && defenseIncomplete && rosterCandidate(probe, aic, character, player, 0x184,
            playerValue(player, 0x115EEF8), 8, 1, candidates[DefenseRole])) mask |= 1U << DefenseRole;
        const int raidMaximum = reinterpret_cast<TwoIntQuery>(0x4D12A0)(aic, character - 1, player);
        probe.role = RaidRole;
        if (requested.eligibleWeights.values[RaidRole] > 0 && playerValue(player, 0x115EEE4) + recruited[RaidRole] < raidMaximum
            && rosterCandidate(probe, aic, character, player, 0x1AC,
                playerValue(player, 0x115EEFC), 8, 2, candidates[RaidRole])) mask |= 1U << RaidRole;
        probe.role = AttackRole;
        if (requested.eligibleWeights.values[AttackRole] > 0 && !(facts & AttackActive)) {
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
            && rangedMaximum >= 0 && playerValue(player, 0x115F734) + ranged
            < rangedMaximum + playerValue(player, 0x115F738) / 2
            && eligible(probe, player, rangedType, 6, -1, candidates[SortieRole]))
            mask |= 1U << SortieRole;
        else if (meleeType != 5 && meleeType != 29 && meleeType != 30 && meleeType != 37
            && meleeMaximum >= 0 && playerValue(player, 0x115F73C) + melee < meleeMaximum
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
            playerValue(player, 0x115E868 + resource * 4) = amount;
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
        if (drawBounded(plan.totalWeight, takeRandom, 0, draw) != DrawSucceeded) break;
        observation.rngSamples = draw.samplesConsumed;
        const int role = selectRecruitmentRole(plan, draw.ticket);
        if (role < 0 || role > 3) break;
        observation.role = role;
        ++observation.decisions[role];
        if (role == AttackRole) {
            BoundedDraw unitDraw;
            if (drawBounded(attackerCount, takeRandom, 0, unitDraw) != DrawSucceeded) break;
            observation.rngSamples += unitDraw.samplesConsumed;
            candidates[role] = attackers[unitDraw.ticket];
        }
        const Candidate& candidate = candidates[role];
        const bool european = candidate.unitType >= 22 && candidate.unitType <= 28;
        const int unit = (european ? native.european : native.nonEuropean)(native.units,
            candidate.unitType, candidate.building, player, 0);
        if (!unit) break;
        assign(aic, player, character, role, candidate, unit);
        observation.unit = unit;
        observation.unitType = candidate.unitType;
        observation.hireUID = memory<unsigned int>(0x13885E4 + unit * 0x490);
        observation.hireTick = observation.tick;
        observation.hireRole = role;
        ++observation.hires[role];
        ++recruited[role];
        if (role == AttackRole) ++recruitedAttack[candidate.behaviour - 10];
        if (role == SortieRole) { if (candidate.behaviour == 6) ++ranged; else ++melee; }
    }
    return 1;
}

} // namespace SHC141
} // namespace AicTactics
