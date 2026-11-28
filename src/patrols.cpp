#include "aic_tactics/patrols.hpp"
#include "aic_tactics/diagnostics.hpp"
#include "aic_tactics/patrol_geometry.hpp"
#include "aic_tactics/runtime.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

int patrolOrders[17];
int patrolFormation[17];
int patrolSpacing[17];
int patrolExtraGroups[17];
int patrolMinSize[17];
int patrolRadius[17];
int siegeScreen[17];
int siegeScreenForward[17];
int siegeScreenExtraGroups[17];
int patrolOrdersFallback = 1;
int patrolFormationFallback = RoutesPatrolFormation;
int patrolSpacingFallback = 12;
int patrolExtraGroupsFallback = 0;
int patrolMinSizeFallback = 4;
int patrolRadiusFallback = 12;
int siegeScreenFallback = 1;
int siegeScreenForwardFallback = 4;
int siegeScreenExtraGroupsFallback = 0;
PatrolRoute patrolRoutes[9][PatrolRoutesPerPlayer];

namespace {
const unsigned int PlayerStride = 0x39F4;
const int FirstPatrolSlot = 15;
const int SecondPatrolSlot = 16;
const int FirstSiegeDefenceSlot = 190;
const int SecondSiegeDefenceSlot = 191;
const int PatrolRole = 16;
const int SiegeDefenceRole = 19;
const int TribeCapacity = 1250;
// Packed isometric tile count of a 400x400 map (OpenSHC Map/TileMapState.hpp),
// also the size of one distance layer.
const int TileCapacity = 80400;
// Per-player pathing state stride holding each AI's distance layer index.
const unsigned int DistanceLayerPlayerStride = 0x177BC;
// Unit field the native siege-defence rally reads as "busy fighting" when <= 0x60.
const unsigned int UnitClosestEnemy = 0x2A6;
const int FightRange = 0x60;
// Smarter Patrols' tested route timing and tolerances.
const unsigned int PatrolStepTicks = 1600;
const int PatrolArrived = 5;
const int PatrolRingSlack = 6;
const int PatrolRayLength = 180;
// Fixed work per AI update across every non-native patrol group.
const int PatrolPathBudget = 6;
const int PatrolOrderBudget = 3;
const int PatrolMoveBudget = 16;
const int PatrolReturnBudget = 16;
const int PatrolRetreatBudget = 64;
// Unit types that shoot: archer, crossbowman, Arabian archer, slinger, horse archer, fire thrower.
bool ranged(int type)
{
    return type == 22 || type == 23 || type == 70 || type == 72 || type == 74 || type == 76;
}

template<class T> T& at(unsigned int address) { return *reinterpret_cast<T*>(address); }
typedef void (__thiscall *PlayerAction)(void*, int);
typedef void (__thiscall *TribeAction)(void*, int);
typedef void (__thiscall *TribeReturn)(void*, int, int);
typedef int (__thiscall *TribeQuery)(void*, int, int);
typedef int (__thiscall *TribeMove)(void*, int, int, int, int, int, int);

struct Budget { int paths; int orders; int moves; };
// One patrol group in fixed order: slot 15, slot 16, then detached records by index.
struct Group { int tribe; TilePoint leader; int route; };
// A split pool entry: a native slot (record -1) or a detached patrol record.
struct Pool { int tribe; int record; int size; int word; int bit; };

unsigned int playerAddress(int player) { return nativeBindings.players + player * PlayerStride; }
unsigned int tribeAddress(int tribe) { return nativeBindings.tribes + tribe * nativeBindings.tribeStride; }
unsigned int unitAddress(int unit) { return nativeBindings.unitRecords + unit * 0x490; }
unsigned int tick() { return at<unsigned int>(nativeBindings.gameTick); }
int tribeSize(int tribe) { return at<short>(tribeAddress(tribe) + 0x5C); }

int character(int player)
{
    return at<int>(playerAddress(player) + 0x2300) - 1;
}

bool patrolsOn(int player, int& ai)
{
    ai = character(player);
    return ai >= 1 && ai <= 16 && setting(patrolOrders, ai, patrolOrdersFallback) == 1;
}

// A living leader whose unit record links back to its tribe and player.
bool leaderPosition(int player, int tribe, TilePoint& position)
{
    const unsigned int address = tribeAddress(tribe);
    const int leader = at<short>(address + 0x5A);
    if (leader <= 0 || leader >= static_cast<int>(nativeBindings.unitCapacity)) return false;
    const unsigned int unit = unitAddress(leader);
    if (at<short>(unit + 0x2D8) != tribe || at<unsigned int>(unit + 0x2E4) != at<unsigned int>(address + 0x34)
        || at<short>(unit + 0x96) != player || at<short>(unit + 0x8C) != 2
        || at<short>(unit + 0x2A0) != 0 || at<int>(unit + 0x3C8) <= 0) return false;
    position.x = at<short>(unit + 0xC4);
    position.y = at<short>(unit + 0xC6);
    return position.x > 0 && position.x < 399 && position.y > 0 && position.y < 399;
}

// The native dispatcher's slot checks (UID still matches) plus owner and lifecycle.
int slotTribe(int player, int slot)
{
    const int tribe = at<short>(playerAddress(player) + 0x310C + slot * 2);
    if (tribe <= 0 || tribe >= TribeCapacity) return 0;
    const unsigned int address = tribeAddress(tribe);
    if (at<unsigned int>(address + 0x34) != at<unsigned int>(playerAddress(player) + 0x329C + slot * 4)
        || at<int>(address + 0x2C) != player || at<short>(address + 0x40) != 2) return 0;
    return tribe;
}

// A valid slot tribe with a valid leader, so a recycled record or a dying
// leader is never used.
int patrolTribe(int player, int slot, TilePoint& leader)
{
    const int tribe = slotTribe(player, slot);
    return tribe && leaderPosition(player, tribe, leader) ? tribe : 0;
}

bool patrolRecord(int player, int index)
{
    return detachedGroups[player][index].duty == PatrolDuty && detachedGroupValid(player, index);
}

// The keep's entrance tile (BuildingEntryInfo x/y), as the AI's home checks use.
bool keepPosition(int player, TilePoint& position)
{
    if (player < 1 || player > 8 || at<int>(playerAddress(player) + 0x94) == 0) return false;
    position.x = at<int>(playerAddress(player) + 0x98);
    position.y = at<int>(playerAddress(player) + 0x9C);
    return position.x > 0 && position.x < 399 && position.y > 0 && position.y < 399;
}

bool playable(TilePoint point)
{
    return point.x > 0 && point.x < 399 && point.y > 0 && point.y < 399
        && at<unsigned char>(nativeBindings.playableTiles + point.y * 400 + point.x) != 0;
}

// The same playable-map test the native move instruction applies first.
int stationTile(TilePoint station)
{
    if (!playable(station)) return 0;
    const int tile = at<int>(nativeBindings.mapRows + station.y * 12) + station.x;
    return tile > 0 && tile < TileCapacity ? tile : 0;
}

void nativeDestination(int tribe)
{
    reinterpret_cast<TribeAction>(nativeBindings.attackPatrolDestination)(
        reinterpret_cast<void*>(nativeBindings.troopValueState), tribe);
}

enum Attempt { Ordered, Rejected, OutOfBudget };

// One candidate station: playable map, the group's own path query, then the
// group move.
Attempt tryStation(void* owner, int tribe, TilePoint station, Budget& budget)
{
    const int tile = stationTile(station);
    if (!tile) return Rejected;
    if (budget.paths <= 0 || budget.orders <= 0) return OutOfBudget;
    --budget.paths;
    if (!reinterpret_cast<TribeQuery>(nativeBindings.tribePath)(owner, tribe, tile)) return Rejected;
    --budget.orders;
    return moveGroup(tribe, station.x, station.y) ? Ordered : Rejected;
}

// Wing and Mirror stations. Group 1 takes the station beside slot 15 (Wing) or
// across the keep (Mirror); every later group stands `spacing` beside the group
// before it (Wing), or before the one before it (Mirror), away from the group
// that one follows, so Wing forms one line and Mirror two. Returns true when the
// group was handled (ordered, at its station, or out of budget this update).
bool stationGroup(void* aic, int player, int ai, int formation, const Group* groups, int index, Budget& budget)
{
    TilePoint keep, home;
    const int target = at<int>(playerAddress(player) + 0x2BD8);
    if (!groups[0].tribe || target == player || !keepPosition(target, keep)) return false;
    const Group& self = groups[index];
    const int spacing = patrolSpacing[ai] ? patrolSpacing[ai] : patrolSpacingFallback;
    TilePoint stations[2];
    int count = 0;
    if (index == 1 && formation == WingPatrolFormation) {
        count = wingPatrolStations(groups[0].leader, keep, self.leader, spacing, stations);
    } else if (index == 1) {
        if (!keepPosition(player, home)) return false;
        count = mirrorPatrolStations(groups[0].leader, keep, home, stations);
    } else {
        const int anchor = formation == WingPatrolFormation ? index - 1 : index - 2;
        const int behind = formation == WingPatrolFormation ? index - 2 : index - 4;
        TilePoint prefer = self.leader;
        if (behind >= 0) {
            prefer.x = 2 * groups[anchor].leader.x - groups[behind].leader.x;
            prefer.y = 2 * groups[anchor].leader.y - groups[behind].leader.y;
        }
        count = wingPatrolStations(groups[anchor].leader, keep, prefer, spacing, stations);
    }
    // Do not restart a march that has already reached its station.
    for (int station = 0; station < count; ++station)
        if (patrolDistance(stations[station], self.leader) <= 3) return true;
    for (int station = 0; station < count; ++station) {
        const Attempt attempt = tryStation(aic, self.tribe, stations[station], budget);
        if (attempt != Rejected) return true;
    }
    return false;
}

unsigned int distanceLayer(int player)
{
    const int index = at<int>(nativeBindings.distanceLayerIndex + player * DistanceLayerPlayerStride);
    return index >= 0 && index <= 8 ? nativeBindings.distanceLayers + index * TileCapacity : 0;
}

// The first playable tile outward along a ring direction whose walking distance
// from the target castle (the AI's distance layer, as the native patrol search
// reads it) lies within the ring.
bool ringPoint(unsigned int layer, TilePoint keep, int direction, int radius, TilePoint& out)
{
    for (int step = 1; step <= PatrolRayLength; ++step) {
        const TilePoint point = patrolRingStep(keep, direction, step);
        if (point.x < 0 || point.x >= 400 || point.y < 0 || point.y >= 400) return false;
        if (!playable(point)) continue;
        const int tile = at<int>(nativeBindings.mapRows + point.y * 12) + point.x;
        if (tile <= 0 || tile >= TileCapacity) continue;
        const int value = at<unsigned char>(layer + tile);
        if (value >= radius && value <= radius + PatrolRingSlack) {
            out = point;
            return true;
        }
    }
    return false;
}

int routeDirection(int kind, int front, int point)
{
    return kind == FrontSweepRoute ? frontSweepDirection(front, point) : point;
}

// The route point nearest the group's leader: where a new route starts, and
// where it resumes after the native search had the group.
void nearestRoutePoint(int kind, int front, TilePoint keep, TilePoint leader, PatrolRoute& route)
{
    const int direction = patrolRingDirection(leader.x - keep.x, leader.y - keep.y);
    if (kind != FrontSweepRoute) {
        route.point = direction < 0 ? front : direction;
        return;
    }
    int offset = direction < 0 ? 0 : patrolRingWrap(direction - front + PatrolRingDirections / 2)
        - PatrolRingDirections / 2;
    if (offset < -FrontSweepSpan) offset = -FrontSweepSpan;
    if (offset > FrontSweepSpan) offset = FrontSweepSpan;
    route.point = offset + FrontSweepSpan;
}

// Smarter Patrols' routes: the next point on arrival; after PatrolStepTicks
// without getting closer (not counting time in a fight) one alternate point;
// after a second stall, or when both candidates are rejected, the native search
// for PatrolStepTicks before the route resumes from the point nearest the leader.
void routeGroup(void* aic, int player, int ai, int kind, int round, const Group& group, Budget& budget)
{
    TilePoint keep, home;
    const int target = at<int>(playerAddress(player) + 0x2BD8);
    const unsigned int layer = distanceLayer(player);
    if (target == player || !keepPosition(target, keep) || !layer) {
        nativeDestination(group.tribe);
        return;
    }
    if (!keepPosition(player, home)) home = group.leader;
    int front = patrolRingDirection(home.x - keep.x, home.y - keep.y);
    if (front < 0) front = 0;
    const unsigned int uid = at<unsigned int>(tribeAddress(group.tribe) + 0x34);
    const unsigned int now = tick();
    PatrolRoute& route = patrolRoutes[player][group.route];
    if (route.uid != uid || route.target != target) {
        std::memset(&route, 0, sizeof(route));
        route.uid = uid;
        route.target = target;
        if (kind == FrontSweepRoute && round % 2) {
            // A later round sweeps from the other end.
            route.point = FrontSweepPositions - 1;
            route.step = -1;
        } else {
            route.step = kind == CounterclockwiseRoute ? -1 : 1;
            nearestRoutePoint(kind, front, keep, group.leader, route);
            if (kind != FrontSweepRoute) route.point = patrolRingWrap(route.point - route.step * RouteLag * round);
        }
    }
    // A group whose route changed (an earlier group is gone) keeps its saved point.
    if (kind == FrontSweepRoute && (route.point < 0 || route.point >= FrontSweepPositions))
        nearestRoutePoint(kind, front, keep, group.leader, route);
    if (kind == ClockwiseRoute) route.step = 1;
    if (kind == CounterclockwiseRoute) route.step = -1;
    if (route.stalls >= 2) {
        if (now - route.tick < PatrolStepTicks) {
            nativeDestination(group.tribe);
            return;
        }
        route.stalls = 0;
        route.x = route.y = 0;
        nearestRoutePoint(kind, front, keep, group.leader, route);
    }
    if (route.x) {
        TilePoint point;
        point.x = route.x;
        point.y = route.y;
        const int distance = patrolDistance(group.leader, point);
        const int closest = at<short>(unitAddress(at<short>(tribeAddress(group.tribe) + 0x5A)) + UnitClosestEnemy);
        if (distance < route.distance || closest <= FightRange) {
            if (distance < route.distance) route.distance = distance;
            route.tick = now;
        }
        if (distance > PatrolArrived && now - route.tick <= PatrolStepTicks) return;
        if (distance <= PatrolArrived) route.stalls = 0;
        else if (++route.stalls >= 2) {
            route.tick = now;
            nativeDestination(group.tribe);
            return;
        }
        nextRoutePoint(kind, route.point, route.step);
        route.x = route.y = 0;
    }
    const int radius = setting(patrolRadius, ai, patrolRadiusFallback);
    int point = route.point, step = route.step;
    for (int candidate = 0; candidate < 2; ++candidate) {
        TilePoint station;
        if (ringPoint(layer, keep, routeDirection(kind, front, point), radius, station)) {
            const Attempt attempt = tryStation(aic, group.tribe, station, budget);
            if (attempt == OutOfBudget) return;
            if (attempt == Ordered) {
                route.point = point;
                route.step = step;
                route.x = station.x;
                route.y = station.y;
                route.distance = patrolDistance(group.leader, station);
                route.tick = now;
                if (group.route > 0) {
                    detachedGroups[player][group.route - 1].x = station.x;
                    detachedGroups[player][group.route - 1].y = station.y;
                }
                return;
            }
        }
        nextRoutePoint(kind, point, step);
    }
    route.stalls = 2;
    route.tick = now;
    nativeDestination(group.tribe);
}

// `index` counts the groups after native slot 15 (group 1), so group
// g = index + 1 takes route index % 3 in round index / 3.
void orderGroup(void* aic, int player, int ai, int formation, const Group* groups, int index, Budget& budget)
{
    // The native dispatcher sets this stance on its patrol tribe before ordering it.
    at<short>(tribeAddress(groups[index].tribe) + nativeBindings.tribeStance) = 1;
    if (formation == RoutesPatrolFormation) {
        routeGroup(aic, player, ai, index % 3, index / 3, groups[index], budget);
        return;
    }
    if ((formation == WingPatrolFormation || formation == MirrorPatrolFormation)
        && stationGroup(aic, player, ai, formation, groups, index, budget)) return;
    nativeDestination(groups[index].tribe);
}

const char* dutyName(int duty) { return duty == SiegeDefenceDuty ? "siege defence" : "patrol"; }

// Returns the members handed back.
int handBack(void* aic, int player, int index, int budget)
{
    const int before = detachedGroupSize(player, index);
    const char* const name = dutyName(detachedGroups[player][index].duty);
    const int left = returnDetachedGroup(aic, player, index, budget);
    if (detachedGroups[player][index].duty == NoDuty) {
        std::memset(&patrolRoutes[player][1 + index], 0, sizeof(PatrolRoute));
        diagnostic("player %d: extra %s group %d is back in the AI's troops", player, name, index + 1);
    }
    return before > left ? before - left : 0;
}

// The highest-numbered member of the role below the entry's cursor, except the leader.
int nextMember(Pool& entry, int role)
{
    const int leader = at<short>(tribeAddress(entry.tribe) + 0x5A);
    for (; entry.word >= 0; --entry.word, entry.bit = 15) {
        const unsigned int bits = at<unsigned short>(tribeAddress(entry.tribe) + 0x60 + entry.word * 2);
        for (; entry.bit >= 0; --entry.bit) {
            if (!(bits & (1U << entry.bit))) continue;
            const int unit = entry.word * 16 + entry.bit;
            if (unit == leader || unit <= 0 || unit >= static_cast<int>(nativeBindings.unitCapacity)
                || at<short>(unitAddress(unit) + 0x2D8) != entry.tribe
                || at<short>(unitAddress(unit) + 0x42A) != role) continue;
            --entry.bit;
            return unit;
        }
    }
    return 0;
}

// Splits the two native slots of a role (patrol 15-16, siege defence 190-191)
// into `extra` detached groups of the duty and keeps them balanced (Smarter
// Patrols' sizing: at most the wanted number of groups while each keeps the
// minimum size; tight balance while a group is still below it, then within
// the minimum). Units only move into detached groups: the native slots refill
// from the game's own recruitment. Surplus groups go back to the native
// slots, smallest first.
void organizeDetachedGroups(void* aic, int player, int ai, int duty, int firstSlot, int role, int extra,
    Budget& budget)
{
    int minimum = setting(patrolMinSize, ai, patrolMinSizeFallback);
    if (minimum < 1) minimum = 1;
    Pool pool[2 + DetachedGroupsPerPlayer];
    int count = 0, slots = 0, live = 0, total = 0;
    const int words = static_cast<int>(nativeBindings.tribeMemberWords);
    for (int slot = firstSlot; slot <= firstSlot + 1; ++slot) {
        const int tribe = slotTribe(player, slot);
        if (!tribe) continue;
        const Pool entry = {tribe, -1, tribeSize(tribe), words - 1, 15};
        pool[count++] = entry;
        total += entry.size;
        ++slots;
    }
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index) {
        if (detachedGroups[player][index].duty != duty) continue;
        if (!detachedGroupValid(player, index) || detachedGroups[player][index].returning) {
            budget.moves -= handBack(aic, player, index, budget.moves);
            continue;
        }
        const Pool entry = {detachedGroups[player][index].tribe, index,
            detachedGroupSize(player, index), words - 1, 15};
        pool[count++] = entry;
        total += entry.size;
        ++live;
    }
    int desired = 0, opened = -1;
    if (extra > 0 && slots > 0) {
        int wanted = slots + extra;
        const int fit = total / minimum > 1 ? total / minimum : 1;
        if (wanted > fit) wanted = fit;
        desired = wanted > slots ? wanted - slots : 0;
    }
    if (live > desired) {
        int smallest = -1;
        for (int entry = 0; entry < count; ++entry)
            if (pool[entry].record >= 0 && (smallest < 0 || pool[entry].size <= pool[smallest].size))
                smallest = entry;
        budget.moves -= handBack(aic, player, pool[smallest].record, budget.moves);
        pool[smallest] = pool[--count];
        --live;
    } else if (live < desired) {
        const int index = openDetachedGroup(player, duty);
        if (index >= 0) {
            opened = index;
            detachedGroups[player][index].target = at<int>(playerAddress(player) + 0x2BD8);
            const Pool entry = {detachedGroups[player][index].tribe, index, 0, words - 1, 15};
            pool[count++] = entry;
            ++live;
        }
    }
    if (!live) return;
    int tolerance = minimum;
    for (int entry = 0; entry < count; ++entry)
        if (pool[entry].record >= 0 && pool[entry].size < minimum) tolerance = 1;
    while (budget.moves > 0) {
        int big = 0, small = -1;
        for (int entry = 0; entry < count; ++entry) {
            if (pool[entry].size > pool[big].size) big = entry;
            if (pool[entry].record >= 0 && (small < 0 || pool[entry].size < pool[small].size)) small = entry;
        }
        if (small < 0 || pool[big].size - pool[small].size <= tolerance) break;
        const int unit = nextMember(pool[big], role);
        if (!unit) break;
        --budget.moves;
        if (!moveUnitToDetachedGroup(player, pool[small].record, unit)) continue;
        --pool[big].size;
        ++pool[small].size;
    }
    if (opened >= 0)
        diagnostic("player %d: extra %s group %d opens (%d troops)", player, dutyName(duty), opened + 1,
            detachedGroupSize(player, opened));
}

