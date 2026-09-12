local M = {}
local names = {'gameTick','rngState','rngValue','rngNext','initialDefenseTicks','aicRecords',
    'units','unitRecords','unitCapacity','tribes','tribeStride','tribeMemberWords',
    'tribeStance','tribeTargetBuilding','tribeTargetBuildingUID','buildings','buildingCapacity',
    'players','createTribe','addUnitToTribe','tribePath','entities','entityCapacity','teams',
    'assignMoatDigger','wallDefense','patrolDefense','assignRaider','assignAttacker',
    'findSortieGroup','findAttackGroup','returnTribe','removeUnitFromTribe','relayRaidOrder','mapRows','attackGroupSlots','recruitUpdate','rangedSortieNative','meleeSortieNative',
    'recruitEuropean','recruitNonEuropean','scenarioMode','scenarioCustom','scenarioMission','moat','moatVacancies','findRecruitmentBuilding','attackRecruitType','raidMaximum',
    'defenseTypes','specialDefenders','defenseSlots','raidTypes','equipmentRecipes',
    'selectAttackTarget','computeNervousness','updateAIPlayerState','returnAttack','hasNoTroopsOrAllDiggers','updateRaids','combatValue','troopValues','marketPrice','gameState'}
-- Framework loads every module before enabling Legacy's native patches.
-- Resolve identifying contexts here; hook preflights still check current bytes
-- when a feature is activated after all dependencies have enabled.
function M.resolve()
  local game = require('config.grace').resolveNative()
  assert(type(modules.aicloader.getNativeAICLayout) == 'function',
    'AIC Tactics requires AIC Loader 1.1.4 native storage metadata')
  local layout = modules.aicloader:getNativeAICLayout()
  assert(layout.version == 1 and layout.characters == 16 and layout.stride == 676
      and type(layout.address) == 'number' and layout.address > 0,
    'AIC Tactics: unsupported AIC Loader native layout')
  game.aicRecords = layout.address
  for key, value in pairs(require('native-layout').resolve()) do game[key] = value end
  for key, value in pairs(require('native-group-actions').resolve(game)) do game[key] = value end
  for key, value in pairs(require('native-recruitment').resolve(game)) do game[key] = value end
  for key, value in pairs(require('native-aic-queries').resolve(game)) do game[key] = value end
  for key, value in pairs(require('native-combat-bindings').resolve(game)) do game[key] = value end
  return game
end

function M.initialize(native, game)
  assert(native.nativeBindingsSize == #names * 4, 'AIC Tactics: incompatible native binding ABI')
  game = game or M.resolve()
  for index, name in ipairs(names) do
    core.writeInteger(native.nativeBindings + (index - 1) * 4, game[name])
  end
  native.game = game
end
return M
