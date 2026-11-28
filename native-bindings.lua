local M = {}
local allyNames = require('native-ally-bindings').names
local names = {'gameTick','rngState','rngValue','rngNext','initialDefenseTicks','aicRecords',
    'units','unitRecords','unitCapacity','tribes','tribeStride','tribeMemberWords',
    'tribeStance','tribeTargetBuilding','tribeTargetBuildingUID','buildings','buildingCapacity',
    'players','createTribe','addUnitToTribe','tribePath','entities','entityCapacity','teams',
    'assignMoatDigger','wallDefense','patrolDefense','assignRaider','assignAttacker',
    'findSortieGroup','findAttackGroup','returnTribe','removeUnitFromTribe','relayRaidOrder','mapRows','attackGroupSlots','recruitUpdate','rangedSortieNative','meleeSortieNative',
    'recruitEuropean','recruitNonEuropean','scenarioMode','scenarioCustom','scenarioMission','moat','moatVacancies','findRecruitmentBuilding','attackRecruitType','raidMaximum',
    'defenseTypes','specialDefenders','defenseSlots','raidTypes','equipmentRecipes',
    'selectAttackTarget','computeNervousness','updateAIPlayerState','returnAttack','hasNoTroopsOrAllDiggers','updateRaids','combatValue','troopValues','marketPrice','gameState',
    'originalSiegeTileCheck','siegeTileOccupancyOffset','siegeTileMap','siegeTentPointX',
    'siegeTribeIndexOffset','siegePlacementFail','siegeResourceCheck','siegeGroupMove',
    'clearGroupMoveFlags','siegePlaceTent','popUnitFromTribe','siegeAssaultBatch','siegeUnitCount',
    'siegeGetUnitForIndex','siegeFireRange',
    'attackPatrolDispatch','attackPatrolDestination','troopValueState','playableTiles',
    'areaMap','areaRoute','areaPathFinder','tileLogic','tileWallOwner',
    'distanceLayers','distanceLayerIndex','tribeRallyPoints','tribeRallyCount'}
local function ownerLayout()
  local layout = modules.aicloader:getNativeAICLayout()
  assert(type(layout) == 'table' and layout.version == 1 and layout.characters == 16 and layout.stride == 676
      and type(layout.address) == 'number' and layout.address > 0,
    'AIC Tactics: unsupported AIC Loader native layout')
  return {aicRecords = layout.address}
end

local function bind(game, key, resolver)
  if game.resolved[key] then return end
  local ok, result = pcall(resolver, game)
  game.resolved[key] = ok and true or tostring(result)
  if ok then
    for name, value in pairs(result) do game[name] = value end
  end
end

function M.resolve()
  return {resolved = {}}
end

local owners = {
  AICLayout = ownerLayout,
  Interval = function()
    return {recruitmentInterval = require('native-recruitment').resolveInterval()}
  end,
  Target = function()
    return {targetSelection = require('native-combat-bindings').resolveTargetSelection()}
  end,
  Grace = {'config.grace', 'resolveNative'},
  Layout = {'native-layout'},
  Army = {'native-layout', 'resolveArmy'},
  Projectiles = {'native-layout', 'resolveProjectiles'},
  Groups = {'native-group-actions'},
  Recruitment = {'native-recruitment'},
  Queries = {'native-aic-queries'},
  Market = {'native-combat-bindings', 'resolveMarket'},
  Combat = {'native-combat-bindings'},
  Siege = {'native-siege-bindings'},
  GroupMove = {'native-siege-bindings', 'resolveGroupMove'},
  Engineer = {'native-role-bindings'},
  Patrol = {'native-patrol-bindings'},
  Hunt = {'native-lord-hunt-bindings'},
  Allies = {'native-ally-bindings'},
  Keep = {'native-ally-bindings', 'resolveKeep'},
  Gate = {'native-gate-bindings'},
}

function M.requireBindings(game, keys)
  for _, key in ipairs(keys) do
    local owner = owners[key]
    local resolver = type(owner) == 'table' and require(owner[1])[owner[2] or 'resolve'] or owner
    bind(game, key, resolver)
    assert(game.resolved[key] == true, game.resolved[key])
  end
end

function M.initialize(native, game)
  assert(native.nativeBindingsSize == #names * 4, 'AIC Tactics: incompatible native binding ABI')
  assert(native.allyBindingsSize == #allyNames * 4, 'AIC Tactics: incompatible ally binding ABI')
  native.game = game
end

function M.publish(native)
  local game = native.game
  for index, name in ipairs(names) do
    core.writeInteger(native.nativeBindings + (index - 1) * 4, game[name] or 0)
  end
  for index, name in ipairs(allyNames) do
    core.writeInteger(native.allyBindings + (index - 1) * 4, game[name] or 0)
  end
end
return M
