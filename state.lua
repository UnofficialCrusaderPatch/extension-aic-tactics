local M = {}
local path = 'recruitment-state.bin'
local header = 'AICTACT\014'
local huntArrays = {'lordHuntRetarget', 'lordHuntSiege', 'lordHuntSwitch', 'lordHuntShare', 'lordHuntSplit'}
local huntFallbacks = {'lordHuntRetargetFallback', 'lordHuntSiegeFallback', 'lordHuntSwitchFallback',
  'lordHuntShareFallback', 'lordHuntSplitFallback'}
local configurationBytes, aicBytes, censusWords, harassWords = 16 * 352, 16 * 676, 9 * 80, 9 * 9
-- Per-AI ally settings: one word per setting and character.
local allyBytes = #require('config.allies').settings * 4

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

function M.new(native, legacyInterval, version)
  assert(type(version) == 'string', 'AIC Tactics: missing module version')
  assert(native.configurationSize == 352, 'AIC Tactics: incompatible save ABI')
  assert(native.siegeHarassPlanSize == 36, 'AIC Tactics: incompatible siege plan ABI')
  assert(type(native.captureIntegrityWithIdentity) == 'number'
    and type(native.observeIntegrityBoundaryWithIdentity) == 'number', 'AIC Tactics: incompatible integrity ABI')
  local combat = require('combat-state').new(native)
  local army = require('army-state').new(native)
  local raids = require('raid-state').new(native)
  local groups = require('group-state').new(native)
  local hunts = require('lord-hunt-state').new(native)
  local routes = require('patrol-state').new(native)
  local attacks = require('config.attacks')
  local patrolArrays, routesFormation = attacks.arrays, attacks.formations.Routes
  local allies = require('ally-state').new(native)
  local nativeIntegrity = core.exposeCode(native.captureIntegrityWithIdentity, 4, 0)
  local observeBoundary = core.exposeCode(native.observeIntegrityBoundaryWithIdentity, 4, 0)
  local boundaryIntegrity = core.exposeCode(native.captureBoundaryIntegrity, 0, 0)
  local observed=false
  local function digest()
    local first, second = core.readInteger(native.integrityDigest), core.readInteger(native.integrityDigest + 4)
    if first < 0 then first = first + 4294967296 end
    if second < 0 then second = second + 4294967296 end
    return string.format('aic-tactics-word-digest-v1-%08x%08x', first, second)
  end
  local function legacyIdentity()
    local value = legacyInterval
    if type(value) == 'function' then
      local target
      value, target = value()
      assert(value ~= nil, target)
      return value and 1 or 0, target
    end
    return value and 1 or 0, core.readInteger(native.legacyTargetPolicy)
  end
  local function observe(callback)
    local interval, target = legacyIdentity()
    callback(interval, core.readInteger(native.game.initialDefenseTicks), target, native.game.aicRecords)
  end
  local function active()
    -- The module-level engineer switch participates in match identity.
    if core.readInteger(native.safePlacementFallback) ~= 0 then return true end
    if core.readInteger(native.engineerRoleFallback) ~= 0 then return true end
    if core.readInteger(native.siegePaymentFallback) ~= 0 then return true end
    if core.readInteger(native.siegeHarassFallback) ~= 0 then return true end
    if core.readInteger(native.largeSiegeFallback) ~= 0 then return true end
    -- Lord hunt keeps detachments and their census in this section.
    if core.readInteger(native.lordHuntRetargetFallback) ~= 0
        or core.readInteger(native.lordHuntShareFallback) ~= 0 then return true end
    -- Extra patrol and siege-defence groups and patrol routes keep their records in this section.
    if core.readInteger(native.patrolExtraGroupsFallback) ~= 0
        or core.readInteger(native.siegeScreenExtraGroupsFallback) ~= 0
        or core.readInteger(native.patrolFormationFallback) == routesFormation then return true end
    -- Ally help keeps gifts and relief timers in this section.
    if core.readInteger(native.allyFallback) ~= 0 or core.readInteger(native.allyFallback + 4) ~= 0 then
      return true
    end
    if core.readString(native.allySettings + allyBytes, 16 * allyBytes) ~= string.rep('\0', 16 * allyBytes) then
      return true
    end
    for ai = 1, 16 do
      for _, name in ipairs(huntArrays) do
        if core.readInteger(native[name] + ai * 4) ~= 0 then return true end
      end
      if core.readInteger(native.patrolExtraGroups + ai * 4) > 1
          or core.readInteger(native.siegeScreenExtraGroups + ai * 4) > 1
          or core.readInteger(native.patrolFormation + ai * 4) == routesFormation + 1 then return true end
      if core.readInteger(native.siegePaymentPolicy + ai * 4) ~= 0 then return true end
      if core.readInteger(native.siegeHarassPolicy + ai * 4) ~= 0
          or core.readInteger(native.siegeHarassMinimum + ai * 4) ~= 0 then return true end
      if core.readInteger(native.largeSiegePolicy + ai * 4) ~= 0
          or core.readInteger(native.siegeForceMaximum + ai * 4) ~= 0 then return true end
      local address = native.configuration + ai * 352
      if core.readInteger(address) ~= 0 or core.readInteger(address + 288) ~= 0
          or core.readInteger(address + 292) ~= 0 or core.readInteger(address + 296) ~= 0
          or core.readInteger(address + 316) ~= 0 or core.readInteger(address + 320) ~= 0
          or core.readInteger(address + 344) ~= 0 then return true end
    end
    return false
  end
  local function identity()
    local interval, target = legacyIdentity()
    return header .. word(interval) .. word(core.readInteger(native.game.initialDefenseTicks))
      .. word(target)
      .. word(core.readInteger(native.safePlacementFallback))
      .. word(core.readInteger(native.engineerRoleFallback))
      .. word(core.readInteger(native.siegePaymentFallback))
      .. core.readString(native.siegePaymentPolicy + 4, 16 * 4)
      .. word(core.readInteger(native.siegeHarassFallback))
      .. word(core.readInteger(native.siegeHarassMinimumFallback))
      .. core.readString(native.siegeHarassPolicy + 4, 16 * 4)
      .. core.readString(native.siegeHarassMinimum + 4, 16 * 4)
      .. word(core.readInteger(native.largeSiegeFallback))
      .. word(core.readInteger(native.siegeForceMaximumFallback))
      .. core.readString(native.largeSiegePolicy + 4, 16 * 4)
      .. core.readString(native.siegeForceMaximum + 4, 16 * 4)
      .. core.readString(native.configuration + 352, configurationBytes)
      .. core.readString(native.game.aicRecords, aicBytes)
  end
  -- Multiplayer peers must also agree on settings that keep no saved state.
  -- Those settings stay out of save admission: a save reloads with changed
  -- patrol settings and simply follows them from the next native decision.
  local function matchIdentity()
    local result = {identity(), version}
    for _, name in ipairs(patrolArrays) do result[#result + 1] = core.readString(native[name] + 4, 16 * 4) end
    for _, name in ipairs(patrolArrays) do result[#result + 1] = word(core.readInteger(native[name .. 'Fallback'])) end
    for _, name in ipairs(huntArrays) do result[#result + 1] = core.readString(native[name] + 4, 16 * 4) end
    for _, name in ipairs(huntFallbacks) do result[#result + 1] = word(core.readInteger(native[name])) end
    result[#result + 1] = core.readString(native.allySettings + allyBytes, 16 * allyBytes)
    result[#result + 1] = core.readString(native.allyFallback, allyBytes)
    -- Features another module left off install no hooks even for an AI that opts in.
    local off = {}
    for key in pairs(native.unavailableFeatures) do off[#off + 1] = key end
    table.sort(off)
    result[#result + 1] = table.concat(off, ',')
    return table.concat(result)
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
    for index = 0, harassWords - 1 do
      result[#result + 1] = word(core.readInteger(native.siegeHarassPlans + index * 4))
    end
    for _, value in ipairs(groups.capture()) do result[#result + 1] = word(value) end
    for _, value in ipairs(hunts.capture()) do result[#result + 1] = word(value) end
    for _, value in ipairs(routes.capture()) do result[#result + 1] = word(value) end
    for _, value in ipairs(allies.capture()) do result[#result + 1] = word(value) end
    return table.concat(result)
  end
  local function validate(bytes)
    assert(type(bytes) == 'string', 'AIC Tactics: missing saved recruitment state')
    local expected = identity()
    local size = #expected + 8 + (censusWords + combat.wordCount + army.wordCount + raids.wordCount + harassWords + groups.wordCount + hunts.wordCount + routes.wordCount + allies.wordCount) * 4
    -- Earlier saves carry a 64-character package fingerprint after the header.
    if #bytes == size + 64 and bytes:sub(#header + 1, #header + 64):match('^%x+$') then bytes = bytes:sub(1, #header) .. bytes:sub(#header + 65) end
    assert(#bytes == size and bytes:sub(1, #expected) == expected,
      'AIC Tactics: this save requires its original AIC configuration')
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
    local planValues = {}
    local planOffset = offset + 8 + (censusWords + combat.wordCount + army.wordCount + raids.wordCount) * 4
    for index = 0, harassWords - 1 do
      planValues[index + 1] = number(bytes, planOffset + index * 4)
    end
    for player = 0, 8 do
      local start = player * 9 + 1
      assert(planValues[start] <= 8 and planValues[start + 4] <= 7
          and planValues[start + 5] <= 1 and planValues[start + 6] <= 20
          and planValues[start + 7] <= 399 and planValues[start + 8] <= 399,
        'AIC Tactics: invalid saved siege harassment plan')
    end
    local groupValues = {}
    local groupOffset = planOffset + harassWords * 4
    for index = 0, groups.wordCount - 1 do
      groupValues[index + 1] = number(bytes, groupOffset + index * 4)
    end
    groups.validate(groupValues)
    local huntValues = {}
    local huntOffset = groupOffset + groups.wordCount * 4
    for index = 0, hunts.wordCount - 1 do
      huntValues[index + 1] = number(bytes, huntOffset + index * 4)
    end
    hunts.validate(huntValues, groupValues)
    local routeValues = {}
    local routeOffset = huntOffset + hunts.wordCount * 4
    for index = 0, routes.wordCount - 1 do
      routeValues[index + 1] = number(bytes, routeOffset + index * 4)
    end
    routes.validate(routeValues)
    local allyValues = {}
    local allyOffset = routeOffset + routes.wordCount * 4
    for index = 0, allies.wordCount - 1 do
      allyValues[index + 1] = number(bytes, allyOffset + index * 4)
    end
    allies.validate(allyValues)
    return tick, valid, counts, combatValues, armyValues, raidValues, planValues, groupValues, huntValues,
      routeValues, allyValues
  end
  local function restore(bytes)
    local tick, valid, counts, combatValues, armyValues, raidValues, planValues, groupValues, huntValues,
      routeValues, allyValues =
      validate(bytes)
    -- Validate the entire payload before writing any state.
    core.writeInteger(native.defenseCensusValid, 0)
    for index, count in ipairs(counts) do core.writeInteger(native.defenseTypeCounts + (index - 1) * 4, count) end
    core.writeInteger(native.defenseCensusTick, tick >= 2147483648 and tick - 4294967296 or tick)
    core.writeInteger(native.defenseCensusValid, valid)
    combat.restore(combatValues)
    army.restore(armyValues)
    raids.restore(raidValues)
    groups.restore(groupValues)
    hunts.restore(huntValues, groupValues)
    routes.restore(routeValues)
    allies.restore(allyValues)
    for index, value in ipairs(planValues) do
      core.writeInteger(native.siegeHarassPlans + (index - 1) * 4,
        value >= 2147483648 and value - 4294967296 or value)
    end
  end
  local function initialize()
    core.writeInteger(native.defenseCensusValid, 0)
    core.setMemory(native.defenseTypeCounts, 0, censusWords * 4)
    core.writeInteger(native.defenseCensusTick, 0)
    combat.initialize()
    army.initialize()
    raids.initialize()
    groups.initialize()
    hunts.initialize()
    routes.initialize()
    allies.initialize()
    core.setMemory(native.siegeHarassPlans, 0, harassWords * 4)
  end
  local function writeState(self, handle)
    if type(legacyInterval) == 'function' and legacyInterval() == nil and not active() then return end
    handle:put(path, capture())
  end
  return {
    capture = capture, validate = validate, restore = restore, identity = identity,
    matchIdentity = matchIdentity,
    callbacks = {
      isRequired = active,
      initialize = initialize,
      serialize = writeState,
      capture = writeState,
      integrity = function()
        observe(nativeIntegrity)
        return digest()
      end,
      observeBoundary = function()
        observe(observeBoundary); observed=true
      end,
      boundaryIntegrity = function()
        assert(observed,'AIC Tactics: no observed replay boundary')
        boundaryIntegrity();return digest()
      end,
      validate = function(self, handle)
        if handle.required then
          assert(handle:exists(path), 'AIC Tactics: required policy state is missing from this save')
        end
      end,
      deserialize = function(self, handle)
        -- A matching section is restored even with its features now off: the
        -- detached groups in it go back to their slots at the next AI update.
        -- A save made with other AIC settings loads with empty policy state.
        local ok, err = handle:exists(path) and pcall(validate, handle:get(path))
        if ok then restore(handle:get(path)) return end
        if err then log(INFO, 'AIC Tactics: saved policy state not restored (' .. tostring(err) .. '); starting with empty policy state') end
        initialize()
      end,
    },
  }
end
return M
