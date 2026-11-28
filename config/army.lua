local M = {fields = {'AttackPreparation'}}
function M.defaults() return {AttackPreparation = 'Native'} end
function M.prepare(previous, spec, readNative, resetting, check)
  check = check or assert
  local value = previous and previous.AttackPreparation or 'Native'
  if resetting then value = 'Native'
  elseif spec.AttackPreparation ~= nil then value = spec.AttackPreparation end
  assert(type(value) == 'string', 'AttackPreparation must be a string')
  check(value == 'Native' or value == 'DuringAttack', 'AttackPreparation must be Native or DuringAttack')
  if value == 'DuringAttack' then
    for field, maximum in pairs({AttUnitPatrolGroupsCount=2, AttUnitBackupGroupsCount=3,
        AttUnitSiegeDefGroupsCount=2, AttMainGroupsCount=8}) do
      local count = readNative(field)
      assert(type(count) == 'number', field .. ' must be a number')
      check(count == math.floor(count) and count <= maximum,
        field .. ' exceeds its native group capacity')
    end
  end
  return {AttackPreparation = value}, value == 'DuringAttack' and 1 or 0
end
return M
