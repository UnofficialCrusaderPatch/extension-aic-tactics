local M = {}
M.fields = {'AttackTargetPolicy', 'AttackTargetCommitment'}
local policies = {Inherit = 0, LowestPopulation = 1, FewestTroops = 2,
  LowestCombatPower = 3, Random = 4, LastAggressor = 5}
local commitments = {Default = 0, PerAttack = 1, UntilDefeated = 2}

function M.defaults()
  return {AttackTargetPolicy = 'Inherit', AttackTargetCommitment = 'Default'}
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
  return candidate, {schemaVersion = 1, policy = policy, commitment = commitment}
end

function M.active(candidate)
  return candidate.AttackTargetPolicy ~= 'Inherit' or candidate.AttackTargetCommitment ~= 'Default'
end

return M
