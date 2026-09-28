#include "aic_tactics/runtime.hpp"
#include <cassert>
#include <cstring>

namespace AicTactics { namespace SHC141 {
NativeBindings nativeBindings;
PlayerCensus combatCensus[9];
unsigned int combatCensusTick;
int combatCensusValid;
}}

using namespace AicTactics::SHC141;

namespace {
unsigned char players[9 * 0x39F4 + 0x2400];
unsigned char units[4 * 0x490];
unsigned char tribes[4 * 0x334];
unsigned char aic[16 * 676];
int rows[400 * 3];
int teams[9];
unsigned int tick;
int pathCalls, moveCalls;
int blockedTribe;
int moveX[4], moveY[4];

template<class T> void put(unsigned char* base, unsigned int offset, T value)
{
    *reinterpret_cast<T*>(base + offset) = value;
}

int __fastcall path(void*, void*, int tribe, int tile)
{
    assert(tribe > 0 && tribe < 4 && tile > 0 && tile < 160000);
    ++pathCalls;
    return tribe != blockedTribe;
}

int __fastcall move(void*, void*, int tribe, unsigned int x, unsigned int y,
    int rally, int store, int speed)
{
    assert(tribe > 0 && tribe < 4 && !rally && !store && !speed);
    moveX[tribe] = x;
    moveY[tribe] = y;
    ++moveCalls;
    return 1;
}

void setupEngine(int id, int type)
{
    unsigned char* unit = units + id * 0x490;
    unsigned char* group = tribes + id * 0x334;
    put<short>(unit, 0x8C, 2);
    put<short>(unit, 0x8E, static_cast<short>(type));
    put<int>(unit, 0x98, 100 + id);
    put<short>(unit, 0x96, 1);
    put<short>(unit, 0x2D8, static_cast<short>(id));
    put<int>(unit, 0x2E4, 200 + id);
    put<short>(unit, 0x3B4, 2);
    put<int>(unit, 0x3C8, 100);
    put<short>(unit, 0x42A, 0x15);
    put<short>(unit, 0x432, 2);
    put<short>(unit, 0xC4, 50);
    put<short>(unit, 0xC6, 50);
    put<int>(group, 0x2C, 1);
    put<int>(group, 0x34, 200 + id);
    put<short>(group, 0x5C, 1);
}
}

int main()
{
    nativeBindings.players = reinterpret_cast<unsigned int>(players);
    nativeBindings.unitRecords = reinterpret_cast<unsigned int>(units);
    nativeBindings.unitCapacity = 4;
    nativeBindings.tribes = reinterpret_cast<unsigned int>(tribes);
    nativeBindings.tribeStride = 0x334;
    nativeBindings.aicRecords = reinterpret_cast<unsigned int>(aic);
    nativeBindings.mapRows = reinterpret_cast<unsigned int>(rows);
    nativeBindings.teams = reinterpret_cast<unsigned int>(teams);
    nativeBindings.gameTick = reinterpret_cast<unsigned int>(&tick);
    nativeBindings.tribePath = reinterpret_cast<unsigned int>(&path);
    nativeBindings.siegeGroupMove = reinterpret_cast<unsigned int>(&move);
    for (int row = 0; row < 400; ++row) rows[row * 3] = row * 400;
    teams[1] = 1; teams[2] = 2;
    put<int>(players + 1 * 0x39F4, 0x2300, 2);
    put<int>(players + 2 * 0x39F4, 0x2300, 3);
    put<int>(players + 1 * 0x39F4, 0x2BD8, 2);
    put<int>(players + 1 * 0x39F4, 0x98, 50);
    put<int>(players + 1 * 0x39F4, 0x9C, 50);
    put<int>(players + 2 * 0x39F4, 0x98, 100);
    put<int>(players + 2 * 0x39F4, 0x9C, 100);
    put<int>(aic, 0x1EC, 10);
    combatCensusValid = 1;
    combatCensus[2].lord = 1;
    combatCensus[2].lordUID = 777;
    siegeHarassPolicy[1] = 1;
    siegeHarassMinimum[1] = 4; // Explicit minimum of three engines.
    setupEngine(1, 39);
    setupEngine(2, 77);
    tick = combatCensusTick = 1000;
    resetSiegeHarassCensus();
    countSiegeHarassUnit(1);
    countSiegeHarassUnit(2);
    assert(siegeHarassCensus[1].count == 2);
    assert(suppressNativeSiegeHarassMove(0x490) == 1);
    updateSiegeHarassment(aic, 1);
    assert(pathCalls == 0 && moveCalls == 0);
    setupEngine(3, 39);
    tick = combatCensusTick = 1100;
    resetSiegeHarassCensus();
    for (int id = 1; id <= 3; ++id) countSiegeHarassUnit(id);
    updateSiegeHarassment(aic, 1);
    assert(pathCalls == 3 && moveCalls == 3);
    assert(moveX[1] != moveX[2] || moveY[1] != moveY[2]);
    assert(moveX[2] != moveX[3] || moveY[2] != moveY[3]);
    assert(siegeHarassPlans[1].phase == 1);
    siegeHarassPolicy[1] = 2;
    assert(suppressNativeSiegeHarassMove(0x490) == 0);
    tick = combatCensusTick = 1500;
    updateSiegeHarassment(aic, 1);
    assert(moveCalls == 3);
    siegeHarassPolicy[1] = 1;
    siegeHarassMinimum[1] = 1; // Explicit zero: launch any ready engine.
    std::memset(&siegeHarassPlans[1], 0, sizeof(SiegeHarassPlan));
    tick = combatCensusTick = 1600;
    resetSiegeHarassCensus();
    countSiegeHarassUnit(1);
    updateSiegeHarassment(aic, 1);
    assert(moveCalls == 4);
    combatCensus[2].lord = 0;
    updateSiegeHarassment(aic, 1);
    assert(siegeHarassPlans[1].target == 0);
    // With a complete census but one blocked route, wait for a useful group
    // initially and move the reachable members after the one-month timeout.
    combatCensus[2].lord = 1;
    siegeHarassMinimum[1] = 4;
    blockedTribe = 3;
    std::memset(&siegeHarassPlans[1], 0, sizeof(SiegeHarassPlan));
    tick = combatCensusTick = 2000;
    resetSiegeHarassCensus();
    for (int id = 1; id <= 3; ++id) countSiegeHarassUnit(id);
    const int beforeBlocked = moveCalls;
    updateSiegeHarassment(aic, 1);
    assert(moveCalls == beforeBlocked && pathCalls >= 3);
    tick = combatCensusTick = 2800;
    updateSiegeHarassment(aic, 1);
    assert(moveCalls == beforeBlocked + 2);
    return 0;
}