bool recordsExist(int player, int duty)
{
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index)
        if (detachedGroups[player][index].duty == duty) return true;
    return false;
}

bool siegeDefenceRecord(int player, int index)
{
    return detachedGroups[player][index].duty == SiegeDefenceDuty && detachedGroupValid(player, index)
        && !detachedGroups[player][index].returning;
}

// One siege-defence group's station at `engine`: in front of it toward the
// target keep for a melee leader, then, with `atEngine`, the engine itself.
// Returns true when ordered.
bool stationSiegeDefence(void* tribes, int player, int ai, int tribe, TilePoint engine, bool atEngine)
{
    TilePoint candidates[2], keep, station;
    int count = 0;
    const int type = at<short>(unitAddress(at<short>(tribeAddress(tribe) + 0x5A)) + 0x8E);
    if (!ranged(type) && keepPosition(at<int>(playerAddress(player) + 0x2BD8), keep)
        && siegeScreenStation(engine, keep, setting(siegeScreenForward, ai, siegeScreenForwardFallback), station))
        candidates[count++] = station;
    if (atEngine) candidates[count++] = engine;
    Budget budget = {2, 2, 0};
    for (int candidate = 0; candidate < count; ++candidate)
        if (tryStation(tribes, tribe, candidates[candidate], budget) == Ordered) return true;
    return false;
}

