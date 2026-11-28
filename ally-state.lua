local M = {}

-- Ally help state (src/allies.cpp AllyState): 9 players x 65 words.
-- month, given[25], receivedMonth[25], keySeen[5], keyNow[5], nextAskTick,
-- reliefPauseUntil, reliefCalmSince, reliefRequested.
local players, words = 9, 65
local given, received, keySeen, keyNow, requested = 2, 27, 52, 57, 65

function M.new(native)
  assert(native.allyStateSize == words * 4, 'AIC Tactics: incompatible ally state ABI')
  local count = players * words
  local function validate(values)
    assert(#values == count, 'AIC Tactics: invalid ally state size')
    local function range(value, minimum, maximum)
      assert(value >= minimum and value <= maximum, 'AIC Tactics: invalid saved ally state')
    end
    for player = 0, players - 1 do
      local start = player * words
      local month = values[start + 1]
      range(month, 0, 5368709)
      for index = 0, 24 do
        range(values[start + given + index], 0, 2147483647)
        range(values[start + received + index], 0, 5368710)
      end
      for index = 0, 4 do
        range(values[start + keySeen + index], 0, native.game.buildingCapacity)
        range(values[start + keyNow + index], 0, values[start + keySeen + index])
      end
      range(values[start + requested], 0, 1)
      if player == 0 then
        for index = 1, words do range(values[start + index], 0, 0) end
      end
    end
    return values
  end
  return {wordCount = count, validate = validate,
    capture = function()
      local values = {}
      for index = 0, count - 1 do values[#values + 1] = core.readInteger(native.allyStates + index * 4) end
      return values
    end,
    restore = function(values)
      validate(values)
      for index, value in ipairs(values) do
        core.writeInteger(native.allyStates + (index - 1) * 4,
          value >= 2147483648 and value - 4294967296 or value)
      end
    end,
    initialize = function() core.setMemory(native.allyStates, 0, count * 4) end}
end

return M
