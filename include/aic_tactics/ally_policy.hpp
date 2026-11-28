#ifndef AIC_TACTICS_ALLY_POLICY_HPP
#define AIC_TACTICS_ALLY_POLICY_HPP

// Ally help decisions (goods, gold, relief), free of native memory so they
// run under the portable policy tests.
namespace AicTactics {

const int AllyResourceCount = 25;
const int AllyGold = 15;
const int AllyTicksPerMonth = 800;
// A good received from an ally is not passed on for this many months.
const int AllyRegiftMonths = 3;
// The request slot that carries a gold ask is a short.
const int AllyGoldAskMaximum = 30000;

// One goods gift from a giver to an AI ally that asked for it.
struct AllyGoodsOffer {
    int wanted;            // amount the receiver asked for
    int stock;             // giver's stock of the good
    int giverWantsIt;      // the good is on the giver's own purchase list
    int receivedRecently;  // the giver got this good from an ally within AllyRegiftMonths
    int enoughForAll;      // the native trade minimum test passed for the whole amount
    int receiverWeak;      // the receiver's native strength feeling is weak
    int minimumAfterTrade; // giver's AIC MinimumGoodsRequiredAfterTrade
    int receiverRoom;      // storage the receiver has left before selling, or -1
    int allowance;         // what the giver may still give of this good this month
};

// Monthly allowance: givePercent of (stock + given this month), less what
// was already given this month. 0 gives nothing, 100 the whole stock.
int allyAllowance(int stock, int givenThisMonth, int givePercent);

// Goods sent for one request: all of it while the giver keeps its native
// trade minimum, else up to half to a weak ally if the giver can spare it
// by going down to half that minimum. Never a good the giver wants or got
// lately, never more than the receiver can store or the allowance allows.
int allyGoodsAmount(const AllyGoodsOffer& offer);

// Gold sent for an ask: all of it while the giver keeps `keep`, else up to
// half to a weak ally going down to half of `keep`; within the allowance.
int allyGoldAmount(int gold, int keep, int amount, int receiverWeak, int allowance);

// Unasked gold added to a goods gift: sharePercent of the gold above `keep`,
// only to a poorer ally. A share of 0 never adds gold.
int allyGoldShare(int gold, int keep, int receiverGold, int sharePercent, int allowance);

// Gold for a weak or threatened ally short of its recruit line.
int allyRecruitGold(int need, int gold, int keep, int allowance);

// Asking order of wanted goods: rebuilding materials, weapons, food, rest.
int allyGoodsPriority(int resource, int rebuilding);
bool allyWoodOrStone(int resource);
bool allyFood(int resource);
// Goods the native request routine never asks for (hops, wheat, ale).
bool allyNeverAsked(int resource);

// Troops sent: the share of the idle attack army, rounded up. 0 sends none,
// 100 the whole idle attack army.
int allyReliefSize(int army, int sharePercent);

enum AllyReliefEnd {
    ReliefContinues = 0,
    ReliefAllyGone = 1,
    ReliefOwnDanger = 2,
    ReliefAttack = 3,
    ReliefExpired = 4,
    ReliefThreatOver = 5,
    ReliefCancelled = 6,
    ReliefNoRoute = 7
};

// Hostile combat value at an ally's home that calls for relief; the same
// scale and default as the provocation rules' ThreatPower.
const int ReliefThreatPower = 100;
const unsigned int ReliefMaximumTicks = 12 * AllyTicksPerMonth;
const unsigned int ReliefCalmTicks = AllyTicksPerMonth;
const unsigned int ReliefPauseTicks = 3 * AllyTicksPerMonth;

struct AllyReliefStatus {
    int allyValid;         // still allied, the same lord alive, a keep standing
    int ownDanger;         // the helper's own home is threatened
    int attackDue;         // the helper set out or its attack army is ready
    int requested;         // 1 the ally's help request stands, 0 none, -1 it was withdrawn
    unsigned int age;      // ticks since the relief began
    unsigned int calm;     // ticks since a threat was last seen at the ally's home
    unsigned int minimum;  // ticks the relief stays at least while the home is calm
};

int allyReliefEnd(const AllyReliefStatus& status);

} // namespace AicTactics
#endif
