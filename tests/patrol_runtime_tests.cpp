// Executes the attack patrol and siege-defence owner against mock game data and
// mock native routines on both tribe layouts (Crusader 0x334, Extreme 0x688).
#include "aic_tactics/runtime.hpp"
#include "aic_tactics/patrols.hpp"
#include "aic_tactics/patrol_geometry.hpp"
#include <cassert>
#include <cstring>

namespace AicTactics { namespace SHC141 {
NativeBindings nativeBindings;
int configurationLocked;
}}

using namespace AicTactics;
using namespace AicTactics::SHC141;

namespace {
const int Units = 64;
unsigned char players[9 * 0x39F4];
unsigned char units[Units * 0x490];
unsigned char tribes[1250 * 0x688];
unsigned char playable[400 * 400];
unsigned char layers[9 * 80400];
unsigned char layerIndex[9 * 0x177BC];
int rows[400 * 3];
unsigned int tick = 1000;
void* const AIC = reinterpret_cast<void*>(0x1234);
void* const TROOP_VALUES = reinterpret_cast<void*>(0x5678);
int dispatchCalls, destinationCalls, destinationTribe, pathCalls, moveCalls, moveTribe, moveX, moveY;
int failingMoves, blockedTile, returnCalls, returnedTribe, nextTribe;
unsigned int nextUID;
unsigned int rallyPoints, rallyCount;

template<class T> void put(unsigned char* base, unsigned int offset, T value)
{
    *reinterpret_cast<T*>(base + offset) = value;
}
template<class T> T get(unsigned char* base, unsigned int offset)
{
    return *reinterpret_cast<T*>(base + offset);
}
unsigned char* player(int id) { return players + id * 0x39F4; }
unsigned char* unit(int id) { return units + id * 0x490; }
unsigned char* tribe(int id) { return tribes + id * nativeBindings.tribeStride; }
int tileOf(int x, int y) { return y * 200 + x; }

void live(int id, int owner, unsigned int uid)
{
    put<int>(tribe(id), 0x2C, owner);
    put<unsigned int>(tribe(id), 0x34, uid);
    put<short>(tribe(id), 0x40, 2);
}

// Native thiscall routines, called through __thiscall pointers: __fastcall with
// an ignored edx has the same register use and callee stack cleanup.
void __fastcall dispatch(void* aic, void*, int id)
{
    assert(aic == AIC && id >= 1 && id <= 8);
    ++dispatchCalls;
}
void __fastcall destination(void* state, void*, int id)
{
    assert(state == TROOP_VALUES);
    ++destinationCalls;
    destinationTribe = id;
}
int __fastcall path(void*, void*, int id, int tile)
{
    assert(id > 0 && id < 1250 && tile > 0 && tile < 80400);
    ++pathCalls;
    return tile != blockedTile;
}
void __fastcall clearMoveFlags(void* owner, void*, int)
{
    assert(owner == static_cast<void*>(tribes));
}
int __fastcall move(void* owner, void*, int id, int x, int y, int rally, int store, int speed)
{
    assert(owner == static_cast<void*>(tribes) && !rally && !store && !speed);
    ++moveCalls;
    if (failingMoves > 0) { --failingMoves; return 0; }
    moveTribe = id;
    moveX = x;
    moveY = y;
    return 1;
}
int __fastcall create(void* owner, void*, int id)
{
    assert(owner == static_cast<void*>(tribes) && id == 1);
    live(nextTribe, id, ++nextUID);
    return nextTribe++;
}
void __fastcall remove(void* owner, void*, int id, int from)
{
    assert(owner == static_cast<void*>(tribes) && get<short>(unit(id), 0x2D8) == from);
    put<short>(tribe(from), 0x5C, static_cast<short>(get<short>(tribe(from), 0x5C) - 1));
    const unsigned int word = 0x60 + (id / 16) * 2;
    put<unsigned short>(tribe(from), word, static_cast<unsigned short>(get<unsigned short>(tribe(from), word) & ~(1U << (id % 16))));
    if (get<short>(tribe(from), 0x5A) == id) put<short>(tribe(from), 0x5A, 0);
    put<short>(unit(id), 0x2D8, 0);
}
void __fastcall add(void* owner, void*, int id, int to)
{
    assert(owner == static_cast<void*>(tribes) && get<short>(unit(id), 0x2D8) == 0);
    put<short>(tribe(to), 0x5C, static_cast<short>(get<short>(tribe(to), 0x5C) + 1));
    const unsigned int word = 0x60 + (id / 16) * 2;
    put<unsigned short>(tribe(to), word, static_cast<unsigned short>(get<unsigned short>(tribe(to), word) | (1U << (id % 16))));
    if (!get<short>(tribe(to), 0x5A)) put<short>(tribe(to), 0x5A, static_cast<short>(id));
    put<short>(unit(id), 0x2D8, static_cast<short>(to));
    put<unsigned int>(unit(id), 0x2E4, get<unsigned int>(tribe(to), 0x34));
}
// The game's smallest-group choice: here always slot 15 or slot 190.
int __fastcall find(void* aic, void*, int id, int, int role)
{
    assert(aic == AIC && id == 1);
    return role == 16 ? get<short>(player(1), 0x310C + 15 * 2)
        : role == 19 ? get<short>(player(1), 0x310C + 190 * 2) : 0;
}
void __fastcall home(void* aic, void*, int id, int owner)
{
    assert(aic == AIC && owner == 1);
    ++returnCalls;
    returnedTribe = id;
}

void soldier(int id, int to, int role, int type, int x, int y)
{
    put<short>(unit(id), 0x96, 1);
    put<short>(unit(id), 0x8C, 2);
    put<short>(unit(id), 0x8E, static_cast<short>(type));
    put<short>(unit(id), 0x2A0, 0);
    put<short>(unit(id), 0x2A6, 0x100); // nearest enemy far away: not fighting
    put<int>(unit(id), 0x3C8, 100);
    put<short>(unit(id), 0x42A, static_cast<short>(role));
    put<short>(unit(id), 0xC4, static_cast<short>(x));
    put<short>(unit(id), 0xC6, static_cast<short>(y));
    put<short>(unit(id), 0x2D8, 0);
    add(tribes, 0, id, to);
}

// A slot tribe whose leader stands at (x, y), plus `members` more patrol units.
void group(int slot, int id, unsigned int uid, int leader, int x, int y, int members = 0, int type = 22)
{
    put<short>(player(1), 0x310C + slot * 2, static_cast<short>(id));
    put<unsigned int>(player(1), 0x329C + slot * 4, uid);
    live(id, 1, uid);
    const int role = slot >= 190 ? 19 : 16;
    soldier(leader, id, role, type, x, y);
    for (int index = 0; index < members; ++index) soldier(leader + 1 + index, id, role, type, x, y);
}

void leaderAt(int id, int x, int y)
{
    const int leader = get<short>(tribe(id), 0x5A);
    put<short>(unit(leader), 0xC4, static_cast<short>(x));
    put<short>(unit(leader), 0xC6, static_cast<short>(y));
}

void setup(unsigned int stride, unsigned int stance, int orders, int formation, int spacing)
{
    std::memset(players, 0, sizeof(players));
    std::memset(units, 0, sizeof(units));
    std::memset(tribes, 0, sizeof(tribes));
    std::memset(playable, 1, sizeof(playable));
    std::memset(layerIndex, 0, sizeof(layerIndex));
    std::memset(detachedGroups, 0, sizeof(detachedGroups));
    std::memset(patrolRoutes, 0, sizeof(patrolRoutes));
    std::memset(patrolOrders, 0, sizeof(patrolOrders));
    std::memset(patrolFormation, 0, sizeof(patrolFormation));
    std::memset(patrolSpacing, 0, sizeof(patrolSpacing));
    std::memset(patrolExtraGroups, 0, sizeof(patrolExtraGroups));
    std::memset(patrolMinSize, 0, sizeof(patrolMinSize));
    std::memset(patrolRadius, 0, sizeof(patrolRadius));
    std::memset(siegeScreen, 0, sizeof(siegeScreen));
    std::memset(siegeScreenForward, 0, sizeof(siegeScreenForward));
    std::memset(siegeScreenExtraGroups, 0, sizeof(siegeScreenExtraGroups));
    patrolOrdersFallback = 0;
    patrolFormationFallback = 0;
    patrolSpacingFallback = 12;
    patrolExtraGroupsFallback = 0;
    patrolMinSizeFallback = 4;
    patrolRadiusFallback = 12;
    siegeScreenFallback = 0;
    siegeScreenForwardFallback = 4;
    siegeScreenExtraGroupsFallback = 0;
    configurationLocked = 0;
    dispatchCalls = destinationCalls = destinationTribe = pathCalls = moveCalls = moveTribe = moveX = moveY = 0;
    failingMoves = returnCalls = returnedTribe = 0;
    blockedTile = -1;
    nextTribe = 100;
    nextUID = 9000;
    tick = 1000;
    nativeBindings.players = reinterpret_cast<unsigned int>(players);
    nativeBindings.unitRecords = reinterpret_cast<unsigned int>(units);
    nativeBindings.unitCapacity = Units;
    nativeBindings.tribes = reinterpret_cast<unsigned int>(tribes);
    nativeBindings.tribeStride = stride;
    nativeBindings.tribeStance = stance;
    nativeBindings.tribeMemberWords = Units / 16;
    nativeBindings.gameTick = reinterpret_cast<unsigned int>(&tick);
    nativeBindings.mapRows = reinterpret_cast<unsigned int>(rows);
    nativeBindings.playableTiles = reinterpret_cast<unsigned int>(playable);
    nativeBindings.attackPatrolDispatch = reinterpret_cast<unsigned int>(&dispatch);
    nativeBindings.attackPatrolDestination = reinterpret_cast<unsigned int>(&destination);
    nativeBindings.troopValueState = reinterpret_cast<unsigned int>(TROOP_VALUES);
    nativeBindings.tribePath = reinterpret_cast<unsigned int>(&path);
    nativeBindings.siegeGroupMove = reinterpret_cast<unsigned int>(&move);
    nativeBindings.clearGroupMoveFlags = reinterpret_cast<unsigned int>(&clearMoveFlags);
    nativeBindings.createTribe = reinterpret_cast<unsigned int>(&create);
    nativeBindings.addUnitToTribe = reinterpret_cast<unsigned int>(&add);
    nativeBindings.removeUnitFromTribe = reinterpret_cast<unsigned int>(&remove);
    nativeBindings.findAttackGroup = reinterpret_cast<unsigned int>(&find);
    nativeBindings.returnTribe = reinterpret_cast<unsigned int>(&home);
    nativeBindings.distanceLayers = reinterpret_cast<unsigned int>(layers);
    nativeBindings.distanceLayerIndex = reinterpret_cast<unsigned int>(layerIndex);
    nativeBindings.tribeRallyPoints = rallyPoints;
    nativeBindings.tribeRallyCount = rallyCount;
    for (int row = 0; row < 400; ++row) rows[row * 3] = row * 200;
    // Player 1's distance layer 3: walking distance = Chebyshev distance from (100,100).
    put<int>(layerIndex, 1 * 0x177BC, 3);
    for (int y = 0; y < 400; ++y)
        for (int x = 0; x < 200; ++x) {
            const int dx = x > 100 ? x - 100 : 100 - x, dy = y > 100 ? y - 100 : 100 - y;
            const int distance = dx > dy ? dx : dy;
            layers[3 * 80400 + tileOf(x, y)] = static_cast<unsigned char>(distance > 255 ? 255 : distance);
        }
    // Player 1 is AI character 2 attacking player 2; keeps at (100,300) and (100,100).
    put<int>(player(1), 0x2300, 3);
    put<int>(player(1), 0x2BD8, 2);
    put<int>(player(1), 0x2BA4, 4);
    put<int>(player(1), 0x94, 1);
    put<int>(player(1), 0x98, 100);
    put<int>(player(1), 0x9C, 300);
    put<int>(player(2), 0x94, 1);
    put<int>(player(2), 0x98, 100);
    put<int>(player(2), 0x9C, 100);
    patrolOrders[2] = orders;
    patrolFormation[2] = formation;
    patrolSpacing[2] = spacing;
    group(15, 5, 505, 2, 100, 160);
    group(16, 6, 606, 3, 110, 170);
}

// The first tile outward from the target keep on `direction` at ring distance 12..18.
TilePoint ring(int direction)
{
    TilePoint keep;
    keep.x = 100;
    keep.y = 100;
    for (int step = 1; step < 180; ++step) {
        const TilePoint point = patrolRingStep(keep, direction, step);
        const int value = layers[3 * 80400 + tileOf(point.x, point.y)];
        if (value >= 12 && value <= 18) return point;
    }
    assert(false);
    return keep;
}

int patrolRecords()
{
    int count = 0;
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index)
        if (detachedGroups[1][index].duty == PatrolDuty) ++count;
    return count;
}

