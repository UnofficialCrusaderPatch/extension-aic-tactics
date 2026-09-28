#include "aic_tactics/runtime.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

int siegeHarassFallback = 0;
int siegeHarassMinimumFallback = 3;
int siegeHarassPolicy[17];
int siegeHarassMinimum[17];
SiegeHarassCensus siegeHarassCensus[9];
SiegeHarassPlan siegeHarassPlans[9];

namespace {
const unsigned int PlayerStride = 0x39F4;
bool collecting;
template<class T> T& at(unsigned int address) { return *reinterpret_cast<T*>(address); }
int playerValue(int player, unsigned int offset)
{
    return at<int>(nativeBindings.players + player * PlayerStride + offset);
}
int character(int player)
{
    if (player < 1 || player > 8) return 0;
    const int value = playerValue(player, 0x2300);
    return value >= 2 && value <= 17 ? value : 0;
}
bool enabled(int player)
{
    const int value = character(player);
    if (!value) return false;
    const int policy = siegeHarassPolicy[value - 1];
    return policy == 1 || (policy == 0 && siegeHarassFallback != 0);
}
int minimum(int characterID)
{
    const int encoded = siegeHarassMinimum[characterID - 1];
    return encoded ? encoded - 1 : siegeHarassMinimumFallback;
}
bool validEngine(int player, const SiegeHarassEngine& engine)
{
    if (engine.unit <= 0 || engine.unit >= static_cast<int>(nativeBindings.unitCapacity)
        || engine.tribe <= 0 || engine.tribe >= 1250) return false;
    const unsigned int unit = nativeBindings.unitRecords + engine.unit * 0x490;
    const unsigned int tribe = nativeBindings.tribes + engine.tribe * nativeBindings.tribeStride;
    return at<short>(unit + 0x8C) == 2 && at<int>(unit + 0x98) == engine.uid
        && at<short>(unit + 0x96) == player && at<short>(unit + 0x8E) == engine.type
        && at<short>(unit + 0x2A0) == 0 && at<int>(unit + 0x3C8) > 0
        && at<short>(unit + 0x3B4) == 2 && at<short>(unit + 0x42A) == 0x15
        && at<short>(unit + 0x2D8) == engine.tribe
        && at<unsigned int>(unit + 0x2E4) == static_cast<unsigned int>(engine.tribeUID)
        && at<unsigned int>(tribe + 0x34) == static_cast<unsigned int>(engine.tribeUID)
        && at<int>(tribe + 0x2C) == player && at<short>(tribe + 0x5C) > 0;
}
bool validTarget(int player, int target)
{
    if (target < 1 || target > 8 || target == player) return false;
    return at<int>(nativeBindings.teams + player * 4)
            != at<int>(nativeBindings.teams + target * 4)
        && combatCensus[target].lord > 0;
}
int clamp(int value, int lo, int hi)
{
    return value < lo ? lo : value > hi ? hi : value;
}
int approachRadius(int type)
{
    // Native target acquisition bounds its BFS in map tiles. Leave a quarter
    // of the range for buildings away from the target keep and terrain costs.
    const int range = type == 77
        ? at<int>(nativeBindings.siegeFireRange)
        : at<unsigned char>(nativeBindings.siegeCatapultRange);
    return range >= 16 && range <= 240 ? clamp(range * 3 / 4, 6, 70) : 0;
}
typedef int (__thiscall *Path)(void*, int, int);
typedef int (__thiscall *Move)(void*, int, unsigned int, unsigned int, int, int, int);
}

void __cdecl resetSiegeHarassCensus()
{
    collecting = siegeHarassFallback != 0;
    if (!collecting) {
        for (int characterID = 1; characterID <= 16; ++characterID)
            if (siegeHarassPolicy[characterID] == 1) { collecting = true; break; }
    }
    if (collecting) std::memset(siegeHarassCensus, 0, sizeof(siegeHarassCensus));
}

