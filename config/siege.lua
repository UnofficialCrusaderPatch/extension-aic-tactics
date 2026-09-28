local M = {fields = {'SafeSiegePlacement', 'ActualSiegeResourcePayment'}}

-- Zero means that this AI inherits the module switch. A stored false must
-- remain distinct from a field that was never supplied by the AIC.
function M.defaults() return {} end

function M.prepare(previous, spec, resetting)
  local value, payment
  if not resetting then
    if previous then value = previous.SafeSiegePlacement end
    if previous then payment = previous.ActualSiegeResourcePayment end
    if spec.SafeSiegePlacement ~= nil then value = spec.SafeSiegePlacement end
    if spec.ActualSiegeResourcePayment ~= nil then
      payment = spec.ActualSiegeResourcePayment
    end
  end
  assert(value == nil or type(value) == 'boolean', 'SafeSiegePlacement must be boolean')
  assert(payment == nil or type(payment) == 'boolean',
    'ActualSiegeResourcePayment must be boolean')
  return {SafeSiegePlacement = value, ActualSiegeResourcePayment = payment},
    {value == nil and 0 or value and 1 or 2,
      payment == nil and 0 or payment and 1 or 2}
end

function M.copy(candidate)
  return {SafeSiegePlacement = candidate.SafeSiegePlacement,
    ActualSiegeResourcePayment = candidate.ActualSiegeResourcePayment}
end

return M
