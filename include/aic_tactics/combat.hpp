#ifndef AIC_TACTICS_COMBAT_HPP
#define AIC_TACTICS_COMBAT_HPP

#include "aic_tactics/targets.hpp"
#include "aic_tactics/incidents.hpp"

namespace AicTactics {
namespace SHC141 {

struct CombatConfiguration {
    TargetConfig targeting;
    int activation;
    ProvocationRules provocation;
};

struct PlayerCensus {
    int troops;
    int lord;
    int lordUID;
    int homePower[9];
};

extern PlayerCensus combatCensus[9];
extern unsigned int combatCensusTick;
extern int combatCensusValid;
extern IncidentState incidents[9];
extern TargetState targetStates[9];
extern int targetLifecycle[9];
extern int legacyTargetPolicy;
extern unsigned int originalUnitDamage;
extern unsigned int originalEntityDamage;
extern unsigned int originalFireDamage;

void __cdecl resetCombatCensus();
void __cdecl countCombatUnit(int unit);
void __cdecl completeCombatCensus();
bool qualifyingHomeThreat(int player);
bool offensiveActionsAllowed(int player);
bool targetPolicyActive(int player);
bool committedAttackActive(int player);
bool readTargetContext(int player, TargetContext& context);
int __fastcall observedUnitDamage(void* units, void*, int attacker, int victim);
int __fastcall observedEntityDamage(void* units, void*, int victim, int entity, int third);
int __fastcall observedFireDamage(void* units, void*, int victim, int owner, int third);
void __fastcall selectOpponent(void* aic, void*, int player);
void __fastcall updateOffensiveArmy(void* aic, void*, int player);
void __fastcall updateOffensiveRaids(void* aic, void*, int player);
void __fastcall returnFromAttack(void* aic, void*, int player);
int __cdecl commitOpponent(int player);
int __cdecl preserveRandomWaveRequirement(int playerOffset);

} // namespace SHC141
} // namespace AicTactics
#endif
