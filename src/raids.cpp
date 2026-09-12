#include "aic_tactics/raids.hpp"
#include "aic_tactics/runtime.hpp"
#include "aic_tactics/shc141_groups.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

RaidState raidStates[9];
RaidGroupCensus raidGroupCensus[9][4];
int raidUnitPower[9][1024];
int raidStaticDefenses[9][1024];
unsigned int raidBuildingCensusTick;
int raidBuildingCensusValid;

namespace {
const unsigned int PlayerStride = 0x39F4, Tribes = 0x1667F78, Buildings = 0xF98534, Units = 0x138854C;
template<class T> T& at(unsigned int address) { return *reinterpret_cast<T*>(address); }
int& field(int player, unsigned int address) { return at<int>(address + player * PlayerStride); }
bool collecting;
int groupIndex[1250];
unsigned int groupUID[1250];
typedef void (__thiscall *TwoAction)(void*, int, int);
typedef int (__thiscall *TwoQuery)(void*, int, int);

const RaidConfiguration* configuration(int player)
{
    if (player < 1 || player > 8) return 0;
    const int character = field(player, 0x115E0F8);
    if (character < 2 || character > 17) return 0;
    const RaidConfiguration& config = configurations[character - 1].raids;
    return config.policy == NativeRaids ? 0 : &config;
}

bool hostile(int player, int other)
{
    return other >= 1 && other <= 8 && player != other
        && at<int>(0x117D548 + player * 4) != at<int>(0x117D548 + other * 4);
}

bool validGroup(const ReserveGroup& group, int player)
{
    return group.id > 0 && group.id < 1250
        && at<unsigned int>(Tribes + group.id * 0x334 + 0x34) == group.uid
        && at<int>(Tribes + group.id * 0x334 + 0x2C) == player
        && at<short>(Tribes + group.id * 0x334 + 0x40) == 2;
}

int size(const ReserveGroup& group, int player)
{
    return validGroup(group, player) ? at<short>(Tribes + group.id * 0x334 + 0x5C) : 0;
}

bool combatant(int type)
{
    return (type >= 22 && type <= 28) || type == 37 || (type >= 70 && type <= 76);
}

int category(int type)
{
    if (type == 7 || type == 17 || type == 19 || type == 30 || type == 32 || type == 33 || type == 34) return FoodRaidFocus;
    if ((type >= 3 && type <= 6) || (type >= 12 && type <= 16) || type == 18
        || type == 20 || type == 21 || type == 31) return IndustryRaidFocus;
    if (type == 1 || (type >= 8 && type <= 11) || (type >= 22 && type <= 28)
        || (type >= 35 && type <= 38)) return AnyRaidFocus;
    return -1;
}

bool validBuilding(int player, const RaidConfiguration& config, int id, unsigned int uid)
{
    if (id <= 0 || id >= 2000 || id >= at<int>(0xF98528)) return false;
    const unsigned int address = Buildings + id * 0x32C;
    const int owner = at<short>(address + 0xD6);
    if (!hostile(player, owner)) return false;
    const PlayerCensus& census = combatCensus[owner];
    if (census.lord <= 0 || census.lord >= 2500) return false;
    const unsigned int lord = Units + census.lord * 0x490;
    if (at<short>(lord + 0x8C) != 2 || at<short>(lord + 0x8E) != 55
        || at<short>(lord + 0x96) != owner || at<int>(lord + 0x98) != census.lordUID
        || at<short>(lord + 0x2A0) != 0 || at<int>(lord + 0x3C8) <= 0) return false;
    return at<short>(address + 0xD0) == 2 && at<unsigned int>(address + 0xD8) == uid
        && at<short>(address + 0x2BE) == 0 && category(at<short>(address + 0xD2)) >= 0
        && (config.scope == AnyRaidEnemy || field(player, 0x115E9D0) == owner);
}

void returnHome(void* aic, int player, RaidGroup& group)
{
    group.building = 0;
    group.buildingUID = 0;
    if (validGroup(group.tribe, player)) {
        reinterpret_cast<TwoAction>(0x4CD110)(aic, group.tribe.id, player);
        at<short>(Tribes + group.tribe.id * 0x334 + 0x2E0) = 1;
        at<short>(Tribes + group.tribe.id * 0x334 + 0x2F8) = 0;
        at<unsigned int>(Tribes + group.tribe.id * 0x334 + 0x2FC) = 0;
    }
}

void attack(RaidGroup& group)
{
    typedef void (__thiscall *Relay)(void*, int, int, int, unsigned int, int);
    reinterpret_cast<Relay>(0x5371E0)(reinterpret_cast<void*>(0x1387F38),
        group.tribe.id, 9, group.building, group.buildingUID, 0);
    at<short>(Tribes + group.tribe.id * 0x334 + 0x2F8) = static_cast<short>(group.building);
    at<unsigned int>(Tribes + group.tribe.id * 0x334 + 0x2FC) = group.buildingUID;
    at<short>(Tribes + group.tribe.id * 0x334 + 0x2E0) = 1;
}

int replacementValue(int type, const int prices[4])
{
    if (type < 1 || type >= 110) return 0;
    const unsigned int cost = 0xF98520 + 0x18C7D4 + type * 20;
    __int64 value = at<int>(cost + 16);
    for (int index = 0; index < 4; ++index) {
        const int amount = at<int>(cost + index * 4);
        const int price = prices[index];
        if (amount > 0 && price > 0) value += static_cast<__int64>(amount) * price;
    }
    return value <= 0 ? 0 : value > 512 ? 512 : static_cast<int>(value);
}

bool exposure(const bool enemies[9], int x, int y, int ownPower, int risk, int& penalty)
{
    __int64 mobile = 0, defenses = 0;
    const int cellX = x / 16, cellY = y / 16;
    for (int cy = cellY - 1; cy <= cellY + 1; ++cy) {
        if (cy < 0 || cy >= 32) continue;
        for (int cx = cellX - 1; cx <= cellX + 1; ++cx) {
            if (cx < 0 || cx >= 32) continue;
            for (int enemy = 1; enemy <= 8; ++enemy) if (enemies[enemy]) {
                mobile += raidUnitPower[enemy][cy * 32 + cx];
                defenses += raidStaticDefenses[enemy][cy * 32 + cx];
            }
        }
    }
    const __int64 danger = mobile + defenses * 25;
    const __int64 budget = risk == LowRaidRisk ? ownPower / 4
        : risk == MediumRaidRisk ? ownPower / 2 : static_cast<__int64>(ownPower) * 2;
    if (danger > budget || (risk == LowRaidRisk && defenses != 0)) return false;
    const __int64 score = danger * 64 / (static_cast<__int64>(ownPower) + 1);
    penalty = score > 128 ? 128 : static_cast<int>(score);
    return true;
}

struct Candidate { int id; unsigned int uid; int tile; int score; };

bool chooseTarget(void* aic, int player, int index, const RaidConfiguration& config, int probeBudget)
{
    RaidGroup& group = raidStates[player].groups[index];
    if (!combatCensusValid || at<unsigned int>(nativeBindings.gameTick) - combatCensusTick > 1
        || !raidBuildingCensusValid || at<unsigned int>(nativeBindings.gameTick) - raidBuildingCensusTick > 1) return false;
    const int leader = at<short>(Tribes + group.tribe.id * 0x334 + 0x5A);
    if (leader <= 0 || leader >= 2500) return false;
    const unsigned int unit = Units + leader * 0x490;
    if (at<short>(unit + 0x2D8) != group.tribe.id || at<unsigned int>(unit + 0x2E4) != group.tribe.uid) return false;
    const int fromX = at<short>(unit + 0xC4), fromY = at<short>(unit + 0xC6);
    Candidate candidates[8];
    bool enemies[9] = {false};
    for (int enemy = 1; enemy <= 8; ++enemy) enemies[enemy] = hostile(player, enemy);
    int cellPenalty[1024], values[110], prices[4] = {0};
    for (int cell = 0; cell < 1024; ++cell) cellPenalty[cell] = -2;
    for (int type = 0; type < 110; ++type) values[type] = -1;
    if (config.policy == OpportunisticRaid && config.focus == HighValueRaidFocus) {
        const int resources[4] = {2,4,6,7};
        typedef int (__thiscall *Price)(void*, int);
        for (int resource = 0; resource < 4; ++resource)
            prices[resource] = reinterpret_cast<Price>(0x4588D0)(reinterpret_cast<void*>(0x112B0B8), resources[resource]);
    }
    int count = 0;
    for (int enemy = 1; enemy <= 8; ++enemy) {
        if (!enemies[enemy] || (config.scope == PrimeRaidTarget && field(player, 0x115E9D0) != enemy)) continue;
        int length = field(enemy, 0x115F47C);
        if (length < 0 || length > 100) continue;
        for (int entry = 0; entry < length; ++entry) {
            const int building = at<short>(0x115F3B4 + enemy * PlayerStride + entry * 2);
            if (building <= 0 || building >= 2000) continue;
            const unsigned int address = Buildings + building * 0x32C;
            const unsigned int uid = at<unsigned int>(address + 0xD8);
            if (!validBuilding(player, config, building, uid)) continue;
            bool duplicate = false;
            for (int existing = 0; existing < count; ++existing)
                if (candidates[existing].id == building) duplicate = true;
            if (duplicate) continue;
            bool reserved = false;
            for (int other = 0; other < 4; ++other)
                if (other != index && raidStates[player].groups[other].building == building
                    && raidStates[player].groups[other].buildingUID == uid) reserved = true;
            if (reserved) continue;
            const int x = at<short>(address + 0xFE), y = at<short>(address + 0x100);
            if (x < 0 || x >= 400 || y < 0 || y >= 400) continue;
            int& penalty = cellPenalty[(y / 16) * 32 + x / 16];
            if (penalty == -2 && !exposure(enemies, x, y, raidGroupCensus[player][index].power, config.risk, penalty)) penalty = -1;
            if (penalty < 0) continue;
            int dx = x - fromX, dy = y - fromY;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            int score = (dx > dy ? dx : dy) * 16;
            if (config.policy == OpportunisticRaid) {
                const int type = at<short>(address + 0xD2);
                if (config.focus == HighValueRaidFocus) {
                    if (values[type] < 0) values[type] = replacementValue(type, prices);
                    score -= values[type] / 4;
                }
                else if (config.focus != AnyRaidFocus && config.focus == category(type)) score -= 128;
                score += penalty;
            }
            const int tile = at<int>(0x2337300 + y * 12) + x;
            if (tile <= 0 || tile >= 160000) continue;
            Candidate candidate = {building, uid, tile, score};
            int position = 0;
            while (position < count && (candidates[position].score < score
                || (candidates[position].score == score && candidates[position].id <= building))) ++position;
            if (position >= 8) continue;
            if (count < 8) ++count;
            for (int move = count - 1; move > position; --move) candidates[move] = candidates[move - 1];
            candidates[position] = candidate;
        }
    }
    for (int probe = 0; probe < probeBudget && probe < count; ++probe) {
        const int selected = (group.pathCursor + probe) % count;
        const Candidate& candidate = candidates[selected];
        if (!reinterpret_cast<TwoQuery>(0x4CD250)(aic, group.tribe.id, candidate.tile)) continue;
        group.building = candidate.id;
        group.buildingUID = candidate.uid;
        group.pathCursor = 0;
        attack(group);
        return true;
    }
    group.pathCursor = count ? (group.pathCursor + probeBudget) % count : 0;
    returnHome(aic, player, group);
    return true;
}

void fillGroups(void* aic, int player, int desired, const RaidConfiguration& config)
{
    RaidState& state = raidStates[player];
    bool changed[4] = {false,false,false,false};
    int moved = 0;
    bool poolExhausted = false;
    for (int work = 0; work < 64 && moved < 16 && desired > 0 && !poolExhausted; ++work) {
        if (state.stagingGroup < 0 || state.stagingGroup >= 10) state.stagingGroup = 0;
        if (state.stagingWord < 0 || state.stagingWord >= 157) state.stagingWord = 0;
        const int sourceIndex = state.stagingGroup;
        ReserveGroup source;
        if (sourceIndex < 6) {
            source.id = at<short>(0x115EF04 + player * PlayerStride + (180 + sourceIndex) * 2);
            source.uid = at<unsigned int>(0x115F094 + player * PlayerStride + (180 + sourceIndex) * 4);
        } else source = state.groups[sourceIndex - 6].tribe;
        if (sourceIndex >= 6 && sourceIndex - 6 < desired) {
            state.stagingGroup = (sourceIndex + 1) % 10; state.stagingWord = 0; continue;
        }
        if (!validGroup(source, player) || size(source, player) <= 0) {
            state.stagingGroup = (sourceIndex + 1) % 10; state.stagingWord = 0; continue;
        }
        unsigned int bits = at<unsigned short>(Tribes + source.id * 0x334 + 0x60 + state.stagingWord * 2);
        bool finished = true;
        for (int bit = 0; bit < 16; ++bit) {
            const int id = state.stagingWord * 16 + bit;
            if (!(bits & (1U << bit)) || id <= 0 || id >= 2500) continue;
            const unsigned int unit = Units + id * 0x490;
            if (at<short>(unit + 0x96) != player || at<short>(unit + 0x42A) != 2
                || at<short>(unit + 0x2D8) != source.id || at<unsigned int>(unit + 0x2E4) != source.uid
                || at<short>(unit + 0x8C) != 2 || at<short>(unit + 0x2A0) != 0
                || at<int>(unit + 0x3C8) <= 0) continue;
            const bool fighter = combatant(at<short>(unit + 0x8E));
            int destination = -1, smallest = 2501;
            for (int group = 0; group < desired; ++group) {
                const int count = size(state.groups[group].tribe, player);
                if (!fighter && raidGroupCensus[player][group].combatants == 0) continue;
                if (count < smallest) { destination = group; smallest = count; }
            }
            if (destination < 0) continue;
            RaidGroup& target = state.groups[destination];
            if (!validGroup(target.tribe, player)) {
                TribeAvailability available;
                if (!queryTribeAvailability(reinterpret_cast<const unsigned char*>(Tribes + 0x28), 1250, player, available)
                    || available.freeSlots == 0) { poolExhausted = true; break; }
                typedef int (__thiscall *Create)(void*, int);
                target.tribe.id = reinterpret_cast<Create>(0x5227E0)(reinterpret_cast<void*>(Tribes), player);
                if (target.tribe.id <= 0 || target.tribe.id >= 1250) { poolExhausted = true; break; }
                target.tribe.uid = at<unsigned int>(Tribes + target.tribe.id * 0x334 + 0x34);
                target.building = 0; target.buildingUID = 0; target.pathCursor = 0;
            }
            reinterpret_cast<TwoAction>(0x525A70)(reinterpret_cast<void*>(Tribes), id, source.id);
            reinterpret_cast<TwoAction>(0x522590)(reinterpret_cast<void*>(Tribes), id, target.tribe.id);
            // Keep census snapshots immutable between native census phases.
            // Newly formed groups wait for that census before choosing a target.
            changed[destination] = true;
            if (++moved >= 16) { finished = false; break; }
        }
        if (finished && ++state.stagingWord >= 157) {
            state.stagingWord = 0; state.stagingGroup = (sourceIndex + 1) % 10;
        }
    }
    for (int index = 0; index < 4; ++index) {
        RaidGroup& group = state.groups[index];
        if (index >= desired || size(group.tribe, player) < config.minimumSize
            || raidGroupCensus[player][index].combatants == 0) {
            if (group.building || changed[index]) returnHome(aic, player, group);
        } else if (changed[index]) {
            if (validBuilding(player, config, group.building, group.buildingUID)) attack(group);
            else returnHome(aic, player, group);
        }
    }
}
} // namespace

