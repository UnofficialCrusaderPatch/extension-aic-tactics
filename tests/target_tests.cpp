#include <cassert>
#include <cstring>
#include "aic_tactics/targets.hpp"

using namespace AicTactics;

struct Random { int value; int calls; };
static int take(void* context)
{
    Random& random = *static_cast<Random*>(context);
    ++random.calls;
    return random.value;
}

static TargetContext fixture()
{
    TargetContext context;
    std::memset(&context, 0, sizeof(context));
    context.owner = 1;
    context.nativeFallback = 4;
    for (int player = 1; player <= 4; ++player) {
        Opponent& opponent = context.players[player];
        opponent.eligible = true;
        opponent.lordUID = player * 100;
        opponent.population = player * 10;
        opponent.troops = player == 3 ? 0 : 100;
        opponent.combatPower = player == 4 ? 0 : 100;
    }
    return context;
}

static void defaultsAndMetrics()
{
    TargetState state = {0, 0, 0};
    TargetConfig config = {InheritTarget, DefaultCommitment};
    TargetContext context = fixture();
    Random random = {0, 0};
    // Native bypass leaves even irrelevant/malformed extension inputs alone.
    context.owner = -100;
    state.player = -100;
    assert(updateTarget(config, state, LaunchAttack, context, take, &random) == UseNativeTarget);
    assert(state.player == -100 && random.calls == 0);
    for (int policy = LowestPopulation; policy <= LowestCombatPower; ++policy) {
        context = fixture(); state.player = 0;
        config.policy = policy;
        assert(updateTarget(config, state, InitializeTargets, context, take, &random) == TargetUnchanged);
        assert(state.player == 0);
        assert(updateTarget(config, state, LaunchAttack, context, take, &random) == TargetSelected);
        assert(state.player == policy + 1 && state.attackActive == 1);
        // Better opponents and new native requests do not divert the deployed army.
        context.players[2].population = 0;
        context.players[2].troops = 0;
        context.players[2].combatPower = 0;
        context.nativeFallback = 2;
        assert(updateTarget(config, state, MaintainAttack, context, take, &random) == TargetUnchanged);
        assert(state.player == policy + 1);
        assert(updateTarget(config, state, FinishAttack, context, take, &random) == TargetUnchanged);
        assert(state.player == 0 && state.attackActive == 0 && state.lordUID == 0);
    }
    assert(random.calls == 0);
    config.policy = InheritTarget; config.commitment = PerAttack;
    assert(updateTarget(config, state, LaunchAttack, fixture(), 0, 0) == TargetSelected);
    assert(state.player == 4);
}

static void randomLifetimes()
{
    TargetConfig config = {RandomTarget, DefaultCommitment};
    TargetContext context = fixture();
    TargetState state = {0, 0, 0};
    Random random = {1, 0};
    assert(updateTarget(config, state, InitializeTargets, context, take, &random) == TargetUnchanged);
    assert(random.calls == 0);
    for (int wave = 0; wave < 2; ++wave) {
        assert(updateTarget(config, state, LaunchAttack, context, take, &random) == TargetSelected);
        assert(state.player == 3 && random.calls == wave + 1);
        TargetState restored = state;
        for (int tick = 0; tick < 20; ++tick) {
            assert(updateTarget(config, state, MaintainAttack, context, take, &random) == TargetUnchanged);
            assert(updateTarget(config, restored, MaintainAttack, context, take, &random) == TargetUnchanged);
            assert(state.player == restored.player && state.lordUID == restored.lordUID);
        }
        assert(random.calls == wave + 1);
        assert(updateTarget(config, state, FinishAttack, context, take, &random) == TargetUnchanged);
    }
    config.commitment = UntilDefeated;
    random.calls = 0;
    assert(updateTarget(config, state, InitializeTargets, context, take, &random) == TargetSelected);
    assert(state.player == 3 && state.attackActive == 0 && random.calls == 1);
    assert(updateTarget(config, state, InitializeTargets, context, take, &random) == TargetUnchanged);
    for (int wave = 0; wave < 2; ++wave) {
        assert(updateTarget(config, state, LaunchAttack, context, take, &random) == TargetUnchanged);
        assert(state.attackActive == 1 && state.player == 3);
        assert(updateTarget(config, state, FinishAttack, context, take, &random) == TargetUnchanged);
        assert(state.player == 3 && random.calls == 1);
    }
    context.players[3].eligible = false;
    random.value = 0;
    assert(updateTarget(config, state, LaunchAttack, context, take, &random) == TargetSelected);
    assert(state.player == 2 && random.calls == 2);
}

