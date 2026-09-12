local M = {}
-- Explicit 32-bit fields; addresses, function pointers and scratch data are not saved.
local incidentWords, enemyWords = 658, 73

function M.new(native)
  assert(native.incidentSize == incidentWords * 4, 'AIC Tactics: incompatible incident state ABI')
  local blocks = {
    {native.combatCensusTick, 1}, {native.combatCensusValid, 1},
    {native.combatCensus, 9 * 12}, {native.targetStates, 9 * 3},
    {native.targetLifecycle, 9}, {native.incidents, 9 * incidentWords},
  }
  local size = 0
  for _, block in ipairs(blocks) do size = size + block[2] end
  local function capture()
    local values = {}
    for _, block in ipairs(blocks) do
      for index = 0, block[2] - 1 do values[#values + 1] = core.readInteger(block[1] + index * 4) end
    end
    return values
  end
  local function validate(values)
    assert(#values == size, 'AIC Tactics: invalid combat state size')
    local function range(value, maximum)
      assert(value >= 0 and value <= maximum, 'AIC Tactics: invalid saved combat state')
    end
    range(values[2], 1)
    local total = 0
    for player = 0, 8 do
      local offset = 3 + player * 12
      range(values[offset], 2500)
      total = total + values[offset]
      range(values[offset + 1], 2499)
      if values[offset + 1] == 0 then range(values[offset + 2], 0) end
      for enemy = 0, 8 do range(values[offset + 3 + enemy], 2147483647) end
    end
    range(total, 2500)
    local targetOffset = 3 + 9 * 12
    local lifecycleOffset = targetOffset + 9 * 3
    local incidentsOffset = lifecycleOffset + 9
    for player = 0, 8 do
      local target = targetOffset + player * 3
      range(values[target], 8)
      range(values[target + 2], 1)
      if values[target] == 0 then
        range(values[target + 1], 0)
        range(values[target + 2], 0)
      end
      assert(values[target] == 0 or values[target] ~= player, 'AIC Tactics: saved target attacks its owner')
      range(values[lifecycleOffset + player], 3)
      local incident = incidentsOffset + player * incidentWords
      range(values[incident], 1)
      for enemy = 0, 8 do
        local start = incident + 1 + enemy * enemyWords
        for _, flag in ipairs({0,2,3,4,7}) do range(values[start + flag], 1) end
        for bucket = 0, 31 do range(values[start + 10 + bucket * 2], 100000) end
      end
    end
    return values
  end
  local function restore(values)
    validate(values)
    local offset = 1
    for _, block in ipairs(blocks) do
      for index = 0, block[2] - 1 do
        local value = values[offset]
        core.writeInteger(block[1] + index * 4, value >= 2147483648 and value - 4294967296 or value)
        offset = offset + 1
      end
    end
  end
  return {wordCount = size, capture = capture, validate = validate, restore = restore,
    initialize = function()
      for _, block in ipairs(blocks) do core.setMemory(block[1], 0, block[2] * 4) end
    end}
end

return M
