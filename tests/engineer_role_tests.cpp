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
unsigned char tribes[2 * 0x334 + 0x80];
template<class T> void put(unsigned char* area, unsigned int offset, T value)
{
    *reinterpret_cast<T*>(area + offset) = value;
}
}

int main()
{
    std::memset(players, 0, sizeof(players));
    std::memset(units, 0, sizeof(units));
    std::memset(tribes, 0, sizeof(tribes));
    nativeBindings.players = reinterpret_cast<unsigned int>(players);
    nativeBindings.unitRecords = reinterpret_cast<unsigned int>(units);
    nativeBindings.unitCapacity = 4;
    nativeBindings.tribes = reinterpret_cast<unsigned int>(tribes);
    nativeBindings.tribeStride = 0x334;
    put<int>(players, 1 * 0x39F4 + 0x2300, 2);
    put<int>(players, 2 * 0x39F4 + 0x2300, 3);
    put<short>(units, 1 * 0x490 + 0x8C, 2);
    put<short>(units, 1 * 0x490 + 0x8E, 30);
    put<short>(units, 1 * 0x490 + 0x96, 1);
    put<int>(units, 1 * 0x490 + 0x3C8, 100);
    put<short>(units, 1 * 0x490 + 0x42A, 1);
    put<short>(units, 1 * 0x490 + 0x2D8, 1);
    put<unsigned int>(units, 1 * 0x490 + 0x2E4, 987);
    put<int>(tribes, 0x334 + 0x2C, 1);
    put<unsigned int>(tribes, 0x334 + 0x34, 987);
    put<short>(tribes, 0x334 + 0x40, 2);
    put<unsigned short>(tribes, 0x334 + 0x60, 2);

    engineerRoleFallback = 1;
    assert(countableEngineerRole(1, 1) == 1);
    engineerRoleFallback = 0;
    assert(countableEngineerRole(1, 1) == 0);
    engineerRoleFallback = 1;
    assert(countableEngineerRole(2, 1) == 0);
    put<short>(units, 1 * 0x490 + 0x2D8, 0);
    assert(countableEngineerRole(1, 1) == 0);
    put<short>(units, 1 * 0x490 + 0x2D8, 1);
    put<unsigned int>(units, 1 * 0x490 + 0x2E4, 988);
    assert(countableEngineerRole(1, 1) == 0);
    put<unsigned int>(units, 1 * 0x490 + 0x2E4, 987);
    put<short>(tribes, 0x334 + 0x50, 0x410);
    assert(countableEngineerRole(1, 1) == 0);
    put<short>(tribes, 0x334 + 0x50, 0);
    put<unsigned char>(units, 1 * 0x490 + 0x32F, 2);
    assert(countableEngineerRole(1, 1) == 0);
    put<unsigned char>(units, 1 * 0x490 + 0x32F, 0);
    put<short>(units, 1 * 0x490 + 0x42E, 1);
    assert(countableEngineerRole(1, 1) == 0);
    put<short>(units, 1 * 0x490 + 0x42E, 0);
    put<short>(units, 1 * 0x490 + 0x2A0, 1);
    assert(countableEngineerRole(1, 1) == 0);
    put<short>(units, 1 * 0x490 + 0x2A0, 0);
    put<short>(units, 1 * 0x490 + 0x42A, 10);
    assert(countableEngineerRole(1, 1) == 0);
    put<short>(units, 1 * 0x490 + 0x42A, 2);
    assert(countableEngineerRole(1, 1) == 1);
    put<short>(units, 1 * 0x490 + 0x42A, 5);
    assert(countableEngineerRole(1, 1) == 0);
    return 0;
}