// The siege-defence groups: the two native slots and the live detached ones.
int siegeDefenceGroups(int player)
{
    int groups = 2;
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index)
        if (siegeDefenceRecord(player, index)) ++groups;
    return groups;
}

// The engine of group `order` (0 = slot 190) of `groups`: the groups spread
// evenly over the tribe's rally list from its current entry.
TilePoint rallyEngine(int tribe, int order, int groups, TilePoint current)
{
    const int points = at<short>(tribeAddress(tribe) + nativeBindings.tribeRallyCount);
    const int index = at<short>(tribeAddress(tribe) + nativeBindings.tribeRallyCount - 2);
    if (points <= 1 || points > 10 || index < 0 || index >= points) return current;
    const unsigned int entry = tribeAddress(tribe) + nativeBindings.tribeRallyPoints
        + ((index + order * points / groups) % points) * 4;
    TilePoint engine;
    engine.x = at<short>(entry);
    engine.y = at<short>(entry + 2);
    return engine;
}

// Slot 190 moved on along its rally list: every detached siege-defence group
// that is not fighting takes its own engine after slot 191's.
void moveDetachedSiegeDefence(void* tribes, int player, int ai, int tribe, int groups, TilePoint current)
{
    int order = 2;
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index) {
        if (!siegeDefenceRecord(player, index)) continue;
        DetachedGroup& group = detachedGroups[player][index];
        const TilePoint engine = rallyEngine(tribe, order++, groups, current);
        TilePoint leader;
        if (!leaderPosition(player, group.tribe, leader)
            || at<short>(unitAddress(at<short>(tribeAddress(group.tribe) + 0x5A)) + UnitClosestEnemy) <= FightRange)
            continue;
        // The native rally sets this stance on the slot tribes it orders.
        at<short>(tribeAddress(group.tribe) + nativeBindings.tribeStance) = 2;
        if (stationSiegeDefence(tribes, player, ai, group.tribe, engine, true)) {
            group.x = engine.x;
            group.y = engine.y;
        }
    }
}
} // namespace

