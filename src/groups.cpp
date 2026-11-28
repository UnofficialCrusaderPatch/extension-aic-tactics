#include "aic_tactics/groups.hpp"
#include "aic_tactics/runtime.hpp"
#include "aic_tactics/shc141_groups.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

DetachedGroup detachedGroups[9][DetachedGroupsPerPlayer];

namespace {
const int TribeCapacity = 1250;
template<class T> T& at(unsigned int address) { return *reinterpret_cast<T*>(address); }
typedef int (__thiscall *CreateTribe)(void*, int);
typedef void (__thiscall *TribeAction)(void*, int, int);
typedef int (__thiscall *FindGroup)(void*, int, int, int);

unsigned int tribeAddress(int tribe) { return nativeBindings.tribes + tribe * nativeBindings.tribeStride; }
unsigned int unitAddress(int unit) { return nativeBindings.unitRecords + unit * 0x490; }

bool tribeMatches(int player, int tribe, unsigned int uid)
{
    return tribe > 0 && tribe < TribeCapacity && at<unsigned int>(tribeAddress(tribe) + 0x34) == uid
        && at<int>(tribeAddress(tribe) + 0x2C) == player && at<short>(tribeAddress(tribe) + 0x40) == 2;
}

// A living unit of the player that belongs to the given tribe.
bool member(int player, int unit, int tribe)
{
    if (unit <= 0 || unit >= static_cast<int>(nativeBindings.unitCapacity)) return false;
    const unsigned int address = unitAddress(unit);
    return at<short>(address + 0x96) == player && at<short>(address + 0x8C) == 2
        && at<short>(address + 0x2A0) == 0 && at<int>(address + 0x3C8) > 0
        && at<short>(address + 0x2D8) == tribe
        && at<unsigned int>(address + 0x2E4) == at<unsigned int>(tribeAddress(tribe) + 0x34);
}
} // namespace

int openDetachedGroup(int player, int duty)
{
    if (player < 1 || player > 8 || duty <= NoDuty || duty >= GroupDutyCount) return -1;
    int index = 0;
    while (index < DetachedGroupsPerPlayer && detachedGroups[player][index].duty != NoDuty) ++index;
    if (index == DetachedGroupsPerPlayer) return -1;
    TribeAvailability available;
    if (!queryTribeAvailability(reinterpret_cast<const unsigned char*>(nativeBindings.tribes + 0x28),
            TribeCapacity, nativeBindings.tribeStride, player, available) || available.freeSlots == 0) return -1;
    const int tribe = reinterpret_cast<CreateTribe>(nativeBindings.createTribe)(
        reinterpret_cast<void*>(nativeBindings.tribes), player);
    if (tribe <= 0 || tribe >= TribeCapacity) return -1;
    DetachedGroup& group = detachedGroups[player][index];
    std::memset(&group, 0, sizeof(group));
    group.tribe = tribe;
    group.uid = at<unsigned int>(tribeAddress(tribe) + 0x34);
    group.duty = duty;
    group.startTick = at<unsigned int>(nativeBindings.gameTick);
    return index;
}

bool detachedGroupValid(int player, int index)
{
    if (player < 1 || player > 8 || index < 0 || index >= DetachedGroupsPerPlayer) return false;
    const DetachedGroup& group = detachedGroups[player][index];
    return group.duty != NoDuty && tribeMatches(player, group.tribe, group.uid);
}

int detachedGroupSize(int player, int index)
{
    return detachedGroupValid(player, index)
        ? at<short>(tribeAddress(detachedGroups[player][index].tribe) + 0x5C) : 0;
}

int detachedGroupLeader(int player, int index)
{
    if (!detachedGroupValid(player, index)) return 0;
    const int tribe = detachedGroups[player][index].tribe;
    const int leader = at<short>(tribeAddress(tribe) + 0x5A);
    return member(player, leader, tribe) ? leader : 0;
}

bool moveUnitToDetachedGroup(int player, int index, int unit)
{
    if (!detachedGroupValid(player, index)) return false;
    DetachedGroup& group = detachedGroups[player][index];
    if (group.returning || unit <= 0 || unit >= static_cast<int>(nativeBindings.unitCapacity)) return false;
    const int from = at<short>(unitAddress(unit) + 0x2D8);
    if (from <= 0 || from >= TribeCapacity || from == group.tribe || !member(player, unit, from)) return false;
    // Native add/remove maintain size, selection bits, the unit's tribe UID and speed.
    reinterpret_cast<TribeAction>(nativeBindings.removeUnitFromTribe)(
        reinterpret_cast<void*>(nativeBindings.tribes), unit, from);
    reinterpret_cast<TribeAction>(nativeBindings.addUnitToTribe)(
        reinterpret_cast<void*>(nativeBindings.tribes), unit, group.tribe);
    ++group.launchSize;
    return true;
}

int returnDetachedGroup(void* aic, int player, int index, int budget)
{
    if (!detachedGroupValid(player, index)) {
        if (player >= 1 && player <= 8 && index >= 0 && index < DetachedGroupsPerPlayer)
            std::memset(&detachedGroups[player][index], 0, sizeof(DetachedGroup));
        return 0;
    }
    DetachedGroup& group = detachedGroups[player][index];
    group.returning = 1;
    const int words = static_cast<int>(nativeBindings.tribeMemberWords);
    int moved = 0;
    for (int inspected = 0; inspected < words && moved < budget; ++inspected) {
        if (group.cursor < 0 || group.cursor >= words) group.cursor = 0;
        const unsigned int bits = at<unsigned short>(tribeAddress(group.tribe) + 0x60 + group.cursor * 2);
        bool finished = true;
        for (int bit = 0; bit < 16; ++bit) {
            if (!(bits & (1U << bit))) continue;
            const int unit = group.cursor * 16 + bit;
            if (!member(player, unit, group.tribe)) continue;
            const int role = at<short>(unitAddress(unit) + 0x42A);
            const int destination = role >= 10 && role <= 20
                ? reinterpret_cast<FindGroup>(nativeBindings.findAttackGroup)(aic, player, unit, role) : 0;
            if (destination <= 0 || destination >= TribeCapacity || destination == group.tribe) continue;
            reinterpret_cast<TribeAction>(nativeBindings.removeUnitFromTribe)(
                reinterpret_cast<void*>(nativeBindings.tribes), unit, group.tribe);
            reinterpret_cast<TribeAction>(nativeBindings.addUnitToTribe)(
                reinterpret_cast<void*>(nativeBindings.tribes), unit, destination);
            if (++moved == budget) { finished = false; break; }
        }
        if (finished) ++group.cursor;
    }
    const int left = detachedGroupSize(player, index);
    if (left <= 0) std::memset(&group, 0, sizeof(group));
    return left;
}

void validateDetachedGroups()
{
    for (int player = 1; player <= 8; ++player)
        for (int index = 0; index < DetachedGroupsPerPlayer; ++index)
            if (detachedGroups[player][index].duty != NoDuty && !detachedGroupValid(player, index))
                std::memset(&detachedGroups[player][index], 0, sizeof(DetachedGroup));
}

} // namespace SHC141
} // namespace AicTactics
