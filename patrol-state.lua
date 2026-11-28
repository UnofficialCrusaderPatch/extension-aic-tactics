local M = {}

-- Patrol route records (src/patrols.cpp): 9 players x 9 routes x 9 words:
-- uid, target, point, step, x, y, tick, distance, stalls.
local players, routes, words = 9, 9, 9

function M.new(native)
  assert(native.patrolRouteSize == words * 4, 'AIC Tactics: incompatible patrol route ABI')
  local count = players * routes * words
  local function validate(values)
    assert(#values == count, 'AIC Tactics: invalid patrol route state size')
    local function range(value, low, high)
      assert(value >= low and value <= high, 'AIC Tactics: invalid saved patrol route')
    end
    for index = 0, players * routes - 1 do
      local start = index * words + 1
      if values[start] == 0 then
        for word = 1, words - 1 do range(values[start + word], 0, 0) end
      else
        range(values[start + 1], 1, 8)
        range(values[start + 2], 0, 15)
        assert(values[start + 3] == 1 or values[start + 3] == 4294967295, 'AIC Tactics: invalid saved patrol route')
        range(values[start + 4], 0, 399)
        range(values[start + 5], 0, 399)
        range(values[start + 7], 0, 399)
        range(values[start + 8], 0, 2)
      end
    end
    return values
  end
  return {wordCount = count, validate = validate,
    capture = function()
      local values = {}
      for index = 0, count - 1 do values[#values + 1] = core.readInteger(native.patrolRoutes + index * 4) end
      return values
    end,
    restore = function(values)
      validate(values)
      for index, value in ipairs(values) do
        core.writeInteger(native.patrolRoutes + (index - 1) * 4,
          value >= 2147483648 and value - 4294967296 or value)
      end
    end,
    initialize = function() core.setMemory(native.patrolRoutes, 0, count * 4) end}
end

return M
