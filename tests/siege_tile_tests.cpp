#include "aic_tactics/runtime.hpp"
#include <cassert>
#include <cstring>

namespace AicTactics { namespace SHC141 {
CharacterConfiguration configurations[17];
NativeBindings nativeBindings;
}}

using namespace AicTactics::SHC141;

namespace {
unsigned char players[9 * 0x39F4 + 0x2304];
unsigned char units[4 * 0x490];
unsigned char tiles[0x23D7E0 + 80400 * 2];
unsigned char tribes[3 * 0x334];
unsigned char points[216 * 32];
unsigned char rows[400 * 12];
int placementFail;
int originalCalls;
int resourceCalls;
int resourceResult;
int checkedPlayer;
int checkedCommand;
int placeResult;
int placeCalls;
int popCalls;
int addCalls;
int crewMembers[4];
void setShort(unsigned char* memory, unsigned int offset, short value);

int __fastcall originalTile(void*, void*, int, int, int, int)
{
    ++originalCalls;
    return 0;
}

int __fastcall checkResources(void*, void*, int command, int player, int sound)
{
    assert(sound == 0);
    ++resourceCalls;
    checkedPlayer = player;
    checkedCommand = command;
    return resourceResult;
}

int __fastcall originalPlace(void*, void*, int tribe, int command, unsigned int distance,
    int instruction)
{
    assert(tribe == 1 && command == 190 && distance == 60 && instruction == 15);
    ++placeCalls;
    return placeResult;
}

int __fastcall popCrew(void*, void*, int tribe)
{
    assert(tribe == 1);
    ++popCalls;
    short& size = *reinterpret_cast<short*>(tribes + 0x334 + 0x5C);
    if (size <= 0) return 0;
    const int unit = crewMembers[--size];
    setShort(units, unit * 0x490 + 0x2A0, 0);
    return unit;
}

int __fastcall addCrew(void*, void*, unsigned int unit, int tribe)
{
    assert(unit > 0 && unit < 4 && (tribe == 1 || tribe == 2));
    ++addCalls;
    short& size = *reinterpret_cast<short*>(tribes + tribe * 0x334 + 0x5C);
    ++size;
    setShort(units, unit * 0x490 + 0x2A0, static_cast<short>(tribe));
    return 1;
}

void setShort(unsigned char* memory, unsigned int offset, short value)
{
    *reinterpret_cast<short*>(memory + offset) = value;
}
void setInt(unsigned char* memory, unsigned int offset, int value)
{
    *reinterpret_cast<int*>(memory + offset) = value;
}
}

