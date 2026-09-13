local M = {}
local path = 'recruitment-state.bin'
local header = 'AICTACT\007'
local configurationBytes, aicBytes, censusWords = 16 * 344, 16 * 676, 9 * 80

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

function M.new(native, legacyInterval, fingerprint)
  assert(type(fingerprint) == 'string' and #fingerprint == 64, 'AIC Tactics: missing package fingerprint')
  assert(native.configurationSize == 344, 'AIC Tactics: incompatible save ABI')
  local combat = require('combat-state').new(native)
  local army = require('army-state').new(native)
  local raids = require('raid-state').new(native)
  local nativeIntegrity = core.exposeCode(native.captureIntegrity, 1, 0)
  local observeBoundary = core.exposeCode(native.observeIntegrityBoundary, 1, 0)
  local boundaryIntegrity = core.exposeCode(native.captureBoundaryIntegrity, 0, 0)
  local observed=false
  local function digest()
    local first, second = core.readInteger(native.integrityDigest), core.readInteger(native.integrityDigest + 4)
    if first < 0 then first = first + 4294967296 end
    if second < 0 then second = second + 4294967296 end
    return string.format('aic-tactics-word-digest-v1-%08x%08x', first, second)
  end
  local function active()
    for ai = 1, 16 do
      local address = native.configuration + ai * 344
      if core.readInteger(address) ~= 0 or core.readInteger(address + 288) ~= 0
          or core.readInteger(address + 292) ~= 0 or core.readInteger(address + 296) ~= 0
          or core.readInteger(address + 316) ~= 0 or core.readInteger(address + 320) ~= 0 then return true end
    end
    return false
  end
  local function identity()
    return header .. fingerprint .. word(legacyInterval and 1 or 0) .. word(core.readInteger(native.game.initialDefenseTicks))
      .. word(core.readInteger(native.legacyTargetPolicy))
      .. core.readString(native.configuration + 344, configurationBytes)
      .. core.readString(native.game.aicRecords, aicBytes)
  end
  local function capture()
    local result = {identity(), word(core.readInteger(native.defenseCensusTick)),
      word(core.readInteger(native.defenseCensusValid))}
    for index = 0, censusWords - 1 do
      result[#result + 1] = word(core.readInteger(native.defenseTypeCounts + index * 4))
    end
    for _, value in ipairs(combat.capture()) do result[#result + 1] = word(value) end
    for _, value in ipairs(army.capture()) do result[#result + 1] = word(value) end
    for _, value in ipairs(raids.capture()) do result[#result + 1] = word(value) end
    return table.concat(result)
  end
  local function validate(bytes)
    assert(type(bytes) == 'string', 'AIC Tactics: missing saved recruitment state')
    local expected = identity()
    assert(#bytes == #expected + 8 + (censusWords + combat.wordCount + army.wordCount + raids.wordCount) * 4 and bytes:sub(1, #expected) == expected,
      'AIC Tactics: this save requires its original AIC configuration and module version')
    local offset = #expected + 1
    local tick, valid = number(bytes, offset), number(bytes, offset + 4)
    assert(valid <= 1, 'AIC Tactics: invalid saved defense census')
    local counts, total = {}, 0
    for index = 0, censusWords - 1 do
      local count = number(bytes, offset + 8 + index * 4)
      assert(count <= native.game.unitCapacity and (index >= 80 and index % 80 ~= 0 or count == 0),
        'AIC Tactics: invalid saved defender count')
      counts[index + 1], total = count, total + count
    end
    assert(total <= native.game.unitCapacity, 'AIC Tactics: saved defender count exceeds the native pool')
    local combatValues = {}
    for index = 0, combat.wordCount - 1 do
      combatValues[index + 1] = number(bytes, offset + 8 + (censusWords + index) * 4)
    end
    combat.validate(combatValues)
    local armyValues = {}
    for index = 0, army.wordCount - 1 do
      armyValues[index + 1] = number(bytes, offset + 8 + (censusWords + combat.wordCount + index) * 4)
    end
    army.validate(armyValues)
    local raidValues = {}
    for index = 0, raids.wordCount - 1 do
      raidValues[index + 1] = number(bytes, offset + 8 + (censusWords + combat.wordCount + army.wordCount + index) * 4)
    end
    raids.validate(raidValues)
    return tick, valid, counts, combatValues, armyValues, raidValues
  end
  local function restore(bytes)
    local tick, valid, counts, combatValues, armyValues, raidValues = validate(bytes)
    -- Validate the entire payload before writing any state.
    core.writeInteger(native.defenseCensusValid, 0)
    for index, count in ipairs(counts) do core.writeInteger(native.defenseTypeCounts + (index - 1) * 4, count) end
    core.writeInteger(native.defenseCensusTick, tick >= 2147483648 and tick - 4294967296 or tick)
    core.writeInteger(native.defenseCensusValid, valid)
    combat.restore(combatValues)
    army.restore(armyValues)
    raids.restore(raidValues)
  end
  local function validateAbsent(kind)
    assert(not active() or kind == 'map',
      'AIC Tactics: this old save has no policy state; start a new match with these parameters')
  end
  local function initialize(self, context)
    validateAbsent(context and context.kind)
    core.writeInteger(native.defenseCensusValid, 0)
    core.setMemory(native.defenseTypeCounts, 0, censusWords * 4)
    core.writeInteger(native.defenseCensusTick, 0)
    combat.initialize()
    army.initialize()
    raids.initialize()
  end
  return {
    capture = capture, validate = validate, restore = restore, identity = identity,
    callbacks = {
      isRequired = active,
      initialize = initialize,
      serialize = function(self, handle) handle:put(path, capture()) end,
      capture = function(self, handle) handle:put(path, capture()) end,
      integrity = function()
        nativeIntegrity(legacyInterval and 1 or 0)
        return digest()
      end,
      observeBoundary = function()
        observeBoundary(legacyInterval and 1 or 0); observed=true
      end,
      boundaryIntegrity = function()
        assert(observed,'AIC Tactics: no observed replay boundary')
        boundaryIntegrity();return digest()
      end,
      validate = function(self, handle)
        if handle.required then
          assert(handle:exists(path), 'AIC Tactics: required policy state is missing from this save')
          validate(handle:get(path))
          return
        end
        if not active() then return end
        if handle:exists(path) then validate(handle:get(path)) else validateAbsent(handle.loadKind) end
      end,
      deserialize = function(self, handle)
        if active() and handle:exists(path) then restore(handle:get(path))
        else initialize(nil, {kind=handle.loadKind}) end
      end,
    },
  }
end
return M
