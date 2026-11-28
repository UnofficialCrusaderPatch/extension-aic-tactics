#include "aic_tactics/lord_hunt.hpp"
#include "aic_tactics/diagnostics.hpp"
#include "aic_tactics/groups.hpp"
#include "aic_tactics/runtime.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

int lordHuntRetarget[17];
int lordHuntSiege[17];
int lordHuntSwitch[17];
int lordHuntShare[17];
int lordHuntSplit[17];
int lordHuntRetargetFallback = 1;
int lordHuntSiegeFallback = 0;
int lordHuntSwitchFallback = 80;
int lordHuntShareFallback = 50;
int lordHuntSplitFallback = 50;
LordHuntState lordHunts[9];
LordHuntCensus lordHuntCensus[9];
int lordHuntUnits[9];

namespace {
const unsigned int PlayerStride = 0x39F4;
const int TribeCapacity = 1250;
// Packed isometric tile count of a 400x400 map (OpenSHC Map/TileMapState.hpp).
const int TileCapacity = 80400;
const int LordType = 55;
// UnitInstructionType values of the native group order.
const int OrderAttackUnit = 4;
const int OrderAttackBuilding = 9;
const int OrderAttackWall = 23;
const short AggressiveStance = 2;      // UnitStanceEnum
// Logic layer: wall or gatehouse (OpenSHC Logic1); tiles with 0x2 as well are
// not attacked as walls.
const int WallFlag = 0x100;
const int NotWallFlag = 0x2;
const unsigned int ClosedTicks = 200;  // the way to the lord shut this long ends a hunt
const unsigned int ReorderTicks = 80;  // hunters off the lord get the order again
const int WallSearch = 30;             // tiles around the split troops searched for walls
const int ReturnBudget = 32;           // members handed back per AI update
const int ReachQueries = 8;            // area pairs tested per AI update
const int TargetableLimit = 100;       // the native targetable building list
// The returning mask holds one bit per detached record; this bit asks for the
// native return order once the groups of a finished attack are back.
const int ReturnAfterAttack = 0x100;

template<class T> T& at(unsigned int address) { return *reinterpret_cast<T*>(address); }
typedef int (__thiscall *AreaRoute)(void*, int, int, int, int);
typedef void (__thiscall *Relay)(void*, int, int, int, unsigned int, int);
typedef void (__thiscall *PlayerAction)(void*, int);

// Census scratch, rebuilt at every census reset.
short slotOwner[TribeCapacity];
int collecting[9];
int splitTribes[9][2];

unsigned int playerAddress(int player) { return nativeBindings.players + player * PlayerStride; }
int field(int player, unsigned int offset) { return at<int>(playerAddress(player) + offset); }
unsigned int unitAddress(int unit) { return nativeBindings.unitRecords + unit * 0x490; }
unsigned int tribeAddress(int tribe) { return nativeBindings.tribes + tribe * nativeBindings.tribeStride; }
unsigned int buildingAddress(int building) { return nativeBindings.buildings + 0x14 + building * 0x32C; }

// AIC character 1..16 of an AI player; 0 for a human player.
int character(int player)
{
    if (player < 1 || player > 8) return 0;
    const int value = field(player, 0x2300) - 1;
    return value >= 1 && value <= 16 ? value : 0;
}

bool enabled(int ch)
{
    return setting(lordHuntRetarget, ch, lordHuntRetargetFallback) == 1
        || setting(lordHuntShare, ch, lordHuntShareFallback) > 0;
}

// PlayerData.aiType is zero for a human player on every peer.
bool human(int player) { return player >= 1 && player <= 8 && field(player, 0x2300) == 0; }
int team(int player) { return at<int>(nativeBindings.teams + player * 4); }

int area(int tile)
{
    return tile > 0 && tile < TileCapacity ? at<unsigned short>(nativeBindings.areaMap + tile * 2) : 0;
}

int tileAt(int x, int y)
{
    if (x < 0 || y < 0 || x >= 400 || y >= 400) return 0;
    const int tile = at<int>(nativeBindings.mapRows + y * 12) + x;
    return tile > 0 && tile < TileCapacity ? tile : 0;
}

// Area of the keep tile, as the AI's own breach test reads it.
int keepArea(int player)
{
    return player >= 1 && player <= 8 && field(player, 0x94) ? area(field(player, 0x1E0)) : 0;
}

int distance(int ax, int ay, int bx, int by)
{
    const int dx = ax > bx ? ax - bx : bx - ax;
    const int dy = ay > by ? ay - by : by - ay;
    return dx > dy ? dx : dy;
}

struct Lord { int unit; unsigned int uid; int x; int y; int tile; };

bool liveLord(int player, Lord& lord)
{
    lord.unit = liveCensusLord(player);
    if (!lord.unit) return false;
    const unsigned int address = unitAddress(lord.unit);
    lord.uid = at<unsigned int>(address + 0x98);
    lord.x = at<short>(address + 0xC4);
    lord.y = at<short>(address + 0xC6);
    lord.tile = at<int>(address + 0xD4);
    return true;
}

int unitArea(int unit) { return area(at<int>(unitAddress(unit) + 0xD4)); }

struct Reach {
    int player;
    int count;
    int from[ReachQueries];
    int to[ReachQueries];
    int result[ReachQueries];
};

// The game's own area test, without (mode 0) and with (mode 2) climbing
// links: the two queries the AI uses to call a castle breached. Pairs beyond
// the per-update budget count as unreachable.
bool reaches(Reach& memo, int from, int to)
{
    if (!from || !to) return false;
    if (from == to) return true;
    for (int index = 0; index < memo.count; ++index)
        if (memo.from[index] == from && memo.to[index] == to) return memo.result[index] != 0;
    if (memo.count == ReachQueries) return false;
    void* finder = reinterpret_cast<void*>(nativeBindings.areaPathFinder);
    const AreaRoute route = reinterpret_cast<AreaRoute>(nativeBindings.areaRoute);
    const int result = route(finder, memo.player, to, from, 0) || route(finder, memo.player, to, from, 2) ? 1 : 0;
    memo.from[memo.count] = from;
    memo.to[memo.count] = to;
    memo.result[memo.count++] = result;
    return result != 0;
}

void order(int tribe, int instruction, int target, unsigned int uid)
{
    reinterpret_cast<Relay>(nativeBindings.relayRaidOrder)(reinterpret_cast<void*>(nativeBindings.units),
        tribe, instruction, target, uid, 0);
}

void aggressive(int tribe) { at<short>(tribeAddress(tribe) + nativeBindings.tribeStance) = AggressiveStance; }

// Behaviour types of the attack army, in the order of the AI's breach troop
// table (main army first). Engineers (10), moat diggers (11), laddermen (14)
// and tunnellers (15) keep their own jobs.
int huntRank(int role)
{
    static const int ranks[21] = {0,0,0,0,0,0,0,0,0,0, 0,0,4,3,0,0,7,5,6,2,1};
    return role >= 0 && role <= 20 ? ranks[role] : 0;
}

// Siege engines, their crews and the lord never fight a lord.
bool leftAlone(int type)
{
    return type == 29 || type == 30 || (type >= 39 && type <= 41) || type == 55
        || (type >= 58 && type <= 61) || type == 77;
}

// Archers, crossbowmen, Arabian archers, slingers and horse archers walk with
// the split troops and shoot what they meet instead of attacking buildings.
bool ranged(int type) { return type == 22 || type == 23 || type == 70 || type == 72 || type == 74; }

bool wallOf(int tile, int player, bool searching)
{
    if (tile <= 0 || tile >= TileCapacity) return false;
    const int flags = at<int>(nativeBindings.tileLogic + tile * 4);
    if (!(flags & WallFlag) || (searching && (flags & NotWallFlag))) return false;
    return (at<unsigned char>(nativeBindings.tileWallOwner + tile) & 7) + 1 == player;
}

struct Candidate { int first; int second; int unit; };

bool before(const Candidate& left, const Candidate& right)
{
    if (left.first != right.first) return left.first < right.first;
    if (left.second != right.second) return left.second < right.second;
    return left.unit < right.unit;
}

void insert(Candidate* list, int& count, const Candidate& entry)
{
    int position = count++;
    while (position > 0 && before(entry, list[position - 1])) {
        list[position] = list[position - 1];
        --position;
    }
    list[position] = entry;
}

// Marks a referenced group for return; the budgeted hand-back runs in
// returnGroups. An empty record is cleared at once by the owner.
void handBack(void* aic, int player, LordHuntState& state, int& reference)
{
    if (reference > 0) {
        returnDetachedGroup(aic, player, reference - 1, 0);
        if (detachedGroups[player][reference - 1].duty != NoDuty) state.returning |= 1 << (reference - 1);
    }
    reference = 0;
}

void endSplit(void* aic, int player, LordHuntState& state)
{
    handBack(aic, player, state, state.splitGroup);
    handBack(aic, player, state, state.shooterGroup);
    state.splitTo = 0;
    state.splitOrder = NoSplitOrder;
    state.splitSize = 0;
}

// The attack goes back to the lord's own castle: a new plan while the army
// is strong enough, else the native cleanup.
void turnBack(int ch, int phase, LordHuntState& state, const LordHuntCensus& census, SiegeRetarget& change)
{
    state.host = 0;
    change.castle = state.origin;
    const __int64 required = static_cast<__int64>(state.peak) * setting(lordHuntSwitch, ch, lordHuntSwitchFallback);
    change.phase = phase < 3 || static_cast<__int64>(census.attackTroops) * 100 >= required ? 1 : 8;
}

// The owner frees records whose tribe is gone and reuses them for any duty,
// so a reference holds only while its record keeps the duty it was opened for.
void dropFreed(int player, int& reference, int duty)
{
    if (reference > 0 && detachedGroups[player][reference - 1].duty != duty) reference = 0;
}

void returnGroups(void* aic, int player, LordHuntState& state)
{
    int budget = ReturnBudget;
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index) {
        if (!(state.returning & (1 << index))) continue;
        const DetachedGroup& group = detachedGroups[player][index];
        if ((group.duty != HuntDuty && group.duty != SplitDuty) || !group.returning) {
            state.returning &= ~(1 << index);
            continue;
        }
        if (budget <= 0) continue;
        const int size = detachedGroupSize(player, index);
        const int left = returnDetachedGroup(aic, player, index, budget);
        budget -= size > left ? size - left : 0;
        if (detachedGroups[player][index].duty == NoDuty) state.returning &= ~(1 << index);
    }
    if (state.returning == ReturnAfterAttack) {
        state.returning = 0;
        // Members handed back after the native return order went out get it again.
        const int phase = field(player, 0x2BA4);
        if (phase == 0 || phase == 9)
            reinterpret_cast<PlayerAction>(nativeBindings.returnAttack)(aic, player);
    }
}

