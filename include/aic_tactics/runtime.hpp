#ifndef AIC_TACTICS_RUNTIME_HPP
#define AIC_TACTICS_RUNTIME_HPP

#include "aic_tactics/recruitment.hpp"
#include "aic_tactics/native_bindings.hpp"
#include "aic_tactics/composition.hpp"
#include "aic_tactics/combat.hpp"
#include "aic_tactics/army.hpp"
#include "aic_tactics/raids.hpp"

namespace AicTactics {
namespace SHC141 {

struct CharacterConfiguration {
    RecruitmentPolicy recruitment;
    RoleWeights baseRows[3];
    int defenseComposition;
    int initialDefenseTicks;
    CombatConfiguration combat;
    int preparation;
    RaidConfiguration raids;
};

// Diagnostic counters are observation only; policy never reads them.
struct RecruitmentObservation {
    unsigned int sequence;
    int tick;
    int character;
    int strength;
    int facts;
    int eligibleRoles;
    int condition;
    int role;
    int unit;
    int unitType;
    int attempts;
    int rngSamples;
    unsigned int decisions[4];
    unsigned int hires[4];
    unsigned int hireUID;
    int hireTick;
    int hireRole;
    int probeReasons[4];
    int probeTypes[4];
    int freeGroups;
    unsigned int aicAddress;
    int purchaseResource;
    int purchaseAmount;
};

extern CharacterConfiguration configurations[17];
extern RecruitmentObservation observations[9];
extern int configurationLocked;
extern int* legacyWallCounts;
extern int defenseTypeCounts[9][80];
extern unsigned int defenseCensusTick;
extern int defenseCensusValid;
extern unsigned int integrityDigest[2];
void __cdecl captureIntegrity(int legacyInterval);
void __cdecl observeIntegrityBoundary(int legacyInterval);
void __cdecl captureBoundaryIntegrity();
void __cdecl resetDefenseCensus();
void __cdecl countDefenseUnit(int player, int unitType);
void __cdecl invalidateDefenseCensus();

// Called after the original interval/peasant/spending gate. Nonzero means
// this opportunity was handled; zero resumes the original instructions.
int __cdecl recruitOpportunity(void* aic, int player, int attempts);
void __fastcall rangedSortie(void* aic, void*, int player);
void __fastcall meleeSortie(void* aic, void*, int player);

} // namespace SHC141
} // namespace AicTactics
#endif
