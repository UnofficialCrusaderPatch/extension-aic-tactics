local M = {}

-- Lord hunt records and their census (src/lord_hunt.cpp): 9 players x 17
-- record words, 9 x (2 + 384) census words, and 9 unit counts. The census
-- feeds the next decision, so a reload restores it rather than rebuilding it.
local players, recordWords, capacity = 9, 17, 384
local censusWords = 2 + capacity
local huntDuty, splitDuty = 2, 3

function M.new(native)
  assert(native.lordHuntStateSize == recordWords * 4 and native.lordHuntCensusSize == censusWords * 4,
    'AIC Tactics: incompatible lord hunt state ABI')
  local blocks = {{native.lordHunts, players * recordWords}, {native.lordHuntCensus, players * censusWords},
    {native.lordHuntUnits, players}}
  local count = 0
  for _, block in ipairs(blocks) do count = count + block[2] end
  -- `groups` are the detached group words (group-state.lua): a reference
  -- must name a record of the matching duty.
  local function validate(values, groups)
    local units = native.game.unitCapacity
    assert(#values == count, 'AIC Tactics: invalid lord hunt state size')
    local function range(value, maximum)
      assert(value >= 0 and value <= maximum, 'AIC Tactics: invalid saved lord hunt state')
    end
    local function reference(player, value, duty)
      range(value, 8)
      if value > 0 and groups then
        assert(groups[(player * 8 + value - 1) * 12 + 3] == duty, 'AIC Tactics: invalid saved lord hunt group')
      end
    end
    for player = 0, players - 1 do
      local start = player * recordWords + 1
      range(values[start], 8)
      for offset = 2, 3 do range(values[start + offset], 8) end
      range(values[start + 4], 7)
      range(values[start + 5], 2147483647)
      reference(player, values[start + 6], huntDuty)
      reference(player, values[start + 7], splitDuty)
      reference(player, values[start + 8], splitDuty)
      range(values[start + 9], 2)
      range(values[start + 10], 1)
      range(values[start + 13], 3)
      range(values[start + 14], units)
      range(values[start + 15], 0x1FF)
      if values[start] == 0 then
        for offset = 1, 14 do range(values[start + offset], 0) end
      end
      assert(values[start] ~= player or player == 0, 'AIC Tactics: saved lord hunt attacks its owner')
    end
    local offset = players * recordWords + 1
    for player = 0, players - 1 do
      local start = offset + player * censusWords
      range(values[start], units)
      range(values[start + 1], capacity)
      for index = 0, capacity - 1 do
        local value = values[start + 2 + index]
        if index < values[start + 1] then
          range(value % 65536, units - 1)
          range(math.floor(value / 65536), 1)
          assert(value % 65536 > 0, 'AIC Tactics: invalid saved lord hunt candidate')
        else
          range(value, 0)
        end
      end
    end
    offset = offset + players * censusWords
    for player = 0, players - 1 do range(values[offset + player], units) end
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
    restore = function(values, groups)
      validate(values, groups)
      local position = 1
      for _, block in ipairs(blocks) do
        for index = 0, block[2] - 1 do
          local value = values[position]
          core.writeInteger(block[1] + index * 4, value >= 2147483648 and value - 4294967296 or value)
          position = position + 1
        end
      end
    end,
    initialize = function()
      for _, block in ipairs(blocks) do core.setMemory(block[1], 0, block[2] * 4) end
    end}
end

return M
