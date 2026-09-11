#ifndef AIC_TACTICS_TARGETS_HPP
#define AIC_TACTICS_TARGETS_HPP

#include "aic_tactics/random.hpp"

namespace AicTactics {

enum TargetPolicy {
    InheritTarget, LowestPopulation, FewestTroops, LowestCombatPower,
    RandomTarget, LastAggressor
};

enum TargetCommitment { DefaultCommitment, PerAttack, UntilDefeated };
enum TargetEvent { InitializeTargets, LaunchAttack, MaintainAttack, FinishAttack };
enum TargetStatus {
    UseNativeTarget, TargetUnchanged, TargetSelected, NoTarget,
    TargetNeedsCleanup, InvalidTargetInput, InvalidTargetTransition,
    TargetRandomFailure
};

struct TargetConfig {
    int policy;
    int commitment;
};

// Per-player runtime state. The existing save/restore owner must serialize each
// field explicitly, not compiler padding. Configuration remains per character.
struct TargetState {
    int player;
    int lordUID;
    int attackActive;
};

struct Opponent {
    bool eligible;
    int lordUID;
    int population;
    int troops;
    int combatPower;
    bool qualifiedIncident;
    unsigned int incidentAge;
};

struct TargetContext {
    int owner;
    // Indexed by player ID, 1..8. Entry 0 and the owner's entry are ignored.
    // The native adapter owns alive/hostile/supported checks and verified metrics.
    Opponent players[9];
    int nativeFallback;
};

// LaunchAttack is an admitted native launch, never a render/poll/AI-readiness
// query. State changes only on success. Active invalidation requests native
// cleanup without selecting a replacement. Native bypass does not inspect input.
TargetStatus updateTarget(const TargetConfig& config, TargetState& state,
    TargetEvent event, const TargetContext& context,
    TakeNativeRandomSample takeSample, void* randomContext);

} // namespace AicTactics

#endif