void secondPatrol(unsigned int stride, unsigned int stance)
{
    // Absent fields: only the native dispatcher runs.
    setup(stride, stance, 0, 0, 0);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(dispatchCalls == 1 && destinationCalls == 0 && pathCalls == 0 && moveCalls == 0);
    assert(get<short>(tribe(6), stance) == 0 && configurationLocked == 0);

    // AllGroups with the native formation: the native destination routine for slot 16.
    setup(stride, stance, 2, 1, 0);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(dispatchCalls == 1 && destinationCalls == 1 && destinationTribe == 6);
    assert(get<short>(tribe(6), stance) == 1 && get<short>(tribe(5), stance) == 0);
    assert(configurationLocked == 1 && pathCalls == 0 && moveCalls == 0);

    // Wing 14: the first group heads north; the east station is nearer the second leader.
    setup(stride, stance, 2, 2, 14);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(pathCalls == 1 && moveCalls == 1 && moveX == 114 && moveY == 160 && destinationCalls == 0);

    // A rejected move falls back to the other station, then to the native search.
    setup(stride, stance, 2, 2, 14);
    failingMoves = 1;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(pathCalls == 2 && moveCalls == 2 && moveX == 86 && moveY == 160 && destinationCalls == 0);
    setup(stride, stance, 2, 2, 14);
    failingMoves = 2;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(pathCalls == 2 && moveCalls == 2 && destinationCalls == 1);

    // An unreachable station is skipped.
    setup(stride, stance, 2, 2, 14);
    blockedTile = tileOf(114, 160);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(pathCalls == 2 && moveCalls == 1 && moveX == 86);

    // A station outside the playable map is not even queried.
    setup(stride, stance, 2, 2, 14);
    playable[160 * 400 + 114] = 0;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(pathCalls == 1 && moveCalls == 1 && moveX == 86);

    // Already at the station: no new order, no native search.
    setup(stride, stance, 2, 2, 14);
    leaderAt(6, 113, 161);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(pathCalls == 0 && moveCalls == 0 && destinationCalls == 0);

    // A leader that no longer links back, a dying leader or a recycled UID: no order at all.
    setup(stride, stance, 2, 2, 14);
    put<unsigned int>(unit(3), 0x2E4, 1);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(dispatchCalls == 1 && destinationCalls == 0 && moveCalls == 0 && get<short>(tribe(6), stance) == 0);
    setup(stride, stance, 2, 1, 0);
    put<short>(unit(3), 0x2A0, 1);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(destinationCalls == 0 && get<short>(tribe(6), stance) == 0);
    setup(stride, stance, 2, 1, 0);
    put<unsigned int>(player(1), 0x329C + 16 * 4, 999);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(destinationCalls == 0);
    // A first group without a leader is left to the native dispatcher.
    setup(stride, stance, 2, 2, 14);
    put<short>(unit(2), 0x2A0, 1);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(destinationCalls == 1);

    // Without a target keep the native search is used.
    setup(stride, stance, 2, 2, 14);
    put<int>(player(2), 0x94, 0);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(pathCalls == 0 && destinationCalls == 1);

    // Mirror on the home axis uses the keep's flanks; far away it waits for the native search.
    setup(stride, stance, 2, 3, 0);
    leaderAt(5, 100, 140);
    leaderAt(6, 120, 150);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(pathCalls == 1 && moveCalls == 1 && moveX == 60 && moveY == 100);
    setup(stride, stance, 2, 3, 0);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(pathCalls == 0 && destinationCalls == 1);

    // An absent field uses the module setting; an explicit Native switches it off.
    setup(stride, stance, 0, 0, 0);
    patrolOrdersFallback = 1;
    patrolFormationFallback = 1;
    patrolSpacingFallback = 14;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(pathCalls == 1 && moveCalls == 1 && moveX == 114 && moveY == 160 && destinationCalls == 0);
    setup(stride, stance, 1, 0, 0);
    patrolOrdersFallback = 1;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(dispatchCalls == 1 && destinationCalls == 0 && moveCalls == 0 && configurationLocked == 0);

    // Human players are never ordered.
    setup(stride, stance, 2, 2, 14);
    put<int>(player(1), 0x2300, 0);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(dispatchCalls == 1 && destinationCalls == 0 && moveCalls == 0 && configurationLocked == 0);
}

