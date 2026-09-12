local M = {}
local path = 'recruitment-state.bin'
local header = 'AICTACT\002'
local configurationBytes, aicBytes, censusWords = 16 * 284, 16 * 676, 9 * 80

local function word(value)
  if value < 0 then value = value + 4294967296 end
  local bytes = {}
  for index = 1, 4 do
    bytes[index] = string.char(value % 256)
    value = math.floor(value / 256)
  end
  return table.concat(bytes)
end

local function number(bytes, offset)
  local a,b,c,d = bytes:byte(offset, offset + 3)
  return a + b * 256 + c * 65536 + d * 16777216
end

function M.new(native, legacyInterval)
  assert(native.configurationSize == 284, 'AIC Tactics: incompatible save ABI')
  local function active()
    for ai = 1, 16 do
      if core.readInteger(native.configuration + ai * 284) ~= 0 then return true end
    end
    return false
  end
  local function identity()
    return header .. word(legacyInterval and 1 or 0)
      .. core.readString(native.configuration + 284, configurationBytes)
      .. core.readString(0x23FC8E8 + 676, aicBytes)
  end
  local function capture()
    local result = {identity(), word(core.readInteger(native.defenseCensusTick)),
      word(core.readInteger(native.defenseCensusValid))}
    for index = 0, censusWords - 1 do
      result[#result + 1] = word(core.readInteger(native.defenseTypeCounts + index * 4))
    end
    return table.concat(result)
  end
  local function validate(bytes)
    assert(type(bytes) == 'string', 'AIC Tactics: missing saved recruitment state')
    local expected = identity()
    assert(#bytes == #expected + 8 + censusWords * 4 and bytes:sub(1, #expected) == expected,
      'AIC Tactics: this save requires its original AIC configuration and module version')
    local offset = #expected + 1
    local tick, valid = number(bytes, offset), number(bytes, offset + 4)
    assert(valid <= 1, 'AIC Tactics: invalid saved defense census')
    local counts, total = {}, 0
    for index = 0, censusWords - 1 do
      local count = number(bytes, offset + 8 + index * 4)
      assert(count <= 2500 and (index >= 80 and index % 80 ~= 0 or count == 0),
        'AIC Tactics: invalid saved defender count')
      counts[index + 1], total = count, total + count
    end
    assert(total <= 2500, 'AIC Tactics: saved defender count exceeds the native pool')
    return tick, valid, counts
  end
  local function restore(bytes)
    local tick, valid, counts = validate(bytes)
    -- Validate the entire payload before writing any state.
    core.writeInteger(native.defenseCensusValid, 0)
    for index, count in ipairs(counts) do core.writeInteger(native.defenseTypeCounts + (index - 1) * 4, count) end
    core.writeInteger(native.defenseCensusTick, tick >= 2147483648 and tick - 4294967296 or tick)
    core.writeInteger(native.defenseCensusValid, valid)
  end
  local function initialize()
    assert(not active() or core.readInteger(0x1FE7DA8) == 0,
      'AIC Tactics: this old save has no policy state; start a new match with these parameters')
    core.writeInteger(native.defenseCensusValid, 0)
    core.setMemory(native.defenseTypeCounts, 0, censusWords * 4)
    core.writeInteger(native.defenseCensusTick, 0)
  end
  return {
    capture = capture, validate = validate, restore = restore,
    callbacks = {
      initialize = initialize,
      serialize = function(self, handle) handle:put(path, capture()) end,
      deserialize = function(self, handle)
        if handle:exists(path) then restore(handle:get(path)) else initialize() end
      end,
    },
  }
end
return M
