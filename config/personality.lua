local recruitment = require('config.recruitment')
local targets = require('config.targets')
local M = {fields = {}}
for _, component in ipairs({recruitment, targets}) do
  for _, field in ipairs(component.fields) do M.fields[#M.fields + 1] = field end
end

local function combine(authored, targeting)
  for _, field in ipairs(targets.fields) do authored[field] = targeting[field] end
  return authored
end

function M.defaults()
  return combine(recruitment.defaults(), targets.defaults())
end

function M.active(candidate)
  return candidate.RecruitPolicy == 'WeightedRoles' or targets.active(candidate)
end

function M.prepare(previous, spec, readNative, resetting)
  local authored, compiled = recruitment.prepare(previous, spec, readNative, resetting)
  local targetAuthored, targetCompiled = targets.prepare(previous, spec, resetting)
  return combine(authored, targetAuthored),
    {schemaVersion = 2, recruitment = compiled, targeting = targetCompiled}
end

function M.copy(candidate)
  return combine(recruitment.copy(candidate), candidate)
end

return M