SiegeRetarget finish(void* aic, int player, LordHuntState& state)
{
    diagnostic("player %d: the attack on player %d is over; lord hunt troops return", player, state.origin);
    SiegeRetarget change = {0, 0};
    if (state.host && field(player, 0x2BD8) == state.host) change.castle = state.origin;
    const int groups = state.huntGroup | state.splitGroup | state.shooterGroup;
    handBack(aic, player, state, state.huntGroup);
    handBack(aic, player, state, state.splitGroup);
    handBack(aic, player, state, state.shooterGroup);
    const int returning = state.returning | (groups && state.returning ? ReturnAfterAttack : 0);
    std::memset(&state, 0, sizeof(state));
    state.returning = returning;
    return change;
}

void openSplit(int player, LordHuntState& state, const LordHuntCensus& census, int castle, int share)
{
    Candidate list[LordHuntCandidateCapacity];
    int count = 0;
    const int kx = field(castle, 0x98), ky = field(castle, 0x9C);
    for (int index = 0; index < census.candidateCount; ++index) {
        const int entry = census.candidates[index];
        if (entry >> 16 != MainArmySource) continue;
        const unsigned int address = unitAddress(entry & 0xFFFF);
        const Candidate candidate = {distance(at<short>(address + 0xC4), at<short>(address + 0xC6), kx, ky), 0,
            entry & 0xFFFF};
        insert(list, count, candidate);
    }
    const int chosen = (count * share + 50) / 100;
    int melee = -1, shooters = -1;
    for (int index = 0; index < chosen; ++index) {
        const int unit = list[index].unit;
        const unsigned int address = unitAddress(unit);
        const int tribe = at<short>(address + 0x2D8);
        if (tribe <= 0 || tribe >= TribeCapacity || slotOwner[tribe] != player) continue;
        int& group = ranged(at<short>(address + 0x8E)) ? shooters : melee;
        if (group < 0) group = openDetachedGroup(player, SplitDuty);
        if (group < 0) continue;
        if (moveUnitToDetachedGroup(player, group, unit)) {
            DetachedGroup& record = detachedGroups[player][group];
            record.target = castle;
            record.x = kx;
            record.y = ky;
        }
    }
    state.splitGroup = melee >= 0 ? melee + 1 : 0;
    state.shooterGroup = shooters >= 0 ? shooters + 1 : 0;
    state.splitTo = castle;
    state.splitOrder = NoSplitOrder;
    state.splitSize = 0;
}

