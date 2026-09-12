#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "aic_tactics/runtime.hpp"

typedef char ConfigurationABI[sizeof(AicTactics::SHC141::CharacterConfiguration) == 344 ? 1 : -1];
typedef char ReserveABI[sizeof(AicTactics::SHC141::ReserveState) == 196 ? 1 : -1];
typedef char RaidABI[sizeof(AicTactics::SHC141::RaidState) == 96 ? 1 : -1];
typedef char IncidentABI[sizeof(AicTactics::IncidentState) == 2632 ? 1 : -1];
typedef char CombatCensusABI[sizeof(AicTactics::SHC141::PlayerCensus) == 48 ? 1 : -1];

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
        "countRaidBuilding", "completeRaidBuildingCensus", "integrityDigest", "captureIntegrity",
        "observeIntegrityBoundary", "captureBoundaryIntegrity"};
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
        reinterpret_cast<unsigned int>(&observeIntegrityBoundary), reinterpret_cast<unsigned int>(&captureBoundaryIntegrity)};
    createTable(state, 0, sizeof(names) / sizeof(names[0]));
    for (unsigned int i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        pushNumber(state, values[i]);
        setField(state, -2, names[i]);
    }
    return 1;
}
