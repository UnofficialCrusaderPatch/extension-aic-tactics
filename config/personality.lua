local recruitment = require('config.recruitment')
local targets = require('config.targets')
local army = require('config.army')
local raids = require('config.raids')
local siege = require('config.siege')
local attacks = require('config.attacks')
local lordHunt = require('config.lord-hunt')
local allies = require('config.allies')
local M = {fields = {}}
for _, component in ipairs({recruitment, targets, army, raids, siege, attacks, lordHunt, allies}) do
  for _, field in ipairs(component.fields) do M.fields[#M.fields + 1] = field end
end

local function combine(authored, targeting)
  for _, field in ipairs(targets.fields) do authored[field] = targeting[field] end
  return authored
end

function M.defaults()
  local result = combine(recruitment.defaults(), targets.defaults())
  result.AttackPreparation = 'Native'
  for field, value in pairs(raids.defaults()) do result[field] = value end
  for field, value in pairs(siege.defaults()) do result[field] = value end
  for field, value in pairs(attacks.defaults()) do result[field] = value end
  for field, value in pairs(lordHunt.defaults()) do result[field] = value end
  for field, value in pairs(allies.defaults()) do result[field] = value end
  return result
end

-- Whether the character's 352-byte record carries a policy. The attack
-- patrol, lord hunt and ally fields live in separate per-AI arrays outside
-- that record (and outside save admission), so they must not change it.
function M.recordActive(candidate)
  return candidate.RecruitPolicy == 'WeightedRoles' or targets.active(candidate)
    or candidate.AttackPreparation ~= 'Native'
    or candidate.RaidTargetPolicy ~= 'Native' or candidate.SafeSiegePlacement ~= nil
    or candidate.ActualSiegeResourcePayment ~= nil
    or candidate.CoordinatedSiegeHarassment ~= nil
    or candidate.SiegeHarassMinEngines ~= nil
    or candidate.LargerSiegeForces ~= nil or candidate.SiegeForceMax ~= nil
end

function M.active(candidate)
  return M.recordActive(candidate) or attacks.active(candidate) or lordHunt.active(candidate) or allies.active(candidate)
end

function M.prepare(previous, spec, readNative, resetting, check)
  check = check or assert
  local authored, compiled = recruitment.prepare(previous, spec, readNative, resetting, check)
  local targetAuthored, targetCompiled = targets.prepare(previous, spec, resetting, check)
  local armyAuthored, armyCompiled = army.prepare(previous, spec, readNative, resetting, check)
  -- Preparing a second wave requires the deployed army's target to stay fixed.
  -- Preserve the authored Default so switching preparation back restores Native.
  if armyCompiled == 1 and targetCompiled.commitment == 0 then targetCompiled.commitment = 1 end
  local raidAuthored, raidCompiled = raids.prepare(previous, spec, resetting, check)
  local siegeAuthored, siegeCompiled = siege.prepare(previous, spec, resetting, check)
  local attackAuthored, attackCompiled = attacks.prepare(previous, spec, resetting, check)
  local huntAuthored, huntCompiled = lordHunt.prepare(previous, spec, resetting, check)
  local allyAuthored, allyCompiled = allies.prepare(previous, spec, resetting, check)
  for field, value in pairs(attackAuthored) do authored[field] = value end
  for field, value in pairs(huntAuthored) do authored[field] = value end
  for field, value in pairs(allyAuthored) do authored[field] = value end
  for field, value in pairs(raidAuthored) do authored[field] = value end
  authored.AttackPreparation = armyAuthored.AttackPreparation
  authored.SafeSiegePlacement = siegeAuthored.SafeSiegePlacement
  authored.ActualSiegeResourcePayment = siegeAuthored.ActualSiegeResourcePayment
  authored.CoordinatedSiegeHarassment = siegeAuthored.CoordinatedSiegeHarassment
  authored.SiegeHarassMinEngines = siegeAuthored.SiegeHarassMinEngines
  authored.LargerSiegeForces = siegeAuthored.LargerSiegeForces
  authored.SiegeForceMax = siegeAuthored.SiegeForceMax
  if spec.ProvocationRules ~= nil and not resetting then
    local used = targetAuthored.AttackActivation == 'AfterProvocation'
      or targetCompiled.policy == 5
    if compiled.mode == 1 then
      for _, row in ipairs(compiled.conditions) do
        used = used or row.requiredFacts % 2 == 1 or row.forbiddenFacts % 2 == 1
      end
    end
    check(used, 'ProvocationRules requires AfterProvocation, LastAggressor or a HomeUnderThreat condition')
  end
  return combine(authored, targetAuthored),
    {schemaVersion = 10, recruitment = compiled, targeting = targetCompiled, preparation = armyCompiled,
      raids = raidCompiled, siege = siegeCompiled, attacks = attackCompiled, lordHunt = huntCompiled,
      allies = allyCompiled}
end

function M.copy(candidate)
  local result = combine(recruitment.copy(candidate), targets.copy(candidate))
  result.AttackPreparation = candidate.AttackPreparation
  for field, value in pairs(raids.copy(candidate)) do result[field] = value end
  result.SafeSiegePlacement = candidate.SafeSiegePlacement
  result.ActualSiegeResourcePayment = candidate.ActualSiegeResourcePayment
  result.CoordinatedSiegeHarassment = candidate.CoordinatedSiegeHarassment
  result.SiegeHarassMinEngines = candidate.SiegeHarassMinEngines
  result.LargerSiegeForces = candidate.LargerSiegeForces
  result.SiegeForceMax = candidate.SiegeForceMax
  for field, value in pairs(attacks.copy(candidate)) do result[field] = value end
  for field, value in pairs(lordHunt.copy(candidate)) do result[field] = value end
  for field, value in pairs(allies.copy(candidate)) do result[field] = value end
  return result
end

return M
