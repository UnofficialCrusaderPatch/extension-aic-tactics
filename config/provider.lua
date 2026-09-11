local schema = require('config.personality')
local M = {}
local owner = 'aic-tactics'

-- The backend owns native configuration storage and simulation admission.
-- Its prepare operation must be pure and return reversible commit/rollback.
function M.register(loader, backend)
  assert(type(backend) == 'table' and type(backend.prepare) == 'function',
    'AIC Tactics requires a native configuration backend')
  local states = {}
  local function handles(ai, spec, resetting)
    if resetting then return states[ai] ~= nil end
    if states[ai] and schema.active(states[ai]) then return true end
    for _, field in ipairs(schema.fields) do if spec[field] ~= nil then return true end end
    return false
  end
  local registered = {}
  local ok, reason = pcall(function()
    for _, name in ipairs(schema.fields) do
      local field = name
      loader:registerAdditionalAICValue(owner, field, function(ai, value)
        assert(value == nil, 'AIC Tactics setters require an atomic loader update')
        return schema.copy(states[ai] or schema.defaults())[field]
      end, function(ai)
        -- A character never authored through this provider has no backend state.
        assert(states[ai] == nil, 'AIC Tactics reset requires an atomic loader update')
      end)
      registered[#registered + 1] = field
    end
    loader:registerAICUpdateProvider(owner, {
      handles = handles,
      prepare = function(ai, spec, readNative, resetting)
        local previous = states[ai]
        local candidate, compiled = schema.prepare(previous, spec, readNative, resetting)
        local operation = backend.prepare(ai, schema.copy(candidate), compiled)
        assert(type(operation) == 'table' and type(operation.commit) == 'function'
          and type(operation.rollback) == 'function', 'Invalid AIC Tactics backend operation')
        -- Detach callbacks as well as input tables from mutable backend objects.
        local commit, rollback = operation.commit, operation.rollback
        return {
          commit = function()
            commit()
            if resetting then states[ai] = nil else states[ai] = candidate end
          end,
          rollback = function()
            local restored, failure = pcall(rollback)
            states[ai] = previous
            if not restored then error(failure, 0) end
          end,
        }
      end,
    })
  end)
  if not ok then
    for index = #registered, 1, -1 do loader:unregisterAdditionalAICValue(owner, registered[index]) end
    error(reason, 0)
  end
end

return M
