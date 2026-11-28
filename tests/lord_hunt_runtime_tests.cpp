// Executes the lord hunt decisions against mock game data and mock native
// area, order and group routines on both tribe layouts (Crusader 0x334,
// Extreme 0x688).
#include "aic_tactics/runtime.hpp"
#include "aic_tactics/groups.hpp"
#include "aic_tactics/lord_hunt.hpp"
#include <cassert>
#include <cstring>

namespace AicTactics { namespace SHC141 {
NativeBindings nativeBindings;
unsigned int combatCensusTick;
int combatCensusValid;
int censusLord[9];
int liveCensusLord(int player) { return player >= 1 && player <= 8 ? censusLord[player] : 0; }
}}

using namespace AicTactics::SHC141;

namespace {
const int Human = 1, Attacker = 2, Ally = 3;
const int Field = 1, Home = 2, Shelter = 3;     // areas
const int SlotTribe = 100;
unsigned char players[9 * 0x39F4];
unsigned char units[64 * 0x490];
unsigned char tribes[1250 * 0x688];
unsigned char buildings[0x14 + 16 * 0x32C];
unsigned char aic[16 * 676];
int teams[9];
unsigned short areas[80400];
int rows[400 * 3];
int logic[80400];
unsigned char wallOwner[80400];
unsigned int tick = 5000;
unsigned int nextUID = 900;
int nextTribe = 40;
bool connected[4][4];   // walking (mode 0)
bool climbing[4][4];    // climbing links (mode 2)
int queries;
int lastOrderTribe, lastOrder, lastOrderTarget;
unsigned int lastOrderUID;
int moves, lastMoveTribe, lastMoveX, lastMoveY;
int nativeReturns;
void* const AIC = reinterpret_cast<void*>(0x1234);

template<class T> void put(unsigned char* base, unsigned int offset, T value) { *reinterpret_cast<T*>(base + offset) = value; }
template<class T> T get(unsigned char* base, unsigned int offset) { return *reinterpret_cast<T*>(base + offset); }
unsigned char* player(int id) { return players + id * 0x39F4; }
unsigned char* unit(int id) { return units + id * 0x490; }
unsigned char* tribe(int id) { return tribes + id * nativeBindings.tribeStride; }
unsigned char* building(int id) { return buildings + 0x14 + id * 0x32C; }
int tileAt(int x, int y) { return y * 200 + x; }

void live(int id, int owner, unsigned int uid)
{
    put<int>(tribe(id), 0x2C, owner);
    put<unsigned int>(tribe(id), 0x34, uid);
    put<short>(tribe(id), 0x40, 2);
}

int __fastcall create(void* owner, void*, int who)
{
    assert(owner == static_cast<void*>(tribes));
    live(nextTribe, who, ++nextUID);
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
int __fastcall find(void* owner, void*, int who, int, int role)
{
    assert(owner == AIC && who == Attacker && role >= 10 && role <= 20);
    return SlotTribe;
}
int __fastcall route(void* finder, void*, int who, int to, int from, int mode)
{
    assert(finder == reinterpret_cast<void*>(0x5678) && who == Attacker && (mode == 0 || mode == 2));
    assert(to >= 1 && to <= 3 && from >= 1 && from <= 3);
    ++queries;
    return mode == 0 ? connected[from][to] : climbing[from][to];
}
void __fastcall relay(void* owner, void*, int id, int instruction, int target, unsigned int uid, int extra)
{
    assert(owner == static_cast<void*>(units) && extra == 0);
    lastOrderTribe = id;
    lastOrder = instruction;
    lastOrderTarget = target;
    lastOrderUID = uid;
}
void __fastcall clearMoveFlags(void* owner, void*, int)
{
    assert(owner == static_cast<void*>(tribes));
}
int __fastcall moveTribe(void* owner, void*, int id, int x, int y, int a, int b, int c)
{
    assert(owner == static_cast<void*>(tribes) && a == 0 && b == 0 && c == 0);
    ++moves;
    lastMoveTribe = id;
    lastMoveX = x;
    lastMoveY = y;
    return 1;
}
void __fastcall nativeReturn(void* owner, void*, int who)
{
    assert(owner == AIC && who == Attacker);
    ++nativeReturns;
}

void place(int id, int owner, int type, int role, int x, int y, int area)
{
    put<short>(unit(id), 0x8C, 2);
    put<short>(unit(id), 0x8E, static_cast<short>(type));
    put<short>(unit(id), 0x96, static_cast<short>(owner));
    put<unsigned int>(unit(id), 0x98, 7000 + id);
    put<short>(unit(id), 0xC4, static_cast<short>(x));
    put<short>(unit(id), 0xC6, static_cast<short>(y));
    put<int>(unit(id), 0xD4, tileAt(x, y));
    areas[tileAt(x, y)] = static_cast<unsigned short>(area);
    put<short>(unit(id), 0x2A0, 0);
    put<int>(unit(id), 0x3C8, 100);
    put<short>(unit(id), 0x42A, static_cast<short>(role));
}

void keep(int id, int x, int y, int area)
{
    put<int>(player(id), 0x94, 1);
    put<int>(player(id), 0x98, x);
    put<int>(player(id), 0x9C, y + 2);
    put<int>(player(id), 0x1E0, tileAt(x, y));
    areas[tileAt(x, y)] = static_cast<unsigned short>(area);
}

void census()
{
    resetLordHuntCensus();
    for (int id = 1; id < 64; ++id)
        if (get<short>(unit(id), 0x8C) == 2 && get<int>(unit(id), 0x3C8) > 0)
            countLordHuntUnit(id, get<short>(unit(id), 0x96), get<short>(unit(id), 0x8E));
    combatCensusTick = tick;
    combatCensusValid = 1;
}

int groupOf(int reference) { return reference ? detachedGroups[Attacker][reference - 1].tribe : 0; }
int sizeOf(int reference) { return reference ? detachedGroupSize(Attacker, reference - 1) : 0; }

// Human player 1 (team 1) has his keep in area 2 and his lord hiding in the
// keep area 3 of his ally, player 3. AI player 2 (character 2, team 2)
// attacks him with twenty main army troops standing in the open, area 1:
// units 10..29 at increasing distance from the human's keep; IDs divisible
// by four are archers.
void setup(unsigned int stride)
{
    std::memset(players, 0, sizeof(players));
    std::memset(units, 0, sizeof(units));
    std::memset(tribes, 0, sizeof(tribes));
    std::memset(buildings, 0, sizeof(buildings));
    std::memset(aic, 0, sizeof(aic));
    std::memset(areas, 0, sizeof(areas));
    std::memset(logic, 0, sizeof(logic));
    std::memset(wallOwner, 0, sizeof(wallOwner));
    std::memset(connected, 0, sizeof(connected));
    std::memset(climbing, 0, sizeof(climbing));
    std::memset(detachedGroups, 0, sizeof(detachedGroups));
    std::memset(lordHunts, 0, sizeof(lordHunts));
    std::memset(lordHuntRetarget, 0, sizeof(lordHuntRetarget));
    std::memset(lordHuntSiege, 0, sizeof(lordHuntSiege));
    std::memset(lordHuntSwitch, 0, sizeof(lordHuntSwitch));
    std::memset(lordHuntShare, 0, sizeof(lordHuntShare));
    std::memset(lordHuntSplit, 0, sizeof(lordHuntSplit));
    std::memset(censusLord, 0, sizeof(censusLord));
    for (int y = 0; y < 400; ++y) rows[y * 3] = y * 200;
    nextTribe = 40;
    nativeReturns = moves = queries = 0;
    lastOrder = lastOrderTribe = 0;
    nativeBindings.players = reinterpret_cast<unsigned int>(players);
    nativeBindings.unitRecords = reinterpret_cast<unsigned int>(units);
    nativeBindings.units = reinterpret_cast<unsigned int>(units);
    nativeBindings.unitCapacity = 64;
    nativeBindings.tribes = reinterpret_cast<unsigned int>(tribes);
    nativeBindings.tribeStride = stride;
    nativeBindings.tribeMemberWords = 4;
    nativeBindings.tribeStance = stride == 0x334 ? 0x2E0 : 0x634;
    nativeBindings.buildings = reinterpret_cast<unsigned int>(buildings);
    nativeBindings.buildingCapacity = 16;
    nativeBindings.aicRecords = reinterpret_cast<unsigned int>(aic);
    nativeBindings.teams = reinterpret_cast<unsigned int>(teams);
    nativeBindings.gameTick = reinterpret_cast<unsigned int>(&tick);
    nativeBindings.mapRows = reinterpret_cast<unsigned int>(rows);
    nativeBindings.areaMap = reinterpret_cast<unsigned int>(areas);
    nativeBindings.areaPathFinder = 0x5678;
    nativeBindings.areaRoute = reinterpret_cast<unsigned int>(&route);
    nativeBindings.tileLogic = reinterpret_cast<unsigned int>(logic);
    nativeBindings.tileWallOwner = reinterpret_cast<unsigned int>(wallOwner);
    nativeBindings.createTribe = reinterpret_cast<unsigned int>(&create);
    nativeBindings.removeUnitFromTribe = reinterpret_cast<unsigned int>(&remove);
    nativeBindings.addUnitToTribe = reinterpret_cast<unsigned int>(&add);
    nativeBindings.findAttackGroup = reinterpret_cast<unsigned int>(&find);
    nativeBindings.relayRaidOrder = reinterpret_cast<unsigned int>(&relay);
    nativeBindings.siegeGroupMove = reinterpret_cast<unsigned int>(&moveTribe);
    nativeBindings.clearGroupMoveFlags = reinterpret_cast<unsigned int>(&clearMoveFlags);
    nativeBindings.returnAttack = reinterpret_cast<unsigned int>(&nativeReturn);
    teams[Human] = 1; teams[Attacker] = 2; teams[Ally] = 1; teams[4] = 3;
    put<int>(player(Attacker), 0x2300, 3);
    put<int>(player(Ally), 0x2300, 4);
    put<int>(player(4), 0x2300, 5);
    put<int>(player(Attacker), 0x2BA4, 3);
    put<int>(player(Attacker), 0x2BD8, Human);
    put<int>(aic + 676, 0x1F4, 20);       // AttForceBase of character 2
    keep(Human, 100, 100, Home);
    keep(Ally, 200, 100, Shelter);
    keep(4, 300, 300, Field);
    connected[Field][Field] = connected[Home][Home] = connected[Shelter][Shelter] = true;
    place(1, Human, 55, 0, 201, 101, Shelter);
    censusLord[Human] = 1;
    place(2, Human, 1, 0, 101, 101, Home);   // a worker at home
    put<int>(player(Human), 0x3684, 1);
    put<short>(player(Human), 0x35BC, 5);
    put<short>(building(5), 0xD0, 2);
    put<short>(building(5), 0xD6, Human);
    put<unsigned int>(building(5), 0xD8, 4321);
    put<short>(building(5), 0xFE, 120);
    put<short>(building(5), 0x100, 100);
    areas[tileAt(120, 100)] = Field;
    live(SlotTribe, Attacker, 77);
    put<short>(player(Attacker), 0x310C + 192 * 2, SlotTribe);
    put<unsigned int>(player(Attacker), 0x329C + 192 * 4, 77);
    for (int id = 10; id < 30; ++id) {
        place(id, Attacker, id % 4 == 0 ? 22 : 24, 20, 130 + (id - 10), 100, Field);
        add(tribes, 0, id, SlotTribe);
    }
}

void hidingLordGetsTheSiegeAndHomeGetsASplit()
{
    census();
    assert(lordHuntCensus[Attacker].attackTroops == 20 && lordHuntCensus[Attacker].candidateCount == 20);
    assert(lordHuntUnits[Human] == 1);
    const SiegeRetarget change = updateLordHunt(AIC, Attacker);
    const LordHuntState& state = lordHunts[Attacker];
    assert(change.castle == Ally && change.phase == 1);
    assert(state.origin == Human && state.host == Ally && state.splitTo == Human && state.peak == 20);
    assert(lordHuntSiegeCastle(Attacker, Human) == Ally && lordHuntSiegeCastle(Attacker, Ally) == Ally);
    assert(lordHuntSiegeCastle(Attacker, 4) == 4 && lordHuntSiegeCastle(Human, Human) == Human);
    // Half the army, closest to the human's keep first: units 10..19,
    // archers (12, 16) in their own group.
    assert(sizeOf(state.splitGroup) == 8 && sizeOf(state.shooterGroup) == 2);
    for (int id = 10; id < 30; ++id) {
        const int at = get<short>(unit(id), 0x2D8);
        if (id >= 20) assert(at == SlotTribe);
        else assert(at == (id % 4 == 0 ? groupOf(state.shooterGroup) : groupOf(state.splitGroup)));
    }
    assert(detachedGroups[Attacker][state.splitGroup - 1].duty == SplitDuty);
    // Melee split troops attack the reachable building; archers walk to it.
    assert(state.splitOrder == SplitAttacksBuilding && lastOrderTribe == groupOf(state.splitGroup));
    assert(lastOrder == 9 && lastOrderTarget == 5 && lastOrderUID == 4321);
    assert(lastMoveTribe == groupOf(state.shooterGroup) && lastMoveX == 120 && lastMoveY == 100);
    assert(get<short>(tribe(groupOf(state.splitGroup)), nativeBindings.tribeStance) == 2);
    // The redirect acts once per attack.
    put<int>(player(Attacker), 0x2BD8, Ally);
    put<int>(player(Attacker), 0x2BA4, 2);
    ++tick;
    census();
    const SiegeRetarget again = updateLordHunt(AIC, Attacker);
    assert(again.castle == 0 && again.phase == 0);
}

void reachableLordDrawsAHuntThatReturnsWhenTheWayCloses()
{
    // A ladder opens the shelter to the army in the field.
    climbing[Field][Shelter] = true;
    put<int>(player(Attacker), 0x2BA4, 4);
    ++tick;
    census();
    assert(lordHuntCensus[Attacker].candidateCount == 20);
    updateLordHunt(AIC, Attacker);
    LordHuntState& state = lordHunts[Attacker];
    // Half of the ten main army troops, closest to the lord first (units 29..25).
    assert(state.huntGroup && state.huntWave == 1 && state.huntSource == MainArmySource);
    assert(sizeOf(state.huntGroup) == 5 && detachedGroups[Attacker][state.huntGroup - 1].duty == HuntDuty);
    for (int id = 25; id < 30; ++id) assert(get<short>(unit(id), 0x2D8) == groupOf(state.huntGroup));
    assert(lastOrder == 4 && lastOrderTarget == 1 && lastOrderUID == 7001);
    // The way closes: the hunters go back after 200 ticks.
    climbing[Field][Shelter] = false;
    const int hunt = state.huntGroup;
    for (int step = 0; step < 5; ++step) {
        tick += 60;
        census();
        updateLordHunt(AIC, Attacker);
    }
    assert(!state.huntGroup && (state.returning & (1 << (hunt - 1))));
    tick += 10;
    census();
    updateLordHunt(AIC, Attacker);
    for (int id = 25; id < 30; ++id) assert(get<short>(unit(id), 0x2D8) == SlotTribe);
    assert(detachedGroups[Attacker][hunt - 1].duty == NoDuty && !(state.returning & (1 << (hunt - 1))));
    assert(nativeReturns == 0);
}

void lordLeavingTheShelterTurnsTheAttackBack()
{
    LordHuntState& state = lordHunts[Attacker];
    const int split = state.splitGroup;
    place(1, Human, 55, 0, 150, 150, Field);
    ++tick;
    census();
    const SiegeRetarget change = updateLordHunt(AIC, Attacker);
    assert(change.castle == Human && change.phase == 1);
    assert(!state.host && !state.splitTo && !state.splitGroup && (state.phase & LordHuntSwitchedBack));
    assert(detachedGroups[Attacker][split - 1].returning == 1);
    assert(lordHuntSiegeCastle(Attacker, Human) == Human);
}

void endOfAttackReturnsEveryGroupAndRepeatsTheNativeReturn()
{
    put<int>(player(Attacker), 0x2BD8, Human);
    put<int>(player(Attacker), 0x2BA4, 9);
    for (int step = 0; step < 4; ++step) {
        ++tick;
        census();
        updateLordHunt(AIC, Attacker);
    }
    for (int id = 10; id < 30; ++id) assert(get<short>(unit(id), 0x2D8) == SlotTribe);
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index)
        assert(detachedGroups[Attacker][index].duty == NoDuty);
    assert(lordHunts[Attacker].origin == 0 && lordHunts[Attacker].returning == 0 && nativeReturns == 1);
}

void finishedRedirectPutsTheTargetBack()
{
    census();
    updateLordHunt(AIC, Attacker);
    assert(lordHunts[Attacker].host == Ally);
    put<int>(player(Attacker), 0x2BD8, Ally);
    put<int>(player(Attacker), 0x2BA4, 8);
    const SiegeRetarget change = updateLordHunt(AIC, Attacker);
    assert(change.castle == Human && lordHunts[Attacker].origin == 0);
}

void weakenedArmyDoesNotSwitchCastles()
{
    put<int>(aic + 676, 0x1F4, 40);   // the army was 40 strong; 20 is below 80%
    census();
    const SiegeRetarget change = updateLordHunt(AIC, Attacker);
    assert(change.castle == 0 && lordHunts[Attacker].host == 0 && !(lordHunts[Attacker].phase & LordHuntHidingDecided));
    // Before the march (state 2) the switch is free.
    put<int>(player(Attacker), 0x2BA4, 2);
    census();
    assert(updateLordHunt(AIC, Attacker).castle == Ally);
}

void homeSiegeChoiceSendsTheSplitToTheShelter()
{
    lordHuntSiege[2] = 2;                 // character 2: HomeCastle
    census();
    const SiegeRetarget change = updateLordHunt(AIC, Attacker);
    assert(change.castle == 0 && lordHunts[Attacker].host == 0 && lordHunts[Attacker].splitTo == Ally);
    assert(sizeOf(lordHunts[Attacker].splitGroup) + sizeOf(lordHunts[Attacker].shooterGroup) == 10);
}

void emptyHomeSendsTheWholeArmy()
{
    put<short>(unit(2), 0x8C, 0);         // the worker is gone
    lordHuntSiege[2] = 2;
    census();
    assert(lordHuntUnits[Human] == 0);
    const SiegeRetarget change = updateLordHunt(AIC, Attacker);
    assert(change.castle == Ally && lordHunts[Attacker].splitTo == 0 && !lordHunts[Attacker].splitGroup);
}

void onlyHumanTargetsAndSwitchedOffCharactersAreFollowed()
{
    put<int>(player(Attacker), 0x2BD8, 4);
    censusLord[4] = 3;
    place(3, 4, 55, 0, 301, 301, Field);
    census();
    assert(updateLordHunt(AIC, Attacker).castle == 0 && lordHunts[Attacker].origin == 0);
    put<int>(player(Attacker), 0x2BD8, Human);
    lordHuntRetarget[2] = 1;              // character 2: off
    lordHuntShare[2] = 1;                 // hunt share 0
    census();
    assert(lordHuntCensus[Attacker].candidateCount == 0);
    assert(updateLordHunt(AIC, Attacker).castle == 0 && lordHunts[Attacker].origin == 0);
}

void deadLordEndsTheAttackRecord()
{
    census();
    updateLordHunt(AIC, Attacker);
    assert(lordHunts[Attacker].host == Ally);
    censusLord[Human] = 0;
    put<int>(player(Attacker), 0x2BD8, Ally);
    const SiegeRetarget change = updateLordHunt(AIC, Attacker);
    assert(change.castle == Human && lordHunts[Attacker].origin == 0 && lordHunts[Attacker].returning);
}

void splitTroopsFallBackToWallsThenTheKeep()
{
    lordHuntSiege[2] = 2;
    areas[tileAt(120, 100)] = Home;       // the building is now behind the walls
    for (int x = 110; x <= 112; ++x) {
        logic[tileAt(x, 100)] = 0x100;
        wallOwner[tileAt(x, 100)] = Human - 1;
    }
    logic[tileAt(111, 100)] |= 0x2;       // not attacked as a wall
    areas[tileAt(113, 100)] = Field;
    lordHuntSiege[2] = 1;                 // full siege on the shelter, split to home
    census();
    updateLordHunt(AIC, Attacker);
    const LordHuntState& state = lordHunts[Attacker];
    assert(state.splitTo == Human && state.splitOrder == SplitAttacksWall);
    assert(lastOrder == 23 && lastOrderTarget == tileAt(112, 100));
    // No wall either: they walk to the keep, ordered once.
    for (int x = 110; x <= 112; ++x) logic[tileAt(x, 100)] = 0;
    tick += 100;
    census();
    updateLordHunt(AIC, Attacker);
    assert(state.splitOrder == SplitMoves && lastMoveX == 100 && lastMoveY == 102);
    const int before = moves;
    tick += 100;
    census();
    updateLordHunt(AIC, Attacker);
    assert(moves == before);
}

void freedHuntRecordIsNotTakenForAPatrol()
{
    climbing[Field][Shelter] = true;
    census();
    updateLordHunt(AIC, Attacker);
    LordHuntState& state = lordHunts[Attacker];
    const int hunt = state.huntGroup;
    assert(hunt && state.huntWave == 1 && sizeOf(hunt) == 10);
    // Every hunter dies; the owner frees the record and the patrol dispatcher
    // reuses it before the hunt update of the same AI turn.
    const int hunters = groupOf(hunt);
    for (int id = 10; id < 30; ++id)
        if (get<short>(unit(id), 0x2D8) == hunters) put<int>(unit(id), 0x3C8, 0);
    put<short>(tribe(hunters), 0x40, 0);
    ++tick;
    validateDetachedGroups();
    census();
    assert(!state.huntGroup && state.huntWave == 1);
    const int patrol = openDetachedGroup(Attacker, PatrolDuty);
    assert(patrol == hunt - 1);
    const int patrolTribe = detachedGroups[Attacker][patrol].tribe;
    updateLordHunt(AIC, Attacker);
    assert(detachedGroups[Attacker][patrol].duty == PatrolDuty && !detachedGroups[Attacker][patrol].returning);
    assert(!state.returning && lastOrderTribe != patrolTribe);
    // The second wave goes with the rest of the main army.
    assert(state.huntWave == 2 && state.huntGroup && state.huntGroup != hunt && sizeOf(state.huntGroup) == 10);
}

void switchedOffSettingsUnwindTheAttack()
{
    census();
    updateLordHunt(AIC, Attacker);
    LordHuntState& state = lordHunts[Attacker];
    const int split = state.splitGroup;
    assert(state.host == Ally && split);
    put<int>(player(Attacker), 0x2BD8, Ally);
    lordHuntRetarget[2] = 1;              // character 2: retarget off
    assert(lordHuntSiegeCastle(Attacker, Human) == Human);
    ++tick;
    census();
    const SiegeRetarget change = updateLordHunt(AIC, Attacker);
    assert(change.castle == Human && change.phase == 1 && !state.host && !state.splitTo && !state.splitGroup);
    assert(detachedGroups[Attacker][split - 1].returning == 1);
    put<int>(player(Attacker), 0x2BD8, Human);
    climbing[Field][Shelter] = true;
    ++tick;
    census();
    updateLordHunt(AIC, Attacker);
    const int hunt = state.huntGroup;
    assert(hunt && sizeOf(hunt) > 0);
    lordHuntShare[2] = 1;                 // hunt share 0: both settings off
    ++tick;
    census();
    assert(lordHuntCensus[Attacker].attackTroops == 20);
    updateLordHunt(AIC, Attacker);
    assert(!state.huntGroup && !state.huntWave && detachedGroups[Attacker][hunt - 1].returning == 1);
}

void run(unsigned int stride)
{
    setup(stride);
    hidingLordGetsTheSiegeAndHomeGetsASplit();
    reachableLordDrawsAHuntThatReturnsWhenTheWayCloses();
    lordLeavingTheShelterTurnsTheAttackBack();
    endOfAttackReturnsEveryGroupAndRepeatsTheNativeReturn();
    setup(stride);
    finishedRedirectPutsTheTargetBack();
    setup(stride);
    weakenedArmyDoesNotSwitchCastles();
    setup(stride);
    homeSiegeChoiceSendsTheSplitToTheShelter();
    setup(stride);
    emptyHomeSendsTheWholeArmy();
    setup(stride);
    onlyHumanTargetsAndSwitchedOffCharactersAreFollowed();
    setup(stride);
    deadLordEndsTheAttackRecord();
    setup(stride);
    splitTroopsFallBackToWallsThenTheKeep();
    setup(stride);
    freedHuntRecordIsNotTakenForAPatrol();
    setup(stride);
    switchedOffSettingsUnwindTheAttack();
}
}

int main()
{
    run(0x334);
    run(0x688);
    return 0;
}
