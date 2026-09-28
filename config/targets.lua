local M = {}
M.fields = {'AttackTargetPolicy', 'AttackTargetCommitment', 'AttackActivation', 'ProvocationRules'}
local policies = {Inherit = 0, LowestPopulation = 1, FewestTroops = 2,
  LowestCombatPower = 3, Random = 4, LastAggressor = 5}
local commitments = {Default = 0, PerAttack = 1, UntilDefeated = 2}
local ruleBounds = {ThreatPower = {1,100000}, CombatTicks = {1,9600},
  LossPower = {1,100000}, WindowTicks = {32,9600}}
local function decodePolicy(value)
  if policies[value] ~= nil then return policies[value], nil, value end
  if value == 'InheritPerAttack' then return 0, 1, 'Inherit' end
end

function M.defaults()
  return {AttackTargetPolicy = 'Inherit', AttackTargetCommitment = 'Default',
    AttackActivation = 'Immediate',
    ProvocationRules = {ThreatPower = 100, CombatTicks = 200, LossPower = 100, WindowTicks = 800}}
end

local function rules(value)
  assert(type(value) == 'table' and getmetatable(value) == nil, 'ProvocationRules must be a plain record')
  for key in pairs(value) do assert(ruleBounds[key], 'Unknown ProvocationRules field: ' .. tostring(key)) end
  local result = {}
  for key, bound in pairs(ruleBounds) do
    local number = value[key]
    assert(type(number) == 'number' and number == math.floor(number)
      and number >= bound[1] and number <= bound[2],
      'ProvocationRules.' .. key .. ' must be an integer from ' .. bound[1] .. ' to ' .. bound[2])
    result[key] = number
  end
  assert(result.WindowTicks % 32 == 0, 'ProvocationRules.WindowTicks must be a multiple of 32')
  assert(result.CombatTicks <= result.WindowTicks, 'ProvocationRules.CombatTicks must not exceed WindowTicks')
  return result
end

local function targetChoice(value)
  if type(value) ~= 'table' then return value, nil, nil end
  assert(getmetatable(value) == nil, 'AttackTargetPolicy must be a plain record')
  for key in pairs(value) do
    assert(key == 'Choice' or key == 'UntilDefeated' or key == 'Provocation',
      'Unknown AttackTargetPolicy field: ' .. tostring(key))
  end
  assert(policies[value.Choice] ~= nil, 'AttackTargetPolicy.Choice must name a target selection rule')
  assert(value.UntilDefeated == nil or type(value.UntilDefeated) == 'boolean',
    'AttackTargetPolicy.UntilDefeated must be boolean')
  if value.Provocation ~= nil then
    assert(value.Choice == 'LastAggressor', 'AttackTargetPolicy.Provocation requires LastAggressor')
    assert(type(value.Provocation) == 'table' and getmetatable(value.Provocation) == nil,
      'AttackTargetPolicy.Provocation must be a plain record')
  end
  return value.Choice, value.Provocation, value.UntilDefeated
end

function M.prepare(previous, spec, resetting)
  local candidate = M.defaults()
  for _, field in ipairs(M.fields) do
    if not resetting then
      if previous and previous[field] ~= nil then candidate[field] = previous[field] end
      if spec[field] ~= nil then candidate[field] = spec[field] end
    end
  end
  local choice, nested, untilDefeated = targetChoice(candidate.AttackTargetPolicy)
  if not resetting and spec.AttackTargetPolicy ~= nil and nested ~= nil then
    assert(spec.ProvocationRules == nil,
      'Set provocation rules inside AttackTargetPolicy or in ProvocationRules, not both')
    local merged = rules(previous and previous.ProvocationRules or M.defaults().ProvocationRules)
    for key, value in pairs(nested) do
      assert(ruleBounds[key], 'Unknown AttackTargetPolicy.Provocation field: ' .. tostring(key))
      merged[key] = value
    end
    candidate.ProvocationRules = rules(merged)
  else
    candidate.ProvocationRules = rules(candidate.ProvocationRules)
  end
  if type(candidate.AttackTargetPolicy) == 'table' then
    candidate.AttackTargetPolicy = {Choice = choice, UntilDefeated = untilDefeated,
      Provocation = nested and rules(candidate.ProvocationRules) or nil}
  end
  -- A newly authored combined policy supersedes an old commitment setting.
  -- Old AICs may still supply both original fields and retain their meaning.
  if not resetting and spec.AttackTargetPolicy ~= nil and spec.AttackTargetCommitment == nil then
    candidate.AttackTargetCommitment = 'Default'
  end
  local policy, embeddedCommitment, base = decodePolicy(choice)
  if untilDefeated then embeddedCommitment = 2 end
  assert(policy ~= nil, 'AttackTargetPolicy must be Inherit, InheritPerAttack or a supported selection policy')
  if not resetting and spec.AttackTargetCommitment ~= nil and embeddedCommitment ~= nil then
    if spec.AttackTargetPolicy ~= nil then
      error('Set commitment in AttackTargetPolicy or AttackTargetCommitment, not both')
    end
    -- An old setter can replace the lifetime of a previously combined policy.
    candidate.AttackTargetPolicy = base
    embeddedCommitment = nil
  end
  local commitment = commitments[candidate.AttackTargetCommitment]
  assert(commitment ~= nil, 'AttackTargetCommitment must be Default, PerAttack or UntilDefeated')
  if embeddedCommitment ~= nil then
    assert(commitment == 0, 'Combined AttackTargetPolicy conflicts with AttackTargetCommitment')
    commitment = embeddedCommitment
  end
  assert(candidate.AttackActivation == 'Immediate' or candidate.AttackActivation == 'AfterProvocation',
    'AttackActivation must be Immediate or AfterProvocation')
  return candidate, {schemaVersion = 2, policy = policy, commitment = commitment,
    activation = candidate.AttackActivation == 'AfterProvocation' and 1 or 0,
    rules = {candidate.ProvocationRules.ThreatPower, candidate.ProvocationRules.CombatTicks,
      candidate.ProvocationRules.LossPower, candidate.ProvocationRules.WindowTicks}}
end

function M.active(candidate)
  local choice = targetChoice(candidate.AttackTargetPolicy)
  return choice ~= 'Inherit' or candidate.AttackTargetCommitment ~= 'Default'
    or candidate.AttackActivation ~= 'Immediate'
end

function M.copy(candidate)
  local choice, nested, untilDefeated = targetChoice(candidate.AttackTargetPolicy)
  return {AttackTargetPolicy = type(candidate.AttackTargetPolicy) == 'table'
      and {Choice = choice, UntilDefeated = untilDefeated,
        Provocation = nested and rules(candidate.ProvocationRules) or nil} or choice,
    AttackTargetCommitment = candidate.AttackTargetCommitment,
    AttackActivation = candidate.AttackActivation, ProvocationRules = rules(candidate.ProvocationRules)}
end

return M
