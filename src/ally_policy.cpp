#include "aic_tactics/ally_policy.hpp"

namespace AicTactics {

namespace {
int smaller(int a, int b) { return a < b ? a : b; }
int positive(int value) { return value > 0 ? value : 0; }
// floor(value * share / 100) for share 0..100 without overflow.
unsigned int percent(unsigned int value, int share)
{
    const unsigned int part = static_cast<unsigned int>(share);
    return value / 100 * part + value % 100 * part / 100;
}
}

int allyAllowance(int stock, int givenThisMonth, int givePercent)
{
    if (givePercent <= 0 || stock <= 0) return 0;
    if (givePercent > 100) givePercent = 100;
    const unsigned int done = static_cast<unsigned int>(positive(givenThisMonth));
    const unsigned int allowed = percent(static_cast<unsigned int>(stock) + done, givePercent);
    if (allowed <= done) return 0;
    return allowed - done > static_cast<unsigned int>(stock) ? stock : static_cast<int>(allowed - done);
}

int allyGoodsAmount(const AllyGoodsOffer& offer)
{
    if (offer.wanted <= 0 || offer.giverWantsIt || offer.receivedRecently) return 0;
    int send = 0;
    if (offer.enoughForAll) send = offer.wanted;
    else if (offer.receiverWeak)
        send = smaller(offer.wanted / 2, offer.stock - (positive(offer.minimumAfterTrade) + 1) / 2);
    if (send > 0 && offer.receiverRoom >= 0) send = smaller(send, offer.receiverRoom);
    if (send > 0) send = smaller(send, offer.allowance);
    return positive(send);
}

int allyGoldAmount(int gold, int keep, int amount, int receiverWeak, int allowance)
{
    if (amount <= 0) return 0;
    int send = 0;
    if (gold >= keep && amount <= gold - keep) send = amount;
    else if (receiverWeak) send = smaller(amount / 2, gold - (positive(keep) + 1) / 2);
    if (send > 0) send = smaller(send, allowance);
    return positive(send);
}

int allyGoldShare(int gold, int keep, int receiverGold, int sharePercent, int allowance)
{
    if (sharePercent <= 0 || gold < keep || gold <= receiverGold) return 0;
    const unsigned int share = percent(static_cast<unsigned int>(gold - keep), sharePercent > 100 ? 100 : sharePercent);
    return positive(smaller(static_cast<int>(share), allowance));
}

int allyRecruitGold(int need, int gold, int keep, int allowance)
{
    if (need <= 0) return 0;
    return positive(smaller(smaller(need, gold - keep), allowance));
}

bool allyWoodOrStone(int resource) { return resource == 2 || resource == 4; }
bool allyFood(int resource) { return resource >= 10 && resource <= 13; }
bool allyNeverAsked(int resource) { return resource == 3 || resource == 9 || resource == 14; }

int allyGoodsPriority(int resource, int rebuilding)
{
    if (rebuilding && allyWoodOrStone(resource)) return 0;
    if (resource >= 17 && resource <= 24) return 1;
    if (allyFood(resource)) return 2;
    return 3;
}

int allyReliefSize(int army, int sharePercent)
{
    if (army <= 0 || sharePercent <= 0) return 0;
    if (sharePercent >= 100) return army;
    // Armies are bounded by the native unit pool (at most 10000 units).
    return (army * sharePercent + 99) / 100;
}

int allyReliefEnd(const AllyReliefStatus& status)
{
    if (!status.allyValid) return ReliefAllyGone;
    if (status.ownDanger) return ReliefOwnDanger;
    if (status.attackDue) return ReliefAttack;
    if (status.age >= ReliefMaximumTicks) return ReliefExpired;
    if (status.requested < 0) return ReliefCancelled;
    if (status.requested == 0 && status.calm >= ReliefCalmTicks && status.age >= status.minimum)
        return ReliefThreatOver;
    return ReliefContinues;
}

} // namespace AicTactics