void splits(unsigned int stride, unsigned int stance)
{
    // Slot 15 with 20 patrol units, no slot 16; two extra groups (stored 2 + 1) of at least 4.
    setup(stride, stance, 2, 1, 0);
    put<short>(player(1), 0x310C + 16 * 2, 0);
    for (int id = 10; id < 29; ++id) soldier(id, 5, 16, 22, 100, 160);
    patrolExtraGroups[2] = 3;
    dispatchAttackPatrols(AIC, 0, 1);
    // One new group per update, balanced tightly while a group is below the minimum.
    assert(patrolRecords() == 1 && detachedGroups[1][0].tribe == 100 && detachedGroups[1][0].target == 2);
    assert(detachedGroupSize(1, 0) == 10 && get<short>(tribe(5), 0x5C) == 10);
    assert(get<short>(tribe(5), 0x5A) == 2 && get<short>(unit(2), 0x2D8) == 5);
    // The highest-numbered members move first, never the leader.
    assert(get<short>(unit(28), 0x2D8) == 100 && get<short>(unit(19), 0x2D8) == 100 && get<short>(unit(18), 0x2D8) == 5);
    assert(get<short>(tribe(100), stance) == 1 && destinationCalls == 1 && destinationTribe == 100);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(patrolRecords() == 2 && detachedGroupSize(1, 1) == 6);
    assert(detachedGroupSize(1, 0) == 7 && get<short>(tribe(5), 0x5C) == 7);
    // Within the minimum size of each other once every group has it: no more transfers.
    dispatchAttackPatrols(AIC, 0, 1);
    assert(patrolRecords() == 2 && detachedGroupSize(1, 0) == 7 && detachedGroupSize(1, 1) == 6);

    // Too few troops for the wanted groups: 7 units allow one group of 4.
    setup(stride, stance, 2, 1, 0);
    put<short>(player(1), 0x310C + 16 * 2, 0);
    for (int id = 10; id < 16; ++id) soldier(id, 5, 16, 22, 100, 160);
    patrolExtraGroups[2] = 3;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(patrolRecords() == 0 && get<short>(tribe(5), 0x5C) == 7);

    // Two native slots and one extra group: slots keep their duties, the extra group joins.
    setup(stride, stance, 2, 1, 0);
    for (int id = 10; id < 20; ++id) soldier(id, 5, 16, 22, 100, 160);
    for (int id = 20; id < 30; ++id) soldier(id, 6, 16, 22, 110, 170);
    patrolExtraGroups[2] = 2;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(patrolRecords() == 1 && detachedGroupSize(1, 0) >= 4);
    assert(destinationCalls == 2);

    // A setting change on load dissolves surplus groups, smallest first, into the slots.
    patrolExtraGroups[2] = 1;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(patrolRecords() == 0 && get<short>(tribe(5), 0x5C) + get<short>(tribe(6), 0x5C) == 22);

    // After the attack leaves states 3-5 every detached member rejoins slot 15.
    setup(stride, stance, 2, 4, 0);
    for (int id = 10; id < 30; ++id) soldier(id, 5, 16, 22, 100, 160);
    patrolExtraGroups[2] = 3;
    dispatchAttackPatrols(AIC, 0, 1);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(patrolRecords() == 2 && patrolRoutes[1][0].uid == 606);
    finishAttackPatrols(AIC, 1);
    assert(patrolRecords() == 2);
    put<int>(player(1), 0x2BA4, 6);
    finishAttackPatrols(AIC, 1);
    assert(patrolRoutes[1][0].uid == 0);
    while (patrolRecords()) finishAttackPatrols(AIC, 1);
    assert(get<short>(tribe(5), 0x5C) == 21 && patrolRoutes[1][1].uid == 0 && patrolRoutes[1][2].uid == 0);

    // A retreat hands everything back before the native order, in one budget.
    setup(stride, stance, 2, 1, 0);
    for (int id = 10; id < 30; ++id) soldier(id, 5, 16, 22, 100, 160);
    patrolExtraGroups[2] = 2;
    dispatchAttackPatrols(AIC, 0, 1);
    dispatchAttackPatrols(AIC, 0, 1);
    retreatAttackPatrols(AIC, 1);
    assert(patrolRecords() == 0 && get<short>(tribe(5), 0x5C) == 21 && returnCalls == 0);
    // Members without a native slot to go to walk home as a group.
    setup(stride, stance, 2, 1, 0);
    for (int id = 10; id < 30; ++id) soldier(id, 5, 16, 22, 100, 160);
    patrolExtraGroups[2] = 2;
    dispatchAttackPatrols(AIC, 0, 1);
    put<short>(player(1), 0x310C + 15 * 2, 0);
    retreatAttackPatrols(AIC, 1);
    assert(patrolRecords() == 1 && returnCalls == 1 && returnedTribe == 100);

    // A detached group whose tribe was recycled is dropped, not ordered; a new one replaces it.
    setup(stride, stance, 2, 1, 0);
    for (int id = 10; id < 30; ++id) soldier(id, 5, 16, 22, 100, 160);
    patrolExtraGroups[2] = 2;
    dispatchAttackPatrols(AIC, 0, 1);
    put<unsigned int>(tribe(100), 0x34, 1);
    destinationCalls = 0;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(patrolRecords() == 1 && detachedGroups[1][0].tribe == 101 && destinationTribe == 101);
}

