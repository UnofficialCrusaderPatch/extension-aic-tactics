#include "aic_tactics/army.hpp"
#include "aic_tactics/runtime.hpp"
#include "aic_tactics/shc141_groups.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

ReserveState reserves[9];
int reserveGroupOwner[1250];
unsigned int reserveGroupUID[1250];

namespace {
const unsigned int PlayerStride = 0x39F4;
const unsigned int& Tribes = nativeBindings.tribes;
const unsigned int& Units = nativeBindings.unitRecords;
const int slots[22] = {18,17,11,12,13,14,15,16,186,187,188,189,190,191,192,193,194,195,196,197,198,199};
const int roles[22] = {10,11,12,13,14,15,16,16,17,17,17,18,19,19,20,20,20,20,20,20,20,20};
template<class T> T& at(unsigned int address) { return *reinterpret_cast<T*>(address); }
int& field(int player, unsigned int address) { return at<int>(address + player * PlayerStride); }
int reservePlayer;
int excludedCounts[9][11];
typedef void (__thiscall *PlayerAction)(void*, int);
typedef int (__thiscall *TwoQuery)(void*, int, int);
typedef void (__thiscall *TwoAction)(void*, int, int);

bool enabled(int player)
{
    if (player < 1 || player > 8) return false;
    const int character = field(player, (nativeBindings.players + 0x2300));
    return character >= 2 && character <= 17
        && configurations[character - 1].preparation == PrepareDuringAttack;
}

int aicValue(void* aic, int player, int offset)
{
    const int character = field(player, (nativeBindings.players + 0x2300));
    if (character < 2 || character > 17) return 0;
    return *reinterpret_cast<int*>(static_cast<unsigned char*>(aic) + (character - 1) * 0x2A4 + offset);
}

bool validGroup(const ReserveGroup& group, int player)
{
    return group.id > 0 && group.id < 1250
        && at<unsigned int>(Tribes + group.id * nativeBindings.tribeStride + 0x34) == group.uid
        && at<int>(Tribes + group.id * nativeBindings.tribeStride + 0x2C) == player
        && at<short>(Tribes + group.id * nativeBindings.tribeStride + 0x40) == 2;
}

ReserveGroup activeGroup(int player, int index)
{
    ReserveGroup result;
    result.id = at<short>((nativeBindings.players + 0x310C) + player * PlayerStride + slots[index] * 2);
    result.uid = at<unsigned int>((nativeBindings.players + 0x329C) + player * PlayerStride + slots[index] * 4);
    return result;
}

void setActiveGroup(int player, int index, const ReserveGroup& group)
{
    at<short>((nativeBindings.players + 0x310C) + player * PlayerStride + slots[index] * 2) = static_cast<short>(group.id);
    at<unsigned int>((nativeBindings.players + 0x329C) + player * PlayerStride + slots[index] * 4) = group.uid;
}

void indexGroups(int player)
{
    for (int index = 0; index < 22; ++index) {
        ReserveGroup& group = reserves[player].groups[index];
        if (!validGroup(group, player)) { group.id = 0; group.uid = 0; continue; }
        reserveGroupOwner[group.id] = player;
        reserveGroupUID[group.id] = group.uid;
    }
}

int size(const ReserveGroup& group, int player)
{
    if (!validGroup(group, player)) return 0;
    const int count = at<short>(Tribes + group.id * nativeBindings.tribeStride + 0x5C);
    return count > 0 && count <= static_cast<int>(nativeBindings.unitCapacity) ? count : 0;
}

int combatLimit(void* aic, int player)
{
    // The native writer already applied the selected Legacy growth/cap and RNG.
    const __int64 desired = static_cast<__int64>(aicValue(aic, player, 0x1F4)) + field(player, (nativeBindings.players + 0x38A0));
    return desired <= 0 ? 0 : desired > nativeBindings.unitCapacity ? static_cast<int>(nativeBindings.unitCapacity) : static_cast<int>(desired);
}

int roleLimit(void* aic, int player, int role)
{
    const int offsets[11] = {0x23C,0x244,0x250,0x24C,0x254,0x258,0x260,0x26C,0x278,0x280,0x298};
    if (role < 10 || role > 20) return 0;
    int result = aicValue(aic, player, offsets[role - 10]);
    if (role == 10) {
        const __int64 wave = static_cast<__int64>(field(player, (nativeBindings.players + 0x3924))) * 4;
        if (wave < result) result = wave <= 0 ? 0 : static_cast<int>(wave);
    }
    if (role == 20 && result < combatLimit(aic, player)) result = combatLimit(aic, player);
    return result <= 0 ? 0 : result > static_cast<int>(nativeBindings.unitCapacity) ? static_cast<int>(nativeBindings.unitCapacity) : result;
}

bool destinationRoom(void* aic, int player, int role)
{
    int first = 0, count = 0;
    for (int index = 0; index < 22; ++index) {
        if (roles[index] == role) { if (!count) first = index; ++count; }
    }
    if (!count) return false;
    int configured = 1;
    if (role == 16) configured = aicValue(aic, player, 0x264);
    if (role == 17) configured = aicValue(aic, player, 0x270);
    if (role == 19) configured = aicValue(aic, player, 0x284);
    if (role == 20) configured = aicValue(aic, player, 0x29C);
    if (configured < 1) configured = 1;
    if (configured > count) return false;
    for (int index = first; index < first + configured; ++index) {
        const ReserveGroup group = activeGroup(player, index);
        if (!validGroup(group, player)) {
            // The native selector checks UID, not lifecycle. Wait for its
            // pending deletion to retire the UID before admitting a purchase.
            if (group.id > 0 && group.id < 1250
                && at<unsigned int>(Tribes + group.id * nativeBindings.tribeStride + 0x34) == group.uid) return false;
            TribeAvailability available;
            return queryTribeAvailability(reinterpret_cast<const unsigned char*>(Tribes + 0x28),
                1250, nativeBindings.tribeStride, player, available) && available.freeSlots > 0;
        }
        const int groupLimit = configured > 1 ? 1000 : 3200;
        const int poolLimit = static_cast<int>(nativeBindings.unitCapacity);
        if (size(group, player) < (groupLimit < poolLimit ? groupLimit : poolLimit)) return true;
    }
    return false;
}

void transferReturningReserve(void* aic, int player)
{
    ReserveState& state = reserves[player];
    int moved = 0, inspectedWords = 0;
    // Only the native groups' member bitsets are walked. The saved cursors keep
    // dense and empty groups within the same work budget on every peer.
    while (moved < 16 && inspectedWords < 64) {
        if (state.transferSlot < 0 || state.transferSlot >= 22) state.transferSlot = 0;
        if (state.transferWord < 0 || state.transferWord >= static_cast<int>(nativeBindings.tribeMemberWords)) state.transferWord = 0;
        const int index = state.transferSlot;
        const int role = roles[index];
        ReserveGroup& source = state.groups[index];
        if (!validGroup(source, player) || size(source, player) == 0) {
            state.transferSlot = (index + 1) % 22;
            state.transferWord = 0;
            ++inspectedWords;
            continue;
        }
        const int current = field(player, (nativeBindings.players + 0x2BA8) + (role - 10) * 4);
        const int combat = field(player, (nativeBindings.players + 0x30F0)) - field(player, (nativeBindings.players + 0x2BA8));
        if (current >= roleLimit(aic, player, role) || (role != 10 && combat >= combatLimit(aic, player))) {
            state.transferSlot = (index + 1) % 22;
            state.transferWord = 0;
            ++inspectedWords;
            continue;
        }
        unsigned int bits = at<unsigned short>(Tribes + source.id * nativeBindings.tribeStride + 0x60 + state.transferWord * 2);
        ++inspectedWords;
        bool wordFinished = true;
        for (int bit = 0; bit < 16; ++bit) {
            const int unit = state.transferWord * 16 + bit;
            if (!(bits & (1U << bit)) || unit <= 0 || unit >= static_cast<int>(nativeBindings.unitCapacity) || !isReserveUnit(unit)) continue;
            const unsigned int address = Units + unit * 0x490;
            if (at<short>(address + 0x8C) != 2 || at<short>(address + 0x2A0) != 0 || at<int>(address + 0x3C8) <= 0
                || at<short>(address + 0x42A) != role) continue;
            if (!destinationRoom(aic, player, role)) return;
            typedef int (__thiscall *FindGroup)(void*, int, int, int);
            const int destination = reinterpret_cast<FindGroup>(0x4CCD20)(aic, player, unit, role);
            if (destination <= 0 || destination >= 1250 || destination == source.id) return;
            // Native add/remove maintain size, selection bits, unit group UID
            // and movement speed. No raw membership or unit-order writes.
            reinterpret_cast<TwoAction>(0x525A70)(reinterpret_cast<void*>(Tribes), unit, source.id);
            reinterpret_cast<TwoAction>(nativeBindings.addUnitToTribe)(reinterpret_cast<void*>(Tribes), unit, destination);
            ++field(player, (nativeBindings.players + 0x2BA8) + (role - 10) * 4);
            ++field(player, (nativeBindings.players + 0x30F0));
            ++moved;
            if (moved == 16 || field(player, (nativeBindings.players + 0x2BA8) + (role - 10) * 4) >= roleLimit(aic, player, role)
                || (role != 10 && field(player, (nativeBindings.players + 0x30F0)) - field(player, (nativeBindings.players + 0x2BA8)) >= combatLimit(aic, player))) {
                wordFinished = false;
                break;
            }
        }
        if (wordFinished && ++state.transferWord >= static_cast<int>(nativeBindings.tribeMemberWords)) {
            state.transferWord = 0;
            state.transferSlot = (index + 1) % 22;
        }
    }
}
} // namespace

