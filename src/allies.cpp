#include "aic_tactics/allies.hpp"
#include "aic_tactics/diagnostics.hpp"
#include "aic_tactics/groups.hpp"
#include "aic_tactics/patrols.hpp"
#include "aic_tactics/runtime.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

AllyBindings allyBindings;
int allySettings[17][AllySettingCount];
int allyFallback[AllySettingCount] = {1, 1, 1000, 300, 25, 30, 75, 0, 0, 0};
AllyState allyStates[9];

namespace {
const unsigned int PlayerStride = 0x39F4;
const int TribeCapacity = 1250;
const int SlotCount = 200;
// Member moves into or out of a relief group per AI update.
const int TransferBudget = 32;
// A refused ask waits four AI updates (one month) before asking again.
const unsigned int RetryTicks = AllyTicksPerMonth;
// Mercenary post, barracks, armoury, granary, market: without them a lord
// cannot defend itself, so wood and stone reach it first while it rebuilds.
const int KeyBuildings[AllyKeyBuildingCount] = {8, 9, 11, 19, 26};
// Native player fields (PlayerData).
const unsigned int Keep = 0x94, KeyX = 0x98, KeyY = 0x9C, KeepTile = 0x1E0, CampX = 0x1D8, CampY = 0x1DC;
const unsigned int Resources = 0x4D0, TotalFood = 0x209C, AivID = 0x22FC, Character = 0x2300;
const unsigned int ToAcquire = 0x2A70, StrengthState = 0x2B74, Phase = 0x2BA4, AttackEngineers = 0x2BA8;
const unsigned int AttackTroops = 0x30F0, StrengthFeeling = 0x30F4, Nervous = 0x3820;
const unsigned int HelpRequest = 0x384C, RequestDelay = 0x3898, WaveExtra = 0x38A0, HelpAsker = 0x38E4;
const int RequestHelp = 2, WeakFeeling = 1;
// Native building fields (record = buildings + 0x14 + id * 0x32C).
const unsigned int BuildingStride = 0x32C, BuildingLife = 0xD0, BuildingType = 0xD2, BuildingOwner = 0xD6;
const unsigned int BuildingX = 0xEE, BuildingY = 0xF0, BuildingSize = 0xF8, GateLink = 0x2A2, GateState = 0x2A3;
const int GateClosed = 2, GateOpening = 0xB;
// Tiles beyond a gatehouse's edge that count as at its door.
const int GateReach = 5;
// Doors tried per order: the nearest first, each with one path query.
const int DoorAttempts = 3;
const int TileCapacity = 80400;
// Unit state (unit +0x2C0) while walking to its ordered destination.
const short WalkingState = 101;
// AIC record fields (AIC Loader field index * 4).
const int AicRequestDelay = 0xA0, AicMinimumGoods = 0xA4, AicVariance = 0xC0, AicRecruitGold = 0xC4;
const int AicAttackForce = 0x1F4, AicSupportThreshold = 0x1FC;
// AIVState: per plan 0x6D98 bytes from +4 (sub type +0x10, step goal +0x14,
// steps +0x20), steps of 12 bytes from +0x34: state byte (0 not placed yet)
// and building type word.
const unsigned int AivSpec = 0x6D98, AivSubType = 0x14, AivGoal = 0x18, AivTotal = 0x24, AivSteps = 0x38;
const int AivStepRow = 0x922, AivStep = 12, AivMaxSteps = 1000;
// Tunnellers, engineers, laddermen and siege engines stay home first.
bool stayHomeFirst(int type)
{
    return type == 5 || type == 29 || type == 30 || (type >= 39 && type <= 41)
        || (type >= 58 && type <= 61) || type == 77;
}

int keyCounts[9][AllyKeyBuildingCount];

template<class T> T& at(unsigned int address) { return *reinterpret_cast<T*>(address); }
typedef int (__thiscall *ThreeQuery)(void*, int, int, int);
typedef void (__thiscall *ThreeAction)(void*, int, int, int);
typedef void (__thiscall *FourAction)(void*, int, int, int, int);
typedef void (__thiscall *TwoAction)(void*, int, int);
typedef int (__thiscall *OneQuery)(void*, int);
typedef int (__thiscall *TribeQuery)(void*, int, int);

unsigned int playerAddress(int player) { return nativeBindings.players + player * PlayerStride; }
int& field(int player, unsigned int offset) { return at<int>(playerAddress(player) + offset); }
unsigned int tribeAddress(int tribe) { return nativeBindings.tribes + tribe * nativeBindings.tribeStride; }
unsigned int unitAddress(int unit) { return nativeBindings.unitRecords + unit * 0x490; }
unsigned int now() { return at<unsigned int>(nativeBindings.gameTick); }
int month() { return static_cast<int>(now() / AllyTicksPerMonth); }

bool isAI(int player) { return player >= 1 && player <= 8 && field(player, Character) != 0; }
int character(int player)
{
    const int value = isAI(player) ? field(player, Character) - 1 : 0;
    return value >= 1 && value <= 16 ? value : 0;
}
int setting(int player, int which)
{
    const int stored = allySettings[character(player)][which];
    return stored ? stored - 1 : allyFallback[which];
}
int playerAicValue(void* aic, int player, int offset) { return aicValue(aic, field(player, Character), offset); }
int team(int player) { return at<int>(nativeBindings.teams + player * 4); }
bool allied(int a, int b)
{
    return a != b && a >= 1 && a <= 8 && b >= 1 && b <= 8 && team(a) != 0 && team(a) == team(b);
}
bool lordAlive(int player)
{
    return player >= 1 && player <= 8 && reinterpret_cast<OneQuery>(allyBindings.lordAlive)(
        reinterpret_cast<void*>(nativeBindings.units), player) != 0;
}
int& gold(int player) { return field(player, Resources + AllyGold * 4); }
int stock(int player, int resource) { return field(player, Resources + resource * 4); }
bool nervous(int player) { return field(player, Nervous) != 0; }
bool weak(int player) { return field(player, StrengthFeeling) == WeakFeeling; }
bool hasKeep(int player) { return field(player, Keep) != 0; }

AllyState& monthState(int player)
{
    AllyState& state = allyStates[player];
    if (state.month != month()) {
        state.month = month();
        std::memset(state.given, 0, sizeof(state.given));
    }
    return state;
}

int allowance(int player, int resource, int have)
{
    return allyAllowance(have, monthState(player).given[resource], setting(player, AllyMonthlyGive));
}

void recordGift(int giver, int receiver, int resource, int amount)
{
    monthState(giver).given[resource] += amount;
    if (resource != AllyGold) allyStates[receiver].receivedMonth[resource] = month() + 1;
}

void addStatistic(unsigned int table, int player, int value)
{
    at<int>(table + player * 4) += value;
}

// Has the giver built its castle as far as its plan lets it? Until then it
// gives no gold, so its starting gold goes into the castle. Players without a
// plan count as done.
bool castleDone(int player)
{
    const unsigned int state = allyBindings.aivState;
    const int plan = field(player, AivID);
    if (!state || plan <= 0 || plan > 8) return true;
    const unsigned int spec = state + plan * AivSpec;
    if (at<int>(spec + AivSubType) == 0) return true;
    const int total = at<int>(spec + AivTotal);
    if (total <= 0 || total > AivMaxSteps) return true;
    if (at<int>(spec + AivGoal) < total) return false;
    for (int step = 1; step <= total; ++step) {
        const unsigned int entry = state + AivSteps + (plan * AivStepRow + step) * AivStep;
        if (at<short>(entry + 2) != 0 && at<unsigned char>(entry) == 0) return false;
    }
    return true;
}

void sendGoods(int giver, int receiver, int resource, int amount)
{
    void* buildings = reinterpret_cast<void*>(nativeBindings.buildings);
    reinterpret_cast<ThreeAction>(allyBindings.resourceGain)(buildings, receiver, resource, amount);
    reinterpret_cast<FourAction>(allyBindings.resourceLoss)(buildings, giver, resource, amount, 0);
    const int value = reinterpret_cast<ThreeQuery>(allyBindings.sellPrice)(
        reinterpret_cast<void*>(nativeBindings.gameState), giver, resource, amount);
    addStatistic(allyBindings.goodsReceived, receiver, value);
    addStatistic(allyBindings.goodsSent, giver, value);
    recordGift(giver, receiver, resource, amount);
    diagnostic("player %d sends player %d %d of resource %d", giver, receiver, amount, resource);
    // The wish is met, as a purchase meets it; left standing it is asked again.
    field(receiver, ToAcquire + resource * 4) = 0;
}

void sendGold(int giver, int receiver, int amount)
{
    gold(giver) -= amount;
    gold(receiver) += amount;
    addStatistic(allyBindings.goodsReceived, receiver, amount);
    addStatistic(allyBindings.goodsSent, giver, amount);
    recordGift(giver, receiver, AllyGold, amount);
    diagnostic("player %d sends player %d %d gold", giver, receiver, amount);
    if (!isAI(receiver))
        reinterpret_cast<TwoAction>(allyBindings.goodsSentSpeech)(
            reinterpret_cast<void*>(allyBindings.aiState), giver, receiver);
}

// An AI ally with goods help on, not threatened or weak, and better off than
// the receiver by its native strength score.
bool mayGive(int giver, int receiver)
{
    return isAI(giver) && setting(giver, AllyGoods) && allied(giver, receiver) && lordAlive(giver)
        && !nervous(giver) && !weak(giver) && field(giver, StrengthState) > field(receiver, StrengthState);
}

// Best off first: strength score, then gold, then player number.
int givers(int receiver, int list[8])
{
    int count = 0;
    for (int player = 1; player <= 8; ++player) {
        if (!mayGive(player, receiver)) continue;
        int index = count++;
        while (index > 0) {
            const int other = list[index - 1];
            const int a = field(player, StrengthState), b = field(other, StrengthState);
            if (a < b || (a == b && gold(player) <= gold(other))) break;
            list[index] = other;
            --index;
        }
        list[index] = player;
    }
    return count;
}

// AIC storage limit sellExcessGoods holds each good to (MaxWood, MaxStone,
// MaxFood, MaxBeer, MaxEquipment, MaxResourceOther).
int storageField(int resource)
{
    if (resource == 2) return 0xAC;
    if (resource == 4) return 0xB0;
    if (resource == 9 || allyFood(resource)) return 0x80;
    if (resource == 14) return 0xBC;
    if (resource >= 17 && resource <= 24) return 0xB8;
    if (resource == 3 || resource == 6 || resource == 7 || resource == 16) return 0xB4;
    return 0;
}

int storageRoom(void* aic, int player, int resource)
{
    const int offset = storageField(resource);
    if (!offset) return -1;
    const int room = playerAicValue(aic, player, offset) + playerAicValue(aic, player, AicVariance) - stock(player, resource);
    return room > 0 ? room : 0;
}

bool receivedRecently(int player, int resource)
{
    const int got = allyStates[player].receivedMonth[resource];
    return got > 0 && got - 1 <= month() && month() - (got - 1) < AllyRegiftMonths;
}

int goodsAmount(void* aic, int giver, int receiver, int resource, int wanted)
{
    AllyGoodsOffer offer;
    offer.wanted = wanted;
    offer.stock = stock(giver, resource);
    offer.giverWantsIt = field(giver, ToAcquire + resource * 4) > 0;
    offer.receivedRecently = receivedRecently(giver, resource);
    if (offer.giverWantsIt || offer.receivedRecently || offer.stock <= 0) return 0;
    offer.enoughForAll = reinterpret_cast<ThreeQuery>(allyBindings.enoughAfterTrade)(
        aic, giver, resource, wanted) != 0;
    offer.receiverWeak = weak(receiver);
    offer.minimumAfterTrade = playerAicValue(aic, giver, AicMinimumGoods);
    offer.receiverRoom = storageRoom(aic, receiver, resource);
    offer.allowance = allowance(giver, resource, offer.stock);
    return allyGoodsAmount(offer);
}

int goldAmount(int giver, int receiver, int amount)
{
    if (!castleDone(giver)) return 0;
    return allyGoldAmount(gold(giver), setting(giver, AllyGoldKeep), amount, weak(receiver),
        allowance(giver, AllyGold, gold(giver)));
}

void shareGold(int giver, int receiver)
{
    if (!castleDone(giver)) return;
    const int gift = allyGoldShare(gold(giver), setting(giver, AllyGoldKeep), gold(receiver),
        setting(giver, AllyGoldShare), allowance(giver, AllyGold, gold(giver)));
    if (gift > 0) sendGold(giver, receiver, gift);
}

int rebuilding(int player)
{
    const AllyState& state = allyStates[player];
    for (int index = 0; index < AllyKeyBuildingCount; ++index)
        if (state.keySeen[index] > state.keyNow[index]) return KeyBuildings[index];
    return 0;
}

struct Want { int resource; int amount; int priority; };

// The native purchase routine asks only human allies for what the AI cannot
// afford. Here the asker offers those wants to its AI allies directly, by
// the native request rules, and asks for the gold it is still short: from
// AI allies, else from human allies with the native request.
void askAllies(void* aic, int receiver)
{
    AllyState& state = allyStates[receiver];
    if (field(receiver, RequestDelay) > 0 || now() < state.nextAskTick) return;
    Want wants[AllyResourceCount];
    int count = 0, total = 0, cheapest = -1;
    for (int resource = 0; resource < AllyResourceCount; ++resource) {
        const int want = field(receiver, ToAcquire + resource * 4);
        if (want <= 0 || resource == AllyGold) continue;
        const int price = reinterpret_cast<ThreeQuery>(allyBindings.buyPrice)(
            reinterpret_cast<void*>(nativeBindings.gameState), receiver, resource, want);
        total = price > 0x7FFFFFFF - total ? 0x7FFFFFFF : total + (price > 0 ? price : 0);
        if (cheapest < 0 || price < cheapest) cheapest = price;
        if (gold(receiver) < price) {
            Want entry = {resource, want, 0};
            wants[count++] = entry;
        }
    }
    if (!count) return;
    const int rebuild = rebuilding(receiver);
    for (int index = 0; index < count; ++index) {
        wants[index].priority = allyGoodsPriority(wants[index].resource, rebuild);
        for (int other = index; other > 0 && wants[other].priority < wants[other - 1].priority; --other) {
            const Want swap = wants[other];
            wants[other] = wants[other - 1];
            wants[other - 1] = swap;
        }
    }
    int list[8];
    const int available = givers(receiver, list);
    bool accepted = false;
    const bool starving = field(receiver, TotalFood) < 5;
    const bool danger = nervous(receiver);
    for (int index = 0; index < count; ++index) {
        const int resource = wants[index].resource;
        const bool forRebuild = rebuild && allyWoodOrStone(resource);
        if (allyNeverAsked(resource) || (allyFood(resource) && !starving)
            || (wants[index].priority == 3 && danger && !forRebuild)) continue;
        for (int giver = 0; giver < available; ++giver) {
            const int send = goodsAmount(aic, list[giver], receiver, resource, wants[index].amount);
            if (send <= 0) continue;
            sendGoods(list[giver], receiver, resource, send);
            shareGold(list[giver], receiver);
            accepted = true;
            break;
        }
    }
    if (cheapest >= 0 && gold(receiver) < cheapest) {
        int amount = total - gold(receiver);
        if (amount > AllyGoldAskMaximum) amount = AllyGoldAskMaximum;
        if (amount > 0) {
            bool given = false;
            for (int giver = 0; giver < available && !given; ++giver) {
                const int send = goldAmount(list[giver], receiver, amount);
                if (send <= 0) continue;
                sendGold(list[giver], receiver, send);
                given = accepted = true;
            }
            // Human allies see it on their ally screen.
            if (!given) reinterpret_cast<ThreeAction>(allyBindings.requestGoods)(aic, receiver, AllyGold, amount);
        }
    }
    if (accepted) {
        const int delay = playerAicValue(aic, receiver, AicRequestDelay);
        field(receiver, RequestDelay) = delay > 0 ? delay : 0;
    } else {
        state.nextAskTick = now() + RetryTicks;
    }
}

// A weak or threatened lord that cannot pay its recruits gets gold up to its
// recruit line (AIC RecruitGoldThreshold plus its buffer) from a better-off
// ally's gold above that ally's floor.
void recruitGold(void* aic, int receiver)
{
    if (field(receiver, RequestDelay) > 0 || !(weak(receiver) || nervous(receiver))) return;
    const int threshold = playerAicValue(aic, receiver, AicRecruitGold);
    const int need = (threshold > 0 ? threshold : 0) + setting(receiver, AllyRecruitGoldBuffer) - gold(receiver);
    if (need <= 0) return;
    int list[8];
    const int available = givers(receiver, list);
    for (int index = 0; index < available; ++index) {
        const int giver = list[index];
        const int send = castleDone(giver) ? allyRecruitGold(need, gold(giver), setting(giver, AllyGoldKeep),
            allowance(giver, AllyGold, gold(giver))) : 0;
        if (send <= 0) continue;
        sendGold(giver, receiver, send);
        const int delay = playerAicValue(aic, receiver, AicRequestDelay);
        field(receiver, RequestDelay) = delay > 0 ? delay : 0;
        return;
    }
}

// ---- relief ----

int homeThreat(int player)
{
    if (!combatCensusFresh(now())) return 0;
    int total = 0;
    for (int other = 1; other <= 8; ++other) {
        const int power = combatCensus[player].homePower[other];
        total = power > 0x7FFFFFFF - total ? 0x7FFFFFFF : total + power;
    }
    return total;
}

bool reliefOn(int player)
{
    return isAI(player) && setting(player, AllyRelief) && setting(player, AllyArmyShare) > 0;
}

bool ownDanger(int player)
{
    return nervous(player) || homeThreat(player) >= ReliefThreatPower;
}

// The native readiness comparison: attack troops without engineers reach
// AttForceBase plus this wave's random extra.
bool attackDue(void* aic, int player)
{
    return field(player, Phase) != 0 || static_cast<__int64>(field(player, AttackTroops))
        - field(player, AttackEngineers) >= static_cast<__int64>(playerAicValue(aic, player, AicAttackForce))
        + field(player, WaveExtra);
}

bool helpable(int helper, int ally)
{
    return allied(helper, ally) && hasKeep(ally) && lordAlive(ally);
}

// An attack-role tribe the native AI has registered in a slot.
int attackTribe(int player, int slot)
{
    TilePoint leader;
    const int tribe = validSlotTribe(player, slot, leader);
    if (!tribe) return 0;
    const int role = at<short>(unitAddress(at<short>(tribeAddress(tribe) + 0x5A)) + 0x42A);
    return role >= 10 && role <= 20 ? tribe : 0;
}

int idleArmy(int player)
{
    int total = 0;
    for (int slot = 0; slot < SlotCount; ++slot) {
        const int tribe = attackTribe(player, slot);
        if (tribe) total += at<short>(tribeAddress(tribe) + 0x5C);
    }
    return total;
}

// Moves up to `wanted` idle attack troops into the relief group in slot and
// member order, fighters before the troops that stay home first.
int fillRelief(int player, int index, int wanted)
{
    int moved = 0;
    if (wanted > TransferBudget) wanted = TransferBudget;
    for (int pass = 0; pass < 2 && moved < wanted; ++pass) {
        for (int slot = 0; slot < SlotCount && moved < wanted; ++slot) {
            const int tribe = attackTribe(player, slot);
            if (!tribe) continue;
            const int words = static_cast<int>(nativeBindings.tribeMemberWords);
            for (int word = 0; word < words && moved < wanted; ++word) {
                const unsigned int bits = at<unsigned short>(tribeAddress(tribe) + 0x60 + word * 2);
                for (int bit = 0; bit < 16 && bits && moved < wanted; ++bit) {
                    if (!(bits & (1U << bit))) continue;
                    const int unit = word * 16 + bit;
                    if (unit >= static_cast<int>(nativeBindings.unitCapacity)) continue;
                    if (stayHomeFirst(at<short>(unitAddress(unit) + 0x8E)) != (pass == 1)) continue;
                    if (moveUnitToDetachedGroup(player, index, unit)) ++moved;
                }
            }
        }
    }
    return moved;
}

int reliefGroup(int player)
{
    for (int index = 0; index < DetachedGroupsPerPlayer; ++index)
        if (detachedGroups[player][index].duty == ReliefDuty) return index;
    return -1;
}

bool reachable(void* aic, int tribe, TilePoint point)
{
    const int tile = movableTile(point);
    return tile && reinterpret_cast<TribeQuery>(nativeBindings.tribePath)(aic, tribe, tile);
}

bool move(void* aic, int tribe, int x, int y)
{
    TilePoint point = {x, y};
    return reachable(aic, tribe, point) && moveGroup(tribe, x, y);
}

unsigned int buildingAddress(int building) { return nativeBindings.buildings + 0x14 + building * BuildingStride; }
bool building(int id) { return id > 0 && id < static_cast<int>(nativeBindings.buildingCapacity); }
bool gatehouse(unsigned int record)
{
    const int type = at<short>(record + BuildingType);
    return type >= 45 && type <= 48;
}
int area(int tile) { return tile > 0 && tile < TileCapacity ? at<unsigned short>(nativeBindings.areaMap + tile * 2) : 0; }
// The ground inside the player's walls: the area map is built with every gate shut.
int keepArea(int player) { return hasKeep(player) ? area(field(player, KeepTile)) : 0; }

// Inside the player's walls, or on its keep (the keep's roof is not part of
// the ground area).
bool insideWalls(int unit, int player)
{
    const unsigned int address = unitAddress(unit);
    const int home = keepArea(player);
    if (home && area(at<int>(address + 0xD4)) == home) return true;
    const int keep = field(player, Keep);
    if (!building(keep)) return false;
    const unsigned int record = buildingAddress(keep);
    const int size = at<int>(record + BuildingSize);
    const int x = at<short>(address + 0xC4) - at<short>(record + BuildingX);
    const int y = at<short>(address + 0xC6) - at<short>(record + BuildingY);
    return x >= 0 && y >= 0 && x < size && y < size;
}

// A living member of the relief inside the helped ally's walls (`inside`) or
// outside them; with reach >= 0 only within `reach` tiles of (x, y).
bool memberAt(int helper, const DetachedGroup& group, bool inside, int x, int y, int reach)
{
    const int words = static_cast<int>(nativeBindings.tribeMemberWords);
    for (int word = 0; word < words; ++word) {
        const unsigned int bits = at<unsigned short>(tribeAddress(group.tribe) + 0x60 + word * 2);
        for (int bit = 0; bit < 16 && bits; ++bit) {
            if (!(bits & (1U << bit))) continue;
            const int unit = word * 16 + bit;
            if (unit >= static_cast<int>(nativeBindings.unitCapacity)) continue;
            const unsigned int address = unitAddress(unit);
            if (at<short>(address + 0x96) != helper || at<short>(address + 0x8C) != 2
                || at<short>(address + 0x2A0) != 0 || at<short>(address + 0x2D8) != group.tribe) continue;
            if (reach >= 0) {
                const int dx = at<short>(address + 0xC4) - x, dy = at<short>(address + 0xC6) - y;
                if (dx > reach || dx < -reach || dy > reach || dy < -reach) continue;
            }
            if (insideWalls(unit, group.target) == inside) return true;
        }
    }
    return false;
}

// Orders the group to a door of one of the ally's gatehouses, on the outside
// (going in) or the inside (leaving): the tile beside the middle of a
// gatehouse side, nearest the leader first, that the group can walk to.
bool orderToGate(void* aic, DetachedGroup& group, TilePoint leader, bool inside)
{
    const int home = keepArea(group.target);
    if (!home) return false;
    TilePoint tried[DoorAttempts];
    for (int attempt = 0; attempt < DoorAttempts; ++attempt) {
        TilePoint best = {0, 0};
        int bestDistance = -1;
        for (int id = 1; building(id); ++id) {
            const unsigned int record = buildingAddress(id);
            if (at<short>(record + BuildingLife) != 2 || at<short>(record + BuildingOwner) != group.target
                || !gatehouse(record)) continue;
            const int size = at<int>(record + BuildingSize);
            const int x = at<short>(record + BuildingX), y = at<short>(record + BuildingY);
            const TilePoint doors[4] = {{x + size / 2, y - 1}, {x + size / 2, y + size},
                {x - 1, y + size / 2}, {x + size, y + size / 2}};
            for (int door = 0; door < 4; ++door) {
                const int doorArea = area(movableTile(doors[door]));
                if (!doorArea || (doorArea == home) != inside) continue;
                bool seen = false;
                for (int index = 0; index < attempt; ++index)
                    if (tried[index].x == doors[door].x && tried[index].y == doors[door].y) seen = true;
                const int distance = patrolDistance(doors[door], leader);
                if (!seen && (bestDistance < 0 || distance < bestDistance)) {
                    best = doors[door];
                    bestDistance = distance;
                }
            }
        }
        if (bestDistance < 0) return false;
        tried[attempt] = best;
        if (move(aic, group.tribe, best.x, best.y)) {
            group.x = best.x;
            group.y = best.y;
            return true;
        }
    }
    return false;
}

// The relief's place inside: on the helped keep with the native help's order,
// or at the keep's entrance. False when the entrance cannot be reached. A
// group walking to the same tile is not ordered again unless `resend`: each
// native order searches formation tiles and paths every member again.
bool orderInside(void* aic, int helper, DetachedGroup& group, bool resend)
{
    const TilePoint entrance = {field(group.target, KeyX), field(group.target, KeyY)};
    const int keep = field(group.target, Keep);
    if (setting(helper, AllyReliefKeep) && building(keep)) {
        if (!reachable(aic, group.tribe, entrance)) return false;
        // The tile the native order sends the group to.
        const int x = at<short>(buildingAddress(keep) + BuildingX) + 3;
        const int y = at<short>(buildingAddress(keep) + BuildingY) + 4;
        if (resend || group.x != x || group.y != y)
            reinterpret_cast<TwoAction>(allyBindings.sendToKeep)(aic, group.tribe, group.target);
        group.x = x;
        group.y = y;
        return true;
    }
    const bool same = !resend && group.x == entrance.x && group.y == entrance.y;
    if (same ? !reachable(aic, group.tribe, entrance) : !move(aic, group.tribe, entrance.x, entrance.y)) return false;
    group.x = entrance.x;
    group.y = entrance.y;
    return true;
}

// The helped keep, else its camp if the keep cannot be reached. With gate
// opening on, a relief shut out walks to the nearest gate first and waits
// there for it to open. Returns false when the group can reach none of them.
// Members that just joined get the order even at the station: the native add
// gives none.
bool orderRelief(void* aic, int helper, DetachedGroup& group, bool joined)
{
    TilePoint leader;
    if (!tribeLeaderTile(helper, group.tribe, leader)) return true;
    const TilePoint station = {group.x, group.y};
    const bool there = !joined && patrolDistance(station, leader) <= 4;
    const bool gates = setting(helper, AllyReliefGates) != 0;
    if (there && (!gates || memberAt(helper, group, true, 0, 0, -1))) return true;
    // The native help order sets this stance on the tribes it sends.
    at<short>(tribeAddress(group.tribe) + nativeBindings.tribeStance) = 2;
    const int head = at<short>(tribeAddress(group.tribe) + 0x5A);
    if (orderInside(aic, helper, group, joined || at<short>(unitAddress(head) + 0x2C0) != WalkingState)) return true;
    if (gates) {
        // Waiting at a door until the gate opens.
        if (there && (group.x != field(group.target, CampX) || group.y != field(group.target, CampY))) return true;
        if (orderToGate(aic, group, leader, false)) return true;
    }
    if (move(aic, group.tribe, field(group.target, CampX), field(group.target, CampY))) {
        group.x = field(group.target, CampX);
        group.y = field(group.target, CampY);
        return true;
    }
    return false;
}

bool requestStands(int helper, int ally)
{
    return field(helper, HelpRequest) == RequestHelp && field(helper, HelpAsker) == ally;
}

// Home, or for troops shut inside the ally's walls the inside door of its
// nearest gate, which opens for them. The group's tile is (0, 0) until a door
// is chosen; near the chosen door the order is not repeated.
void leaveAlly(void* aic, int helper, DetachedGroup& group)
{
    TilePoint leader;
    if (move(aic, group.tribe, field(helper, CampX), field(helper, CampY))
        || !tribeLeaderTile(helper, group.tribe, leader)) return;
    const TilePoint door = {group.x, group.y};
    if ((!door.x && !door.y) || patrolDistance(door, leader) > 4) orderToGate(aic, group, leader, true);
}

void endRelief(void* aic, int helper, int index, int reason)
{
    DetachedGroup& group = detachedGroups[helper][index];
    AllyState& state = allyStates[helper];
    static const char* const reasons[] = {"", "ally gone", "own danger", "own attack", "time up", "threat over",
        "switched off", "no route"};
    diagnostic("player %d stops helping player %d (%s)", helper, group.target,
        reason > 0 && reason <= ReliefNoRoute ? reasons[reason] : "");
    if (state.reliefRequested && requestStands(helper, group.target)) field(helper, HelpRequest) = 0;
    state.reliefRequested = 0;
    state.reliefPauseUntil = reason == ReliefAttack ? 0 : now() + ReliefPauseTicks;
    if (setting(helper, AllyReliefGates) && allied(helper, group.target)
        && memberAt(helper, group, true, 0, 0, -1)) {
        group.leaving = now() ? now() : 1;
        group.x = 0;
        group.y = 0;
        leaveAlly(aic, helper, group);
        return;
    }
    move(aic, group.tribe, field(helper, CampX), field(helper, CampY));
    returnDetachedGroup(aic, helper, index, TransferBudget);
}

// The ally to relieve: a standing help request first, else the most
// threatened ally home (ties by player number).
int reliefTarget(void* aic, int helper, int& requested)
{
    requested = 0;
    const int asker = field(helper, HelpAsker);
    if (field(helper, HelpRequest) == RequestHelp && helpable(helper, asker)) {
        requested = 1;
        return asker;
    }
    if (now() < allyStates[helper].reliefPauseUntil
        || field(helper, AttackTroops) < playerAicValue(aic, helper, AicSupportThreshold)) return 0;
    int best = 0, bestThreat = ReliefThreatPower - 1;
    for (int ally = 1; ally <= 8; ++ally) {
        if (!helpable(helper, ally)) continue;
        const int threat = homeThreat(ally);
        if (threat > bestThreat) { best = ally; bestThreat = threat; }
    }
    return best;
}

void openRelief(void* aic, int helper)
{
    if (attackDue(aic, helper) || ownDanger(helper)) return;
    int requested = 0;
    const int ally = reliefTarget(aic, helper, requested);
    if (!ally) return;
    const int size = allyReliefSize(idleArmy(helper), setting(helper, AllyArmyShare));
    if (size <= 0) return;
    const int index = openDetachedGroup(helper, ReliefDuty);
    if (index < 0) return;
    DetachedGroup& group = detachedGroups[helper][index];
    group.target = ally;
    group.targetUID = static_cast<unsigned int>(combatCensus[ally].lordUID);
    group.x = field(ally, KeyX);
    group.y = field(ally, KeyY);
    if (!fillRelief(helper, index, size)) {
        returnDetachedGroup(aic, helper, index, TransferBudget);
        return;
    }
    AllyState& state = allyStates[helper];
    state.reliefRequested = requested;
    state.reliefCalmSince = now();
    diagnostic("player %d helps player %d with %d troops%s", helper, ally, detachedGroupSize(helper, index),
        requested ? " on request" : "");
    if (!orderRelief(aic, helper, group, true)) endRelief(aic, helper, index, ReliefNoRoute);
}

void updateRelief(void* aic, int helper)
{
    // A character with relief on but no army share sends no troops, also
    // when asked; its accepted request is dropped.
    if (setting(helper, AllyRelief) && setting(helper, AllyArmyShare) == 0
        && field(helper, HelpRequest) == RequestHelp) field(helper, HelpRequest) = 0;
    if (field(helper, Phase) != 0) allyStates[helper].reliefPauseUntil = 0;
    const int index = reliefGroup(helper);
    if (index < 0) {
        if (reliefOn(helper)) openRelief(aic, helper);
        return;
    }
    DetachedGroup& group = detachedGroups[helper][index];
    if (!detachedGroupValid(helper, index)) {
        allyStates[helper].reliefRequested = 0;
        allyStates[helper].reliefPauseUntil = now() + ReliefPauseTicks;
        returnDetachedGroup(aic, helper, index, TransferBudget);
        return;
    }
    if (group.returning) {
        returnDetachedGroup(aic, helper, index, TransferBudget);
        return;
    }
    AllyState& state = allyStates[helper];
    // The record's target is a player only for this duty; a bad one ends it.
    if (group.target < 1 || group.target > 8) {
        endRelief(aic, helper, index, ReliefAllyGone);
        return;
    }
    // An ended relief leaves the ally's walls before its troops rejoin, for
    // at most one month.
    if (group.leaving) {
        if (allied(helper, group.target) && now() - group.leaving < ReliefCalmTicks
            && memberAt(helper, group, true, 0, 0, -1)) {
            leaveAlly(aic, helper, group);
            return;
        }
        group.leaving = 0;
        move(aic, group.tribe, field(helper, CampX), field(helper, CampY));
        returnDetachedGroup(aic, helper, index, TransferBudget);
        return;
    }
    if (homeThreat(group.target) >= ReliefThreatPower) state.reliefCalmSince = now();
    // The helped ally asked while the relief was already on its way.
    if (requestStands(helper, group.target)) state.reliefRequested = 1;
    AllyReliefStatus status;
    status.allyValid = helpable(helper, group.target) && (!combatCensusFresh(now())
        || static_cast<unsigned int>(combatCensus[group.target].lordUID) == group.targetUID);
    status.ownDanger = ownDanger(helper);
    status.attackDue = attackDue(aic, helper);
    status.requested = state.reliefRequested ? (requestStands(helper, group.target) ? 1 : -1) : 0;
    status.age = now() - group.startTick;
    status.calm = now() - state.reliefCalmSince;
    status.minimum = static_cast<unsigned int>(setting(helper, AllyReliefMinMonths)) * AllyTicksPerMonth;
    int reason = reliefOn(helper) ? allyReliefEnd(status) : ReliefCancelled;
    if (reason == ReliefContinues) {
        const int size = detachedGroupSize(helper, index);
        const int wanted = allyReliefSize(idleArmy(helper) + size, setting(helper, AllyArmyShare)) - size;
        const bool joined = wanted > 0 && fillRelief(helper, index, wanted) > 0;
        if (!orderRelief(aic, helper, group, joined)) reason = ReliefNoRoute;
    }
    if (reason != ReliefContinues) endRelief(aic, helper, index, reason);
}

void updateAllies(void* aic, int player)
{
    // The ally bindings are published only when an ally feature is on.
    if (!allyBindings.lordAlive) return;
    if (!isAI(player) || !character(player) || !lordAlive(player)) return;
    if (setting(player, AllyGoods) && team(player) != 0) {
        askAllies(aic, player);
        recruitGold(aic, player);
    }
    updateRelief(aic, player);
}
} // namespace