bool raidPoliciesEnabled()
{
    for (int character = 1; character <= 16; ++character)
        if (configurations[character].raids.policy != NativeRaids) return true;
    return false;
}

void resetRaidUnitCensus()
{
    collecting = raidPoliciesEnabled();
    if (!collecting) return;
    std::memset(raidUnitPower, 0, sizeof(raidUnitPower));
    std::memset(raidGroupCensus, 0, sizeof(raidGroupCensus));
    std::memset(groupIndex, 0, sizeof(groupIndex));
    std::memset(groupUID, 0, sizeof(groupUID));
    for (int player = 1; player <= 8; ++player) if (configuration(player)) {
        for (int group = 0; group < 4; ++group) {
            const ReserveGroup& tribe = raidStates[player].groups[group].tribe;
            if (!validGroup(tribe, player)) continue;
            groupIndex[tribe.id] = player * 4 + group + 1;
            groupUID[tribe.id] = tribe.uid;
        }
    }
}

void countRaidUnit(int unit, int power)
{
    if (!collecting || unit <= 0 || unit >= 2500) return;
    const unsigned int address = Units + unit * 0x490;
    const int player = at<short>(address + 0x96);
    const int x = at<short>(address + 0xC4), y = at<short>(address + 0xC6);
    if (player < 1 || player > 8 || x < 0 || x >= 512 || y < 0 || y >= 512) return;
    if (power > 0) {
        int& cell = raidUnitPower[player][(y / 16) * 32 + x / 16];
        cell = power > 0x7FFFFFFF - cell ? 0x7FFFFFFF : cell + power;
    }
    const int group = at<short>(address + 0x2D8);
    if (group <= 0 || group >= 1250 || groupIndex[group] == 0
        || at<unsigned int>(address + 0x2E4) != groupUID[group]) return;
    const int index = groupIndex[group] - 1;
    if (index / 4 != player) return;
    RaidGroupCensus& census = raidGroupCensus[player][index % 4];
    if (combatant(at<short>(address + 0x8E))) ++census.combatants;
    if (power > 0) census.power = power > 0x7FFFFFFF - census.power ? 0x7FFFFFFF : census.power + power;
}

