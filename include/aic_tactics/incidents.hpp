#ifndef AIC_TACTICS_INCIDENTS_HPP
#define AIC_TACTICS_INCIDENTS_HPP

namespace AicTactics {

enum AttackActivation { ImmediateAttack, AfterProvocation };

struct ProvocationRules {
    int threatPower;
    int combatTicks;
    int lossPower;
    int windowTicks;
};

// Fixed storage, indexed by defending and attacking player, never character.
// Each loss bucket owns its simulation epoch; serialization includes both.
struct LossBucket {
    unsigned int epoch;
    unsigned int value;
};

struct HostileIncident {
    int qualified;
    unsigned int qualifiedTick;
    int qualifiedInCurrent;
    int homeQualified;
    int combatStarted;
    unsigned int combatStart;
    unsigned int lastHomeDamage;
    int hasDamage;
    unsigned int lastDamage;
    LossBucket losses[32];
};

struct IncidentState {
    int awake;
    HostileIncident enemies[9];
};

struct IncidentInput {
    unsigned int tick;
    int attacker;
    int hostile;
    int healthLost;
    int lordDamaged;
    int militaryLossPower;
    int atHome;
    int homeEnemyPower;
};

// Receives confirmed native damage observations only. Presence, building loss,
// allied damage and unknown provenance cannot wake a personality.
void observeIncident(const ProvocationRules& rules, IncidentState& state,
    const IncidentInput& input);
bool homeUnderThreat(const ProvocationRules& rules, const IncidentState& state,
    unsigned int tick, const int* hostilePlayers);
void forgetFormerEnemies(IncidentState& state, const int* hostilePlayers);

} // namespace AicTactics
#endif
