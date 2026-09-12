#ifndef AIC_TACTICS_RAIDS_HPP
#define AIC_TACTICS_RAIDS_HPP

#include "aic_tactics/army.hpp"

namespace AicTactics {
namespace SHC141 {

enum RaidPolicy { NativeRaids, NearestReachableRaid, OpportunisticRaid };
enum RaidFocus { AnyRaidFocus, FoodRaidFocus, IndustryRaidFocus, HighValueRaidFocus };
enum RaidRisk { LowRaidRisk, MediumRaidRisk, HighRaidRisk };
enum RaidScope { PrimeRaidTarget, AnyRaidEnemy };
struct RaidConfiguration {
    int policy;
    int groups;
    int minimumSize;
    int focus;
    int risk;
    int scope;
};
struct RaidGroup {
    ReserveGroup tribe;
    int building;
    unsigned int buildingUID;
    int pathCursor;
};
struct RaidState {
    int decisionGroup;
    int stagingGroup;
    int stagingWord;
    unsigned int retargetPending;
    RaidGroup groups[4];
};
struct RaidGroupCensus { int combatants; int power; };
extern RaidState raidStates[9];
extern RaidGroupCensus raidGroupCensus[9][4];
extern int raidUnitPower[9][1024];
extern int raidStaticDefenses[9][1024];
extern unsigned int raidBuildingCensusTick;
extern int raidBuildingCensusValid;
bool raidPoliciesEnabled();
void resetRaidUnitCensus();
void countRaidUnit(int unit, int power);
void __cdecl resetRaidBuildingCensus();
void __cdecl countRaidBuilding(int building);
void __cdecl completeRaidBuildingCensus();
bool updateSplitRaids(void* aic, int player);

} // namespace SHC141
} // namespace AicTactics
#endif