void __fastcall dispatchAttackPatrols(void* aic, void*, int player)
{
    // Native slot 15 keeps its original order, timing and searches.
    reinterpret_cast<PlayerAction>(nativeBindings.attackPatrolDispatch)(aic, player);
    int ai;
    if (player < 1 || player > 8 || !patrolsOn(player, ai)) return;
    // A running match fixes the configuration, as the census does for others.
    configurationLocked = 1;
    Budget budget = {PatrolPathBudget, PatrolOrderBudget, PatrolMoveBudget};
    organizeDetachedGroups(aic, player, ai, PatrolDuty, FirstPatrolSlot, PatrolRole,
        setting(patrolExtraGroups, ai, patrolExtraGroupsFallback), budget);
    Group groups[1 + PatrolRoutesPerPlayer];
    std::memset(groups, 0, sizeof(groups));
    int count = 1;
    groups[0].tribe = patrolTribe(player, FirstPatrolSlot, groups[0].leader);
    groups[0].route = -1;
    for (int entry = 0; entry < PatrolRoutesPerPlayer; ++entry) {
        int tribe = 0;
        if (entry == 0) {
            if (!slotTribe(player, SecondPatrolSlot)) continue;
            tribe = slotTribe(player, SecondPatrolSlot);
        } else {
            if (!patrolRecord(player, entry - 1) || detachedGroups[player][entry - 1].returning) continue;
            tribe = detachedGroups[player][entry - 1].tribe;
        }
        Group& group = groups[count];
        if (!leaderPosition(player, tribe, group.leader)) continue;
        group.tribe = tribe;
        group.route = entry;
        ++count;
    }
    const int formation = setting(patrolFormation, ai, patrolFormationFallback);
    for (int index = 1; index < count; ++index) orderGroup(aic, player, ai, formation, groups, index, budget);
}

