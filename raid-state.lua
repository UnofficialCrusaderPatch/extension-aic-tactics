local M = {}

function M.new(native)
  assert(native.raidStateSize == 96, 'AIC Tactics: incompatible raid state ABI')
  -- Census snapshots influence the next decision. Rebuilding them on load would
  -- change that decision if units or buildings moved after the saved census.
  local blocks = {
    {native.raidStates, 9 * 24}, {native.raidGroupCensus, 9 * 4 * 2},
    {native.raidUnitPower, 9 * 1024}, {native.raidStaticDefenses, 9 * 1024},
    {native.raidBuildingCensusTick, 1}, {native.raidBuildingCensusValid, 1},
  }
  local count = 0
  for _, block in ipairs(blocks) do count = count + block[2] end
  local function validate(values)
    assert(#values == count, 'AIC Tactics: invalid raid state size')
    local function range(value, maximum)
      assert(value >= 0 and value <= maximum, 'AIC Tactics: invalid saved raid state')
    end
    for player = 0, 8 do
      local start = player * 24 + 1
      range(values[start], 3)
      range(values[start + 1], 9)
      range(values[start + 2], native.game.tribeMemberWords - 1)
      range(values[start + 3], 15)
      for group = 0, 3 do
        local offset = start + 4 + group * 5
        range(values[offset], 1249)
        if values[offset] == 0 then range(values[offset + 1], 0) end
        range(values[offset + 2], native.game.buildingCapacity - 1)
        if values[offset + 2] == 0 then range(values[offset + 3], 0) end
        range(values[offset + 4], 7)
      end
    end
    local offset = 9 * 24 + 1
    local total = 0
    for group = 0, 9 * 4 - 1 do
      range(values[offset + group * 2], native.game.unitCapacity)
      total = total + values[offset + group * 2]
      range(values[offset + group * 2 + 1], 2147483647)
    end
    range(total, native.game.unitCapacity)
    offset = offset + 9 * 4 * 2
    for index = 0, 9 * 1024 - 1 do range(values[offset + index], 2147483647) end
    offset = offset + 9 * 1024
    total = 0
    for index = 0, 9 * 1024 - 1 do
      range(values[offset + index], native.game.buildingCapacity)
      total = total + values[offset + index]
    end
    range(total, native.game.buildingCapacity)
    range(values[count], 1)
    return values
  end
  return {wordCount = count, validate = validate,
    capture = function()
      local values = {}
      for _, block in ipairs(blocks) do
        for index = 0, block[2] - 1 do values[#values + 1] = core.readInteger(block[1] + index * 4) end
      end
      return values
    end,
    restore = function(values)
      validate(values)
      local offset = 1
      for _, block in ipairs(blocks) do
        for index = 0, block[2] - 1 do
          local value = values[offset]
          core.writeInteger(block[1] + index * 4, value >= 2147483648 and value - 4294967296 or value)
          offset = offset + 1
        end
      end
    end,
    initialize = function()
      for _, block in ipairs(blocks) do core.setMemory(block[1], 0, block[2] * 4) end
    end}
end

return M
