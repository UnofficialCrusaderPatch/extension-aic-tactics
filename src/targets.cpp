#include "aic_tactics/targets.hpp"

namespace AicTactics {

static bool eligible(const TargetContext& context, int player)
{
    return player >= 1 && player <= 8 && player != context.owner &&
        context.players[player].eligible;
}

static bool validTarget(const TargetContext& context, const TargetState& state)
{
    return eligible(context, state.player) &&
        context.players[state.player].lordUID == state.lordUID;
}

static int metric(const Opponent& player, int policy)
{
    if (policy == LowestPopulation) return player.population;
    if (policy == FewestTroops) return player.troops;
    return player.combatPower;
}

static TargetStatus select(const TargetConfig& config, const TargetContext& context,
    TakeNativeRandomSample takeSample, void* randomContext, int& selected)
{
    selected = 0;
    if (config.policy == InheritTarget) {
        if (eligible(context, context.nativeFallback)) selected = context.nativeFallback;
    } else if (config.policy == RandomTarget) {
        int roster[8], count = 0;
        for (int player = 1; player <= 8; ++player)
            if (eligible(context, player)) roster[count++] = player;
        if (count) {
            BoundedDraw draw;
            if (drawBounded(count, takeSample, randomContext, draw) != DrawSucceeded)
                return TargetRandomFailure;
            selected = roster[draw.ticket];
        }
    } else if (config.policy == LastAggressor) {
        for (int player = 1; player <= 8; ++player) {
            if (!eligible(context, player) || !context.players[player].qualifiedIncident)
                continue;
            if (!selected || context.players[player].incidentAge < context.players[selected].incidentAge)
                selected = player;
        }
        if (!selected && eligible(context, context.nativeFallback))
            selected = context.nativeFallback;
    } else {
        for (int player = 1; player <= 8; ++player) {
            if (!eligible(context, player)) continue;
            if (metric(context.players[player], config.policy) < 0)
                return InvalidTargetInput;
            if (!selected || metric(context.players[player], config.policy) <
                    metric(context.players[selected], config.policy))
                selected = player;
        }
    }
    return selected ? TargetSelected : NoTarget;
}

TargetStatus updateTarget(const TargetConfig& config, TargetState& state,
    TargetEvent event, const TargetContext& context,
    TakeNativeRandomSample takeSample, void* randomContext)
{
    if (config.policy == InheritTarget && config.commitment == DefaultCommitment)
        return UseNativeTarget;
    if (config.policy < InheritTarget || config.policy > LastAggressor ||
        config.commitment < DefaultCommitment || config.commitment > UntilDefeated ||
        context.owner < 1 || context.owner > 8 ||
        event < InitializeTargets || event > FinishAttack ||
        state.player < 0 || state.player > 8 ||
        (state.player == 0 && state.lordUID != 0) ||
        state.attackActive < 0 || state.attackActive > 1 ||
        (state.attackActive && !state.player))
        return InvalidTargetInput;
    const int commitment = config.commitment == DefaultCommitment ? PerAttack : config.commitment;
    const bool valid = validTarget(context, state);
    if (event == FinishAttack) {
        state.attackActive = 0;
        if (commitment == PerAttack || !valid) {
            state.player = 0;
            state.lordUID = 0;
        }
        return TargetUnchanged;
    }
    if (state.attackActive) {
        if (event != MaintainAttack) return InvalidTargetTransition;
        return valid ? TargetUnchanged : TargetNeedsCleanup;
    }
    if (event == MaintainAttack) return InvalidTargetTransition;
    if (commitment == UntilDefeated && valid) {
        if (event == LaunchAttack) state.attackActive = 1;
        return TargetUnchanged;
    }
    if (event == InitializeTargets &&
        (commitment != UntilDefeated || config.policy != RandomTarget))
        return TargetUnchanged;

    int selected;
    TargetStatus result = select(config, context, takeSample, randomContext, selected);
    if (result != TargetSelected && result != NoTarget) return result;
    state.player = selected;
    state.lordUID = selected ? context.players[selected].lordUID : 0;
    state.attackActive = selected && event == LaunchAttack ? 1 : 0;
    return result;
}

} // namespace AicTactics
