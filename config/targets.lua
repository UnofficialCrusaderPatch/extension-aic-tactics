local M = {}
M.fields = {'AttackTargetPolicy', 'AttackTargetCommitment', 'AttackActivation', 'ProvocationRules'}
local policies = {Inherit = 0, LowestPopulation = 1, FewestTroops = 2,
  LowestCombatPower = 3, Random = 4, LastAggressor = 5}
local commitments = {Default = 0, PerAttack = 1, UntilDefeated = 2}

function M.defaults()
  return {AttackTargetPolicy = 'Inherit', AttackTargetCommitment = 'Default',
    AttackActivation = 'Immediate',
    ProvocationRules = {ThreatPower = 100, CombatTicks = 200, LossPower = 100, WindowTicks = 800}}
end

local function rules(value)
  assert(type(value) == 'table' and getmetatable(value) == nil, 'ProvocationRules must be a plain record')
  local bounds = {ThreatPower = {1,100000}, CombatTicks = {1,9600},
    LossPower = {1,100000}, WindowTicks = {32,9600}}
  for key in pairs(value) do assert(bounds[key], 'Unknown ProvocationRules field: ' .. tostring(key)) end
  local result = {}
  for key, bound in pairs(bounds) do
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

function M.prepare(previous, spec, resetting)
  local candidate = M.defaults()
  for _, field in ipairs(M.fields) do
    if not resetting then
      if previous and previous[field] ~= nil then candidate[field] = previous[field] end
      if spec[field] ~= nil then candidate[field] = spec[field] end
    end
  end
  local policy = policies[candidate.AttackTargetPolicy]
  local commitment = commitments[candidate.AttackTargetCommitment]
  assert(policy ~= nil, 'AttackTargetPolicy must be Inherit, LowestPopulation, FewestTroops, LowestCombatPower, Random or LastAggressor')
  assert(commitment ~= nil, 'AttackTargetCommitment must be Default, PerAttack or UntilDefeated')
  assert(candidate.AttackActivation == 'Immediate' or candidate.AttackActivation == 'AfterProvocation',
    'AttackActivation must be Immediate or AfterProvocation')
  candidate.ProvocationRules = rules(candidate.ProvocationRules)
  return candidate, {schemaVersion = 2, policy = policy, commitment = commitment,
    activation = candidate.AttackActivation == 'AfterProvocation' and 1 or 0,
    rules = {candidate.ProvocationRules.ThreatPower, candidate.ProvocationRules.CombatTicks,
      candidate.ProvocationRules.LossPower, candidate.ProvocationRules.WindowTicks}}
end

function M.active(candidate)
  return candidate.AttackTargetPolicy ~= 'Inherit' or candidate.AttackTargetCommitment ~= 'Default'
    or candidate.AttackActivation ~= 'Immediate'
end

function M.copy(candidate)
  return {AttackTargetPolicy = candidate.AttackTargetPolicy,
    AttackTargetCommitment = candidate.AttackTargetCommitment,
    AttackActivation = candidate.AttackActivation, ProvocationRules = rules(candidate.ProvocationRules)}
end

return M
