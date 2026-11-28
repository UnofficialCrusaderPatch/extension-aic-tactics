// Ally help decisions: allowances, goods and gold amounts, relief size and
// the relief end conditions.
#include "aic_tactics/ally_policy.hpp"
#include <cassert>

using namespace AicTactics;

namespace {
AllyGoodsOffer offer()
{
    AllyGoodsOffer result;
    result.wanted = 40;
    result.stock = 200;
    result.giverWantsIt = 0;
    result.receivedRecently = 0;
    result.enoughForAll = 1;
    result.receiverWeak = 0;
    result.minimumAfterTrade = 100;
    result.receiverRoom = -1;
    result.allowance = 1000;
    return result;
}

void allowances()
{
    assert(allyAllowance(100, 0, 30) == 30);
    // What was given this month counts: 30% of (70 + 30) is already given.
    assert(allyAllowance(70, 30, 30) == 0);
    assert(allyAllowance(80, 20, 30) == 10);
    assert(allyAllowance(100, 0, 0) == 0);
    assert(allyAllowance(100, 0, 100) == 100);
    assert(allyAllowance(0, 0, 100) == 0);
    assert(allyAllowance(2000000000, 2000000000, 100) == 2000000000);
}

void goods()
{
    AllyGoodsOffer o = offer();
    assert(allyGoodsAmount(o) == 40);
    o.giverWantsIt = 1;
    assert(allyGoodsAmount(o) == 0);
    o = offer();
    o.receivedRecently = 1;
    assert(allyGoodsAmount(o) == 0);
    // Below the trade minimum only a weak ally gets up to half, the giver
    // going down to half its minimum: min(20, 60 - 50).
    o = offer();
    o.enoughForAll = 0;
    o.stock = 60;
    assert(allyGoodsAmount(o) == 0);
    o.receiverWeak = 1;
    assert(allyGoodsAmount(o) == 10);
    // Never more than the receiver stores or the giver may give this month.
    o = offer();
    o.receiverRoom = 15;
    assert(allyGoodsAmount(o) == 15);
    o.receiverRoom = 0;
    assert(allyGoodsAmount(o) == 0);
    o = offer();
    o.allowance = 7;
    assert(allyGoodsAmount(o) == 7);
    o.wanted = 0;
    assert(allyGoodsAmount(o) == 0);
}

void gold()
{
    // Asked gold: all while the giver keeps its floor.
    assert(allyGoldAmount(3000, 1000, 2000, 0, 5000) == 2000);
    assert(allyGoldAmount(3000, 1000, 2001, 0, 5000) == 0);
    // A weak ally gets up to half, the giver going down to half its floor.
    assert(allyGoldAmount(1200, 1000, 1000, 1, 5000) == 500);
    assert(allyGoldAmount(800, 1000, 1000, 1, 5000) == 300);
    assert(allyGoldAmount(3000, 1000, 2000, 0, 900) == 900);
    assert(allyGoldAmount(3000, 0, 3000, 0, 5000) == 3000);
    // Unasked share: of the gold above the floor, only to a poorer ally.
    assert(allyGoldShare(3000, 1000, 500, 25, 5000) == 500);
    assert(allyGoldShare(3000, 1000, 3000, 25, 5000) == 0);
    assert(allyGoldShare(900, 1000, 0, 25, 5000) == 0);
    assert(allyGoldShare(3000, 1000, 500, 25, 100) == 100);
    // A share of 0 adds no gold; asked gold above is unaffected.
    assert(allyGoldShare(3000, 1000, 500, 0, 5000) == 0);
    // Recruit gold: up to the need, from above the floor, within the allowance.
    assert(allyRecruitGold(400, 3000, 1000, 5000) == 400);
    assert(allyRecruitGold(400, 1200, 1000, 5000) == 200);
    assert(allyRecruitGold(400, 900, 1000, 5000) == 0);
    assert(allyRecruitGold(400, 3000, 1000, 50) == 50);
    assert(allyRecruitGold(0, 3000, 1000, 5000) == 0);
}

void priorities()
{
    assert(allyGoodsPriority(2, 0) == 3 && allyGoodsPriority(2, 9) == 0);
    assert(allyGoodsPriority(4, 9) == 0 && allyGoodsPriority(17, 9) == 1);
    assert(allyGoodsPriority(24, 0) == 1 && allyGoodsPriority(11, 0) == 2);
    assert(allyGoodsPriority(6, 9) == 3);
    assert(allyNeverAsked(3) && allyNeverAsked(9) && allyNeverAsked(14) && !allyNeverAsked(2));
    assert(allyFood(10) && allyFood(13) && !allyFood(9) && !allyFood(14));
}

void relief()
{
    // The share rounds up; 0 sends none, 100 everyone.
    assert(allyReliefSize(40, 75) == 30 && allyReliefSize(41, 75) == 31);
    assert(allyReliefSize(1, 1) == 1 && allyReliefSize(40, 0) == 0);
    assert(allyReliefSize(40, 100) == 40 && allyReliefSize(0, 75) == 0);
    AllyReliefStatus status = {1, 0, 0, 0, 100, 100, 0};
    assert(allyReliefEnd(status) == ReliefContinues);
    status.calm = ReliefCalmTicks;
    assert(allyReliefEnd(status) == ReliefThreatOver);
    // The minimum stay holds a calm relief, not the other ends.
    status.minimum = 6 * AllyTicksPerMonth;
    assert(allyReliefEnd(status) == ReliefContinues);
    status.attackDue = 1;
    assert(allyReliefEnd(status) == ReliefAttack);
    status.attackDue = 0;
    status.age = status.minimum;
    assert(allyReliefEnd(status) == ReliefThreatOver);
    status.age = 100;
    status.minimum = 0;
    // A standing request keeps the relief until it is withdrawn.
    status.requested = 1;
    assert(allyReliefEnd(status) == ReliefContinues);
    status.requested = -1;
    assert(allyReliefEnd(status) == ReliefCancelled);
    status.requested = 1;
    status.age = ReliefMaximumTicks;
    assert(allyReliefEnd(status) == ReliefExpired);
    status.attackDue = 1;
    assert(allyReliefEnd(status) == ReliefAttack);
    status.ownDanger = 1;
    assert(allyReliefEnd(status) == ReliefOwnDanger);
    status.allyValid = 0;
    assert(allyReliefEnd(status) == ReliefAllyGone);
}
}

int main()
{
    allowances();
    goods();
    gold();
    priorities();
    relief();
    return 0;
}
