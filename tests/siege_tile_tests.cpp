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
unsigned char tiles[0x23D7E0 + 8];
int originalCalls;

int __fastcall originalTile(void*, void*, int, int, int, int)
{
    ++originalCalls;
    return 0;
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
    setShort(units, 1 * 0x490 + 0x2A0, 1);
    assert(checkedSiegeTile(tiles, 0, 0, 1, 190, 0) == 0 && originalCalls == 3);
    setShort(units, 1 * 0x490 + 0x2A0, 0);
    assert(checkedSiegeTile(tiles, 0, 0, 1, 189, 0) == 0 && originalCalls == 4);
    assert(checkedSiegeTile(tiles, 0, 160000, 1, 190, 0) == 0 && originalCalls == 5);
    return 0;
}
