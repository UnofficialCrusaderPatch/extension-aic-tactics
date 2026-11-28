#ifndef AIC_TACTICS_ALLIES_HPP
#define AIC_TACTICS_ALLIES_HPP

#include "aic_tactics/ally_policy.hpp"

namespace AicTactics {
namespace SHC141 {

// Ally help: relief troops for a threatened ally (human or AI) and goods and
// gold between AI allies, in place of the native help that only answers a
// human's request.

// Resolved once from native code (native-ally-bindings.lua).
struct AllyBindings {
    unsigned int resourceGain;     // BuildingsState: gain(player, resource, amount)
    unsigned int resourceLoss;     // BuildingsState: loss(player, resource, amount, 0)
    unsigned int sellPrice;        // GameState: value(player, resource, amount)
    unsigned int buyPrice;         // GameState: price(player, resource, amount)
    unsigned int goodsReceived;    // int[9] end-of-game statistic, by value
    unsigned int goodsSent;        // int[9]
    unsigned int enoughAfterTrade; // AICState: stock >= AIC trade minimum + amount
    unsigned int requestGoods;     // AICState: request(player, resource, amount)
    unsigned int goodsSentSpeech;  // AICState: speech(sender, receiver)
    unsigned int lordAlive;        // UnitsState: alive(player)
    unsigned int aiState;          // AICState instance
    unsigned int aivState;         // AIVState instance (castle build plans)
    unsigned int sendToKeep;       // AICState: sendUnitsToKeep(tribe, player), onto the keep
    unsigned int gateChange;       // BuildingsState: apply a gate's open/close change(building, 1, 0)
};
extern AllyBindings allyBindings;

enum AllySetting {
    AllyRelief = 0,            // boolean
    AllyGoods = 1,             // boolean
    AllyGoldKeep = 2,          // donor floor, 0..100000
    AllyRecruitGoldBuffer = 3, // 0..5000 added to the AIC RecruitGoldThreshold
    AllyGoldShare = 4,         // percent of gold above the floor added unasked
    AllyMonthlyGive = 5,       // percent of each good given per month
    AllyArmyShare = 6,         // percent of the idle attack army sent as relief
    AllyReliefGates = 7,       // boolean: gates open for the relief
    AllyReliefKeep = 8,        // boolean: the relief stands on the ally's keep
    AllyReliefMinMonths = 9,   // 0..12 months the relief stays at least
    AllySettingCount = 10
};
// Indexed by installed AIC character 1..16: 0 uses the module setting,
// otherwise the character's value + 1 (booleans: 1 off, 2 on).
extern int allySettings[17][AllySettingCount];
// Module settings (booleans 0/1), written at enable.
extern int allyFallback[AllySettingCount];

const int AllyKeyBuildingCount = 5;

// Saved per player (AIC Tactics state, replay digest).
struct AllyState {
    int month;                               // month index (tick / 800) of `given`
    int given[AllyResourceCount];            // given away this month, gold at 15
    int receivedMonth[AllyResourceCount];    // month + 1 a good last came from an ally, 0 never
    int keySeen[AllyKeyBuildingCount];       // most of each key building standing this match
    int keyNow[AllyKeyBuildingCount];        // standing at the last building census
    unsigned int nextAskTick;                // a refused ask waits until this tick
    unsigned int reliefPauseUntil;           // no unasked relief before this tick
    unsigned int reliefCalmSince;            // tick the helped home was last threatened
    int reliefRequested;                     // the relief serves a native help request
};
extern AllyState allyStates[9];

// Combat census: a player whose home threat relief needs (an AI with relief
// on, or an ally of one) joins the census' home observers.
bool allyReliefWatched(int player);
// Building census (shared with raids): key buildings per player.
void resetAllyBuildingCensus();
void countAllyBuilding(int building);
void completeAllyBuildingCensus();

// One per-AI update inside updateOffensiveArmy. While the scope lives, a
// help request that relief serves is hidden from the native state update,
// which would otherwise send every attack troop and skip the rest of the
// update. At scope end, after the native update and reserve bookkeeping,
// the request is restored and the ally duties run.
class AllyUpdate {
public:
    AllyUpdate(void* aic, int player);
    ~AllyUpdate();
private:
    void* aic;
    int player;
    int hidden;
};

// The player's gold request to an AI ally. Allowed answers the ally screen's
// test (read only); the command carries out the answer in the synchronized
// command handler and returns 1 when it handled it.
int __cdecl allyGoldRequestAllowed(int player, int amount);
int __cdecl allyGoldRequestCommand(int giver, int resourceIndex, int amount, int receiver);

// Called from the native gate logic per uncaptured gatehouse per tick, before
// its open/close part. Returns 1 when a relief with gate opening on waits at
// the gatehouse on the far side of its walls; the gate is then opened and the
// native part is skipped. Reads synchronized state only.
int __cdecl allyGateHeld(int building);

} // namespace SHC141
} // namespace AicTactics
#endif