bool allyReliefWatched(int player)
{
    if (reliefOn(player)) return true;
    for (int other = 1; other <= 8; ++other)
        if (allied(player, other) && reliefOn(other)) return true;
    return false;
}

void resetAllyBuildingCensus()
{
    std::memset(keyCounts, 0, sizeof(keyCounts));
}

void countAllyBuilding(int building)
{
    if (building <= 0 || building >= 2000) return;
    const unsigned int address = nativeBindings.buildings + 0x14 + building * 0x32C;
    const int type = at<short>(address + 0xD2), owner = at<short>(address + 0xD6);
    if (owner < 1 || owner > 8 || at<short>(address + 0xD0) != 2) return;
    for (int index = 0; index < AllyKeyBuildingCount; ++index)
        if (KeyBuildings[index] == type) ++keyCounts[owner][index];
}

void completeAllyBuildingCensus()
{
    for (int player = 1; player <= 8; ++player) {
        AllyState& state = allyStates[player];
        for (int index = 0; index < AllyKeyBuildingCount; ++index) {
            state.keyNow[index] = keyCounts[player][index];
            if (state.keyNow[index] > state.keySeen[index]) state.keySeen[index] = state.keyNow[index];
        }
    }
}

AllyUpdate::AllyUpdate(void* aicState, int aiPlayer) : aic(aicState), player(aiPlayer), hidden(0)
{
    if (character(player) && setting(player, AllyRelief) && field(player, HelpRequest) == RequestHelp) {
        field(player, HelpRequest) = 0;
        hidden = 1;
    }
}