bool reserveRecruitmentActive(int player) { return reservePlayer == player && player != 0; }

int __cdecl isReserveUnit(int unit)
{
    if (unit <= 0 || unit >= static_cast<int>(nativeBindings.unitCapacity)) return 0;
    const unsigned int address = Units + unit * 0x490;
    const int group = at<short>(address + 0x2D8);
    if (group <= 0 || group >= 1250) return 0;
    const int player = reserveGroupOwner[group];
    if (!enabled(player) || at<short>(address + 0x96) != player
        || at<unsigned int>(address + 0x2E4) != reserveGroupUID[group]) return 0;
    ReserveGroup identity = {group, reserveGroupUID[group]};
    if (!validGroup(identity, player)) return 0;
    return (at<unsigned short>(Tribes + group * nativeBindings.tribeStride + 0x60 + (unit / 16) * 2)
        & (1U << (unit % 16))) != 0 ? 1 : 0;
}

bool reserveRoleAvailable(void* aic, int player, int role)
{
    if (!enabled(player)) return true;
    // This also guards the shared native type lookup used by defense and raids.
    // Preparation owns only the offensive army's roles.
    if (role < 10 || role > 20) return true;
    if (reserves[player].returning && field(player, (nativeBindings.players + 0x2BA4)) == 0) {
        // Recruitment precedes the army update. Give already paid reserves
        // their bounded transfer opportunity before buying this role again.
        for (int index = 0; index < 22; ++index)
            if (roles[index] == role && size(reserves[player].groups[index], player) > 0) return false;
    }
    int combat = 0, roleCount = 0;
    for (int index = 0; index < 22; ++index) {
        const int count = size(activeGroup(player, index), player);
        if (roles[index] != 10) combat += count;
        if (roles[index] == role) roleCount += count;
    }
    return roleCount < roleLimit(aic, player, role)
        && (role == 10 || combat < combatLimit(aic, player)) && destinationRoom(aic, player, role);
}

