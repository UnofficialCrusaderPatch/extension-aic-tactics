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

// A per-character setting array (index = AIC character 1..16) holds 0 to use
// the module setting, else the character's value + 1.
inline int setting(const int values[17], int character, int fallback)
{
    return values[character] ? values[character] - 1 : fallback;
}

// A field of an AIC record (AIC Loader field index * 4) in the native AI
// state; `character` is the player's AI type, 2..17.
inline int aicValue(void* aic, int character, int offset)
{
    return *reinterpret_cast<int*>(static_cast<unsigned char*>(aic) + (character - 1) * 0x2A4 + offset);
}

// The native group move of a tribe to tile x, y. Members keep the flag an
// earlier native order left, and the group move skips flagged members, so it
// is cleared first, as the AI does before its own group moves. Without the
// clear routine (its features off) the move runs without it.
inline bool moveGroup(int tribe, int x, int y)
{
    void* tribes = reinterpret_cast<void*>(nativeBindings.tribes);
    if (nativeBindings.clearGroupMoveFlags)
        reinterpret_cast<void (__thiscall*)(void*, int)>(nativeBindings.clearGroupMoveFlags)(tribes, tribe);
    return reinterpret_cast<int (__thiscall*)(void*, int, int, int, int, int, int)>(nativeBindings.siegeGroupMove)(
        tribes, tribe, x, y, 0, 0, 0) != 0;
}

struct CharacterConfiguration {
    RecruitmentPolicy recruitment;
    RoleWeights baseRows[3];
    int defenseComposition;
    int initialDefenseTicks;
    CombatConfiguration combat;
    int preparation;
    RaidConfiguration raids;
    int safeSiegePlacement; // 0=module fallback, 1=on, 2=off
    int siegeTowerCrew; // 0=module fallback, 1=off, 2=on
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
// Owner module answers filled from Lua; 0 keeps the game's value.
extern unsigned char ownerDefenseRows[17][80];           // [ai][unitType] -> row
extern unsigned char ownerDefenseGroups[17][20][10][11]; // [ai][row][slots - 1][patrolGroups] -> groups + 1
extern unsigned char ownerRaidTypes[80];                 // [unitType] -> raid type
extern int defenseTypeCounts[9][80];
extern unsigned int defenseCensusTick;
extern int defenseCensusValid;
extern int safePlacementFallback;
extern int engineerRoleFallback;
extern int siegePaymentFallback;
extern int siegePaymentPolicy[17];
extern int largeSiegeFallback;
extern int siegeForceMaximumFallback;
extern int largeSiegePolicy[17];
extern int siegeForceMaximum[17];
extern int siegeHarassFallback;
extern int siegeCatapultRange;
extern int siegeHarassMinimumFallback;
extern int siegeHarassPolicy[17];
extern int siegeHarassMinimum[17];
struct SiegeHarassEngine {
    int unit;
    int uid;
    int tribe;
    int tribeUID;
    int type;
};
struct SiegeHarassCensus {
    int count;
    SiegeHarassEngine engines[20];
};
struct SiegeHarassPlan {
    int target;
    int lordUID;
    unsigned int firstSeenTick;
    unsigned int nextDecisionTick;
    int cursor;
    int phase;
    int issuedCount;
    int x;
    int y;
};
extern SiegeHarassCensus siegeHarassCensus[9];
extern SiegeHarassPlan siegeHarassPlans[9];
void __cdecl resetSiegeHarassCensus();
void __cdecl countSiegeHarassUnit(int unit);
void __cdecl updateSiegeHarassment(void* aic, int player);
int __cdecl suppressNativeSiegeHarassMove(int unitOffset);
int __cdecl countableEngineerRole(int player, int unit);
int __fastcall checkedSiegeTile(void* tileMap, void*, int tile, int player, int command, int flags);
int __cdecl siegePlacementPolicyEnabled(int player);
int __cdecl siegeTentCandidateAllowed(int tribe, int point);
int __cdecl failedSiegeTent(int tribe, int point);
int __fastcall placeSiegeTentAndRecoverEngineers(void* troopValue, void*, int tribe,
    int command, unsigned int distance, int instruction);
void __fastcall buildLargerSiegeForce(void* aic, void*, int player);
int __cdecl siegeResourceAdmission(int player, int command);
int __cdecl siegePaymentEnabledForGoldOffset(int playerStrideOffset);
extern unsigned int integrityDigest[2];
void __cdecl captureIntegrity(int legacyInterval, unsigned int defenseTicks, int targetPolicy, const void* aicRecords);
void __cdecl observeIntegrityBoundary(int legacyInterval, unsigned int defenseTicks, int targetPolicy, const void* aicRecords);
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
