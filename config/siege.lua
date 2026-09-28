local M = {fields = {'SafeSiegePlacement', 'ActualSiegeResourcePayment',
  'CoordinatedSiegeHarassment', 'SiegeHarassMinEngines'}}

-- Zero means that this AI inherits the module switch. A stored false must
-- remain distinct from a field that was never supplied by the AIC.
function M.defaults() return {} end

function M.prepare(previous, spec, resetting)
  local value, payment, coordination, minimum
  if not resetting then
    if previous then value = previous.SafeSiegePlacement end
    if previous then payment = previous.ActualSiegeResourcePayment end
    if previous then coordination = previous.CoordinatedSiegeHarassment end
    if previous then minimum = previous.SiegeHarassMinEngines end
    if spec.SafeSiegePlacement ~= nil then value = spec.SafeSiegePlacement end
    if spec.ActualSiegeResourcePayment ~= nil then
      payment = spec.ActualSiegeResourcePayment
    end
    if spec.CoordinatedSiegeHarassment ~= nil then
      coordination = spec.CoordinatedSiegeHarassment
    end
    if spec.SiegeHarassMinEngines ~= nil then minimum = spec.SiegeHarassMinEngines end
  end
  assert(value == nil or type(value) == 'boolean', 'SafeSiegePlacement must be boolean')
  assert(payment == nil or type(payment) == 'boolean',
    'ActualSiegeResourcePayment must be boolean')
  assert(coordination == nil or type(coordination) == 'boolean',
    'CoordinatedSiegeHarassment must be boolean')
  assert(minimum == nil or (type(minimum) == 'number' and minimum == math.floor(minimum)
      and minimum >= 0 and minimum <= 20),
    'SiegeHarassMinEngines must be an integer from 0 to 20')
  return {SafeSiegePlacement = value, ActualSiegeResourcePayment = payment,
      CoordinatedSiegeHarassment = coordination, SiegeHarassMinEngines = minimum},
    {value == nil and 0 or value and 1 or 2,
      payment == nil and 0 or payment and 1 or 2,
      coordination == nil and 0 or coordination and 1 or 2,
      minimum == nil and 0 or minimum + 1}
end

function M.copy(candidate)
  return {SafeSiegePlacement = candidate.SafeSiegePlacement,
    ActualSiegeResourcePayment = candidate.ActualSiegeResourcePayment,
    CoordinatedSiegeHarassment = candidate.CoordinatedSiegeHarassment,
    SiegeHarassMinEngines = candidate.SiegeHarassMinEngines}
end

return M
