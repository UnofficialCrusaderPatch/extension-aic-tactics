local context = require('native-context')
local M = {}

-- AICState::canNavigateUnitsFromTileToTargetTile (native-layout's tribe path
-- owner): area of the leader's tile and of the target tile from the area map,
-- then the path finder's area routine (player, area, area, mode 0).
local tribePathSignature = '8B 44 24 04 69 C0 ? ? ? ? 8B 88 ? ? ? ? 0F BF 80 ? ? ? ? 69 C0 90 04 00 00 '..
  '8B 90 ? ? ? ? 0F BF 04 55 ? ? ? ? 8B 54 24 08 0F BF 14 55 ? ? ? ? 6A 00 52 50 51 B9 ? ? ? ? E8 ? ? ? ? '..
  'F7 D8 1B C0 F7 D8 C2 08 00'
-- GameStateStructures::canUnitsNavigateFromKeepToKeep, the climbing-link half
-- of the AI's breach test: keep fields, area map, then the same area routine
-- with mode 2. Its mode-0 twin differs only in the pushed mode.
local keepSignature = '56 8B 74 24 08 8B C6 69 C0 F4 39 00 00 03 C1 83 B8 ? ? ? ? 00 75 09 B8 01 00 00 00 5E C2 08 00 '..
  '8B 54 24 0C 69 D2 F4 39 00 00 03 CA 83 B9 ? ? ? ? 00 74 E2 8B 89 ? ? ? ? 8B 90 ? ? ? ? '..
  '0F B7 04 4D ? ? ? ? 66 85 C0 75 06 33 C0 5E C2 08 00 0F B7 0C 55 ? ? ? ? 66 85 C9 74 ED '..
  '66 3B C8 74 B1 0F BF C9 6A 02 51 0F BF D0 52 56 B9 ? ? ? ? E8'
-- A native wall-owner check: logic flags of the tile, then the wall owner
-- byte decoded as (byte & 7) + 1 and compared with a player.
local wallOwnerSignature = 'A1 ? ? ? ? 84 1C 85 ? ? ? ? 0F 84 ? ? ? ? 0F B6 90 ? ? ? ? A1 ? ? ? ? '..
  '83 E2 07 03 D7 3B D0 0F 85 ? ? ? ? 69 C0 F4 39 00 00'
-- A native test of the wall-or-gatehouse logic flag (0x100).
local wallFlagSignature = '85 C0 0F 84 ? ? ? ? F7 04 9D ? ? ? ? 00 01 00 00 0F 84'
-- chooseRandomBuildingIDFromPlayersArrayOfBuildings: a player's targetable
-- building count (+0x3684), its short list (+0x35BC) and the building type.
local targetableSignature = '8B 4C 24 04 69 C9 F4 39 00 00 53 0F B6 1D ? ? ? ? 56 8B B1 ? ? ? ? 33 C0 83 E3 07 '..
  '85 F6 8D 50 2E 7E ? 55 57 8D A9 ? ? ? ? 89 74 24 14 0F BF 75 00 85 F6 74 ? 33 C9 85 D2 7E ? '..
  '8B FE 69 FF 2C 03 00 00 0F BF BF ? ? ? ?'
-- TileMapState layer offsets (OpenSHC Map/TileMapState.hpp).
local logicLayer, wallOwnerLayer, areaLayer, rowTable = 0x165160, 0x2C6E50, 0x363ED0, 0x8A40F8

function M.resolve(game)
  local read = core.readInteger
  local path = game.tribePath
  context.verify(path, 'tribe path', tribePathSignature)
  local area, finder = read(path + 39), read(path + 61)
  local route = context.call(path + 65, 'area routine')
  assert(read(path + 51) == area and area > 0 and finder > 0,
    'AIC Tactics: inconsistent native tribe path area query')

  local keep = context.find('keep breach test', keepSignature)
  local hasKeep, keepTile = read(keep + 17), read(keep + 56)
  assert(read(keep + 47) == hasKeep and read(keep + 62) == keepTile
      and keepTile - hasKeep == 0x1E0 - 0x94 and game.gameState + hasKeep == game.players + 0x94
      and read(keep + 70) == area and read(keep + 89) == area and read(keep + 115) == finder
      and context.call(keep + 119, 'keep area routine') == route,
    'AIC Tactics: inconsistent native keep breach test')

  local tileMap = game.siegeTileMap
  local owner = context.find('wall owner check', wallOwnerSignature)
  local flag = context.find('wall flag test', wallFlagSignature)
  local logic, wallOwner = read(owner + 8), read(owner + 21)
  assert(area == tileMap + areaLayer and game.mapRows == tileMap + rowTable
      and logic == tileMap + logicLayer and read(flag + 11) == logic
      and wallOwner == tileMap + wallOwnerLayer,
    'AIC Tactics: inconsistent native tile map layers')

  local targetable = context.find('targetable buildings', targetableSignature)
  assert(read(targetable + 21) == game.players + 0x3684 and read(targetable + 41) == game.players + 0x35BC
      and read(targetable + 74) == game.buildings + 0x14 + 0xD2,
    'AIC Tactics: inconsistent native targetable building list')

  return {areaMap = area, areaRoute = route, areaPathFinder = finder,
    tileLogic = logic, tileWallOwner = wallOwner, keepBreachTest = keep, wallOwnerCheck = owner}
end

return M