void checkHiding(int player, int ch, int phase, LordHuntState& state, const LordHuntCensus& census,
    const Lord& lord, Reach& memo, SiegeRetarget& change)
{
    const int origin = state.origin;
    const int lordArea = area(lord.tile);
    if (!lordArea || keepArea(origin) == lordArea) return;
    int host = 0;
    for (int other = 1; other <= 8 && !host; ++other)
        if (other != origin && other != player && team(other) == team(origin) && team(other) != team(player)
            && keepArea(other) == lordArea) host = other;
    if (!host) return;
    // Not hidden while any main army troop can walk to him: that is the hunt's job.
    for (int index = 0; index < census.candidateCount; ++index) {
        const int entry = census.candidates[index];
        if (entry >> 16 == MainArmySource && reaches(memo, unitArea(entry & 0xFFFF), lordArea)) return;
    }
    const __int64 required = static_cast<__int64>(state.peak) * setting(lordHuntSwitch, ch, lordHuntSwitchFallback);
    if (phase >= 3 && static_cast<__int64>(census.attackTroops) * 100 < required) return;
    // His own castle still holds people and buildings: part of the army attacks it.
    const int share = lordHuntUnits[origin] > 0 && field(origin, 0x3684) > 0
        ? setting(lordHuntSplit, ch, lordHuntSplitFallback) : 0;
    int splitTo = host;
    if (setting(lordHuntSiege, ch, lordHuntSiegeFallback) == 0 || share == 0) {
        state.host = host;
        change.castle = host;
        change.phase = 1;
        splitTo = origin;
    }
    state.phase |= LordHuntHidingDecided;
    if (share > 0) openSplit(player, state, census, splitTo, share);
    diagnostic("player %d: the lord of player %d hides in the castle of player %d; siege on player %d, "
        "%d%% of the main army to player %d", player, origin, host, state.host ? host : origin, share, splitTo);
}

