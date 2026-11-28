// Executes ally help (goods, gold, relief) against mock game data and mock
// native routines on both tribe layouts (Crusader 0x334, Extreme 0x688).
#include "aic_tactics/runtime.hpp"
#include "aic_tactics/allies.hpp"
#include "aic_tactics/groups.hpp"
#include <cassert>
#include <cstring>

namespace AicTactics { namespace SHC141 {
NativeBindings nativeBindings;
int configurationLocked;
PlayerCensus combatCensus[9];
unsigned int combatCensusTick;
int combatCensusValid;
}}

using namespace AicTactics;
using namespace AicTactics::SHC141;

namespace {
const unsigned int Stride = 0x39F4;
unsigned char players[9 * 0x39F4];
unsigned char units[64 * 0x490];
unsigned char tribes[1250 * 0x688];
unsigned char buildings[0x14 + 8 * 0x32C];
unsigned char aicRecords[17 * 0x2A4];
unsigned char aiv[0x38 + 9 * 0x6D98];
unsigned char playable[400 * 400];
int rows[400 * 3];
unsigned short areas[80400];
int teams[9];
int received[9], sent[9];
int lordAliveFlags[9];
unsigned int tick = 4000;
unsigned int nextUID = 900;
int nextTribe = 40;
void* const AIC = aicRecords;
int pathOK, pathOnly, moves, moveTribe, moveX, moveY, speeches, requests, requestPlayer, requestAmount;
int keepOrders, keepTribe, keepOwner, gateChanges, gateBuilding;

template<class T> T& at(unsigned char* base, unsigned int offset) { return *reinterpret_cast<T*>(base + offset); }
unsigned char* player(int id) { return players + id * Stride; }
unsigned char* unit(int id) { return units + id * 0x490; }
unsigned char* tribe(int id) { return tribes + id * nativeBindings.tribeStride; }
int& field(int id, unsigned int offset) { return at<int>(player(id), offset); }
int& gold(int id) { return field(id, 0x4D0 + 15 * 4); }
int& stock(int id, int resource) { return field(id, 0x4D0 + resource * 4); }
int& aic(int character, unsigned int offset) { return at<int>(aicRecords, character * 0x2A4 + offset); }

// Native thiscall routines, called through __thiscall pointers: __fastcall with
// an ignored edx has the same register use and callee stack cleanup.
void __fastcall gain(void* owner, void*, int id, int resource, int amount)
{
    assert(owner == static_cast<void*>(buildings));
    stock(id, resource) += amount;
}
void __fastcall loss(void* owner, void*, int id, int resource, int amount, int flag)
{
    assert(owner == static_cast<void*>(buildings) && flag == 0);
    stock(id, resource) -= amount;
}
int __fastcall sellPrice(void* state, void*, int, int, int amount)
{
    assert(state == reinterpret_cast<void*>(0x7000));
    return amount * 2;
}
int __fastcall buyPrice(void* state, void*, int, int, int amount)
{
    assert(state == reinterpret_cast<void*>(0x7000));
    return amount * 10;
}
int __fastcall enough(void* state, void*, int id, int resource, int amount)
{
    assert(state == AIC);
    return stock(id, resource) >= aic(field(id, 0x2300) - 1, 0xA4) + amount;
}
void __fastcall requestGoods(void* state, void*, int id, int resource, int amount)
{
    assert(state == AIC && resource == 15);
    ++requests;
    requestPlayer = id;
    requestAmount = amount;
}
void __fastcall speech(void* state, void*, int, int)
{
    assert(state == AIC);
    ++speeches;
}
int __fastcall lordAlive(void* state, void*, int id)
{
    assert(state == reinterpret_cast<void*>(0x8000));
    return lordAliveFlags[id];
}
int __fastcall path(void* state, void*, int, int tile)
{
    assert(state == AIC && tile > 0);
    return pathOK || tile == pathOnly;
}
void __fastcall toKeep(void* state, void*, int id, int owner)
{
    assert(state == AIC);
    ++keepOrders;
    keepTribe = id;
    keepOwner = owner;
}
void __fastcall gateChange(void* owner, void*, int id, int open, int flag)
{
    assert(owner == static_cast<void*>(buildings) && open == 1 && flag == 0);
    ++gateChanges;
    gateBuilding = id;
}
void __fastcall clearMoveFlags(void* owner, void*, int)
{
    assert(owner == static_cast<void*>(tribes));
}
int __fastcall move(void* owner, void*, int id, int x, int y, int a, int b, int c)
{
    assert(owner == static_cast<void*>(tribes) && !a && !b && !c);
    ++moves;
    moveTribe = id;
    moveX = x;
    moveY = y;
    return 1;
}
void live(int id, int owner, unsigned int uid)
{
    at<int>(tribe(id), 0x2C) = owner;
    at<unsigned int>(tribe(id), 0x34) = uid;
    at<short>(tribe(id), 0x40) = 2;
}
int __fastcall create(void* owner, void*, int id)
{
    assert(owner == static_cast<void*>(tribes));
    live(nextTribe, id, ++nextUID);
    return nextTribe++;
}
void __fastcall removeUnit(void* owner, void*, int id, int from)
{
    assert(owner == static_cast<void*>(tribes) && at<short>(unit(id), 0x2D8) == from);
    at<short>(tribe(from), 0x5C) = static_cast<short>(at<short>(tribe(from), 0x5C) - 1);
    unsigned short& bits = at<unsigned short>(tribe(from), 0x60 + (id / 16) * 2);
    bits = static_cast<unsigned short>(bits & ~(1U << (id % 16)));
    if (at<short>(tribe(from), 0x5A) == id) at<short>(tribe(from), 0x5A) = 0;
    at<short>(unit(id), 0x2D8) = 0;
}
void __fastcall addUnit(void* owner, void*, int id, int to)
{
    assert(owner == static_cast<void*>(tribes) && at<short>(unit(id), 0x2D8) == 0);
    at<short>(tribe(to), 0x5C) = static_cast<short>(at<short>(tribe(to), 0x5C) + 1);
    unsigned short& bits = at<unsigned short>(tribe(to), 0x60 + (id / 16) * 2);
    bits = static_cast<unsigned short>(bits | (1U << (id % 16)));
    if (!at<short>(tribe(to), 0x5A)) at<short>(tribe(to), 0x5A) = static_cast<short>(id);
    at<short>(unit(id), 0x2D8) = static_cast<short>(to);
    at<unsigned int>(unit(id), 0x2E4) = at<unsigned int>(tribe(to), 0x34);
}
int __fastcall findGroup(void* state, void*, int id, int, int role)
{
    assert(state == AIC && id == 2);
    return role == 20 ? 10 : role == 16 ? 11 : 0;
}

void soldier(int id, int owner, int from, int role, int type, int x, int y)
{
    at<short>(unit(id), 0x96) = static_cast<short>(owner);
    at<short>(unit(id), 0x8C) = 2;
    at<short>(unit(id), 0x8E) = static_cast<short>(type);
    at<short>(unit(id), 0x2A0) = 0;
    at<int>(unit(id), 0x3C8) = 100;
    at<short>(unit(id), 0x42A) = static_cast<short>(role);
    at<short>(unit(id), 0xC4) = static_cast<short>(x);
    at<short>(unit(id), 0xC6) = static_cast<short>(y);
    at<short>(unit(id), 0x2D8) = 0;
    addUnit(tribes, 0, id, from);
}

void slot(int owner, int index, int id, unsigned int uid)
{
    live(id, owner, uid);
    at<short>(player(owner), 0x310C + index * 2) = static_cast<short>(id);
    at<unsigned int>(player(owner), 0x329C + index * 4) = uid;
}

void update(int id)
{
    AllyUpdate scope(AIC, id);
}

void setup(unsigned int stride)
{
    std::memset(players, 0, sizeof(players));
    std::memset(units, 0, sizeof(units));
    std::memset(tribes, 0, sizeof(tribes));
    std::memset(buildings, 0, sizeof(buildings));
    std::memset(aicRecords, 0, sizeof(aicRecords));
    std::memset(aiv, 0, sizeof(aiv));
    std::memset(detachedGroups, 0, sizeof(detachedGroups));
    std::memset(allyStates, 0, sizeof(allyStates));
    std::memset(allySettings, 0, sizeof(allySettings));
    std::memset(combatCensus, 0, sizeof(combatCensus));
    std::memset(received, 0, sizeof(received));
    std::memset(sent, 0, sizeof(sent));
    std::memset(playable, 1, sizeof(playable));
    std::memset(areas, 0, sizeof(areas));
    for (int y = 0; y < 400; ++y) rows[y * 3] = y * 400;
    tick = 4000;
    nextTribe = 40;
    pathOK = 1;
    pathOnly = 0;
    moves = speeches = requests = keepOrders = gateChanges = 0;
    nativeBindings.players = reinterpret_cast<unsigned int>(players);
    nativeBindings.unitRecords = reinterpret_cast<unsigned int>(units);
    nativeBindings.units = 0x8000;
    nativeBindings.unitCapacity = 64;
    nativeBindings.tribes = reinterpret_cast<unsigned int>(tribes);
    nativeBindings.tribeStride = stride;
    nativeBindings.tribeMemberWords = 4;
    nativeBindings.tribeStance = stride == 0x334 ? 0x2E0 : 0x634;
    nativeBindings.buildings = reinterpret_cast<unsigned int>(buildings);
    nativeBindings.buildingCapacity = 8;
    nativeBindings.areaMap = reinterpret_cast<unsigned int>(areas);
    nativeBindings.gameState = 0x7000;
    nativeBindings.gameTick = reinterpret_cast<unsigned int>(&tick);
    nativeBindings.teams = reinterpret_cast<unsigned int>(teams);
    nativeBindings.playableTiles = reinterpret_cast<unsigned int>(playable);
    nativeBindings.mapRows = reinterpret_cast<unsigned int>(rows);
    nativeBindings.tribePath = reinterpret_cast<unsigned int>(&path);
    nativeBindings.siegeGroupMove = reinterpret_cast<unsigned int>(&move);
    nativeBindings.clearGroupMoveFlags = reinterpret_cast<unsigned int>(&clearMoveFlags);
    nativeBindings.createTribe = reinterpret_cast<unsigned int>(&create);
    nativeBindings.addUnitToTribe = reinterpret_cast<unsigned int>(&addUnit);
    nativeBindings.removeUnitFromTribe = reinterpret_cast<unsigned int>(&removeUnit);
    nativeBindings.findAttackGroup = reinterpret_cast<unsigned int>(&findGroup);
    allyBindings.resourceGain = reinterpret_cast<unsigned int>(&gain);
    allyBindings.resourceLoss = reinterpret_cast<unsigned int>(&loss);
    allyBindings.sellPrice = reinterpret_cast<unsigned int>(&sellPrice);
    allyBindings.buyPrice = reinterpret_cast<unsigned int>(&buyPrice);
    allyBindings.goodsReceived = reinterpret_cast<unsigned int>(received);
    allyBindings.goodsSent = reinterpret_cast<unsigned int>(sent);
    allyBindings.enoughAfterTrade = reinterpret_cast<unsigned int>(&enough);
    allyBindings.requestGoods = reinterpret_cast<unsigned int>(&requestGoods);
    allyBindings.goodsSentSpeech = reinterpret_cast<unsigned int>(&speech);
    allyBindings.lordAlive = reinterpret_cast<unsigned int>(&lordAlive);
    allyBindings.aiState = reinterpret_cast<unsigned int>(aicRecords);
    allyBindings.aivState = 0;
    allyBindings.sendToKeep = reinterpret_cast<unsigned int>(&toKeep);
    allyBindings.gateChange = reinterpret_cast<unsigned int>(&gateChange);
    // 1 human, 2 and 3 AI allies (characters 2 and 3), 4 an enemy AI (character 4).
    const int teamOf[9] = {0, 1, 1, 1, 2, 0, 0, 0, 0};
    for (int id = 0; id < 9; ++id) { teams[id] = teamOf[id]; lordAliveFlags[id] = id >= 1 && id <= 4; }
    field(2, 0x2300) = 3;
    field(3, 0x2300) = 4;
    field(4, 0x2300) = 5;
    allySettings[4][AllyRelief] = 1;
    for (int id = 1; id <= 4; ++id) {
        field(id, 0x94) = 1;
        field(id, 0x98) = 50 * id;
        field(id, 0x9C) = 50 * id;
        field(id, 0x1D8) = 50 * id + 10;
        field(id, 0x1DC) = 50 * id + 10;
    }
    field(2, 0x2B74) = 6;
    field(3, 0x2B74) = 2;
    for (int character = 2; character <= 4; ++character) {
        aic(character, 0xA0) = 4;      // AIRequestDelay
        aic(character, 0xA4) = 100;    // MinimumGoodsRequiredAfterTrade
        aic(character, 0xB0) = 100;    // MaxStone
        aic(character, 0xC4) = 800;    // RecruitGoldThreshold
        aic(character, 0x1F4) = 50;    // AttForceBase
        aic(character, 0x1FC) = 5;     // AttForceSupportAllyThreshold
    }
    combatCensusValid = 1;
    combatCensusTick = tick;
    for (int id = 1; id <= 4; ++id) combatCensus[id].lordUID = 100 + id;
}

void goods(unsigned int stride)
{
    setup(stride);
    // Player 3 cannot afford 40 stone (400 gold); its ally 2 is better off.
    field(3, 0x2A70 + 4 * 4) = 40;
    gold(3) = 100;
    stock(2, 4) = 200;
    gold(2) = 500;
    update(3);
    assert(stock(3, 4) == 40 && stock(2, 4) == 160 && field(3, 0x2A70 + 16) == 0);
    assert(received[3] == 80 && sent[2] == 80 && field(3, 0x3898) == 4);
    assert(allyStates[2].given[4] == 40 && allyStates[3].receivedMonth[4] == static_cast<int>(tick / 800) + 1);
    // Gold above the floor is not there, and 2's gold is below its floor:
    // the gold ask (400 - 100) goes to the human ally through the native request.
    assert(gold(2) == 500 && requests == 1 && requestPlayer == 3 && requestAmount == 300);
    // The monthly allowance counts what was given: 30% of 200 is 60, 40 given.
    field(3, 0x3898) = 0;
    field(3, 0x2A70 + 16) = 40;
    update(3);
    assert(stock(3, 4) == 60 && allyStates[2].given[4] == 60);
    // A cold load restores the record: the allowance stays used up.
    AllyState saved[9];
    std::memcpy(saved, allyStates, sizeof(saved));
    std::memset(allyStates, 0, sizeof(allyStates));
    std::memcpy(allyStates, saved, sizeof(saved));
    field(3, 0x3898) = 0;
    field(3, 0x2A70 + 16) = 40;
    update(3);
    assert(stock(3, 4) == 60 && allyStates[3].nextAskTick == tick + 800);
    // A refused ask waits a month before asking again.
    update(3);
    assert(requests == 3);
    tick += 800;
    combatCensusTick = tick;
    update(3);
    assert(stock(3, 4) == 100 && allyStates[2].given[4] == 40);
    // Player 3 got stone lately: it never passes that on to player 2.
    field(2, 0x2B74) = 1;
    field(3, 0x2B74) = 9;
    field(2, 0x2A70 + 16) = 10;
    gold(2) = 0;
    update(2);
    assert(stock(3, 4) == 100 && allyStates[2].nextAskTick == tick + 800);
}

void rebuild(unsigned int stride)
{
    setup(stride);
    // Player 3 had a barracks and lost it: wood and stone reach it under threat.
    at<short>(buildings, 0x14 + 1 * 0x32C + 0xD2) = 9;
    at<short>(buildings, 0x14 + 1 * 0x32C + 0xD6) = 3;
    at<short>(buildings, 0x14 + 1 * 0x32C + 0xD0) = 2;
    resetAllyBuildingCensus();
    countAllyBuilding(1);
    completeAllyBuildingCensus();
    assert(allyStates[3].keySeen[1] == 1 && allyStates[3].keyNow[1] == 1);
    field(3, 0x3820) = 1;
    field(3, 0x2A70 + 16) = 20;
    stock(2, 4) = 200;
    update(3);
    assert(stock(3, 4) == 0);
    at<short>(buildings, 0x14 + 1 * 0x32C + 0xD0) = 0;
    resetAllyBuildingCensus();
    countAllyBuilding(1);
    completeAllyBuildingCensus();
    assert(allyStates[3].keySeen[1] == 1 && allyStates[3].keyNow[1] == 0);
    allyStates[3].nextAskTick = 0;
    update(3);
    assert(stock(3, 4) == 20);
}

void gift(unsigned int stride)
{
    setup(stride);
    // Unasked gold share with goods: 25% of the gold above the 1000 floor.
    field(3, 0x2A70 + 16) = 40;
    gold(3) = 100;
    stock(2, 4) = 200;
    gold(2) = 3000;
    update(3);
    assert(gold(2) == 2500 && gold(3) == 600 && allyStates[2].given[15] == 500 && requests == 0);
    // A share of 0 adds no gold, but asked gold is still sent.
    setup(stride);
    allySettings[2][AllyGoldShare] = 1;
    field(3, 0x2A70 + 16) = 40;
    gold(3) = 100;
    stock(2, 4) = 200;
    gold(2) = 3000;
    update(3);
    assert(gold(2) == 2700 && gold(3) == 400 && requests == 0);
    // Recruit gold: a weak lord short of its recruit line (800 + 300).
    setup(stride);
    field(3, 0x30F4) = 1;
    gold(3) = 600;
    gold(2) = 3000;
    update(3);
    assert(gold(3) == 1100 && gold(2) == 2500 && field(3, 0x3898) == 4);
    // An unfinished castle plan gives no gold.
    setup(stride);
    allyBindings.aivState = reinterpret_cast<unsigned int>(aiv);
    field(2, 0x22FC) = 1;
    at<int>(aiv, 0x6D98 + 0x14) = 1;
    at<int>(aiv, 0x6D98 + 0x24) = 3;
    at<int>(aiv, 0x6D98 + 0x18) = 2;
    field(3, 0x30F4) = 1;
    gold(3) = 600;
    gold(2) = 3000;
    update(3);
    assert(gold(3) == 600 && allyGoldRequestAllowed(2, 100) == 0);
    at<int>(aiv, 0x6D98 + 0x18) = 3;
    at<short>(aiv, 0x38 + (0x922 + 2) * 12 + 2) = 32;
    assert(allyGoldRequestAllowed(2, 100) == 0);
    at<unsigned char>(aiv, 0x38 + (0x922 + 2) * 12) = 3;
    assert(allyGoldRequestAllowed(2, 100) == 1);
}

void humanGold(unsigned int stride)
{
    setup(stride);
    gold(2) = 3000;
    // The ally screen's test and the command use the same limit: the gold above
    // the 1000 floor, within the month's allowance (30% of 3000 = 900).
    assert(allyGoldRequestAllowed(2, 900) == 1 && allyGoldRequestAllowed(2, 901) == 0);
    assert(allyGoldRequestCommand(2, 14, 1500, 1) == 1 && gold(2) == 2100 && gold(1) == 900);
    assert(speeches == 1 && allyStates[2].given[15] == 900 && received[1] == 900);
    assert(allyGoldRequestAllowed(2, 1) == 0);
    allySettings[2][AllyMonthlyGive] = 101;
    allySettings[2][AllyGoldKeep] = 1201;
    assert(allyGoldRequestAllowed(2, 900) == 1 && allyGoldRequestAllowed(2, 901) == 0);
    // Gold moved since the test: never below the floor.
    assert(allyGoldRequestCommand(2, 14, 1000, 1) == 1 && gold(2) == 1200 && gold(1) == 1800);
    // Goods, a human giver and goods help off stay native.
    assert(allyGoldRequestCommand(2, 3, 10, 1) == 0 && allyGoldRequestCommand(1, 14, 10, 2) == 0);
    allySettings[2][AllyGoods] = 1;
    assert(allyGoldRequestAllowed(2, 1) == 0 && allyGoldRequestCommand(2, 14, 10, 1) == 0);
}

int relieved(int id) { return detachedGroups[id][0].duty == ReliefDuty ? detachedGroupSize(id, 0) : -1; }

void army()
{
    // Player 2: 8 main army troops (one engineer) and 4 patrol troops.
    slot(2, 192, 10, 300);
    slot(2, 15, 11, 301);
    for (int id = 1; id <= 8; ++id) soldier(id, 2, 10, 20, id == 1 ? 30 : 22, 110, 110);
    for (int id = 9; id <= 12; ++id) soldier(id, 2, 11, 16, 24, 110, 110);
    field(2, 0x30F0) = 12;
}

void relief(unsigned int stride)
{
    setup(stride);
    army();
    assert(allyReliefWatched(1) && allyReliefWatched(2) && allyReliefWatched(3) && !allyReliefWatched(4));
    // Calm: nobody goes.
    update(2);
    assert(relieved(2) == -1);
    // The human ally's home is threatened: 75% of 12 = 9 go, the engineer stays.
    combatCensus[1].homePower[4] = 150;
    update(2);
    assert(relieved(2) == 9 && detachedGroups[2][0].target == 1 && detachedGroups[2][0].targetUID == 101);
    assert(at<short>(unit(1), 0x2D8) == 10 && moves == 1 && moveX == 50 && moveY == 50);
    assert(at<short>(tribe(detachedGroups[2][0].tribe), nativeBindings.tribeStance) == 2);
    // A march already at its station is not restarted.
    const int group = detachedGroups[2][0].tribe;
    for (int id = 2; id <= 12; ++id) { at<short>(unit(id), 0xC4) = 50; at<short>(unit(id), 0xC6) = 51; }
    update(2);
    assert(moves == 1 && relieved(2) == 9);
    // A cold load restores the record and the timers: the same relief continues.
    AllyState saved[9];
    DetachedGroup records[9][DetachedGroupsPerPlayer];
    std::memcpy(saved, allyStates, sizeof(saved));
    std::memcpy(records, detachedGroups, sizeof(records));
    std::memset(allyStates, 0, sizeof(allyStates));
    std::memset(detachedGroups, 0, sizeof(detachedGroups));
    std::memcpy(allyStates, saved, sizeof(saved));
    std::memcpy(detachedGroups, records, sizeof(records));
    update(2);
    assert(relieved(2) == 9 && detachedGroups[2][0].tribe == group);
    // The threat ends; after a calm month the troops go home and rejoin.
    combatCensus[1].homePower[4] = 0;
    tick += 799;
    combatCensusTick = tick;
    update(2);
    assert(relieved(2) == 9);
    tick += 1;
    combatCensusTick = tick;
    update(2);
    assert(relieved(2) == -1 && moveTribe == group && moveX == 110 && moveY == 110);
    assert(at<short>(unit(2), 0x2D8) == 10 && at<short>(unit(9), 0x2D8) == 11);
    assert(allyStates[2].reliefPauseUntil == tick + ReliefPauseTicks);
    // Paused: a new threat is not answered until the pause ends.
    combatCensus[1].homePower[4] = 150;
    update(2);
    assert(relieved(2) == -1);
    tick += ReliefPauseTicks;
    combatCensusTick = tick;
    update(2);
    assert(relieved(2) == 9);
    // Its own attack calls the relief home at once, without a pause.
    field(2, 0x2BA4) = 1;
    update(2);
    assert(relieved(2) == -1 && allyStates[2].reliefPauseUntil == 0);
    field(2, 0x2BA4) = 0;
    // Own danger.
    update(2);
    assert(relieved(2) == 9);
    field(2, 0x3820) = 1;
    update(2);
    assert(relieved(2) == -1);
    field(2, 0x3820) = 0;
    // Expiry.
    tick += ReliefPauseTicks;
    combatCensusTick = tick;
    update(2);
    assert(relieved(2) == 9);
    tick += ReliefMaximumTicks;
    combatCensusTick = tick;
    update(2);
    assert(relieved(2) == -1);
    // An unreachable ally is not helped.
    for (int id = 1; id <= 12; ++id) { at<short>(unit(id), 0xC4) = 110; at<short>(unit(id), 0xC6) = 110; }
    tick += ReliefPauseTicks;
    combatCensusTick = tick;
    pathOK = 0;
    update(2);
    assert(relieved(2) == -1);
}

void reinforcement(unsigned int stride)
{
    setup(stride);
    army();
    combatCensus[1].homePower[4] = 150;
    update(2);
    assert(relieved(2) == 9 && moves == 1);
    for (int id = 2; id <= 12; ++id) { at<short>(unit(id), 0xC4) = 50; at<short>(unit(id), 0xC6) = 51; }
    // A recruit at the helper's camp joins the group standing at the keep.
    soldier(13, 2, 10, 20, 22, 110, 110);
    field(2, 0x30F0) = 13;
    update(2);
    assert(relieved(2) == 10 && moves == 2 && moveTribe == detachedGroups[2][0].tribe && moveX == 50);
}

void request(unsigned int stride)
{
    setup(stride);
    army();
    // The human asked; the native update must not see the request.
    field(2, 0x384C) = 2;
    field(2, 0x38E4) = 1;
    {
        AllyUpdate scope(AIC, 2);
        assert(field(2, 0x384C) == 0);
    }
    assert(field(2, 0x384C) == 2 && relieved(2) == 9 && allyStates[2].reliefRequested == 1);
    // A standing request keeps the relief without a threat.
    tick += 2 * 800;
    combatCensusTick = tick;
    update(2);
    assert(relieved(2) == 9);
    // Withdrawn: the troops go home.
    field(2, 0x384C) = 0;
    update(2);
    assert(relieved(2) == -1 && allyStates[2].reliefRequested == 0);
    // Expired: the request is dropped as well.
    field(2, 0x384C) = 2;
    update(2);
    assert(relieved(2) == 9);
    tick += ReliefMaximumTicks;
    combatCensusTick = tick;
    update(2);
    assert(relieved(2) == -1 && field(2, 0x384C) == 0);
    // A request from the ally already relieved keeps that relief past a calm month.
    tick += ReliefPauseTicks;
    combatCensusTick = tick;
    combatCensus[1].homePower[4] = 150;
    update(2);
    assert(relieved(2) == 9 && allyStates[2].reliefRequested == 0);
    field(2, 0x384C) = 2;
    update(2);
    assert(allyStates[2].reliefRequested == 1);
    combatCensus[1].homePower[4] = 0;
    tick += 2 * 800;
    combatCensusTick = tick;
    update(2);
    assert(relieved(2) == 9);
    field(2, 0x384C) = 0;
    update(2);
    assert(relieved(2) == -1);
    // A share of 0 sends no troops, also when asked.
    allySettings[2][AllyArmyShare] = 1;
    field(2, 0x384C) = 2;
    update(2);
    assert(relieved(2) == -1 && field(2, 0x384C) == 0);
    // Relief off keeps the native help: the request is not hidden.
    allySettings[2][AllyRelief] = 1;
    field(2, 0x384C) = 2;
    {
        AllyUpdate scope(AIC, 2);
        assert(field(2, 0x384C) == 2);
    }
    assert(relieved(2) == -1 && field(2, 0x384C) == 2);
}

unsigned char* building(int id) { return buildings + 0x14 + id * 0x32C; }
void place(int id, int x, int y, int tile)
{
    at<short>(unit(id), 0xC4) = static_cast<short>(x);
    at<short>(unit(id), 0xC6) = static_cast<short>(y);
    at<int>(unit(id), 0xD4) = tile;
}

void keep(unsigned int stride)
{
    setup(stride);
    army();
    // Player 1's keep is building 1 at (48, 46).
    at<short>(building(1), 0xEE) = 48;
    at<short>(building(1), 0xF0) = 46;
    allySettings[2][AllyReliefKeep] = 2;
    combatCensus[1].homePower[4] = 150;
    update(2);
    assert(relieved(2) == 9 && keepOrders == 1 && moves == 0);
    assert(keepTribe == detachedGroups[2][0].tribe && keepOwner == 1);
    assert(detachedGroups[2][0].x == 51 && detachedGroups[2][0].y == 50);
    // Shut out without gate opening: the camp, as before.
    allySettings[2][AllyReliefKeep] = 0;
    tick += ReliefMaximumTicks;
    combatCensusTick = tick;
    update(2);
    tick += ReliefPauseTicks;
    combatCensusTick = tick;
    pathOK = 0;
    pathOnly = 60 * 400 + 60;
    update(2);
    assert(relieved(2) == 9 && moveX == 60 && moveY == 60);
}

void gates(unsigned int stride)
{
    setup(stride);
    army();
    // Player 1's walls: its keep tile (50, 50) in area 7. Its gatehouse,
    // building 2 at (40, 60) size 5, has one door outside: (42, 65), area 3.
    field(1, 0x1E0) = 50 * 400 + 50;
    areas[50 * 400 + 50] = 7;
    areas[65 * 400 + 42] = 3;
    areas[59 * 400 + 42] = 7;
    at<short>(building(1), 0xEE) = 200;
    at<short>(building(1), 0xF0) = 200;
    at<short>(building(2), 0xD0) = 2;
    at<short>(building(2), 0xD2) = 46;
    at<short>(building(2), 0xD6) = 1;
    at<short>(building(2), 0xEE) = 40;
    at<short>(building(2), 0xF0) = 60;
    at<int>(building(2), 0xF8) = 5;
    at<unsigned char>(building(2), 0x2A2) = 2;
    allySettings[2][AllyReliefGates] = 2;
    // Shut out: the relief walks to the outside door.
    pathOK = 0;
    pathOnly = 65 * 400 + 42;
    combatCensus[1].homePower[4] = 150;
    update(2);
    assert(relieved(2) == 9 && moves == 1 && moveX == 42 && moveY == 65);
    // Not there yet: the gate keeps the native logic.
    assert(allyGateHeld(2) == 0 && allyGateHeld(1) == 0);
    // At the door: the gate opens and the native part is skipped.
    for (int id = 2; id <= 12; ++id) place(id, 42, 66, 66 * 400 + 42);
    assert(allyGateHeld(2) == 1 && gateChanges == 1 && gateBuilding == 2);
    assert(at<unsigned char>(building(2), 0x2A3) == 0xB);
    // Waiting at the door is not restarted; once open the way in is taken.
    update(2);
    assert(moves == 1);
    pathOK = 1;
    update(2);
    assert(moves == 2 && moveX == 50 && moveY == 50);
    // Inside, its own attack ends the relief: the troops leave through the gate.
    for (int id = 2; id <= 12; ++id) place(id, 50, 50, 50 * 400 + 50);
    field(2, 0x2BA4) = 1;
    update(2);
    assert(relieved(2) == 9 && detachedGroups[2][0].leaving == tick && moveX == 110);
    assert(allyGateHeld(2) == 0);
    place(2, 42, 59, 59 * 400 + 42);
    assert(allyGateHeld(2) == 1);
    update(2);
    assert(relieved(2) == 9);
    // Everyone out: the troops rejoin their groups.
    for (int id = 2; id <= 12; ++id) place(id, 42, 70, 0);
    update(2);
    assert(relieved(2) == -1 && allyGateHeld(2) == 0);
}

void run(unsigned int stride)
{
    goods(stride);
    rebuild(stride);
    gift(stride);
    humanGold(stride);
    reinforcement(stride);
    relief(stride);
    request(stride);
    keep(stride);
    gates(stride);
}
}

int main()
{
    run(0x334);
    run(0x688);
    return 0;
}
