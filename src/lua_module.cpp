#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "aic_tactics/runtime.hpp"
#include "aic_tactics/patrols.hpp"
#include "aic_tactics/groups.hpp"
#include "aic_tactics/lord_hunt.hpp"
#include "aic_tactics/allies.hpp"
#include "aic_tactics/diagnostics.hpp"

typedef char ConfigurationABI[sizeof(AicTactics::SHC141::CharacterConfiguration) == 352 ? 1 : -1];
typedef char ReserveABI[sizeof(AicTactics::SHC141::ReserveState) == 196 ? 1 : -1];
typedef char RaidABI[sizeof(AicTactics::SHC141::RaidState) == 96 ? 1 : -1];
typedef char IncidentABI[sizeof(AicTactics::IncidentState) == 2632 ? 1 : -1];
typedef char DetachedGroupABI[sizeof(AicTactics::SHC141::DetachedGroup) == 48 ? 1 : -1];
typedef char PatrolRouteABI[sizeof(AicTactics::SHC141::PatrolRoute) == 36 ? 1 : -1];
typedef char CombatCensusABI[sizeof(AicTactics::SHC141::PlayerCensus) == 48 ? 1 : -1];
typedef char LordHuntABI[sizeof(AicTactics::SHC141::LordHuntState) == 68 ? 1 : -1];
typedef char LordHuntCensusABI[sizeof(AicTactics::SHC141::LordHuntCensus) == 1544 ? 1 : -1];
typedef char AllyStateABI[sizeof(AicTactics::SHC141::AllyState) == 260 ? 1 : -1];
typedef char AllyBindingsABI[sizeof(AicTactics::SHC141::AllyBindings) == 56 ? 1 : -1];

// The bridge only publishes addresses. These stable Lua C API calls use the
// host's lua.dll; no Lua VM or allocator is embedded in the module.
struct lua_State;
typedef void (__cdecl *CreateTable)(lua_State*, int, int);
typedef void (__cdecl *PushNumber)(lua_State*, double);
typedef void (__cdecl *SetField)(lua_State*, int, const char*);

