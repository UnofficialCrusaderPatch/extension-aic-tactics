#include <cassert>
#include <cstring>
#include "aic_tactics/incidents.hpp"
using namespace AicTactics;

int main()
{
    ProvocationRules rules = {100, 200, 100, 800};
    IncidentState state;
    std::memset(&state, 0, sizeof(state));
    IncidentInput hit = {100, 2, 1, 1, 0, 0, 1, 100};
    // Presence or allied damage never starts an incident.
    hit.healthLost = 0; observeIncident(rules, state, hit);
    assert(!state.awake && !state.enemies[2].hasDamage);
    hit.healthLost = 1; hit.hostile = 0; observeIncident(rules, state, hit);
    assert(!state.enemies[2].hasDamage);
    hit.hostile = 1;
    for (hit.tick = 100; hit.tick < 300; hit.tick += 50) {
        observeIncident(rules, state, hit); assert(!state.awake);
    }
    observeIncident(rules, state, hit);
    assert(state.awake && state.enemies[2].qualifiedTick == 300);
    hit.tick = 350; observeIncident(rules, state, hit);
    assert(state.enemies[2].qualifiedTick == 300);
    int enemies[9] = {0}; enemies[2] = 1;
    assert(homeUnderThreat(rules, state, 1150, enemies));
    assert(!homeUnderThreat(rules, state, 1151, enemies));
    enemies[2] = 0; forgetFormerEnemies(state, enemies);
    assert(state.awake && !state.enemies[2].qualified && !state.enemies[2].hasDamage);

    std::memset(&state, 0, sizeof(state));
    hit.atHome = 0; hit.militaryLossPower = 49; hit.tick = 100;
    observeIncident(rules, state, hit); assert(!state.awake);
    hit.tick = 899; hit.militaryLossPower = 50;
    observeIncident(rules, state, hit); assert(!state.awake);
    hit.tick = 900; hit.militaryLossPower = 49;
    observeIncident(rules, state, hit); assert(!state.awake); // old bucket expired
    hit.militaryLossPower = 1; observeIncident(rules, state, hit);
    assert(state.awake && state.enemies[2].qualifiedTick == 900);
    hit.tick = 1701; hit.lordDamaged = 1; hit.militaryLossPower = 0;
    observeIncident(rules, state, hit);
    assert(state.enemies[2].qualifiedTick == 1701 && !state.enemies[2].homeQualified);

    // Lord damage is immediate; damage duration alone needs continuous hits.
    std::memset(&state, 0, sizeof(state));
    hit.atHome = 1; hit.lordDamaged = 0; hit.tick = 0;
    observeIncident(rules, state, hit);
    hit.tick = 201; observeIncident(rules, state, hit); assert(!state.awake);
    hit.lordDamaged = 1; observeIncident(rules, state, hit);
    assert(state.awake && state.enemies[2].homeQualified);
    return 0;
}
