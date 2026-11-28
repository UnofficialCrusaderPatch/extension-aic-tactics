-- Per-character attack patrol and siege-defence settings. An absent field
-- uses the module setting from Customizations; an explicit value switches the
-- feature for this character only. Stored per AI as 0 (use the module setting)
-- or the chosen value + 1, like the siege fields; PatrolSpacing stores the
-- value itself.
local M = {fields = {'AICTactics_PatrolOrders', 'AICTactics_PatrolFormation', 'AICTactics_PatrolSpacing',
  'AICTactics_PatrolExtraGroups', 'AICTactics_PatrolMinSize', 'AICTactics_PatrolRadius',
  'AICTactics_SiegeScreen', 'AICTactics_SiegeScreenForward', 'AICTactics_SiegeScreenExtraGroups',
  'AICTactics_SiegeTowerCrew'}}
local orders = {Native = 0, AllGroups = 1}
local formations = {Native = 0, Wing = 1, Mirror = 2, Routes = 3}
local screens = {Native = 0, EngineApproach = 1}
-- Field, module setting, lowest, highest, default.
local numbers = {
  {'AICTactics_PatrolExtraGroups', 'patrolExtraGroups', 0, 6, 0},
  {'AICTactics_PatrolMinSize', 'patrolMinSize', 1, 50, 4},
  {'AICTactics_PatrolRadius', 'patrolRadius', 4, 100, 12},
  {'AICTactics_SiegeScreenForward', 'siegeScreenForward', 0, 10, 4},
  {'AICTactics_SiegeScreenExtraGroups', 'siegeScreenExtraGroups', 0, 6, 0},
}

M.orders, M.formations, M.screens = orders, formations, screens
-- The native per-AI arrays (int[17]) the compiled values go to; each has a
-- module setting word named <array>Fallback.
M.arrays = {'patrolOrders', 'patrolFormation', 'patrolSpacing', 'patrolExtraGroups', 'patrolMinSize',
  'patrolRadius', 'siegeScreen', 'siegeScreenForward', 'siegeScreenExtraGroups'}

local function integer(value, low, high)
  return type(value) == 'number' and value == math.floor(value) and value >= low and value <= high
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
  local order, formation, spacing = result.AICTactics_PatrolOrders, result.AICTactics_PatrolFormation,
    result.AICTactics_PatrolSpacing
  local screen = result.AICTactics_SiegeScreen
  assert(order == nil or type(order) == 'string', 'AICTactics_PatrolOrders must be a string')
  check(order == nil or orders[order] ~= nil, 'AICTactics_PatrolOrders must be Native or AllGroups')
  assert(formation == nil or type(formation) == 'string', 'AICTactics_PatrolFormation must be a string')
  check(formation == nil or formations[formation] ~= nil,
    'AICTactics_PatrolFormation must be Native, Wing, Mirror or Routes')
  assert(spacing == nil or type(spacing) == 'number', 'AICTactics_PatrolSpacing must be a number')
  check(spacing == nil or integer(spacing, 4, 40), 'AICTactics_PatrolSpacing must be an integer from 4 to 40')
  assert(screen == nil or type(screen) == 'string', 'AICTactics_SiegeScreen must be a string')
  check(screen == nil or screens[screen] ~= nil, 'AICTactics_SiegeScreen must be Native or EngineApproach')
  local crew = result.AICTactics_SiegeTowerCrew
  assert(crew == nil or type(crew) == 'boolean', 'AICTactics_SiegeTowerCrew must be a boolean')
  local stored = {}
  for _, number in ipairs(numbers) do
    local value = result[number[1]]
    assert(value == nil or type(value) == 'number', number[1] .. ' must be a number')
    check(value == nil or integer(value, number[3], number[4]),
      string.format('%s must be an integer from %d to %d', number[1], number[3], number[4]))
    stored[number[1]] = value == nil and 0 or value + 1
  end
  -- In the order of M.arrays.
  return result, {order == nil and 0 or orders[order] + 1, formation == nil and 0 or formations[formation] + 1,
    spacing or 0, stored.AICTactics_PatrolExtraGroups, stored.AICTactics_PatrolMinSize,
    stored.AICTactics_PatrolRadius, screen == nil and 0 or screens[screen] + 1,
    stored.AICTactics_SiegeScreenForward, stored.AICTactics_SiegeScreenExtraGroups, crew == nil and 0 or crew and 2 or 1}
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

-- Module settings: the defaults for every AI, in the order of
-- native.configureAttackPatrols.
function M.fallbacks(config)
  local option = require('config.settings')
  local order = option(config, 'patrolOrders', 'Native', orders)
  local formation = option(config, 'patrolFormation', 'Native', formations)
  local spacing = option(config, 'patrolSpacing', 12, nil, 4, 40)
  local screen = option(config, 'siegeScreen', 'Native', screens)
  local values = {}
  for _, number in ipairs(numbers) do
    values[#values + 1] = option(config, number[2], number[5], nil, number[3], number[4])
  end
  return orders[order], formations[formation], spacing, values[1], values[2], values[3], screens[screen], values[4],
    values[5]
end

return M