void __cdecl resetRaidBuildingCensus()
{
    raidBuildingCensusValid = 0;
    std::memset(raidStaticDefenses, 0, sizeof(raidStaticDefenses));
}

void __cdecl countRaidBuilding(int building)
{
    if (building <= 0 || building >= 2000) return;
    const unsigned int address = Buildings + building * 0x32C;
    const int type = at<short>(address + 0xD2);
    if (type != 60 && type != 61 && !(type >= 45 && type <= 48) && !(type >= 74 && type <= 78)
        && type != 67 && type != 68 && type != 86 && type != 87) return;
    const int player = at<short>(address + 0xD6);
    const int x = at<unsigned short>(address + 0xEE), y = at<unsigned short>(address + 0xF0);
    if (player < 1 || player > 8 || x >= 512 || y >= 512 || at<short>(address + 0xD0) != 2) return;
    ++raidStaticDefenses[player][(y / 16) * 32 + x / 16];
}

void __cdecl completeRaidBuildingCensus()
{
    raidBuildingCensusTick = at<unsigned int>(nativeBindings.gameTick);
    raidBuildingCensusValid = 1;
}

bool updateSplitRaids(void* aic, int player)
{
    const RaidConfiguration* config = configuration(player);
    if (!config) return false;
    RaidState& state = raidStates[player];
    for (int index = 0; index < 4; ++index) {
        RaidGroup& group = state.groups[index];
        if (!validGroup(group.tribe, player)) {
            std::memset(&group, 0, sizeof(group));
            std::memset(&raidGroupCensus[player][index], 0, sizeof(RaidGroupCensus));
        } else if (group.building && !validBuilding(player, *config, group.building, group.buildingUID)) {
            returnHome(aic, player, group);
            group.pathCursor = 0;
        }
    }
    int desired = field(player, 0x115EEE4) / config->minimumSize;
    if (desired < 0) desired = 0;
    if (desired > config->groups) desired = config->groups;
    fillGroups(aic, player, desired, *config);
    int& timer = field(player, 0x115E970);
    const int character = field(player, 0x115E0F8);
    const int interval = *reinterpret_cast<int*>(static_cast<unsigned char*>(aic) + (character - 1) * 0x2A4 + 0x1F0);
    if (interval <= 1 || timer < 0 || timer >= interval - 1) timer = 0;
    else ++timer;
    if (timer == 0) state.retargetPending = (1U << config->groups) - 1;
    if (state.decisionGroup < 0 || state.decisionGroup >= 4) state.decisionGroup = 0;
    const int index = state.decisionGroup;
    state.decisionGroup = (index + 1) % 4;
    RaidGroup& group = state.groups[index];
    if (index >= desired || !validGroup(group.tribe, player) || size(group.tribe, player) < config->minimumSize
        || raidGroupCensus[player][index].combatants == 0) return true;
    if (!validBuilding(player, *config, group.building, group.buildingUID)
        || (state.retargetPending & (1U << index))) {
        if (chooseTarget(aic, player, index, *config, 2)) state.retargetPending &= ~(1U << index);
    } else {
        // Detect lost area connectivity at this group's native decision phase.
        // Reserve the second path query for a bounded replacement attempt.
        const unsigned int building = Buildings + group.building * 0x32C;
        const int x = at<short>(building + 0xFE), y = at<short>(building + 0x100);
        if (x < 0 || x >= 400 || y < 0 || y >= 400) {
            returnHome(aic, player, group);
            return true;
        }
        const int leader = at<short>(Tribes + group.tribe.id * 0x334 + 0x5A);
        if (leader <= 0 || leader >= 2500) return true;
        const unsigned int unit = Units + leader * 0x490;
        if (at<short>(unit + 0x2D8) != group.tribe.id || at<unsigned int>(unit + 0x2E4) != group.tribe.uid) return true;
        const int tile = at<int>(0x2337300 + y * 12) + x;
        if (tile <= 0 || tile >= 160000) { returnHome(aic, player, group); return true; }
        if (!reinterpret_cast<TwoQuery>(0x4CD250)(aic, group.tribe.id, tile)) {
            returnHome(aic, player, group);
            chooseTarget(aic, player, index, *config, 1);
        }
    }
    return true;
}

} // namespace SHC141
} // namespace AicTactics
