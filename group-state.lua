local M = {}

-- Detached group records (src/groups.cpp): 9 players x 8 groups x 12 words.
local players, groups, words = 9, 8, 12

function M.new(native)
  assert(native.detachedGroupSize == words * 4, 'AIC Tactics: incompatible detached group ABI')
  local count = players * groups * words
  local function validate(values)
    assert(#values == count, 'AIC Tactics: invalid detached group state size')
    local function range(value, maximum)
      assert(value >= 0 and value <= maximum, 'AIC Tactics: invalid saved detached group')
    end
    for index = 0, players * groups - 1 do
      local start = index * words + 1
      range(values[start], 1249)
      range(values[start + 2], 5)
      if values[start + 2] == 0 then range(values[start], 0) end
      range(values[start + 8], native.game.unitCapacity)
      range(values[start + 9], 1)
      range(values[start + 10], native.game.tribeMemberWords - 1)
      -- Only a relief records when its troops began to leave.
      if values[start + 2] ~= 4 then range(values[start + 11], 0) end
    end
    return values
  end
  return {wordCount = count, validate = validate,
    capture = function()
      local values = {}
      for index = 0, count - 1 do values[#values + 1] = core.readInteger(native.detachedGroups + index * 4) end
      return values
    end,
    restore = function(values)
      validate(values)
      for index, value in ipairs(values) do
        core.writeInteger(native.detachedGroups + (index - 1) * 4,
          value >= 2147483648 and value - 4294967296 or value)
      end
    end,
    initialize = function() core.setMemory(native.detachedGroups, 0, count * 4) end}
end

return M