// The lord left the castle the attack turned to: the split troops rejoin
// the army and the attack goes back to his own castle.
void checkShelter(void* aic, int player, int ch, int phase, LordHuntState& state,
    const LordHuntCensus& census, const Lord& lord, SiegeRetarget& change)
{
    const int hiding = state.host ? state.host : state.splitTo != state.origin ? state.splitTo : 0;
    if (!hiding || (state.phase & LordHuntSwitchedBack)) return;
    const int lordArea = area(lord.tile);
    if (!lordArea || lordArea == keepArea(hiding)) return;
    state.phase |= LordHuntSwitchedBack;
    diagnostic("player %d: the lord of player %d left the castle of player %d; the split troops rejoin%s",
        player, state.origin, hiding, state.host ? " and the siege returns to his castle" : "");
    endSplit(aic, player, state);
    if (state.host) turnBack(ch, phase, state, census, change);
}

// A building's entrance, else the tile below its corner.
void buildingTile(unsigned int address, int& x, int& y)
{
    x = at<short>(address + 0xFE);
    y = at<short>(address + 0x100);
    if (x > 0 || y > 0) return;
    x = at<short>(address + 0xEE);
    y = at<short>(address + 0xF0) + at<int>(address + 0xF8);
}

bool nearestBuilding(Reach& memo, int castle, int fromArea, int x, int y, int& building, unsigned int& uid,
    int& bx, int& by)
{
    int count = field(castle, 0x3684);
    if (count > TargetableLimit) count = TargetableLimit;
    Candidate list[TargetableLimit];
    int listed = 0;
    for (int index = 0; index < count; ++index) {
        const int id = at<short>(playerAddress(castle) + 0x35BC + index * 2);
        if (id <= 0 || id >= static_cast<int>(nativeBindings.buildingCapacity)) continue;
        const unsigned int address = buildingAddress(id);
        if (at<short>(address + 0xD0) == 0 || at<short>(address + 0xD6) != castle) continue;
        int ex = 0, ey = 0;
        buildingTile(address, ex, ey);
        if (!tileAt(ex, ey)) continue;
        const Candidate candidate = {distance(ex, ey, x, y), id, id};
        insert(list, listed, candidate);
    }
    for (int index = 0; index < listed; ++index) {
        const unsigned int address = buildingAddress(list[index].unit);
        int ex = 0, ey = 0;
        buildingTile(address, ex, ey);
        if (!reaches(memo, fromArea, area(tileAt(ex, ey)))) continue;
        building = list[index].unit;
        uid = at<unsigned int>(address + 0xD8);
        bx = ex;
        by = ey;
        return true;
    }
    return false;
}