int main()
{
    std::memset(players, 0, sizeof(players));
    std::memset(units, 0, sizeof(units));
    std::memset(tiles, 0, sizeof(tiles));
    nativeBindings.players = reinterpret_cast<unsigned int>(players);
    nativeBindings.unitRecords = reinterpret_cast<unsigned int>(units);
    nativeBindings.unitCapacity = 4;
    nativeBindings.siegeTileOccupancyOffset = 0x23D7E0;
    nativeBindings.originalSiegeTileCheck = reinterpret_cast<unsigned int>(&originalTile);
    nativeBindings.siegeTileMap = reinterpret_cast<unsigned int>(tiles);
    nativeBindings.siegeTentPointX = reinterpret_cast<unsigned int>(points);
    nativeBindings.tribes = reinterpret_cast<unsigned int>(tribes);
    nativeBindings.tribeStride = 0x334;
    nativeBindings.mapRows = reinterpret_cast<unsigned int>(rows);
    nativeBindings.siegeTribeIndexOffset = 0x2D8;
    nativeBindings.siegePlacementFail = reinterpret_cast<unsigned int>(&placementFail);
    nativeBindings.siegeResourceCheck = reinterpret_cast<unsigned int>(&checkResources);
    nativeBindings.gameState = 1;
    nativeBindings.siegePlaceTent = reinterpret_cast<unsigned int>(&originalPlace);
    nativeBindings.popUnitFromTribe = reinterpret_cast<unsigned int>(&popCrew);
    nativeBindings.addUnitToTribe = reinterpret_cast<unsigned int>(&addCrew);
    setInt(players, 1 * 0x39F4 + 0x2300, 2);
    setInt(players, 2 * 0x39F4 + 0x2300, 3);
    setShort(tiles, 0x23D7E0, 1);
    setShort(units, 1 * 0x490 + 0x8E, 39);
    setShort(units, 1 * 0x490 + 0x96, 1);

    // Two AIs with opposite explicit choices can coexist. An absent value
    // inherits the module fallback, and disabling never changes native calls.
    safePlacementFallback = 1;
    assert(checkedSiegeTile(tiles, 0, 0, 1, 190, 0) == 1 && originalCalls == 0);
    configurations[1].safeSiegePlacement = 2;
    assert(checkedSiegeTile(tiles, 0, 0, 1, 190, 0) == 0 && originalCalls == 1);
    configurations[2].safeSiegePlacement = 1;
    setShort(units, 1 * 0x490 + 0x96, 2);
    assert(checkedSiegeTile(tiles, 0, 0, 2, 190, 0) == 1 && originalCalls == 1);
    safePlacementFallback = 0;
    configurations[1].safeSiegePlacement = 0;
    assert(checkedSiegeTile(tiles, 0, 0, 1, 190, 0) == 0 && originalCalls == 2);
    configurations[1].safeSiegePlacement = 1;
    setShort(units, 1 * 0x490 + 0x96, 1);
    assert(checkedSiegeTile(tiles, 0, 0, 1, 190, 0) == 1 && originalCalls == 2);
    assert(checkedSiegeTile(tiles, 0, 0, 1, 210, 0) == 1 && originalCalls == 2);
    assert(checkedSiegeTile(tiles, 0, 0, 1, 358, 0) == 1 && originalCalls == 2);
    setShort(units, 1 * 0x490 + 0x8E, 30);
    assert(checkedSiegeTile(tiles, 0, 0, 1, 190, 0) == 1 && originalCalls == 2);
    setShort(units, 1 * 0x490 + 0x8E, 39);
    setShort(units, 1 * 0x490 + 0x2A0, 1);
    assert(checkedSiegeTile(tiles, 0, 0, 1, 190, 0) == 0 && originalCalls == 3);
    setShort(units, 1 * 0x490 + 0x2A0, 0);
    assert(checkedSiegeTile(tiles, 0, 0, 1, 189, 0) == 0 && originalCalls == 4);
    assert(checkedSiegeTile(tiles, 0, 160000, 1, 190, 0) == 0 && originalCalls == 5);

    // The search should skip an occupied candidate and keep searching. A
    // different personality can explicitly retain the native selector.
    setInt(tribes, 0x334 + 0x2C, 1);
    setInt(points, 32, 10);
    setInt(points, 36, 10);
    for (int y = 0; y < 400; ++y) setInt(rows, y * 12, y * 100);
    configurations[1].safeSiegePlacement = 1;
    assert(siegeTentCandidateAllowed(1, 1) == 1);
    const int neighbor = 9 * 100 + 9;
    setShort(tiles, 0x23D7E0 + neighbor * 2, 1);
    assert(siegeTentCandidateAllowed(1, 1) == 0);
    setShort(tiles, 0x23D7E0 + neighbor * 2, 0);
    setShort(tiles, 0x2029B0 + neighbor * 2, 3);
    assert(siegeTentCandidateAllowed(1, 1) == 0);
    setInt(tribes, 0x334 + 0x2C, 2);
    configurations[2].safeSiegePlacement = 2;
    assert(siegeTentCandidateAllowed(1, 1) == 1);
    assert(siegeTentCandidateAllowed(1, 216) == 1);
    assert(siegeTentCandidateAllowed(0, 216) == 1);
    configurations[2].safeSiegePlacement = 0;
    assert(siegeTentCandidateAllowed(1, 1) == 1);
    setInt(tribes, 0x334 + 0x2C, 1);
    setInt(points, 32, 0);
    assert(siegeTentCandidateAllowed(1, 1) == 0);

    // A failed final native admission must undo only the new reservation and
    // must not issue a construction order for building zero.
    setInt(tribes, 0x334 + 0x34, 42);
    setShort(tribes, 0x334 + 0x2D8, 1);
    setShort(tribes, 0x334 + 0x2DA, 2);
    setInt(points, 32 + 0x10, 1);
    setInt(points, 32 + 0x14, 42);
    setInt(points, 32 + 0x18, 3);
    placementFail = 0;
    assert(failedSiegeTent(1, 1) == 0);
    assert(*reinterpret_cast<int*>(points + 32 + 0x10) == 1);
    placementFail = 1;
    assert(failedSiegeTent(1, 1) == 1);
    assert(*reinterpret_cast<int*>(points + 32 + 0x10) == 0);
    assert(*reinterpret_cast<int*>(points + 32 + 0x14) == 0);
    assert(*reinterpret_cast<int*>(points + 32 + 0x18) == 0);
    assert(*reinterpret_cast<int*>(points + 64 + 0x10) == 1);
    assert(*reinterpret_cast<int*>(points + 64 + 0x14) == 42);
    assert(*reinterpret_cast<short*>(tribes + 0x334 + 0x2D8) == 2);
    configurations[1].safeSiegePlacement = 2;
    setInt(points, 32 + 0x10, 1);
    assert(failedSiegeTent(1, 1) == 0);
    assert(*reinterpret_cast<int*>(points + 32 + 0x10) == 1);

    siegePaymentFallback = 0;
    siegePaymentPolicy[1] = 1;
    resourceResult = 0;
    assert(siegeResourceAdmission(1, 190) == 0 && resourceCalls == 1);
    assert(checkedPlayer == 1 && checkedCommand == 190);
    assert(siegePaymentEnabledForGoldOffset(0x39F4) == 1);
    assert(siegeResourceAdmission(1, 210) == 0 && resourceCalls == 2);
    assert(checkedCommand == 210);
    siegePaymentPolicy[1] = 2;
    assert(siegeResourceAdmission(1, 190) == 1 && resourceCalls == 2);
    assert(siegePaymentEnabledForGoldOffset(0x39F4) == 0);
    siegePaymentPolicy[1] = 0;
    siegePaymentFallback = 1;
    resourceResult = 1;
    assert(siegeResourceAdmission(1, 190) == 1 && resourceCalls == 3);
    assert(siegePaymentEnabledForGoldOffset(0x39F4) == 1);
    assert(siegeResourceAdmission(1, 189) == 1 && resourceCalls == 3);
    assert(siegeResourceAdmission(0, 190) == 1 && resourceCalls == 3);
    assert(siegePaymentEnabledForGoldOffset(0x39F4 + 1) == 0);

    // The assault caller ignores a failed placement. Return only its newly
    // selected engineer crew to the native attack-engineer group, and leave
    // successful construction and explicit native policy untouched.
    siegePaymentFallback = 0;
    siegePaymentPolicy[1] = 2;
    safePlacementFallback = 0;
    configurations[1].safeSiegePlacement = 1;
    setInt(tribes, 0x334 + 0x2C, 1);
    setInt(tribes, 2 * 0x334 + 0x2C, 1);
    setInt(tribes, 2 * 0x334 + 0x34, 77);
    setShort(players, 0x39F4 + 0x3130, 2);
    setInt(players, 0x39F4 + 0x32E4, 77);
    setShort(units, 2 * 0x490 + 0x8E, 30);
    setShort(units, 2 * 0x490 + 0x96, 1);
    setShort(units, 2 * 0x490 + 0x42A, 10);
    setShort(units, 3 * 0x490 + 0x8E, 30);
    setShort(units, 3 * 0x490 + 0x96, 1);
    setShort(units, 3 * 0x490 + 0x42A, 10);
    crewMembers[0] = 2;
    crewMembers[1] = 3;
    setShort(tribes, 0x334 + 0x5C, 2);
    setShort(tribes, 2 * 0x334 + 0x5C, 0);
    placeResult = 0;
    assert(placeSiegeTentAndRecoverEngineers(0, 0, 1, 190, 60, 15) == 0);
    assert(placeCalls == 1 && popCalls == 2 && addCalls == 2);
    assert(*reinterpret_cast<short*>(tribes + 0x334 + 0x5C) == 0);
    assert(*reinterpret_cast<short*>(tribes + 2 * 0x334 + 0x5C) == 2);
    setShort(tribes, 0x334 + 0x5C, 2);
    placeResult = 1;
    assert(placeSiegeTentAndRecoverEngineers(0, 0, 1, 190, 60, 15) == 1);
    assert(placeCalls == 2 && popCalls == 2 && addCalls == 2);
    placeResult = 0;
    configurations[1].safeSiegePlacement = 2;
    assert(placeSiegeTentAndRecoverEngineers(0, 0, 1, 190, 60, 15) == 0);
    assert(placeCalls == 3 && popCalls == 2 && addCalls == 2);
    return 0;
}