void resetReserveCensus()
{
    std::memset(reserveGroupOwner, 0, sizeof(reserveGroupOwner));
    std::memset(reserveGroupUID, 0, sizeof(reserveGroupUID));
    std::memset(excludedCounts, 0, sizeof(excludedCounts));
    for (int player = 1; player <= 8; ++player) if (enabled(player)) indexGroups(player);
}

void countReserveUnit(int unit)
{
    if (!isReserveUnit(unit)) return;
    const unsigned int address = Units + unit * 0x490;
    // Match the original AI role census's admission; its normal loop can still
    // visit dying records before their native military flag is cleared.
    const int type = at<short>(address + 0x8E);
    const int role = at<short>(address + 0x42A);
    if (at<short>(address + 0x2A4) == 0 || type == 55 || role < 10 || role > 20
        || (type == 30 && role != 10)) return;
    ++excludedCounts[at<short>(address + 0x96)][role - 10];
}

void completeReserveCensus()
{
    for (int player = 1; player <= 8; ++player) {
        if (!enabled(player)) continue;
        int total = 0;
        for (int role = 0; role < 11; ++role) {
            const int count = excludedCounts[player][role];
            int& original = field(player, (nativeBindings.players + 0x2BA8) + role * 4);
            if (count > original) continue;
            original -= count;
            total += count;
        }
        int& attack = field(player, (nativeBindings.players + 0x30F0));
        if (total <= attack) attack -= total;
    }
}