bool nearestWall(Reach& memo, int castle, int fromArea, int x, int y, int& wall, int& wx, int& wy)
{
    static const int sides[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    int best = -1;
    for (int ty = y - WallSearch < 0 ? 0 : y - WallSearch; ty <= y + WallSearch && ty < 400; ++ty)
        for (int tx = x - WallSearch < 0 ? 0 : x - WallSearch; tx <= x + WallSearch && tx < 400; ++tx) {
            const int tile = tileAt(tx, ty);
            if (!wallOf(tile, castle, true)) continue;
            const int d = distance(tx, ty, x, y);
            if (best >= 0 && d >= best) continue;
            for (int side = 0; side < 4; ++side) {
                if (!reaches(memo, fromArea, area(tileAt(tx + sides[side][0], ty + sides[side][1])))) continue;
                best = d;
                wall = tile;
                wx = tx;
                wy = ty;
                break;
            }
        }
    return best >= 0;
}

bool splitTargetHolds(int player, const LordHuntState& state)
{
    if (!state.splitGroup) return false;
    const DetachedGroup& group = detachedGroups[player][state.splitGroup - 1];
    if (state.splitOrder == SplitAttacksBuilding) {
        if (group.target <= 0 || group.target >= static_cast<int>(nativeBindings.buildingCapacity)) return false;
        const unsigned int address = buildingAddress(group.target);
        return at<short>(address + 0xD0) != 0 && at<unsigned int>(address + 0xD8) == group.targetUID
            && at<short>(address + 0xD6) == state.splitTo;
    }
    return state.splitOrder == SplitAttacksWall && wallOf(group.target, state.splitTo, false);
}

// The split troops have no siege plan: the melee troops attack the nearest
// building of that castle they can walk to, then the next, else the nearest
// wall they can stand next to, else they walk to the keep. Ranged troops
// walk with them.
void orderSplit(void* aic, int player, LordHuntState& state, Reach& memo)
{
    if (!state.splitTo) return;
    if (state.splitGroup && detachedGroupSize(player, state.splitGroup - 1) <= 0)
        handBack(aic, player, state, state.splitGroup);
    if (state.shooterGroup && detachedGroupSize(player, state.shooterGroup - 1) <= 0)
        handBack(aic, player, state, state.shooterGroup);
    if (!state.splitGroup && !state.shooterGroup) {
        endSplit(aic, player, state);
        return;
    }
    const int size = (state.splitGroup ? detachedGroupSize(player, state.splitGroup - 1) : 0)
        + (state.shooterGroup ? detachedGroupSize(player, state.shooterGroup - 1) : 0);
    if (state.splitSize && size > state.splitSize) state.splitOrder = NoSplitOrder;
    state.splitSize = size;
    if (splitTargetHolds(player, state)) return;
    const unsigned int tick = at<unsigned int>(nativeBindings.gameTick);
    if (state.splitOrder == SplitMoves && tick - state.splitSearchTick < ReorderTicks) return;
    const int lead = detachedGroupLeader(player, (state.splitGroup ? state.splitGroup : state.shooterGroup) - 1);
    if (!lead) return;
    state.splitSearchTick = tick;
    const int lx = at<short>(unitAddress(lead) + 0xC4), ly = at<short>(unitAddress(lead) + 0xC6);
    const int fromArea = unitArea(lead);
    DetachedGroup* melee = state.splitGroup ? &detachedGroups[player][state.splitGroup - 1] : 0;
    DetachedGroup* shooters = state.shooterGroup ? &detachedGroups[player][state.shooterGroup - 1] : 0;
    int x = 0, y = 0;
    bool ordered = false;
    if (melee) {
        int target = 0, tx = 0, ty = 0;
        unsigned int uid = 0;
        if (nearestBuilding(memo, state.splitTo, fromArea, lx, ly, target, uid, tx, ty)) {
            order(melee->tribe, OrderAttackBuilding, target, uid);
            state.splitOrder = SplitAttacksBuilding;
            ordered = true;
        } else if (nearestWall(memo, state.splitTo, fromArea, lx, ly, target, tx, ty)) {
            order(melee->tribe, OrderAttackWall, target, 0);
            state.splitOrder = SplitAttacksWall;
            ordered = true;
        }
        if (ordered) {
            melee->target = target;
            melee->targetUID = uid;
            melee->x = x = tx;
            melee->y = y = ty;
        }
    }
    if (!ordered) {
        if (state.splitOrder == SplitMoves) return;
        x = field(state.splitTo, 0x98);
        y = field(state.splitTo, 0x9C);
        if (melee) {
            moveGroup(melee->tribe, x, y);
            melee->target = state.splitTo;
            melee->targetUID = 0;
            melee->x = x;
            melee->y = y;
        }
        state.splitOrder = SplitMoves;
    }
    if (melee) aggressive(melee->tribe);
    if (shooters) {
        moveGroup(shooters->tribe, x, y);
        shooters->x = x;
        shooters->y = y;
        aggressive(shooters->tribe);
    }
}

void startHunt(void* aic, int player, int share, int wave, int source, LordHuntState& state,
    const LordHuntCensus& census, const Lord& lord, Reach& memo)
{
    const int lordArea = area(lord.tile);
    Candidate list[LordHuntCandidateCapacity];
    int reachable = 0, candidates = 0;
    const int splitTribe = state.splitGroup ? detachedGroups[player][state.splitGroup - 1].tribe : -1;
    const int shooterTribe = state.shooterGroup ? detachedGroups[player][state.shooterGroup - 1].tribe : -1;
    for (int index = 0; index < census.candidateCount; ++index) {
        const int entry = census.candidates[index];
        if (entry >> 16 != source) continue;
        const int unit = entry & 0xFFFF;
        const unsigned int address = unitAddress(unit);
        const int tribe = at<short>(address + 0x2D8);
        if (source == MainArmySource ? tribe <= 0 || tribe >= TribeCapacity || slotOwner[tribe] != player
                : tribe != splitTribe && tribe != shooterTribe) continue;
        ++candidates;
        if (!reaches(memo, unitArea(unit), lordArea)) continue;
        const Candidate candidate = {huntRank(at<short>(address + 0x42A)),
            distance(at<short>(address + 0xC4), at<short>(address + 0xC6), lord.x, lord.y), unit};
        insert(list, reachable, candidate);
    }
    if (!reachable) return;
    int chosen = reachable;
    if (wave == 1) {
        chosen = (candidates * share + 99) / 100;
        if (chosen > reachable) chosen = reachable;
    }
    const int index = openDetachedGroup(player, HuntDuty);
    if (index < 0) return;
    DetachedGroup& group = detachedGroups[player][index];
    group.target = lord.unit;
    group.targetUID = lord.uid;
    group.x = lord.x;
    group.y = lord.y;
    int moved = 0;
    for (int candidate = 0; candidate < chosen; ++candidate)
        if (moveUnitToDetachedGroup(player, index, list[candidate].unit)) ++moved;
    state.huntGroup = index + 1;
    if (!moved) {
        handBack(aic, player, state, state.huntGroup);
        return;
    }
    order(group.tribe, OrderAttackUnit, lord.unit, lord.uid);
    aggressive(group.tribe);
    diagnostic("player %d: wave %d, %d of %d troops hunt the lord of player %d", player, wave, moved, candidates,
        state.origin);
    state.huntWave = wave;
    state.huntSource = source;
    state.huntOrderTick = at<unsigned int>(nativeBindings.gameTick);
    state.huntClosedTick = 0;
    if (source == SplitTroopSource) state.splitOrder = NoSplitOrder;
}

void huntLord(void* aic, int player, int share, LordHuntState& state, const LordHuntCensus& census,
    const Lord& lord, Reach& memo)
{
    const int lordArea = area(lord.tile);
    const unsigned int tick = at<unsigned int>(nativeBindings.gameTick);
    if (state.huntGroup && detachedGroupSize(player, state.huntGroup - 1) > 0) {
        const int leader = detachedGroupLeader(player, state.huntGroup - 1);
        if (!leader || !reaches(memo, unitArea(leader), lordArea)) {
            if (!state.huntClosedTick) state.huntClosedTick = tick ? tick : 1;
            if (tick - state.huntClosedTick >= ClosedTicks) {
                diagnostic("player %d: the hunters cannot reach the lord of player %d; they return", player,
                    state.origin);
                handBack(aic, player, state, state.huntGroup);
                state.huntWave = 0;
                state.huntClosedTick = 0;
            }
            return;
        }
        state.huntClosedTick = 0;
        const unsigned int address = unitAddress(leader);
        if (tick - state.huntOrderTick >= ReorderTicks
            && (at<short>(address + 0x39C) != OrderAttackUnit || at<short>(address + 0x39E) != lord.unit)) {
            order(detachedGroups[player][state.huntGroup - 1].tribe, OrderAttackUnit, lord.unit, lord.uid);
            state.huntOrderTick = tick;
        }
        return;
    }
    int source = state.splitTo && keepArea(state.splitTo) == lordArea ? SplitTroopSource : MainArmySource;
    if (state.huntWave) {
        // Every hunter is gone: the rest of that army goes, once.
        const int wave = state.huntWave;
        source = state.huntSource;
        handBack(aic, player, state, state.huntGroup);
        if (wave >= 2) {
            diagnostic("player %d: the second wave of hunters is gone; the hunt ends", player);
            state.phase |= LordHuntOver;
            return;
        }
        startHunt(aic, player, share, 2, source, state, census, lord, memo);
        return;
    }
    if (!(state.phase & LordHuntOver)) startHunt(aic, player, share, 1, source, state, census, lord, memo);
}
} // namespace