void __cdecl finishAttackPatrols(void* aic, int player)
{
    if (player < 1 || player > 8) return;
    int ai;
    const int state = at<int>(playerAddress(player) + 0x2BA4);
    if (state >= 3 && state <= 5 && patrolsOn(player, ai)) return;
    if (patrolRoutes[player][0].uid) std::memset(&patrolRoutes[player][0], 0, sizeof(PatrolRoute));
    if (!recordsExist(player, PatrolDuty)) return;
    int budget = PatrolReturnBudget;
    for (int index = 0; index < DetachedGroupsPerPlayer && budget > 0; ++index)
        if (detachedGroups[player][index].duty == PatrolDuty) budget -= handBack(aic, player, index, budget);
}

void __cdecl retreatAttackPatrols(void* aic, int player)
{
    if (player < 1 || player > 8) return;
    std::memset(&patrolRoutes[player][0], 0, sizeof(PatrolRoute));
    if (!recordsExist(player, PatrolDuty) && !recordsExist(player, SiegeDefenceDuty)) return;
    int budget = PatrolRetreatBudget;
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index) {
        const int duty = detachedGroups[player][index].duty;
        if (duty == PatrolDuty || duty == SiegeDefenceDuty) budget -= handBack(aic, player, index, budget);
    }
    // Members beyond the budget walk home as a group and rejoin at later updates.
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index) {
        const int duty = detachedGroups[player][index].duty;
        if ((duty == PatrolDuty || duty == SiegeDefenceDuty) && detachedGroupValid(player, index)
            && detachedGroupSize(player, index) > 0)
            reinterpret_cast<TribeReturn>(nativeBindings.returnTribe)(aic, detachedGroups[player][index].tribe, player);
    }
}