void __cdecl countSiegeHarassUnit(int id)
{
    if (!collecting || id <= 0 || id >= static_cast<int>(nativeBindings.unitCapacity)) return;
    const unsigned int unit = nativeBindings.unitRecords + id * 0x490;
    const int type = at<short>(unit + 0x8E);
    if ((type != 39 && type != 77) || at<short>(unit + 0x8C) != 2
        || at<short>(unit + 0x42A) != 0x15 || at<short>(unit + 0x3B4) != 2
        || at<short>(unit + 0x2A0) != 0 || at<int>(unit + 0x3C8) <= 0) return;
    const int player = at<short>(unit + 0x96);
    if (!enabled(player)) return;
    const int tribeID = at<short>(unit + 0x2D8);
    if (tribeID <= 0 || tribeID >= 1250) return;
    SiegeHarassCensus& census = siegeHarassCensus[player];
    if (census.count >= 20) return; // Native recruitment's separate hard limit.
    SiegeHarassEngine engine;
    engine.unit = id;
    engine.uid = at<int>(unit + 0x98);
    engine.tribe = tribeID;
    engine.tribeUID = at<int>(unit + 0x2E4);
    engine.type = type;
    if (validEngine(player, engine)) census.engines[census.count++] = engine;
}

int __cdecl suppressNativeSiegeHarassMove(int unitOffset)
{
    if (unitOffset < 0 || unitOffset % 0x490 != 0) return 0;
    const int id = unitOffset / 0x490;
    if (id <= 0 || id >= static_cast<int>(nativeBindings.unitCapacity)) return 0;
    const unsigned int unit = nativeBindings.unitRecords + unitOffset;
    const int type = at<short>(unit + 0x8E);
    if ((type != 39 && type != 77) || at<short>(unit + 0x42A) != 0x15
        || at<short>(unit + 0x3B4) != 2) return 0;
    const int player = at<short>(unit + 0x96);
    if (!enabled(player) || !combatCensusValid) return 0;
    const int characterID = character(player);
    if (at<int>(nativeBindings.aicRecords + (characterID - 2) * 676 + 0x1EC) <= 0)
        return 0;
    return validTarget(player, playerValue(player, 0x2BD8)) ? 1 : 0;
}