void __fastcall recruitWithReserve(void* aic, void*, int player)
{
    if (!enabled(player) || !reserves[player].deployed || reserves[player].returning || reservePlayer) {
        reinterpret_cast<PlayerAction>(0x4D3AE0)(aic, player);
        return;
    }
    ReserveState& state = reserves[player];
    ReserveGroup deployed[22];
    int savedCounts[11], counts[11] = {0};
    int oldSizes[22];
    int total = 0;
    for (int index = 0; index < 22; ++index) {
        deployed[index] = activeGroup(player, index);
        if (!validGroup(state.groups[index], player)) { state.groups[index].id = 0; state.groups[index].uid = 0; }
        oldSizes[index] = size(state.groups[index], player);
        counts[roles[index] - 10] += oldSizes[index];
        total += oldSizes[index];
        setActiveGroup(player, index, state.groups[index]);
    }
    for (int role = 0; role < 11; ++role) {
        savedCounts[role] = field(player, (nativeBindings.players + 0x2BA8) + role * 4);
        field(player, (nativeBindings.players + 0x2BA8) + role * 4) = counts[role];
    }
    const int savedTotal = field(player, (nativeBindings.players + 0x30F0));
    const int savedPhase = field(player, (nativeBindings.players + 0x2BA4));
    const int savedCursor = field(player, (nativeBindings.players + 0x3108));
    field(player, (nativeBindings.players + 0x30F0)) = total;
    field(player, (nativeBindings.players + 0x2BA4)) = 0;
    field(player, (nativeBindings.players + 0x3108)) = state.recruitCursor;
    reservePlayer = player;
    reinterpret_cast<PlayerAction>(0x4D3AE0)(aic, player);
    reservePlayer = 0;
    state.recruitCursor = field(player, (nativeBindings.players + 0x3108));
    field(player, (nativeBindings.players + 0x3108)) = savedCursor;
    field(player, (nativeBindings.players + 0x2BA4)) = savedPhase;
    field(player, (nativeBindings.players + 0x30F0)) = savedTotal;
    for (int role = 0; role < 11; ++role) field(player, (nativeBindings.players + 0x2BA8) + role * 4) = savedCounts[role];
    for (int index = 0; index < 22; ++index) {
        state.groups[index] = activeGroup(player, index);
        setActiveGroup(player, index, deployed[index]);
        if (size(state.groups[index], player) > oldSizes[index]) {
            reinterpret_cast<TwoAction>(0x4CD110)(aic, state.groups[index].id, player);
            at<short>(Tribes + state.groups[index].id * nativeBindings.tribeStride + nativeBindings.tribeStance) = 1;
        }
    }
    indexGroups(player);
}

int __fastcall reserveRecruitType(void* aic, void*, int player, int role)
{
    if (!reserveRoleAvailable(aic, player, role)) return 0;
    return reinterpret_cast<TwoQuery>(0x4CC250)(aic, player, role);
}

void prepareArmyUpdate(void* aic, int player)
{
    if (!enabled(player)) return;
    ReserveState& state = reserves[player];
    if (state.returning && field(player, (nativeBindings.players + 0x2BA4)) == 0) transferReturningReserve(aic, player);
}

void finishArmyUpdate(int player, int phaseBefore)
{
    const bool reserveDeployed = enabled(player) && reserves[player].deployed && !reserves[player].returning;
    if (!reserveDeployed && !committedAttackActive(player)) return;
    int& phase = field(player, (nativeBindings.players + 0x2BA4));
    if (phase == 0 && phaseBefore == 9) {
        // A native timeout alone does not prove that surviving troops received
        // return orders. Keep cleanup pending if its destination was absent.
        if (!(targetLifecycle[player] & 2) && !(enabled(player) && reserves[player].returning)) phase = 8;
        return;
    }
    if (targetLifecycle[player] & 2) return;
    if (phase == 0 && phaseBefore == 6) phase = 6;
    else if (phase == 0) phase = 8; // Failed initial rally uses the native cleanup path.
}

void noteArmyLaunch(int player)
{
    if (!enabled(player)) return;
    reserves[player].deployed = 1;
    reserves[player].returning = 0;
}

void noteArmyReturn(int player)
{
    if (!enabled(player)) return;
    reserves[player].deployed = 0;
    reserves[player].returning = 1;
}

bool returnArmyToCampfire(void* aic, int player)
{
    if ((!enabled(player) && !targetPolicyActive(player)) || field(player, (nativeBindings.players + 0x1D4)) <= 0) return false;
    for (int index = 0; index < 22; ++index) {
        const ReserveGroup group = activeGroup(player, index);
        if (!validGroup(group, player) || size(group, player) <= 0) continue;
        reinterpret_cast<TwoAction>(0x4CD110)(aic, group.id, player);
        at<short>(Tribes + group.id * nativeBindings.tribeStride + nativeBindings.tribeStance) = 1;
    }
    return true;
}

} // namespace SHC141
} // namespace AicTactics
