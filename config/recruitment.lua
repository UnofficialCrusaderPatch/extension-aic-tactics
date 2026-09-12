local M = {}
M.version = 2

M.fields = {
  'RecruitPolicy', 'RecruitProbSortieDefault', 'RecruitProbSortieWeak',
  'RecruitProbSortieStrong', 'RecruitConditions',
  'DefRecruitComposition',
}
local strengths = {'Default', 'Weak', 'Strong'}
local strengthNumbers = {Default = 0, Weak = 1, Strong = 2}
local facts = {HomeUnderThreat = 1, AttackActive = 2, DefenseIncomplete = 4, EquipmentSurplus = 8}
local roles = {'Defense', 'Raid', 'Attack', 'Sortie'}
local rowKeys = {When = true, Defense = true, Raid = true, Attack = true, Sortie = true}

local function integer(value, low, high, name)
  assert(type(value) == 'number' and value == math.floor(value)
    and value >= low and value <= high, name .. ' must be an integer from ' .. low .. ' to ' .. high)
  return value
end

local function plainTable(value, name)
  assert(type(value) == 'table' and getmetatable(value) == nil, name .. ' must be a plain table')
end

local function conditions(value)
  plainTable(value, 'RecruitConditions')
  local count = 0
  for key in pairs(value) do
    integer(key, 1, 8, 'RecruitConditions index')
    count = count + 1
  end
  local authored, compiled = {}, {}
  for index = 1, count do
    local row = value[index]
    plainTable(row, 'RecruitConditions row ' .. index)
    for key in pairs(row) do assert(rowKeys[key], 'Unknown recruitment row key: ' .. tostring(key)) end
    plainTable(row.When, 'RecruitConditions.When')
    local when, strength, required, forbidden = {}, -1, 0, 0
    for key, item in pairs(row.When) do
      if key == 'Strength' then
        assert(strengthNumbers[item] ~= nil, 'Strength must be Default, Weak or Strong')
        strength = strengthNumbers[item]
      else
        assert(facts[key], 'Unknown recruitment condition: ' .. tostring(key))
        assert(type(item) == 'boolean', key .. ' must be boolean')
        if item then required = required + facts[key] else forbidden = forbidden + facts[key] end
      end
      when[key] = item
    end
    local saved, weights, sum = {When = when}, {}, 0
    for roleIndex, role in ipairs(roles) do
      local weight = integer(row[role], 0, 100, 'RecruitConditions.' .. role)
      saved[role], weights[roleIndex] = weight, weight
      sum = sum + weight
    end
    assert(sum == 100, 'RecruitConditions row ' .. index .. ' must total 100')
    authored[index] = saved
    compiled[index] = {strength = strength, requiredFacts = required,
      forbiddenFacts = forbidden, weights = weights}
  end
  return authored, compiled
end

function M.defaults()
  return {RecruitPolicy = 'Native', RecruitProbSortieDefault = 0,
    RecruitProbSortieWeak = 0, RecruitProbSortieStrong = 0, RecruitConditions = {},
    DefRecruitComposition = 'Native'}
end

function M.prepare(previous, spec, readNative, resetting)
  local candidate = M.defaults()
  for _, field in ipairs(M.fields) do
    if not resetting then
      if previous and previous[field] ~= nil then candidate[field] = previous[field] end
      if spec[field] ~= nil then candidate[field] = spec[field] end
    end
  end
  local mode = candidate.RecruitPolicy
  assert(mode == 'Native' or mode == 'WeightedRoles', 'RecruitPolicy must be Native or WeightedRoles')
  if mode == 'Native' and not resetting then
    for index = 2, #M.fields do
      assert(spec[M.fields[index]] == nil, M.fields[index] .. ' requires RecruitPolicy=WeightedRoles')
    end
  end
  for _, strength in ipairs(strengths) do
    local field = 'RecruitProbSortie' .. strength
    integer(candidate[field], 0, 100, field)
  end
  local rows
  assert(candidate.DefRecruitComposition == 'Native' or candidate.DefRecruitComposition == 'PreserveSlots',
    'DefRecruitComposition must be Native or PreserveSlots')
  candidate.RecruitConditions, rows = conditions(candidate.RecruitConditions)
  local compiled = {schemaVersion = M.version, mode = mode == 'Native' and 0 or 1,
    conditions = rows, baseRows = {},
    defenseComposition = candidate.DefRecruitComposition == 'PreserveSlots' and 1 or 0}
  if mode == 'WeightedRoles' then
    for index, strength in ipairs(strengths) do
      local weights = {
        readNative('RecruitProbDef' .. strength), readNative('RecruitProbRaid' .. strength),
        readNative('RecruitProbAttack' .. strength), candidate['RecruitProbSortie' .. strength],
      }
      local sum = 0
      for role = 1, 4 do sum = sum + integer(weights[role], 0, 100, strengths[index] .. ' weight') end
      assert(sum == 100, 'Recruitment ' .. strength .. ' row must total 100')
      compiled.baseRows[index] = weights
    end
  end
  return candidate, compiled
end

-- Only validated, bounded data reaches this copy operation.
function M.copy(candidate)
  local result = {}
  for _, field in ipairs(M.fields) do result[field] = candidate[field] end
  result.RecruitConditions = conditions(candidate.RecruitConditions)
  return result
end

return M
