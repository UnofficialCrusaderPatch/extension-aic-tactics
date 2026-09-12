#include "aic_tactics/incidents.hpp"
#include <cstring>

namespace AicTactics {

static bool valid(const ProvocationRules& rules)
{
    return rules.threatPower > 0 && rules.threatPower <= 100000
        && rules.lossPower > 0 && rules.lossPower <= 100000
        && rules.combatTicks > 0 && rules.combatTicks <= rules.windowTicks
        && rules.windowTicks >= 32 && rules.windowTicks <= 9600
        && rules.windowTicks % 32 == 0;
}

void observeIncident(const ProvocationRules& rules, IncidentState& state,
    const IncidentInput& input)
{
    if (!valid(rules) || !input.hostile || input.attacker < 1 || input.attacker > 8
        || input.healthLost <= 0) return;
    HostileIncident& incident = state.enemies[input.attacker];
    const unsigned int window = static_cast<unsigned int>(rules.windowTicks);
    const bool newIncident = !incident.hasDamage || input.tick - incident.lastDamage > window;
    if (newIncident) {
        incident.qualifiedInCurrent = 0;
        incident.combatStarted = 0;
        incident.homeQualified = 0;
    }
    incident.hasDamage = 1;
    incident.lastDamage = input.tick;
    bool qualifies = input.lordDamaged != 0;
    if (input.atHome && input.homeEnemyPower >= rules.threatPower) {
        // A quarter of the required duration is the longest permitted gap.
        unsigned int gap = static_cast<unsigned int>(rules.combatTicks / 4);
        if (!gap) gap = 1;
        if (!incident.combatStarted || input.tick - incident.lastHomeDamage > gap) {
            incident.combatStarted = 1;
            incident.combatStart = input.tick;
        }
        incident.lastHomeDamage = input.tick;
        if (input.tick - incident.combatStart >= static_cast<unsigned int>(rules.combatTicks)) {
            incident.homeQualified = 1;
            qualifies = true;
        }
    } else if (input.atHome) {
        incident.combatStarted = 0;
    }
    if (input.lordDamaged && input.atHome) {
        incident.homeQualified = 1;
        incident.lastHomeDamage = input.tick;
    }

    if (input.militaryLossPower > 0) {
        const unsigned int quantum = window / 32;
        const unsigned int epoch = input.tick / quantum;
        LossBucket& bucket = incident.losses[epoch % 32];
        if (bucket.epoch != epoch) { bucket.epoch = epoch; bucket.value = 0; }
        const unsigned int loss = static_cast<unsigned int>(input.militaryLossPower);
        const unsigned int threshold = static_cast<unsigned int>(rules.lossPower);
        bucket.value = loss >= threshold || bucket.value >= threshold - loss
            ? threshold : bucket.value + loss;
        unsigned int lossTotal = 0;
        for (int index = 0; index < 32; ++index) {
            const LossBucket& item = incident.losses[index];
            // Dropping the oldest partial bucket never admits out-of-window losses.
            if (epoch - item.epoch < 32) lossTotal += item.value;
        }
        if (lossTotal >= static_cast<unsigned int>(rules.lossPower)) qualifies = true;
    }
    if (qualifies) {
        state.awake = 1;
        // A continuing incident does not become "newest" for every arrow.
        if (!incident.qualifiedInCurrent) incident.qualifiedTick = input.tick;
        incident.qualified = 1;
        incident.qualifiedInCurrent = 1;
    }
}

bool homeUnderThreat(const ProvocationRules& rules, const IncidentState& state,
    unsigned int tick, const int* hostilePlayers)
{
    if (!valid(rules) || !hostilePlayers) return false;
    for (int player = 1; player <= 8; ++player) {
        const HostileIncident& incident = state.enemies[player];
        if (hostilePlayers[player] && incident.homeQualified
            && tick - incident.lastHomeDamage <= static_cast<unsigned int>(rules.windowTicks)) return true;
    }
    return false;
}

void forgetFormerEnemies(IncidentState& state, const int* hostilePlayers)
{
    if (!hostilePlayers) return;
    for (int player = 1; player <= 8; ++player)
        if (!hostilePlayers[player] && (state.enemies[player].hasDamage || state.enemies[player].qualified))
            std::memset(&state.enemies[player], 0, sizeof(HostileIncident));
}

} // namespace AicTactics
