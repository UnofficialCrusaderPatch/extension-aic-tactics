-- Per-character lord hunt settings. An absent field uses the module setting
-- from Customizations; an explicit value changes it for this character only.
-- Stored per AI as 0 (use the module setting) or the chosen value + 1.
local M = {fields = {'AICTactics_LordHuntRetarget', 'AICTactics_LordHuntSiege',
  'AICTactics_LordHuntSwitchPercent', 'AICTactics_LordHuntShare', 'AICTactics_LordHuntSplitShare'}}
local sieges = {HidingCastle = 0, HomeCastle = 1}
local percents = {'AICTactics_LordHuntSwitchPercent', 'AICTactics_LordHuntShare', 'AICTactics_LordHuntSplitShare'}

M.sieges = sieges

local function percent(value)
  return type(value) == 'number' and value == math.floor(value) and value >= 0 and value <= 100
end

function M.defaults() return {} end

function M.prepare(previous, spec, resetting, check)
  check = check or assert
  local result = {}
  if not resetting then
    for _, field in ipairs(M.fields) do
      if previous and previous[field] ~= nil then result[field] = previous[field] end
      if spec[field] ~= nil then result[field] = spec[field] end
    end
  end
  local retarget, siege = result.AICTactics_LordHuntRetarget, result.AICTactics_LordHuntSiege
  assert(retarget == nil or type(retarget) == 'boolean', 'AICTactics_LordHuntRetarget must be boolean')
  assert(siege == nil or type(siege) == 'string', 'AICTactics_LordHuntSiege must be a string')
  check(siege == nil or sieges[siege] ~= nil, 'AICTactics_LordHuntSiege must be HidingCastle or HomeCastle')
  for _, field in ipairs(percents) do
    local value = result[field]
    assert(value == nil or type(value) == 'number', field .. ' must be a number')
    check(value == nil or percent(value), field .. ' must be an integer from 0 to 100')
  end
  local function stored(value) return value == nil and 0 or value + 1 end
  return result, {retarget == nil and 0 or retarget and 2 or 1, siege == nil and 0 or sieges[siege] + 1,
    stored(result.AICTactics_LordHuntSwitchPercent), stored(result.AICTactics_LordHuntShare),
    stored(result.AICTactics_LordHuntSplitShare)}
end

function M.active(candidate)
  for _, field in ipairs(M.fields) do if candidate[field] ~= nil then return true end end
  return false
end

function M.copy(candidate)
  local result = {}
  for _, field in ipairs(M.fields) do result[field] = candidate[field] end
  return result
end

-- Whether a compiled per-AI value turns the feature on for that AI even when
-- the module setting is off.
function M.enables(compiled)
  return compiled[1] == 2 or compiled[4] > 1
end

-- Module settings: the defaults for every AI.
function M.fallbacks(config)
  local option = require('config.settings')
  local retarget = option(config, 'lordHuntRetarget', false)
  local siege = option(config, 'lordHuntSiege', 'HidingCastle', sieges)
  local switch = option(config, 'lordHuntSwitchPercent', 80, nil, 0, 100)
  local share = option(config, 'lordHuntShare', 0, nil, 0, 100)
  local split = option(config, 'lordHuntSplitShare', 50, nil, 0, 100)
  return retarget and 1 or 0, sieges[siege], switch, share, split
end

return M