void routes(unsigned int stride, unsigned int stance)
{
    // Routes: slot 16 is group 2 and circles the other way from the point nearest its leader (south).
    setup(stride, stance, 2, 4, 0);
    dispatchAttackPatrols(AIC, 0, 1);
    TilePoint point = ring(4);
    assert(moveCalls == 1 && moveTribe == 6 && moveX == point.x && moveY == point.y && point.y == 112);
    assert(patrolRoutes[1][0].uid == 606 && patrolRoutes[1][0].target == 2 && patrolRoutes[1][0].point == 4);
    assert(patrolRoutes[1][0].step == -1);
    // On its way: no new order.
    tick += 100;
    leaderAt(6, 108, 150);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(moveCalls == 1);
    // Arrived: the next point.
    leaderAt(6, point.x, point.y + 2);
    dispatchAttackPatrols(AIC, 0, 1);
    point = ring(3);
    assert(moveCalls == 2 && moveX == point.x && moveY == point.y && patrolRoutes[1][0].point == 3);
    // A rejected next point takes the alternate after it.
    leaderAt(6, point.x, point.y);
    blockedTile = tileOf(ring(2).x, ring(2).y);
    dispatchAttackPatrols(AIC, 0, 1);
    point = ring(1);
    assert(moveX == point.x && moveY == point.y && patrolRoutes[1][0].point == 1);
    // No progress for the step time: one alternate, then the native search.
    blockedTile = -1;
    tick += 1601;
    dispatchAttackPatrols(AIC, 0, 1);
    point = ring(0);
    assert(moveX == point.x && patrolRoutes[1][0].point == 0 && patrolRoutes[1][0].stalls == 1);
    tick += 1601;
    destinationCalls = 0;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(destinationCalls == 1 && patrolRoutes[1][0].stalls == 2);
    tick += 800;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(destinationCalls == 2);
    // Then the route resumes from the point nearest the leader (east of the keep).
    tick += 801;
    leaderAt(6, 130, 101);
    dispatchAttackPatrols(AIC, 0, 1);
    point = ring(0);
    assert(patrolRoutes[1][0].stalls == 0 && moveX == point.x && moveY == point.y);
    // A fight suspends the stall time.
    put<short>(unit(3), 0x2A6, 0x20);
    tick += 1601;
    const int orders = moveCalls;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(moveCalls == orders && patrolRoutes[1][0].stalls == 0);

    // Both candidates rejected: native search.
    setup(stride, stance, 2, 4, 0);
    std::memset(playable, 0, sizeof(playable));
    playable[170 * 400 + 110] = 1;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(moveCalls == 0 && destinationCalls == 1 && patrolRoutes[1][0].stalls == 2);
    // No distance layer for this AI: native search.
    setup(stride, stance, 2, 4, 0);
    put<int>(layerIndex, 1 * 0x177BC, 9);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(moveCalls == 0 && destinationCalls == 1);

    // Group 3 (the first extra group) sweeps the five front points (front = south), turning at the ends.
    setup(stride, stance, 2, 4, 0);
    for (int id = 10; id < 30; ++id) soldier(id, 5, 16, 22, 100, 160);
    patrolExtraGroups[2] = 2;
    dispatchAttackPatrols(AIC, 0, 1);
    assert(patrolRecords() == 1 && patrolRoutes[1][1].point == 2 && patrolRoutes[1][1].step == 1);
    assert(detachedGroups[1][0].x == ring(4).x && detachedGroups[1][0].y == ring(4).y);
    for (int step = 0; step < 3; ++step) {
        leaderAt(100, detachedGroups[1][0].x, detachedGroups[1][0].y);
        dispatchAttackPatrols(AIC, 0, 1);
    }
    assert(patrolRoutes[1][1].point == 3 && patrolRoutes[1][1].step == -1);

    // A route belongs to its target: a new target restarts it.
    put<int>(player(3), 0x94, 1);
    put<int>(player(3), 0x98, 100);
    put<int>(player(3), 0x9C, 100);
    put<int>(player(1), 0x2BD8, 3);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(patrolRoutes[1][0].target == 3);

    // Group 4 circles the first way, a quarter circle behind its nearest point.
    setup(stride, stance, 2, 4, 0);
    for (int id = 10; id < 30; ++id) soldier(id, 5, 16, 22, 100, 160);
    patrolExtraGroups[2] = 3;
    dispatchAttackPatrols(AIC, 0, 1);
    dispatchAttackPatrols(AIC, 0, 1);
    assert(patrolRecords() == 2 && patrolRoutes[1][2].point == 0 && patrolRoutes[1][2].step == 1);
}

