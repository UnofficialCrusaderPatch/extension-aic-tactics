local recruitment = require('config.recruitment')
local targets = require('config.targets')
local army = require('config.army')
local raids = require('config.raids')
local M = {fields = {}}
for _, component in ipairs({recruitment, targets, army, raids}) do
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
  return result
end

function M.active(candidate)
  return candidate.RecruitPolicy == 'WeightedRoles' or targets.active(candidate)
    or candidate.AttackPreparation ~= 'Native'
    or candidate.RaidTargetPolicy ~= 'Native'
end

function M.prepare(previous, spec, readNative, resetting)
  local authored, compiled = recruitment.prepare(previous, spec, readNative, resetting)
  local targetAuthored, targetCompiled = targets.prepare(previous, spec, resetting)
  local armyAuthored, armyCompiled = army.prepare(previous, spec, readNative, resetting)
  -- Preparing a second wave requires the deployed army's target to stay fixed.
  -- Preserve the authored Default so switching preparation back restores Native.
  if armyCompiled == 1 and targetCompiled.commitment == 0 then targetCompiled.commitment = 1 end
  local raidAuthored, raidCompiled = raids.prepare(previous, spec, resetting)
  for field, value in pairs(raidAuthored) do authored[field] = value end
  authored.AttackPreparation = armyAuthored.AttackPreparation
  if spec.ProvocationRules ~= nil and not resetting then
    local used = targetAuthored.AttackActivation == 'AfterProvocation' or targetAuthored.AttackTargetPolicy == 'LastAggressor'
    if compiled.mode == 1 then
      for _, row in ipairs(compiled.conditions) do
        used = used or row.requiredFacts % 2 == 1 or row.forbiddenFacts % 2 == 1
      end
    end
    assert(used, 'ProvocationRules requires AfterProvocation, LastAggressor or a HomeUnderThreat condition')
  end
  return combine(authored, targetAuthored),
    {schemaVersion = 4, recruitment = compiled, targeting = targetCompiled, preparation = armyCompiled, raids = raidCompiled}
end

function M.copy(candidate)
  local result = combine(recruitment.copy(candidate), targets.copy(candidate))
  result.AttackPreparation = candidate.AttackPreparation
  for field, value in pairs(raids.copy(candidate)) do result[field] = value end
  return result
end

return M