AllyUpdate::~AllyUpdate()
{
    if (hidden && field(player, HelpRequest) == 0) field(player, HelpRequest) = RequestHelp;
    updateAllies(aic, player);
}

namespace {
// Gold for a human ally who asks: above the floor and within the month's allowance.
int humanGoldLimit(int player)
{
    const int above = gold(player) - setting(player, AllyGoldKeep);
    const int allowed = allowance(player, AllyGold, gold(player));
    return above < allowed ? above : allowed;
}
} // namespace

int __cdecl allyGoldRequestAllowed(int player, int amount)
{
    if (!isAI(player) || !setting(player, AllyGoods) || amount <= 0 || !castleDone(player)) return 0;
    return amount <= humanGoldLimit(player) ? 1 : 0;
}

int __cdecl allyGoldRequestCommand(int giver, int resourceIndex, int amount, int receiver)
{
    if (resourceIndex + 1 != AllyGold || !isAI(giver) || !setting(giver, AllyGoods)) return 0;
    if (amount > 0 && allied(giver, receiver) && castleDone(giver)) {
        int send = humanGoldLimit(giver);
        if (send > amount) send = amount;
        if (send > 0) sendGold(giver, receiver, send);
    }
    return 1;
}

int __cdecl allyGateHeld(int id)
{
    if (!building(id)) return 0;
    const unsigned int record = buildingAddress(id);
    const int owner = at<short>(record + BuildingOwner);
    if (owner < 1 || owner > 8 || !gatehouse(record)) return 0;
    const int size = at<int>(record + BuildingSize);
    const int x = at<short>(record + BuildingX) + size / 2, y = at<short>(record + BuildingY) + size / 2;
    bool held = false;
    for (int helper = 1; helper <= 8 && !held; ++helper) {
        const int index = reliefGroup(helper);
        if (index < 0 || !setting(helper, AllyReliefGates) || !detachedGroupValid(helper, index)) continue;
        const DetachedGroup& group = detachedGroups[helper][index];
        // Going in, members wait outside; leaving, inside.
        held = !group.returning && group.target == owner
            && memberAt(helper, group, group.leaving != 0, x, y, size / 2 + GateReach);
    }
    if (!held) return 0;
    if (at<unsigned char>(record + GateLink) == GateClosed) {
        at<unsigned char>(record + GateState) = GateOpening;
        reinterpret_cast<ThreeAction>(allyBindings.gateChange)(reinterpret_cast<void*>(nativeBindings.buildings), id, 1, 0);
    }
    return 1;
}

} // namespace SHC141
} // namespace AicTactics