void screens(unsigned int stride, unsigned int stance)
{
    setup(stride, stance, 0, 0, 0);
    group(190, 7, 707, 40, 100, 150, 3, 25);
    group(191, 8, 808, 44, 104, 150, 3, 22);
    const unsigned int list = rallyPoints;
    for (int index = 0; index < 4; ++index) {
        put<short>(tribe(8), list + index * 4, static_cast<short>(90 + index * 10));
        put<short>(tribe(8), list + index * 4 + 2, 140);
    }
    put<short>(tribe(8), rallyCount, 4);
    put<short>(tribe(8), rallyCount - 2, 1);
    // Not on for the character: the native order is passed through unchanged.
    assert(screenSiegeDefenceMove(tribes, 0, 7, 100, 140, 0, 0, 0) == 1);
    assert(moveCalls == 1 && moveX == 100 && moveY == 140 && pathCalls == 0 && configurationLocked == 0);
    // Melee group: stationed four tiles in front of its engine, toward the target keep.
    siegeScreenFallback = 1;
    screenSiegeDefenceMove(tribes, 0, 7, 100, 140, 0, 0, 0);
    assert(moveCalls == 2 && moveX == 100 && moveY == 136 && pathCalls == 1);
    assert(configurationLocked == 1);
    // Unreachable in front: the native engine point.
    blockedTile = tileOf(100, 136);
    screenSiegeDefenceMove(tribes, 0, 7, 100, 140, 0, 0, 0);
    assert(moveX == 100 && moveY == 140);
    blockedTile = -1;
    // Ranged second group: half a lap ahead on its own rally list, at the engine.
    screenSiegeDefenceMove(tribes, 0, 8, 100, 140, 0, 0, 0);
    assert(moveX == 120 && moveY == 140);
    // Forward 0 keeps melee groups at their engines.
    siegeScreenForward[2] = 1;
    screenSiegeDefenceMove(tribes, 0, 7, 100, 140, 0, 0, 0);
    assert(moveX == 100 && moveY == 140);
    // Tribes outside the siege-defence slots and an explicit Native are passed through.
    siegeScreenForward[2] = 0;
    screenSiegeDefenceMove(tribes, 0, 5, 101, 141, 0, 0, 0);
    assert(moveX == 101 && moveY == 141);
    siegeScreen[2] = 1;
    screenSiegeDefenceMove(tribes, 0, 7, 100, 140, 0, 0, 0);
    assert(moveX == 100 && moveY == 140);
}

