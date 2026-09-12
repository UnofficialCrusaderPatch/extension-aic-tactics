#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "aic_tactics/runtime.hpp"

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
        "configurationLocked", "legacyWallCounts", "recruitOpportunity", "rangedSortie", "meleeSortie"};
    const unsigned int values[] = {
        reinterpret_cast<unsigned int>(configurations), sizeof(CharacterConfiguration),
        reinterpret_cast<unsigned int>(observations), sizeof(RecruitmentObservation),
        reinterpret_cast<unsigned int>(&configurationLocked), reinterpret_cast<unsigned int>(&legacyWallCounts),
        reinterpret_cast<unsigned int>(&recruitOpportunity), reinterpret_cast<unsigned int>(&rangedSortie),
        reinterpret_cast<unsigned int>(&meleeSortie)};
    createTable(state, 0, sizeof(names) / sizeof(names[0]));
    for (unsigned int i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        pushNumber(state, values[i]);
        setField(state, -2, names[i]);
    }
    return 1;
}
