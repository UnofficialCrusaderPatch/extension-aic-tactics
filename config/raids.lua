local M = {fields = {'RaidTargetPolicy','RaidGroupCount','RaidMinGroupSize','RaidFocus','RaidRiskTolerance','RaidEnemyScope'}}
local policies = {Native=0, NearestReachable=1, Opportunistic=2, RandomNearby=3}
local focuses = {Any=0, Food=1, Industry=2, HighValue=3}
local risks = {Low=0, Medium=1, High=2}
local scopes = {PrimeTarget=0, AnyEnemy=1}
function M.defaults()
  return {RaidTargetPolicy='Native', RaidGroupCount=1, RaidMinGroupSize=4,
    RaidFocus='Any', RaidRiskTolerance='Medium', RaidEnemyScope='PrimeTarget'}
end
local function focus(value)
  if type(value) ~= 'table' then
    assert(focuses[value] ~= nil, 'RaidFocus must be Any, Food, Industry, HighValue or a percentage record')
    return value, focuses[value]
  end
  assert(getmetatable(value) == nil, 'RaidFocus must be a plain record')
  local result, total = {}, 0
  for key in pairs(value) do
    assert(key == 'Food' or key == 'Industry' or key == 'HighValue',
      'Unknown RaidFocus category: ' .. tostring(key))
  end
  for _, key in ipairs({'Food','Industry','HighValue'}) do
    local amount = value[key] or 0
    assert(type(amount) == 'number' and amount == math.floor(amount)
      and amount >= 0 and amount <= 100, 'RaidFocus.' .. key .. ' must be an integer from 0 to 100')
    result[key] = amount
    total = total + amount
  end
  assert(total <= 100, 'RaidFocus percentages must total at most 100; the remainder is Any')
  return result, 0x1000000 + result.Food + result.Industry * 128 + result.HighValue * 16384
end
function M.prepare(previous, spec, resetting)
  local result = M.defaults()
  for _, field in ipairs(M.fields) do
    if not resetting then
      if previous and previous[field] ~= nil then result[field] = previous[field] end
      if spec[field] ~= nil then result[field] = spec[field] end
    end
  end
  local function integer(value, maximum, name)
    assert(type(value) == 'number' and value == math.floor(value) and value >= 1 and value <= maximum,
      name .. ' must be an integer from 1 to ' .. maximum)
  end
  assert(policies[result.RaidTargetPolicy] ~= nil,
    'RaidTargetPolicy must be Native, NearestReachable, Opportunistic or RandomNearby')
  local authoredFocus, compiledFocus = focus(result.RaidFocus)
  result.RaidFocus = authoredFocus
  assert(risks[result.RaidRiskTolerance] ~= nil, 'RaidRiskTolerance must be Low, Medium or High')
  assert(scopes[result.RaidEnemyScope] ~= nil, 'RaidEnemyScope must be PrimeTarget or AnyEnemy')
  integer(result.RaidGroupCount, 4, 'RaidGroupCount')
  integer(result.RaidMinGroupSize, 256, 'RaidMinGroupSize')
  if result.RaidTargetPolicy == 'Native' and not resetting then
    for index = 2, #M.fields do assert(spec[M.fields[index]] == nil, M.fields[index] .. ' requires a new RaidTargetPolicy') end
  end
  assert(result.RaidTargetPolicy ~= 'NearestReachable' or result.RaidFocus == 'Any',
    'RaidFocus requires Opportunistic or RandomNearby')
  return result, {policies[result.RaidTargetPolicy],result.RaidGroupCount,result.RaidMinGroupSize,
    compiledFocus,risks[result.RaidRiskTolerance],scopes[result.RaidEnemyScope]}
end
function M.copy(candidate)
  local result = {}
  for _, field in ipairs(M.fields) do result[field] = candidate[field] end
  if type(result.RaidFocus) == 'table' then result.RaidFocus = focus(result.RaidFocus) end
  return result
end
return M