int siegeRecords()
{
    int count = 0;
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index)
        if (detachedGroups[1][index].duty == SiegeDefenceDuty) ++count;
    return count;
}

void siegeDefence(unsigned int stride, unsigned int stance)
{
    setup(stride, stance, 0, 0, 0);
    group(190, 7, 707, 40, 100, 150, 11);
    group(191, 8, 808, 52, 104, 150, 3);
    // Off by default, and the extra groups need the engine escort.
    organizeSiegeDefence(AIC, 1);
    siegeScreenExtraGroupsFallback = 2;
    organizeSiegeDefence(AIC, 1);
    assert(siegeRecords() == 0 && configurationLocked == 0);
    // One new group per update, each keeping the minimum size of 4.
    siegeScreenFallback = 1;
    organizeSiegeDefence(AIC, 1);
    assert(siegeRecords() == 1 && configurationLocked == 1);
    organizeSiegeDefence(AIC, 1);
    organizeSiegeDefence(AIC, 1);
    assert(siegeRecords() == 2);
    int detached = 0;
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index)
        if (detachedGroups[1][index].duty == SiegeDefenceDuty) {
            assert(detachedGroupSize(1, index) >= 4);
            detached += detachedGroupSize(1, index);
        }
    assert(detached + get<short>(tribe(7), 0x5C) + get<short>(tribe(8), 0x5C) == 16);
    // Slot 190 moving on takes the detached groups along: groups 3 and 4 of 4
    // stand at the rally entries two and three after slot 190's.
    for (int index = 0; index < 4; ++index) {
        put<short>(tribe(7), rallyPoints + index * 4, static_cast<short>(90 + index * 10));
        put<short>(tribe(7), rallyPoints + index * 4 + 2, 140);
    }
    put<short>(tribe(7), rallyCount, 4);
    put<short>(tribe(7), rallyCount - 2, 0);
    moveCalls = 0;
    screenSiegeDefenceMove(tribes, 0, 7, 90, 140, 0, 0, 0);
    assert(moveCalls == 3 && moveTribe == 7 && moveX == 90);
    int stations = 0;
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index)
        if (detachedGroups[1][index].duty == SiegeDefenceDuty) {
            assert(detachedGroups[1][index].y == 140);
            stations += detachedGroups[1][index].x;
        }
    assert(stations == 110 + 120);
    // Before the wall assault (state 5) every member is back in a native slot.
    put<int>(player(1), 0x2BA4, 5);
    organizeSiegeDefence(AIC, 1);
    assert(siegeRecords() == 0 && get<short>(tribe(7), 0x5C) == 12);
    // A retreat hands them back too.
    put<int>(player(1), 0x2BA4, 4);
    organizeSiegeDefence(AIC, 1);
    assert(siegeRecords() == 1);
    retreatAttackPatrols(AIC, 1);
    assert(siegeRecords() == 0);
}

void run(unsigned int stride, unsigned int stance, unsigned int points, unsigned int count)
{
    rallyPoints = points;
    rallyCount = count;
    secondPatrol(stride, stance);
    splits(stride, stance);
    routes(stride, stance);
    screens(stride, stance);
    siegeDefence(stride, stance);
}
}

int main()
{
    run(0x334, 0x2E0, 0x260, 0x28A);
    run(0x688, 0x634, 0x5B4, 0x5DE);
    return 0;
}