void __cdecl organizeSiegeDefence(void* aic, int player)
{
    if (player < 1 || player > 8) return;
    const int ai = character(player);
    const int state = at<int>(playerAddress(player) + 0x2BA4);
    const int extra = state >= 3 && state <= 4 && ai >= 1 && ai <= 16
        && setting(siegeScreen, ai, siegeScreenFallback) == 1
        ? setting(siegeScreenExtraGroups, ai, siegeScreenExtraGroupsFallback) : 0;
    if (extra > 0) {
        configurationLocked = 1;
        Budget budget = {0, 0, PatrolMoveBudget};
        organizeDetachedGroups(aic, player, ai, SiegeDefenceDuty, FirstSiegeDefenceSlot, SiegeDefenceRole,
            extra, budget);
        return;
    }
    if (!recordsExist(player, SiegeDefenceDuty)) return;
    // The whole group at once, so the wall assault orders every member.
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index)
        if (detachedGroups[player][index].duty == SiegeDefenceDuty)
            handBack(aic, player, index, static_cast<int>(nativeBindings.unitCapacity));
}

int __fastcall screenSiegeDefenceMove(void* tribes, void*, int tribe, int x, int y, int rally, int store, int speed)
{
    const TribeMove move = reinterpret_cast<TribeMove>(nativeBindings.siegeGroupMove);
    if (tribe <= 0 || tribe >= TribeCapacity) return move(tribes, tribe, x, y, rally, store, speed);
    const int player = at<int>(tribeAddress(tribe) + 0x2C);
    const int ai = player >= 1 && player <= 8 ? character(player) : 0;
    if (ai < 1 || ai > 16 || setting(siegeScreen, ai, siegeScreenFallback) != 1)
        return move(tribes, tribe, x, y, rally, store, speed);
    configurationLocked = 1;
    const int slot = slotTribe(player, FirstSiegeDefenceSlot) == tribe ? FirstSiegeDefenceSlot
        : slotTribe(player, SecondSiegeDefenceSlot) == tribe ? SecondSiegeDefenceSlot : 0;
    TilePoint leader;
    if (!slot) return move(tribes, tribe, x, y, rally, store, speed);
    if (!leaderPosition(player, tribe, leader)) return move(tribes, tribe, x, y, rally, store, speed);
    TilePoint engine;
    engine.x = x;
    engine.y = y;
    // The second group and the detached ones walk the same native rally list
    // ahead of the first (half a lap with two groups), so they cover different
    // engines.
    const int groups = siegeDefenceGroups(player);
    if (slot == FirstSiegeDefenceSlot) moveDetachedSiegeDefence(tribes, player, ai, tribe, groups, engine);
    else engine = rallyEngine(tribe, 1, groups, engine);
    // At the native destination itself the native move keeps its rally arguments.
    if (stationSiegeDefence(tribes, player, ai, tribe, engine, engine.x != x || engine.y != y)) return 1;
    return move(tribes, tribe, x, y, rally, store, speed);
}

int validSlotTribe(int player, int slot, TilePoint& leader) { return patrolTribe(player, slot, leader); }
bool tribeLeaderTile(int player, int tribe, TilePoint& position) { return leaderPosition(player, tribe, position); }
int movableTile(TilePoint point) { return stationTile(point); }

} // namespace SHC141
} // namespace AicTactics