static void invalidationAndIncidents()
{
    TargetConfig config = {LastAggressor, UntilDefeated};
    TargetContext context = fixture();
    TargetState state = {0, 0, 0};
    assert(updateTarget(config, state, LaunchAttack, context, 0, 0) == TargetSelected);
    assert(state.player == 4); // Historical ranking is the no-incident fallback.
    context.players[2].qualifiedIncident = true;
    context.players[2].incidentAge = 10;
    assert(updateTarget(config, state, MaintainAttack, context, 0, 0) == TargetUnchanged);
    assert(state.player == 4);
    assert(updateTarget(config, state, FinishAttack, context, 0, 0) == TargetUnchanged);
    assert(updateTarget(config, state, LaunchAttack, context, 0, 0) == TargetUnchanged);
    assert(state.player == 4); // Commitment wins over a newer qualifying incident.
    ++context.players[4].lordUID;
    assert(updateTarget(config, state, MaintainAttack, context, 0, 0) == TargetNeedsCleanup);
    assert(state.player == 4 && state.lordUID == 400 && state.attackActive == 1);
    assert(updateTarget(config, state, LaunchAttack, context, 0, 0) == InvalidTargetTransition);
    assert(updateTarget(config, state, FinishAttack, context, 0, 0) == TargetUnchanged);
    assert(state.player == 0);
    context.players[3].qualifiedIncident = true;
    context.players[3].incidentAge = 10;
    assert(updateTarget(config, state, LaunchAttack, context, 0, 0) == TargetSelected);
    assert(state.player == 2); // Equal qualification ages use ascending player ID.
    context.players[2].eligible = false;
    assert(updateTarget(config, state, MaintainAttack, context, 0, 0) == TargetNeedsCleanup);
    assert(state.player == 2);
}

static void boundariesAndIsolation()
{
    TargetContext context = fixture();
    TargetConfig config = {RandomTarget, PerAttack};
    TargetState first = {0, 0, 0}, second = {0, 0, 0};
    Random random = {0, 0};
    for (int mask = 0; mask < 256; ++mask) {
        first.player = first.lordUID = first.attackActive = 0;
        for (int player = 1; player <= 8; ++player) {
            context.players[player].eligible = (mask & (1 << (player - 1))) != 0;
            context.players[player].lordUID = player;
        }
        int expected[7], count = 0;
        for (int player = 2; player <= 8; ++player)
            if (context.players[player].eligible) expected[count++] = player;
        for (int ticket = 0; ticket < (count ? count : 1); ++ticket) {
            first.player = first.lordUID = first.attackActive = 0;
            random.value = ticket; random.calls = 0;
            TargetStatus result = updateTarget(config, first, LaunchAttack, context, take, &random);
            assert(result == (count ? TargetSelected : NoTarget));
            assert(first.player == (count ? expected[ticket] : 0));
            assert(random.calls == (count > 1 ? 1 : 0));
        }
    }
    context = fixture();
    first.player = first.lordUID = first.attackActive = 0;
    random.value = 0;
    assert(updateTarget(config, first, LaunchAttack, context, take, &random) == TargetSelected);
    random.value = 1;
    assert(updateTarget(config, second, LaunchAttack, context, take, &random) == TargetSelected);
    assert(first.player == 2 && second.player == 3);
    first.player = first.lordUID = first.attackActive = 0;
    random.value = 32767; random.calls = 0;
    assert(updateTarget(config, first, LaunchAttack, context, take, &random) == TargetRandomFailure);
    assert(first.player == 0 && first.attackActive == 0 && random.calls == 8);
    config.policy = LowestPopulation;
    context.players[3].population = -1;
    assert(updateTarget(config, first, LaunchAttack, context, 0, 0) == InvalidTargetInput);
    assert(first.player == 0);
    context.players[3].population = 30;
    context.players[2].lordUID = 0;
    assert(updateTarget(config, first, LaunchAttack, context, 0, 0) == InvalidTargetInput);
}

int main()
{
    defaultsAndMetrics();
    randomLifetimes();
    invalidationAndIncidents();
    boundariesAndIsolation();
    return 0;
}