extern "C" __declspec(dllexport) int __cdecl luaopen_aicTactics(lua_State* state)
{
    HMODULE lua = GetModuleHandleA("lua.dll");
    if (!lua) return 0;
    CreateTable createTable = reinterpret_cast<CreateTable>(GetProcAddress(lua, "lua_createtable"));
    PushNumber pushNumber = reinterpret_cast<PushNumber>(GetProcAddress(lua, "lua_pushnumber"));
    SetField setField = reinterpret_cast<SetField>(GetProcAddress(lua, "lua_setfield"));
    if (!createTable || !pushNumber || !setField) return 0;
    using namespace AicTactics::SHC141;
    const char* names[] = {"configuration", "configurationSize", "observations", "observationSize",
        "configurationLocked", "legacyWallCounts", "recruitOpportunity", "rangedSortie", "meleeSortie",
        "resetDefenseCensus", "countDefenseUnit", "invalidateDefenseCensus", "defenseTypeCounts",
        "defenseCensusTick", "defenseCensusValid", "combatCensus", "combatCensusTick", "combatCensusValid",
        "incidents", "incidentSize", "targetStates", "targetLifecycle", "legacyTargetPolicy",
        "originalUnitDamage", "originalEntityDamage", "originalFireDamage",
        "resetCombatCensus", "countCombatUnit", "completeCombatCensus",
        "observedUnitDamage", "observedEntityDamage", "observedFireDamage",
        "commitOpponent", "selectOpponent", "updateOffensiveArmy", "updateOffensiveRaids",
        "returnFromAttack", "preserveRandomWaveRequirement", "reserves", "reserveSize",
        "isReserveUnit", "recruitWithReserve", "reserveRecruitType", "reserveGroupOwner", "reserveGroupUID",
        "raidStates", "raidStateSize", "raidGroupCensus", "raidUnitPower", "raidStaticDefenses",
        "raidBuildingCensusTick", "raidBuildingCensusValid", "resetRaidBuildingCensus",
        "countRaidBuilding", "completeRaidBuildingCensus", "integrityDigest", "captureIntegrityWithIdentity",
        "observeIntegrityBoundaryWithIdentity", "captureBoundaryIntegrity", "nativeBindings", "nativeBindingsSize",
        "safePlacementFallback", "checkedSiegeTile", "siegePlacementPolicyEnabled",
        "siegeTentCandidateAllowed", "failedSiegeTent", "placeSiegeTentAndRecoverEngineers",
        "engineerRoleFallback", "countableEngineerRole", "siegePaymentFallback",
        "siegePaymentPolicy", "siegeResourceAdmission", "siegePaymentEnabledForGoldOffset",
        "siegeHarassFallback", "siegeHarassMinimumFallback", "siegeHarassPolicy",
        "siegeHarassMinimum", "siegeHarassPlans", "siegeHarassPlanSize",
        "suppressNativeSiegeHarassMove", "siegeCatapultRange",
        "largeSiegeFallback",
        "siegeForceMaximumFallback", "largeSiegePolicy", "siegeForceMaximum",
        "buildLargerSiegeForce", "patrolOrders", "patrolFormation", "patrolSpacing",
        "dispatchAttackPatrols", "patrolOrdersFallback", "patrolFormationFallback",
        "patrolSpacingFallback", "detachedGroups", "detachedGroupSize",
        "lordHuntRetarget", "lordHuntSiege", "lordHuntSwitch", "lordHuntShare", "lordHuntSplit",
        "lordHuntRetargetFallback", "lordHuntSiegeFallback", "lordHuntSwitchFallback",
        "lordHuntShareFallback", "lordHuntSplitFallback", "lordHunts", "lordHuntStateSize",
        "lordHuntCensus", "lordHuntCensusSize", "lordHuntUnits",
        "patrolExtraGroups", "patrolMinSize", "patrolRadius", "siegeScreen", "siegeScreenForward",
        "patrolExtraGroupsFallback", "patrolMinSizeFallback", "patrolRadiusFallback",
        "siegeScreenFallback", "siegeScreenForwardFallback", "patrolRoutes", "patrolRouteSize",
        "screenSiegeDefenceMove", "siegeScreenExtraGroups", "siegeScreenExtraGroupsFallback",
        "allyBindings", "allyBindingsSize", "allySettings", "allyFallback", "allyStates", "allyStateSize",
        "allyGoldRequestAllowed", "allyGoldRequestCommand", "allyGateHeld", "diagnosticsLog",
        "ownerDefenseRows", "ownerDefenseGroups", "ownerRaidTypes"};
    const unsigned int values[] = {
        reinterpret_cast<unsigned int>(configurations), sizeof(CharacterConfiguration),
        reinterpret_cast<unsigned int>(observations), sizeof(RecruitmentObservation),
        reinterpret_cast<unsigned int>(&configurationLocked), reinterpret_cast<unsigned int>(&legacyWallCounts),
        reinterpret_cast<unsigned int>(&recruitOpportunity), reinterpret_cast<unsigned int>(&rangedSortie),
        reinterpret_cast<unsigned int>(&meleeSortie), reinterpret_cast<unsigned int>(&resetDefenseCensus),
        reinterpret_cast<unsigned int>(&countDefenseUnit), reinterpret_cast<unsigned int>(&invalidateDefenseCensus),
        reinterpret_cast<unsigned int>(defenseTypeCounts), reinterpret_cast<unsigned int>(&defenseCensusTick),
        reinterpret_cast<unsigned int>(&defenseCensusValid),
        reinterpret_cast<unsigned int>(combatCensus), reinterpret_cast<unsigned int>(&combatCensusTick),
        reinterpret_cast<unsigned int>(&combatCensusValid), reinterpret_cast<unsigned int>(incidents),
        sizeof(AicTactics::IncidentState), reinterpret_cast<unsigned int>(targetStates),
        reinterpret_cast<unsigned int>(targetLifecycle), reinterpret_cast<unsigned int>(&legacyTargetPolicy),
        reinterpret_cast<unsigned int>(&originalUnitDamage), reinterpret_cast<unsigned int>(&originalEntityDamage),
        reinterpret_cast<unsigned int>(&originalFireDamage), reinterpret_cast<unsigned int>(&resetCombatCensus),
        reinterpret_cast<unsigned int>(&countCombatUnit), reinterpret_cast<unsigned int>(&completeCombatCensus),
        reinterpret_cast<unsigned int>(&observedUnitDamage), reinterpret_cast<unsigned int>(&observedEntityDamage),
        reinterpret_cast<unsigned int>(&observedFireDamage), reinterpret_cast<unsigned int>(&commitOpponent),
        reinterpret_cast<unsigned int>(&selectOpponent), reinterpret_cast<unsigned int>(&updateOffensiveArmy),
        reinterpret_cast<unsigned int>(&updateOffensiveRaids), reinterpret_cast<unsigned int>(&returnFromAttack),
        reinterpret_cast<unsigned int>(&preserveRandomWaveRequirement),
        reinterpret_cast<unsigned int>(reserves), sizeof(ReserveState),
        reinterpret_cast<unsigned int>(&isReserveUnit), reinterpret_cast<unsigned int>(&recruitWithReserve),
        reinterpret_cast<unsigned int>(&reserveRecruitType),
        reinterpret_cast<unsigned int>(reserveGroupOwner), reinterpret_cast<unsigned int>(reserveGroupUID),
        reinterpret_cast<unsigned int>(raidStates), sizeof(RaidState),
        reinterpret_cast<unsigned int>(raidGroupCensus), reinterpret_cast<unsigned int>(raidUnitPower),
        reinterpret_cast<unsigned int>(raidStaticDefenses), reinterpret_cast<unsigned int>(&raidBuildingCensusTick),
        reinterpret_cast<unsigned int>(&raidBuildingCensusValid), reinterpret_cast<unsigned int>(&resetRaidBuildingCensus),
        reinterpret_cast<unsigned int>(&countRaidBuilding), reinterpret_cast<unsigned int>(&completeRaidBuildingCensus),
        reinterpret_cast<unsigned int>(integrityDigest), reinterpret_cast<unsigned int>(&captureIntegrity),
        reinterpret_cast<unsigned int>(&observeIntegrityBoundary), reinterpret_cast<unsigned int>(&captureBoundaryIntegrity),
        reinterpret_cast<unsigned int>(&nativeBindings), sizeof(NativeBindings),
        reinterpret_cast<unsigned int>(&safePlacementFallback), reinterpret_cast<unsigned int>(&checkedSiegeTile),
        reinterpret_cast<unsigned int>(&siegePlacementPolicyEnabled),
        reinterpret_cast<unsigned int>(&siegeTentCandidateAllowed),
        reinterpret_cast<unsigned int>(&failedSiegeTent),
        reinterpret_cast<unsigned int>(&placeSiegeTentAndRecoverEngineers),
        reinterpret_cast<unsigned int>(&engineerRoleFallback),
        reinterpret_cast<unsigned int>(&countableEngineerRole),
        reinterpret_cast<unsigned int>(&siegePaymentFallback),
        reinterpret_cast<unsigned int>(siegePaymentPolicy),
        reinterpret_cast<unsigned int>(&siegeResourceAdmission),
        reinterpret_cast<unsigned int>(&siegePaymentEnabledForGoldOffset),
        reinterpret_cast<unsigned int>(&siegeHarassFallback),
        reinterpret_cast<unsigned int>(&siegeHarassMinimumFallback),
        reinterpret_cast<unsigned int>(siegeHarassPolicy),
        reinterpret_cast<unsigned int>(siegeHarassMinimum),
        reinterpret_cast<unsigned int>(siegeHarassPlans), sizeof(SiegeHarassPlan),
        reinterpret_cast<unsigned int>(&suppressNativeSiegeHarassMove),
        reinterpret_cast<unsigned int>(&siegeCatapultRange),
        reinterpret_cast<unsigned int>(&largeSiegeFallback),
        reinterpret_cast<unsigned int>(&siegeForceMaximumFallback),
        reinterpret_cast<unsigned int>(largeSiegePolicy),
        reinterpret_cast<unsigned int>(siegeForceMaximum),
        reinterpret_cast<unsigned int>(&buildLargerSiegeForce),
        reinterpret_cast<unsigned int>(patrolOrders),
        reinterpret_cast<unsigned int>(patrolFormation),
        reinterpret_cast<unsigned int>(patrolSpacing),
        reinterpret_cast<unsigned int>(&dispatchAttackPatrols),
        reinterpret_cast<unsigned int>(&patrolOrdersFallback),
        reinterpret_cast<unsigned int>(&patrolFormationFallback),
        reinterpret_cast<unsigned int>(&patrolSpacingFallback),
        reinterpret_cast<unsigned int>(detachedGroups), sizeof(DetachedGroup),
        reinterpret_cast<unsigned int>(lordHuntRetarget),
        reinterpret_cast<unsigned int>(lordHuntSiege),
        reinterpret_cast<unsigned int>(lordHuntSwitch),
        reinterpret_cast<unsigned int>(lordHuntShare),
        reinterpret_cast<unsigned int>(lordHuntSplit),
        reinterpret_cast<unsigned int>(&lordHuntRetargetFallback),
        reinterpret_cast<unsigned int>(&lordHuntSiegeFallback),
        reinterpret_cast<unsigned int>(&lordHuntSwitchFallback),
        reinterpret_cast<unsigned int>(&lordHuntShareFallback),
        reinterpret_cast<unsigned int>(&lordHuntSplitFallback),
        reinterpret_cast<unsigned int>(lordHunts), sizeof(LordHuntState),
        reinterpret_cast<unsigned int>(lordHuntCensus), sizeof(LordHuntCensus),
        reinterpret_cast<unsigned int>(lordHuntUnits),
        reinterpret_cast<unsigned int>(patrolExtraGroups),
        reinterpret_cast<unsigned int>(patrolMinSize),
        reinterpret_cast<unsigned int>(patrolRadius),
        reinterpret_cast<unsigned int>(siegeScreen),
        reinterpret_cast<unsigned int>(siegeScreenForward),
        reinterpret_cast<unsigned int>(&patrolExtraGroupsFallback),
        reinterpret_cast<unsigned int>(&patrolMinSizeFallback),
        reinterpret_cast<unsigned int>(&patrolRadiusFallback),
        reinterpret_cast<unsigned int>(&siegeScreenFallback),
        reinterpret_cast<unsigned int>(&siegeScreenForwardFallback),
        reinterpret_cast<unsigned int>(patrolRoutes), sizeof(PatrolRoute),
        reinterpret_cast<unsigned int>(&screenSiegeDefenceMove),
        reinterpret_cast<unsigned int>(siegeScreenExtraGroups),
        reinterpret_cast<unsigned int>(&siegeScreenExtraGroupsFallback),
        reinterpret_cast<unsigned int>(&allyBindings), sizeof(AllyBindings),
        reinterpret_cast<unsigned int>(allySettings), reinterpret_cast<unsigned int>(allyFallback),
        reinterpret_cast<unsigned int>(allyStates), sizeof(AllyState),
        reinterpret_cast<unsigned int>(&allyGoldRequestAllowed),
        reinterpret_cast<unsigned int>(&allyGoldRequestCommand),
        reinterpret_cast<unsigned int>(&allyGateHeld), reinterpret_cast<unsigned int>(&diagnosticsLog),
        reinterpret_cast<unsigned int>(ownerDefenseRows), reinterpret_cast<unsigned int>(ownerDefenseGroups),
        reinterpret_cast<unsigned int>(ownerRaidTypes)};
    typedef char ExportCount[sizeof(names) / sizeof(names[0]) == sizeof(values) / sizeof(values[0]) ? 1 : -1];
    createTable(state, 0, sizeof(names) / sizeof(names[0]));
    for (unsigned int i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        pushNumber(state, values[i]);
        setField(state, -2, names[i]);
    }
    return 1;
}
