#include "aic_tactics/combat.hpp"
#include "aic_tactics/runtime.hpp"
#include "aic_tactics/shc141_damage.hpp"
#include <cstring>

namespace AicTactics {
namespace SHC141 {

PlayerCensus combatCensus[9];
unsigned int combatCensusTick;
int combatCensusValid;
IncidentState incidents[9];
TargetState targetStates[9];
int targetLifecycle[9];
int legacyTargetPolicy;
unsigned int originalUnitDamage;
unsigned int originalEntityDamage;
unsigned int originalFireDamage;

namespace {
template<class T> T& at(unsigned int address) { return *reinterpret_cast<T*>(address); }
const unsigned int PlayerStride = 0x39F4;
const unsigned int& Units = nativeBindings.unitRecords;
int homeObservers[9];
int combatValues[80];
typedef void (__thiscall *PlayerAction)(void*, int);
typedef int (__thiscall *PlayerQuery)(void*, int);

int playerValue(int player, unsigned int address) { return at<int>(address + player * PlayerStride); }
const CharacterConfiguration* configuration(int player)
{
    if (player < 1 || player > 8) return 0;
    const int character = playerValue(player, (nativeBindings.players + 0x2300));
    return character >= 2 && character <= 17 ? &configurations[character - 1] : 0;
}

bool militaryPerson(int type)
{
    return type == 5 || (type >= 22 && type <= 30) || type == 37
        || (type >= 70 && type <= 76);
}

bool observesIncidents(int player)
{
    const CharacterConfiguration* config = configuration(player);
    if (!config) return false;
    if (config->combat.activation == AfterProvocation || config->combat.targeting.policy == LastAggressor) return true;
    if (config->recruitment.mode != WeightedRoles) return false;
    for (int index = 0; index < config->recruitment.conditionCount; ++index)
        if ((config->recruitment.conditions[index].requiredFacts
            | config->recruitment.conditions[index].forbiddenFacts) & HomeUnderThreat) return true;
    return false;
}

bool hostile(int player, int other)
{
    return player >= 1 && player <= 8 && other >= 1 && other <= 8 && player != other
        && at<int>(nativeBindings.teams + player * 4) != at<int>(nativeBindings.teams + other * 4);
}

bool atHome(int player, int x, int y)
{
    const int dx = x - playerValue(player, (nativeBindings.players + 0x98));
    const int dy = y - playerValue(player, (nativeBindings.players + 0x9C));
    return dx >= -32 && dx <= 32 && dy >= -32 && dy <= 32 && dx * dx + dy * dy <= 32 * 32;
}

DamageMemory damageMemory()
{
    DamageMemory memory;
    memory.units = reinterpret_cast<const unsigned char*>(Units);
    memory.unitCapacity = nativeBindings.unitCapacity;
    memory.entities = reinterpret_cast<const unsigned char*>(nativeBindings.entities);
    memory.entityCapacity = nativeBindings.entityCapacity;
    memory.teams = reinterpret_cast<const int*>(nativeBindings.teams);
    return memory;
}

bool beginObservedDamage(DamageSource source, int first, int second, DamageProbe& probe)
{
    const int victim = source == UnitDamage ? second : first;
    if (victim <= 0 || victim >= static_cast<int>(nativeBindings.unitCapacity)) return false;
    const int player = at<short>(Units + victim * 0x490 + 0x96);
    return observesIncidents(player) && beginDamage(damageMemory(), source, first, second, probe);
}

void finishObservedDamage(DamageProbe& probe)
{
    DamageObservation damage;
    if (!finishDamage(damageMemory(), probe, damage)) return;
    const CharacterConfiguration* config = configuration(damage.victimOwner);
    if (!config) return;
    IncidentInput input;
    input.tick = at<unsigned int>(nativeBindings.gameTick);
    input.attacker = damage.sourceOwner;
    input.hostile = hostile(damage.victimOwner, damage.sourceOwner) ? 1 : 0;
    input.healthLost = damage.healthLost;
    input.lordDamaged = damage.unitType == 55 ? 1 : 0;
    typedef int (__thiscall *CombatValue)(void*, int);
    input.militaryLossPower = damage.killed && militaryPerson(damage.unitType)
        ? reinterpret_cast<CombatValue>(0x51C360)(reinterpret_cast<void*>(0x1763348), damage.unitType) : 0;
    input.atHome = atHome(damage.victimOwner, damage.x, damage.y) ? 1 : 0;
    input.homeEnemyPower = combatCensusValid && input.tick - combatCensusTick <= 1
        ? combatCensus[damage.victimOwner].homePower[damage.sourceOwner] : 0;
    observeIncident(config->combat.provocation, incidents[damage.victimOwner], input);
}
} // namespace

void __cdecl resetCombatCensus()
{
    resetReserveCensus();
    resetRaidUnitCensus();
    combatCensusValid = 0;
    std::memset(combatCensus, 0, sizeof(combatCensus));
    bool needsPower = raidPoliciesEnabled();
    for (int player = 1; player <= 8; ++player) {
        homeObservers[player] = observesIncidents(player) ? 1 : 0;
        if (homeObservers[player]) needsPower = true;
        int hostilePlayers[9] = {0};
        for (int other = 1; other <= 8; ++other) hostilePlayers[other] = hostile(player, other) ? 1 : 0;
        forgetFormerEnemies(incidents[player], hostilePlayers);
    }
    if (needsPower) {
        typedef int (__thiscall *CombatValue)(void*, int);
        for (int type = 0; type < 80; ++type)
            combatValues[type] = reinterpret_cast<CombatValue>(0x51C360)(reinterpret_cast<void*>(0x1763348), type);
    }
}

void __cdecl countCombatUnit(int unit)
{
    if (unit <= 0 || unit >= static_cast<int>(nativeBindings.unitCapacity)) return;
    countReserveUnit(unit);
    const unsigned int address = Units + unit * 0x490;
    if (at<short>(address + 0x8C) != 2 || at<short>(address + 0x2A0) != 0
        || at<int>(address + 0x3C8) <= 0) return;
    const int player = at<short>(address + 0x96);
    if (player < 1 || player > 8) return;
    const int type = at<short>(address + 0x8E);
    if (type == 55 && !combatCensus[player].lord) {
        combatCensus[player].lord = unit;
        combatCensus[player].lordUID = at<int>(address + 0x98);
    }
    if (militaryPerson(type)) ++combatCensus[player].troops;
    countRaidUnit(unit, type >= 0 && type < 80 ? combatValues[type] : 0);
    if (type < 0 || type >= 80 || combatValues[type] <= 0) return;
    const int x = at<short>(address + 0xC4), y = at<short>(address + 0xC6);
    for (int observer = 1; observer <= 8; ++observer) {
        if (!homeObservers[observer] || !hostile(observer, player) || !atHome(observer, x, y)) continue;
        const int value = combatValues[type];
        int& power = combatCensus[observer].homePower[player];
        power = value > 0x7FFFFFFF - power ? 0x7FFFFFFF : power + value;
    }
}

void __cdecl completeCombatCensus()
{
    completeReserveCensus();
    configurationLocked = 1;
    combatCensusTick = at<unsigned int>(nativeBindings.gameTick);
    combatCensusValid = 1;
    for (int player = 1; player <= 8; ++player) {
        const CharacterConfiguration* config = configuration(player);
        if (config && !combatCensus[player].lord) {
            std::memset(&incidents[player], 0, sizeof(IncidentState));
            std::memset(&targetStates[player], 0, sizeof(TargetState));
            targetLifecycle[player] = 0;
            std::memset(&reserves[player], 0, sizeof(ReserveState));
            std::memset(&raidStates[player], 0, sizeof(RaidState));
            std::memset(raidGroupCensus[player], 0, sizeof(raidGroupCensus[player]));
            continue;
        }
        if (!config || !targetPolicyActive(player) || !combatCensus[player].lord) continue;
        TargetContext context;
        if (!readTargetContext(player, context)) continue;
        if (targetLifecycle[player] & 2) {
            updateTarget(config->combat.targeting, targetStates[player], FinishAttack, context, nativeRandom, 0);
            targetLifecycle[player] &= ~2;
        }
        if (!(targetLifecycle[player] & 1)) {
            const TargetStatus result = updateTarget(config->combat.targeting, targetStates[player],
                InitializeTargets, context, nativeRandom, 0);
            if (result != InvalidTargetInput && result != TargetRandomFailure) targetLifecycle[player] |= 1;
        }
    }
}

bool qualifyingHomeThreat(int player)
{
    const CharacterConfiguration* config = configuration(player);
    if (!config) return false;
    int hostilePlayers[9] = {0};
    for (int other = 1; other <= 8; ++other) hostilePlayers[other] = hostile(player, other) ? 1 : 0;
    return homeUnderThreat(config->combat.provocation, incidents[player],
        at<unsigned int>(nativeBindings.gameTick), hostilePlayers);
}

bool offensiveActionsAllowed(int player)
{
    const CharacterConfiguration* config = configuration(player);
    return !config || config->combat.activation == ImmediateAttack || incidents[player].awake != 0;
}

bool targetPolicyActive(int player)
{
    const CharacterConfiguration* config = configuration(player);
    return config && (config->combat.targeting.policy != InheritTarget
        || config->combat.targeting.commitment != DefaultCommitment);
}

bool committedAttackActive(int player)
{
    return player >= 1 && player <= 8 && targetStates[player].attackActive != 0;
}

bool readTargetContext(int player, TargetContext& context)
{
    std::memset(&context, 0, sizeof(context));
    context.owner = player;
    if (player < 1 || player > 8 || !combatCensusValid
        || at<unsigned int>(nativeBindings.gameTick) - combatCensusTick > 1) return false;
    context.nativeFallback = playerValue(player, (nativeBindings.players + 0x2BD8));
    for (int other = 1; other <= 8; ++other) {
        if (!hostile(player, other)) continue;
        const PlayerCensus& census = combatCensus[other];
        if (census.lord <= 0 || census.lord >= static_cast<int>(nativeBindings.unitCapacity)) continue;
        const unsigned int lord = Units + census.lord * 0x490;
        if (at<short>(lord + 0x8C) != 2 || at<short>(lord + 0x8E) != 55
            || at<short>(lord + 0x96) != other || at<int>(lord + 0x98) != census.lordUID
            || at<short>(lord + 0x2A0) != 0 || at<int>(lord + 0x3C8) <= 0) continue;
        Opponent& opponent = context.players[other];
        opponent.eligible = true;
        opponent.lordUID = census.lordUID;
        opponent.population = playerValue(other, (nativeBindings.players + 0x2180));
        opponent.troops = census.troops;
        opponent.combatPower = playerValue(other, (nativeBindings.players + 0x38E8));
        opponent.qualifiedIncident = incidents[player].enemies[other].qualified != 0;
        opponent.incidentAge = at<unsigned int>(nativeBindings.gameTick) - incidents[player].enemies[other].qualifiedTick;
    }
    return true;
}

int __fastcall observedUnitDamage(void* units, void*, int attacker, int victim)
{
    DamageProbe probe;
    const bool observing = beginObservedDamage(UnitDamage, attacker, victim, probe);
    typedef int (__thiscall *Original)(void*, int, int);
    const int result = reinterpret_cast<Original>(originalUnitDamage)(units, attacker, victim);
    if (observing) finishObservedDamage(probe);
    return result;
}

int __fastcall observedEntityDamage(void* units, void*, int victim, int entity, int third)
{
    DamageProbe probe;
    const bool observing = beginObservedDamage(EntityDamage, victim, entity, probe);
    typedef int (__thiscall *Original)(void*, int, int, int);
    const int result = reinterpret_cast<Original>(originalEntityDamage)(units, victim, entity, third);
    if (observing) finishObservedDamage(probe);
    return result;
}

int __fastcall observedFireDamage(void* units, void*, int victim, int owner, int third)
{
    DamageProbe probe;
    const bool observing = beginObservedDamage(FireDamage, victim, owner, probe);
    typedef int (__thiscall *Original)(void*, int, int, int);
    const int result = reinterpret_cast<Original>(originalFireDamage)(units, victim, owner, third);
    if (observing) finishObservedDamage(probe);
    return result;
}

int __cdecl commitOpponent(int player)
{
    if (!offensiveActionsAllowed(player)) return 0;
    if (!targetPolicyActive(player)) { noteArmyLaunch(player); return 1; }
    TargetContext context;
    if (!readTargetContext(player, context)) return 0;
    const CharacterConfiguration* config = configuration(player);
    TargetState& state = targetStates[player];
    if (targetLifecycle[player] & 2) {
        updateTarget(config->combat.targeting, state, FinishAttack, context, nativeRandom, 0);
        targetLifecycle[player] &= ~2;
    }
    const TargetStatus status = updateTarget(config->combat.targeting, state,
        state.attackActive ? MaintainAttack : LaunchAttack, context, nativeRandom, 0);
    if (status != TargetSelected && status != TargetUnchanged) return 0;
    if (!state.player) return 0;
    at<int>((nativeBindings.players + 0x2BD4) + player * PlayerStride) = state.player;
    at<int>((nativeBindings.players + 0x2BD8) + player * PlayerStride) = state.player;
    noteArmyLaunch(player);
    return 1;
}

void __fastcall selectOpponent(void* aic, void*, int player)
{
    if (!targetPolicyActive(player)) {
        reinterpret_cast<PlayerAction>(0x4D4680)(aic, player);
        return;
    }
    const CharacterConfiguration* config = configuration(player);
    TargetContext context;
    if (!readTargetContext(player, context)) return;
    TargetState& state = targetStates[player];
    if (state.attackActive) {
        reinterpret_cast<PlayerAction>(0x4D3780)(aic, player);
        if (updateTarget(config->combat.targeting, state, MaintainAttack, context, nativeRandom, 0)
            == TargetNeedsCleanup) {
            if (playerValue(player, (nativeBindings.players + 0x2BA4)) != 9) at<int>((nativeBindings.players + 0x2BA4) + player * PlayerStride) = 8;
            at<int>((nativeBindings.players + 0x2BD8) + player * PlayerStride) = 0;
        } else {
            at<int>((nativeBindings.players + 0x2BD8) + player * PlayerStride) = state.player;
        }
        return;
    }
    if (config->combat.targeting.commitment == UntilDefeated && state.player
        && context.players[state.player].eligible
        && context.players[state.player].lordUID == state.lordUID) {
        reinterpret_cast<PlayerAction>(0x4D3780)(aic, player);
        at<int>((nativeBindings.players + 0x2BD8) + player * PlayerStride) = state.player;
        return;
    }
    // Preserve native nervousness and TargetChoice/request behavior for fallback.
    reinterpret_cast<PlayerAction>(0x4D4680)(aic, player);
    context.nativeFallback = playerValue(player, (nativeBindings.players + 0x2BD8));
    if (config->combat.targeting.policy == RandomTarget) {
        if (config->combat.targeting.commitment == UntilDefeated) {
            updateTarget(config->combat.targeting, state, InitializeTargets, context, nativeRandom, 0);
            if (state.player) at<int>((nativeBindings.players + 0x2BD8) + player * PlayerStride) = state.player;
        }
        return; // PerAttack draws only after the admitted native readiness call.
    }
    TargetState preview = {0, 0, 0};
    const TargetStatus result = updateTarget(config->combat.targeting, preview,
        LaunchAttack, context, nativeRandom, 0);
    if (result == TargetSelected || result == NoTarget)
        at<int>((nativeBindings.players + 0x2BD8) + player * PlayerStride) = preview.player;
}

void __fastcall updateOffensiveArmy(void* aic, void*, int player)
{
    if (!offensiveActionsAllowed(player)) return;
    const int phaseBefore = playerValue(player, (nativeBindings.players + 0x2BA4));
    prepareArmyUpdate(aic, player);
    if (!targetPolicyActive(player)) {
        reinterpret_cast<PlayerAction>(0x4D49E0)(aic, player);
        finishArmyUpdate(player, phaseBefore);
        return;
    }
    TargetContext context;
    if (!readTargetContext(player, context)) return;
    const CharacterConfiguration* config = configuration(player);
    TargetState& state = targetStates[player];
    if (state.attackActive) {
        const TargetStatus status = updateTarget(config->combat.targeting, state,
            MaintainAttack, context, nativeRandom, 0);
        if (status == TargetNeedsCleanup) {
            if (playerValue(player, (nativeBindings.players + 0x2BA4)) != 9) at<int>((nativeBindings.players + 0x2BA4) + player * PlayerStride) = 8;
            at<int>((nativeBindings.players + 0x2BD8) + player * PlayerStride) = 0;
        } else at<int>((nativeBindings.players + 0x2BD8) + player * PlayerStride) = state.player;
    }
    // A help request uses a separate early native dispatch path. Defer its
    // dispatch while this policy owns an army; preserve the request itself.
    int& request = at<int>((nativeBindings.players + 0x384C) + player * PlayerStride);
    const int savedRequest = request;
    if (state.attackActive || config->combat.targeting.policy != InheritTarget) request = 0;
    reinterpret_cast<PlayerAction>(0x4D49E0)(aic, player);
    finishArmyUpdate(player, phaseBefore);
    if (request == 0 && savedRequest != 0) request = savedRequest;
}

void __fastcall returnFromAttack(void* aic, void*, int player)
{
    reinterpret_cast<PlayerAction>(0x4CEA50)(aic, player);
    const CharacterConfiguration* config = configuration(player);
    if (!config || (config->preparation == NativePreparation && !targetPolicyActive(player))) return;
    if (playerValue(player, (nativeBindings.players + 0x2B7C)) != 0
        || reinterpret_cast<PlayerQuery>(0x4CFFD0)(aic, player)
        || returnArmyToCampfire(aic, player)) {
        noteArmyReturn(player);
        if (targetPolicyActive(player) && targetStates[player].attackActive) targetLifecycle[player] |= 2;
    }
}

void __fastcall updateOffensiveRaids(void* aic, void*, int player)
{
    if (offensiveActionsAllowed(player) && !updateSplitRaids(aic, player))
        reinterpret_cast<PlayerAction>(0x4D2A70)(aic, player);
}

int __cdecl preserveRandomWaveRequirement(int playerOffset)
{
    if (playerOffset < 0 || playerOffset % PlayerStride) return 0;
    const CharacterConfiguration* config = configuration(playerOffset / PlayerStride);
    return config && config->combat.targeting.policy == RandomTarget
        && config->combat.targeting.commitment != UntilDefeated ? 1 : 0;
}

} // namespace SHC141
} // namespace AicTactics
