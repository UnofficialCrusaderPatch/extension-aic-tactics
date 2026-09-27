local M = {fields = {'CorrectEngineerRoleCounting'}}

function M.defaults() return {} end

function M.prepare(previous, spec, resetting)
  local value
  if not resetting then
    if previous then value = previous.CorrectEngineerRoleCounting end
    if spec.CorrectEngineerRoleCounting ~= nil then value = spec.CorrectEngineerRoleCounting end
  end
  assert(value == nil or type(value) == 'boolean',
    'CorrectEngineerRoleCounting must be boolean')
  return {CorrectEngineerRoleCounting = value}, value == nil and 0 or value and 1 or 2
end

function M.copy(candidate)
  return {CorrectEngineerRoleCounting = candidate.CorrectEngineerRoleCounting}
end

return M
