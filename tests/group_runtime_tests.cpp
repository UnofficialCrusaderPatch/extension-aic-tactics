// Executes the detached group owner against mock game data and mock native
// group actions on both tribe layouts (Crusader 0x334, Extreme 0x688).
#include "aic_tactics/runtime.hpp"
#include "aic_tactics/groups.hpp"
#include <cassert>
#include <cstring>

namespace AicTactics { namespace SHC141 {
NativeBindings nativeBindings;
}}

using namespace AicTactics::SHC141;

namespace {
unsigned char units[16 * 0x490];
unsigned char tribes[1250 * 0x688];
unsigned int tick = 1234;
unsigned int nextUID = 500;
void* const AIC = reinterpret_cast<void*>(0x1234);
int createdTribe = 7;
int returnTribe = 3;

template<class T> void put(unsigned char* base, unsigned int offset, T value)
{
    *reinterpret_cast<T*>(base + offset) = value;
}
template<class T> T get(unsigned char* base, unsigned int offset)
{
    return *reinterpret_cast<T*>(base + offset);
}
unsigned char* tribe(int id) { return tribes + id * nativeBindings.tribeStride; }
unsigned char* unit(int id) { return units + id * 0x490; }

void live(int id, int player, unsigned int uid)
{
    put<int>(tribe(id), 0x2C, player);
    put<unsigned int>(tribe(id), 0x34, uid);
    put<short>(tribe(id), 0x40, 2);
}

// Native thiscall routines, called through __thiscall pointers: __fastcall with
// an ignored edx has the same register use and callee stack cleanup.
int __fastcall create(void* owner, void*, int player)
{
    assert(owner == static_cast<void*>(tribes) && player == 1);
    live(createdTribe, player, ++nextUID);
    return createdTribe;
}
void __fastcall remove(void* owner, void*, int id, int from)
{
    assert(owner == static_cast<void*>(tribes) && get<short>(unit(id), 0x2D8) == from);
    put<short>(tribe(from), 0x5C, static_cast<short>(get<short>(tribe(from), 0x5C) - 1));
    put<unsigned short>(tribe(from), 0x60, static_cast<unsigned short>(get<unsigned short>(tribe(from), 0x60) & ~(1U << id)));
    put<short>(unit(id), 0x2D8, 0);
}
void __fastcall add(void* owner, void*, int id, int to)
{
    assert(owner == static_cast<void*>(tribes) && get<short>(unit(id), 0x2D8) == 0);
    put<short>(tribe(to), 0x5C, static_cast<short>(get<short>(tribe(to), 0x5C) + 1));
    put<unsigned short>(tribe(to), 0x60, static_cast<unsigned short>(get<unsigned short>(tribe(to), 0x60) | (1U << id)));
    put<short>(unit(id), 0x2D8, static_cast<short>(to));
    put<unsigned int>(unit(id), 0x2E4, get<unsigned int>(tribe(to), 0x34));
}
int __fastcall find(void* aic, void*, int player, int, int role)
{
    assert(aic == AIC && player == 1);
    return role == 20 ? returnTribe : 0;
}

void soldier(int id, int from, int role)
{
    put<short>(unit(id), 0x96, 1);
    put<short>(unit(id), 0x8C, 2);
    put<short>(unit(id), 0x2A0, 0);
    put<int>(unit(id), 0x3C8, 100);
    put<short>(unit(id), 0x42A, static_cast<short>(role));
    put<short>(unit(id), 0x2D8, 0);
    add(tribes, 0, id, from);
}

void setup(unsigned int stride)
{
    std::memset(units, 0, sizeof(units));
    std::memset(tribes, 0, sizeof(tribes));
    std::memset(detachedGroups, 0, sizeof(detachedGroups));
    nativeBindings.unitRecords = reinterpret_cast<unsigned int>(units);
    nativeBindings.unitCapacity = 16;
    nativeBindings.tribes = reinterpret_cast<unsigned int>(tribes);
    nativeBindings.tribeStride = stride;
    nativeBindings.tribeMemberWords = 1;
    nativeBindings.gameTick = reinterpret_cast<unsigned int>(&tick);
    nativeBindings.createTribe = reinterpret_cast<unsigned int>(&create);
    nativeBindings.removeUnitFromTribe = reinterpret_cast<unsigned int>(&remove);
    nativeBindings.addUnitToTribe = reinterpret_cast<unsigned int>(&add);
    nativeBindings.findAttackGroup = reinterpret_cast<unsigned int>(&find);
    live(returnTribe, 1, 300);
    live(2, 1, 200);
    for (int id = 1; id <= 4; ++id) soldier(id, 2, 20);
}

void run(unsigned int stride)
{
    setup(stride);
    const int index = openDetachedGroup(1, HuntDuty);
    assert(index == 0 && detachedGroupValid(1, 0) && detachedGroups[1][0].tribe == createdTribe);
    assert(detachedGroups[1][0].duty == HuntDuty && detachedGroups[1][0].startTick == tick);
    for (int id = 1; id <= 3; ++id) assert(moveUnitToDetachedGroup(1, 0, id));
    assert(detachedGroupSize(1, 0) == 3 && detachedGroups[1][0].launchSize == 3);
    // A dying unit, a foreign unit and a unit already in the group are not moved.
    put<short>(unit(4), 0x2A0, 1);
    assert(!moveUnitToDetachedGroup(1, 0, 4));
    put<short>(unit(4), 0x2A0, 0);
    put<short>(unit(4), 0x96, 2);
    assert(!moveUnitToDetachedGroup(1, 0, 4));
    assert(!moveUnitToDetachedGroup(1, 0, 1));
    // Returning hands members back through the native attack group, in budgets.
    assert(returnDetachedGroup(AIC, 1, 0, 2) == 1 && detachedGroups[1][0].returning == 1);
    assert(!moveUnitToDetachedGroup(1, 0, 4));
    assert(get<short>(unit(1), 0x2D8) == returnTribe && get<short>(unit(2), 0x2D8) == returnTribe);
    assert(returnDetachedGroup(AIC, 1, 0, 2) == 0 && detachedGroups[1][0].duty == NoDuty);
    // A member without a native destination stays until one exists.
    assert(openDetachedGroup(1, ReliefDuty) == 0 && moveUnitToDetachedGroup(1, 0, 1));
    put<short>(unit(1), 0x42A, 16);
    assert(returnDetachedGroup(AIC, 1, 0, 4) == 1 && detachedGroups[1][0].duty == ReliefDuty);
    put<short>(unit(1), 0x42A, 20);
    assert(returnDetachedGroup(AIC, 1, 0, 4) == 0);
    // A record whose tribe was recycled is dropped.
    assert(openDetachedGroup(1, PatrolDuty) == 0);
    put<unsigned int>(tribe(createdTribe), 0x34, 1);
    assert(!detachedGroupValid(1, 0));
    validateDetachedGroups();
    assert(detachedGroups[1][0].duty == NoDuty);
    // Records are limited per player; invalid duties and players are refused.
    for (int slot = 0; slot < DetachedGroupsPerPlayer; ++slot) assert(openDetachedGroup(1, SplitDuty) == slot);
    assert(openDetachedGroup(1, SplitDuty) == -1);
    assert(openDetachedGroup(1, NoDuty) == -1 && openDetachedGroup(9, HuntDuty) == -1);
}
}

int main()
{
    run(0x334);
    run(0x688);
    return 0;
}
