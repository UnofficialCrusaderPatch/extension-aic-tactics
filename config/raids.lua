local M = {fields = {'RaidTargetPolicy','RaidGroupCount','RaidMinGroupSize','RaidFocus','RaidRiskTolerance','RaidEnemyScope'}}
local policies = {Native=0, NearestReachable=1, Opportunistic=2}
local focuses = {Any=0, Food=1, Industry=2, HighValue=3}
local risks = {Low=0, Medium=1, High=2}
local scopes = {PrimeTarget=0, AnyEnemy=1}
function M.defaults()
  return {RaidTargetPolicy='Native', RaidGroupCount=1, RaidMinGroupSize=4,
    RaidFocus='Any', RaidRiskTolerance='Medium', RaidEnemyScope='PrimeTarget'}
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
  assert(policies[result.RaidTargetPolicy] ~= nil, 'RaidTargetPolicy must be Native, NearestReachable or Opportunistic')
  assert(focuses[result.RaidFocus] ~= nil, 'RaidFocus must be Any, Food, Industry or HighValue')
  assert(risks[result.RaidRiskTolerance] ~= nil, 'RaidRiskTolerance must be Low, Medium or High')
  assert(scopes[result.RaidEnemyScope] ~= nil, 'RaidEnemyScope must be PrimeTarget or AnyEnemy')
  integer(result.RaidGroupCount, 4, 'RaidGroupCount')
  integer(result.RaidMinGroupSize, 256, 'RaidMinGroupSize')
  if result.RaidTargetPolicy == 'Native' and not resetting then
    for index = 2, #M.fields do assert(spec[M.fields[index]] == nil, M.fields[index] .. ' requires a new RaidTargetPolicy') end
  end
  assert(result.RaidTargetPolicy ~= 'NearestReachable' or result.RaidFocus == 'Any', 'RaidFocus requires Opportunistic')
  return result, {policies[result.RaidTargetPolicy],result.RaidGroupCount,result.RaidMinGroupSize,
    focuses[result.RaidFocus],risks[result.RaidRiskTolerance],scopes[result.RaidEnemyScope]}
end
function M.copy(candidate)
  local result = {}
  for _, field in ipairs(M.fields) do result[field] = candidate[field] end
  return result
end
return M