void resetLordHuntCensus()
{
    std::memset(lordHuntCensus, 0, sizeof(lordHuntCensus));
    std::memset(lordHuntUnits, 0, sizeof(lordHuntUnits));
    std::memset(slotOwner, 0, sizeof(slotOwner));
    std::memset(collecting, 0, sizeof(collecting));
    std::memset(splitTribes, 0, sizeof(splitTribes));
    for (int player = 1; player <= 8; ++player) {
        // Runs right after the owner dropped the records of vanished tribes.
        LordHuntState& state = lordHunts[player];
        dropFreed(player, state.huntGroup, HuntDuty);
        dropFreed(player, state.splitGroup, SplitDuty);
        dropFreed(player, state.shooterGroup, SplitDuty);
        const int ch = character(player);
        const int phase = field(player, 0x2BA4);
        // A running attack record keeps its census while it unwinds switched-off settings.
        if (!ch || (!enabled(ch) && !state.origin) || phase < 1 || phase > 6) continue;
        collecting[player] = 1;
        const unsigned int address = playerAddress(player);
        for (int slot = 0; slot < 200; ++slot) {
            const int tribe = at<short>(address + 0x310C + slot * 2);
            if (tribe > 0 && tribe < TribeCapacity
                && at<unsigned int>(tribeAddress(tribe) + 0x34) == at<unsigned int>(address + 0x329C + slot * 4))
                slotOwner[tribe] = static_cast<short>(player);
        }
        if (state.splitGroup) splitTribes[player][0] = detachedGroups[player][state.splitGroup - 1].tribe;
        if (state.shooterGroup) splitTribes[player][1] = detachedGroups[player][state.shooterGroup - 1].tribe;
    }
}