void __cdecl updateSiegeHarassment(void* aic, int player)
{
    if (!enabled(player) || !combatCensusValid) return;
    const unsigned int now = at<unsigned int>(nativeBindings.gameTick);
    if (now - combatCensusTick > 1) return;
    SiegeHarassPlan& plan = siegeHarassPlans[player];
    const int target = playerValue(player, 0x2BD8);
    if (!validTarget(player, target)) {
        std::memset(&plan, 0, sizeof(plan));
        return;
    }
    const int lordUID = combatCensus[target].lordUID;
    if (plan.target != target || plan.lordUID != lordUID) {
        std::memset(&plan, 0, sizeof(plan));
        plan.target = target;
        plan.lordUID = lordUID;
    }
    const int characterID = character(player);
    const int nativeMaximum = at<int>(nativeBindings.aicRecords
        + (characterID - 2) * 676 + 0x1EC);
    const SiegeHarassCensus& census = siegeHarassCensus[player];
    if (nativeMaximum <= 0 || census.count == 0) {
        plan.firstSeenTick = 0;
        plan.phase = 0;
        plan.issuedCount = 0;
        return;
    }
    if (plan.firstSeenTick == 0) plan.firstSeenTick = now ? now : 1;
    if (static_cast<int>(now - plan.nextDecisionTick) < 0) return;
    const int desired = clamp(minimum(characterID), 1, clamp(nativeMaximum, 1, 20));
    if (census.count < desired && now - plan.firstSeenTick < 800) {
        plan.nextDecisionTick = now + 100;
        return;
    }
    if (plan.phase == 1 && census.count == plan.issuedCount) {
        int distant = 0, aiming = 0;
        for (int index = 0; index < census.count; ++index) {
            const SiegeHarassEngine& engine = census.engines[index];
            const unsigned int unit = nativeBindings.unitRecords + engine.unit * 0x490;
            int dx = at<short>(unit + 0xC4) - plan.x;
            int dy = at<short>(unit + 0xC6) - plan.y;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            if ((dx > dy ? dx : dy) > 8) ++distant;
            if (at<int>(unit + 0x3BC) > 0 || at<short>(unit + 0x39C) == 9) ++aiming;
        }
        if (distant == 0 && aiming == census.count) {
            plan.nextDecisionTick = now + 400;
            return;
        }
    }
    const int targetX = playerValue(target, 0x98);
    const int targetY = playerValue(target, 0x9C);
    const int ownX = playerValue(player, 0x98);
    const int ownY = playerValue(player, 0x9C);
    if (targetX < 8 || targetX >= 392 || targetY < 8 || targetY >= 392) {
        plan.nextDecisionTick = now + 400;
        return;
    }
    int radius = 70;
    for (int index = 0; index < census.count; ++index) {
        const int engineRadius = approachRadius(census.engines[index].type);
        if (engineRadius < radius) radius = engineRadius;
    }
    if (!radius) {
        plan.nextDecisionTick = now + 400;
        return;
    }
    const int sideX = ownX < targetX ? -1 : 1;
    const int sideY = ownY < targetY ? -1 : 1;
    const int directionX[8] = {sideX, sideX, 0, -sideX, -sideX, -sideX, 0, sideX};
    const int directionY[8] = {0, sideY, sideY, sideY, 0, -sideY, -sideY, -sideY};
    const int choice = plan.cursor & 7;
    plan.cursor = (choice + 1) & 7;
    const int direction = directionX[choice] && directionY[choice]
        ? radius * 3 / 4 : radius;
    int reachable[20], destinationX[20], destinationY[20], count = 0;
    for (int index = 0; index < census.count; ++index) {
        const SiegeHarassEngine& engine = census.engines[index];
        if (!validEngine(player, engine)) continue;
        const unsigned int unit = nativeBindings.unitRecords + engine.unit * 0x490;
        if (at<short>(unit + 0x432) != target) continue;
        // Native siege groups contain one engine. Keep the group together,
        // while longer-range engines stand a few tiles farther back.
        const int row = index / 5;
        const int width = census.count < 5 ? census.count : 5;
        const int column = index % 5 - (width - 1) / 2;
        const int extra = clamp((approachRadius(engine.type) - radius) / 3, 0, 8);
        const int step = clamp(direction + extra - row, 2, 70);
        const int x = targetX + directionX[choice] * step - directionY[choice] * column;
        const int y = targetY + directionY[choice] * step + directionX[choice] * column;
        if (x <= 0 || x >= 399 || y <= 0 || y >= 399) continue;
        const int tile = at<int>(nativeBindings.mapRows + y * 12) + x;
        if (tile <= 0 || tile >= 160000) continue;
        if (reinterpret_cast<Path>(nativeBindings.tribePath)(aic, engine.tribe, tile)) {
            destinationX[count] = x;
            destinationY[count] = y;
            reachable[count++] = index;
        }
    }
    // After the timeout, move a reachable subset, but do not turn a requested
    // group into a serial single-engine assault. Explicit minimum 0/1 allows one.
    const int required = now - plan.firstSeenTick < 800 ? desired
        : desired > 1 ? 2 : 1;
    if (count < required) {
        plan.nextDecisionTick = now + 100;
        return;
    }
    int issued = 0;
    for (int index = 0; index < count; ++index) {
        const SiegeHarassEngine& engine = census.engines[reachable[index]];
        if (reinterpret_cast<Move>(nativeBindings.siegeGroupMove)(
            reinterpret_cast<void*>(nativeBindings.tribes), engine.tribe,
            destinationX[index], destinationY[index], 0, 0, 0)) ++issued;
    }
    if (issued) {
        plan.phase = 1;
        plan.issuedCount = issued;
        plan.x = targetX + directionX[choice] * direction;
        plan.y = targetY + directionY[choice] * direction;
        plan.nextDecisionTick = now + 400;
    } else plan.nextDecisionTick = now + 100;
}

} // namespace SHC141
} // namespace AicTactics
