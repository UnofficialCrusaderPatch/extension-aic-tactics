local M = {fields = {'SafeSiegePlacement'}}

-- Zero means that this AI inherits the module switch. A stored false must
-- remain distinct from a field that was never supplied by the AIC.
function M.defaults() return {} end

function M.prepare(previous, spec, resetting)
  local value
  if not resetting then
    if previous then value = previous.SafeSiegePlacement end
    if spec.SafeSiegePlacement ~= nil then value = spec.SafeSiegePlacement end
  end
  assert(value == nil or type(value) == 'boolean', 'SafeSiegePlacement must be boolean')
  return {SafeSiegePlacement = value}, value == nil and 0 or value and 1 or 2
end

function M.copy(candidate)
  return {SafeSiegePlacement = candidate.SafeSiegePlacement}
end

return M
