local context = require('native-context')
local M = {}

-- The native checker is called once per footprint tile. Keep its ownership of
-- terrain/access validation and guard the occupied-engine case at that call.
function M.resolve(game)
  local callSite = context.find('siege footprint tile check',
    '8B 4C 24 10 8B 5C 24 40 6A 00 51 53 50 8B CE E8 ? ? ? ? 85 C0 B9 01 00 00 00 74 06')
  local original = context.call(callSite + 15, 'siege footprint tile check')
  context.verify(original, 'tile placement owner',
    '83 EC 0C 8B 54 24 18 53 55 56 8B F1 57 8B 7C 24 20 8B 9C BE ? ? ? ?')
  local occupancy = context.find('siege footprint unit occupancy',
    '0F BF 8C 5E ? ? ? ? 85 C9 74 26 0F BF 44 24 28 83 C0 D3 83 F8 21 77 0E '..
    '0F B6 90 ? ? ? ? FF 24 95 ? ? ? ? 51 B9 ? ? ? ? E8 ? ? ? ?')
  local offset = core.readInteger(occupancy + 4)
  assert(offset == 0x23D7E0 and game.unitCapacity >= 2500,
    'AIC Tactics: unsupported native siege footprint layout')
  local noSpot = context.find('siege harassment no-site return',
    '85 C0 0F 84 ? ? ? ? A1 ? ? ? ? 8B 54 24 14 6A 0F 6A 03 55')
  local failed = context.find('siege harassment placement failure',
    '83 3D ? ? ? ? 00 0F 85 ? ? ? ? 8B 0D ? ? ? ? 8B C1 69 C0 2C 03 00 00')
  local noSpotBranch, failedBranch = noSpot + 2, failed + 7
  local exit = noSpotBranch + 6 + core.readInteger(noSpotBranch + 2)
  assert(failedBranch + 6 + core.readInteger(failedBranch + 2) == exit,
    'AIC Tactics: unsupported siege harassment failure flow')
  local timeout = context.find('siege harassment retry timer',
    '8B BE ? ? ? ? 85 FF 74 0F 83 C7 FF 89 BE ? ? ? ?')
  assert(core.readInteger(timeout + 2) == game.players + 0x3918
      and core.readInteger(timeout + 15) == game.players + 0x3918,
    'AIC Tactics: unsupported native siege retry timer')
  return {siegePlacementCall = callSite + 15, originalSiegeTileCheck = original,
    siegeTileOccupancyOffset = offset, siegeNoSpotBranch = noSpotBranch,
    siegeFailedBranch = failedBranch, siegeFailureExit = exit}
end

return M