void countLordHuntUnit(int unit, int player, int type)
{
    if (type != LordType) ++lordHuntUnits[player];
    if (!collecting[player]) return;
    const unsigned int address = unitAddress(unit);
    const int role = at<short>(address + 0x42A);
    if (role < 10 || role > 20) return;
    LordHuntCensus& census = lordHuntCensus[player];
    ++census.attackTroops;
    if (!huntRank(role) || leftAlone(type) || census.candidateCount == LordHuntCandidateCapacity) return;
    const int tribe = at<short>(address + 0x2D8);
    if (tribe <= 0 || tribe >= TribeCapacity) return;
    int source;
    if (slotOwner[tribe] == player) source = MainArmySource;
    else if (tribe == splitTribes[player][0] || tribe == splitTribes[player][1]) source = SplitTroopSource;
    else return;
    census.candidates[census.candidateCount++] = unit | (source << 16);
}

SiegeRetarget updateLordHunt(void* aic, int player)
{
    SiegeRetarget change = {0, 0};
    if (player < 1 || player > 8) return change;
    LordHuntState& state = lordHunts[player];
    if (state.returning) returnGroups(aic, player, state);
    const int ch = character(player);
    const int phase = field(player, 0x2BA4);
    const int target = field(player, 0x2BD8);
    Lord lord = {0, 0, 0, 0, 0};
    if (!state.origin) {
        // An attack on a human player is followed once the native plan exists
        // and the census has counted its army. The native update starts an
        // attack in state 0 and plans it to state 2 in one call, so the census
        // before that call holds no attack troops.
        if (!ch || !enabled(ch) || phase < 2 || phase > 6 || !human(target) || !liveLord(target, lord)
            || !combatCensusFresh(at<unsigned int>(nativeBindings.gameTick)) || !lordHuntCensus[player].attackTroops)
            return change;
        state.origin = target;
        state.originLordUID = static_cast<int>(lord.uid);
    }
    // The attack ended, its opponent changed, or the lord is gone.
    if (!ch || phase < 1 || phase > 6 || (target != state.origin && target != state.host)
        || !liveLord(state.origin, lord) || static_cast<int>(lord.uid) != state.originLordUID)
        return finish(aic, player, state);
    if (!combatCensusFresh(at<unsigned int>(nativeBindings.gameTick))) return change;
    const LordHuntCensus& census = lordHuntCensus[player];
    const int force = at<int>(nativeBindings.aicRecords + (ch - 1) * 676 + 0x1F4);
    if (census.attackTroops > state.peak) state.peak = census.attackTroops;
    if (force > state.peak) state.peak = force;
    if (state.host && target == state.origin) change.castle = state.host;
    Reach memo;
    memo.player = player;
    memo.count = 0;
    if (setting(lordHuntRetarget, ch, lordHuntRetargetFallback) == 1) {
        if (!(state.phase & LordHuntHidingDecided)) checkHiding(player, ch, phase, state, census, lord, memo, change);
        else checkShelter(aic, player, ch, phase, state, census, lord, change);
    } else {
        // Switched off during the attack, e.g. before a reload.
        endSplit(aic, player, state);
        if (state.host) turnBack(ch, phase, state, census, change);
    }
    orderSplit(aic, player, state, memo);
    const int share = setting(lordHuntShare, ch, lordHuntShareFallback);
    if (share > 0) huntLord(aic, player, share, state, census, lord, memo);
    else {
        handBack(aic, player, state, state.huntGroup);
        state.huntWave = 0;
    }
    return change;
}

int lordHuntSiegeCastle(int player, int opponent)
{
    if (player < 1 || player > 8) return opponent;
    const LordHuntState& state = lordHunts[player];
    const int ch = character(player);
    return state.host && ch && setting(lordHuntRetarget, ch, lordHuntRetargetFallback) == 1
        && (opponent == state.origin || opponent == state.host) ? state.host : opponent;
}

} // namespace SHC141
} // namespace AicTactics
