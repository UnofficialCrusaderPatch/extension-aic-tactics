local M = {fields = {'AttackPreparation'}}
function M.defaults() return {AttackPreparation = 'Native'} end
function M.prepare(previous, spec, readNative, resetting)
  local value = resetting and 'Native' or spec.AttackPreparation
    or previous and previous.AttackPreparation or 'Native'
  assert(value == 'Native' or value == 'DuringAttack', 'AttackPreparation must be Native or DuringAttack')
  if value == 'DuringAttack' then
    for field, maximum in pairs({AttUnitPatrolGroupsCount=2, AttUnitBackupGroupsCount=3,
        AttUnitSiegeDefGroupsCount=2, AttMainGroupsCount=8}) do
      local count = readNative(field)
      assert(type(count) == 'number' and count == math.floor(count) and count <= maximum,
        field .. ' exceeds its native group capacity')
    end
  end
  return {AttackPreparation = value}, value == 'DuringAttack' and 1 or 0
end
return M
