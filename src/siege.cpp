#include "aic_tactics/runtime.hpp"

namespace AicTactics {
namespace SHC141 {

int safePlacementFallback = 1;

namespace {
typedef int (__thiscall *TileCheck)(void*, int, int, int, int);

bool siegeConstruction(int command)
{
    return command == 94 || (command >= 190 && command <= 194)
        || command == 210 || command == 211 || command == 358;
}

} // namespace

int __cdecl siegePlacementPolicyEnabled(int player)
{
    if (player < 1 || player > 8) return false;
    const unsigned int playerAddress = nativeBindings.players + player * 0x39F4;
    const int character = *reinterpret_cast<const int*>(playerAddress + 0x2300);
    if (character < 2 || character > 17) return false;
    const int choice = configurations[character - 1].safeSiegePlacement;
    return choice == 1 || (choice == 0 && safePlacementFallback != 0);
}

// Replaces only the native footprint tile check call. All terrain, ownership,
// access and placement decisions still run through the original checker.
int __fastcall checkedSiegeTile(void* tileMap, void*, int tile, int player, int command, int flags)
{
    if (siegeConstruction(command) && tile >= 0 && tile < 160000
        && siegePlacementPolicyEnabled(player)) {
        const short unit = *reinterpret_cast<const short*>(
            reinterpret_cast<const unsigned char*>(tileMap)
                + nativeBindings.siegeTileOccupancyOffset + tile * 2);
        if (unit > 0 && unit < static_cast<int>(nativeBindings.unitCapacity)) {
            const unsigned int record = nativeBindings.unitRecords + unit * 0x490;
            const int owner = *reinterpret_cast<const short*>(record + 0x96);
            const int dying = *reinterpret_cast<const short*>(record + 0x2A0);
            // The native placement path can delete any friendly unit on the
            // footprint, including a waiting engineer or an existing engine.
            if (owner == player && dying == 0) return 1;
        }
    }
    return reinterpret_cast<TileCheck>(nativeBindings.originalSiegeTileCheck)(
        tileMap, tile, player, command, flags);
}

} // namespace SHC141
} // namespace AicTactics
