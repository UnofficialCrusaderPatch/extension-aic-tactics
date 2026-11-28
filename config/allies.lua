-- Per-character ally help settings (src/allies.cpp). An absent field uses the
-- module setting from Customizations; an explicit value changes it for this
-- character only. Stored per AI as 0 (use the module setting) or the value + 1.
local M = {fields = {'AICTactics_AllyRelief', 'AICTactics_AllyGoods', 'AICTactics_AllyGoldKeep',
  'AICTactics_AllyRecruitGoldBuffer', 'AICTactics_AllyGoldSharePercent',
  'AICTactics_AllyMonthlyGivePercent', 'AICTactics_AllyArmySharePercent',
  'AICTactics_AllyReliefGates', 'AICTactics_AllyReliefKeep', 'AICTactics_AllyReliefMinMonths'}}

-- Field, module option, kind, minimum, maximum, default; in storage order.
local settings = {
  {'AICTactics_AllyRelief', 'allyRelief', 'boolean', nil, nil, false},
  {'AICTactics_AllyGoods', 'allyGoods', 'boolean', nil, nil, false},
  {'AICTactics_AllyGoldKeep', 'allyGoldKeep', 'integer', 0, 100000, 1000},
  {'AICTactics_AllyRecruitGoldBuffer', 'allyRecruitGoldBuffer', 'integer', 0, 5000, 300},
  {'AICTactics_AllyGoldSharePercent', 'allyGoldSharePercent', 'integer', 0, 100, 25},
  {'AICTactics_AllyMonthlyGivePercent', 'allyMonthlyGivePercent', 'integer', 0, 100, 30},
  {'AICTactics_AllyArmySharePercent', 'allyArmySharePercent', 'integer', 0, 100, 75},
  {'AICTactics_AllyReliefGates', 'allyReliefGates', 'boolean', nil, nil, false},
  {'AICTactics_AllyReliefKeep', 'allyReliefKeep', 'boolean', nil, nil, false},
  {'AICTactics_AllyReliefMinMonths', 'allyReliefMinMonths', 'integer', 0, 12, 0},
}
M.settings = settings

local function validate(value, setting, name, check)
  if setting[3] == 'boolean' then
    assert(type(value) == 'boolean', name .. ' must be boolean')
    return value and 1 or 0
  end
  assert(type(value) == 'number', name .. ' must be a number')
  check(value == math.floor(value) and value >= setting[4]
      and value <= setting[5],
    string.format('%s must be an integer from %d to %d', name, setting[4], setting[5]))
  return value
end

function M.defaults() return {} end

function M.prepare(previous, spec, resetting, check)
  check = check or assert
  local result, words = {}, {}
  for index, setting in ipairs(settings) do
    local field = setting[1]
    local value
    if not resetting then
      if previous then value = previous[field] end
      if spec[field] ~= nil then value = spec[field] end
    end
    result[field] = value
    words[index] = value == nil and 0 or validate(value, setting, field, check) + 1
  end
  return result, words
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

-- Module settings: the defaults for every AI, in storage order.
function M.fallbacks(config)
  local option = require('config.settings')
  local words = {}
  for index, setting in ipairs(settings) do
    local value = option(config, setting[2], setting[6], nil, setting[4], setting[5])
    if setting[3] == 'boolean' then value = value and 1 or 0 end
    words[index] = value
  end
  return words
end

return M
