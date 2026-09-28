local personality = require('config.personality')
local M = {}

function M.new(native)
  local multiplayerLocked=false
  assert(native.configurationSize == 352, 'AIC Tactics: unsupported configuration ABI')
  local function readRecord(ai)
    local result = {}
    local address = native.configuration + ai * native.configurationSize
    for index = 0, 87 do result[index + 1] = core.readInteger(address + index * 4) end
    return result
  end
  local function writeRecord(ai, words)
    local address = native.configuration + ai * native.configurationSize
    for index = 1, #words do core.writeInteger(address + (index - 1) * 4, words[index]) end
  end
  local function admission()
    assert(not multiplayerLocked and core.readInteger(native.configurationLocked) == 0 and core.readInteger(native.game.gameTick) == 0,
      'AIC Tactics: personality changes require a fresh game process')
  end
  local function anyPolicyActive()
    if core.readInteger(native.safePlacementFallback) ~= 0 then return true end
    if core.readInteger(native.engineerRoleFallback) ~= 0 then return true end
    if core.readInteger(native.siegePaymentFallback) ~= 0 then return true end
    if core.readInteger(native.siegeHarassFallback) ~= 0 then return true end
    if core.readInteger(native.largeSiegeFallback) ~= 0 then return true end
    for ai = 1, 16 do
      if core.readInteger(native.siegePaymentPolicy + ai * 4) ~= 0 then return true end
      if core.readInteger(native.siegeHarassPolicy + ai * 4) ~= 0
          or core.readInteger(native.siegeHarassMinimum + ai * 4) ~= 0 then return true end
      if core.readInteger(native.largeSiegePolicy + ai * 4) ~= 0
          or core.readInteger(native.siegeForceMaximum + ai * 4) ~= 0 then return true end
      local address = native.configuration + ai * native.configurationSize
      for _, offset in ipairs({0,288,292,296,316,320,344,348}) do
        if core.readInteger(address + offset) ~= 0 then return true end
      end
    end
    return false
  end
  return {freezeMultiplayer = function() multiplayerLocked=true end,
    policyActive = anyPolicyActive,
    multiplayerLocked = function() return multiplayerLocked end,
    prepare = function(ai, authored, envelope)
    assert(ai >= 1 and ai <= 16 and ai == math.floor(ai), 'Invalid AI character')
    assert(envelope.schemaVersion == 9, 'Unsupported personality schema')
    local compiled, targeting = envelope.recruitment, envelope.targeting
    assert(compiled.schemaVersion == 3, 'Unsupported recruitment schema')
    assert(targeting.schemaVersion == 2, 'Unsupported targeting schema')
    local previous = readRecord(ai)
    local previousPayment = core.readInteger(native.siegePaymentPolicy + ai * 4)
    local previousHarass = core.readInteger(native.siegeHarassPolicy + ai * 4)
    local previousMinimum = core.readInteger(native.siegeHarassMinimum + ai * 4)
    local previousLarge = core.readInteger(native.largeSiegePolicy + ai * 4)
    local previousForceMaximum = core.readInteger(native.siegeForceMaximum + ai * 4)
    local changesPolicy = compiled.mode ~= 0 or previous[1] ~= 0
      or targeting.policy ~= 0 or targeting.commitment ~= 0 or targeting.activation ~= 0
      or previous[73] ~= 0 or previous[74] ~= 0 or previous[75] ~= 0
      or envelope.preparation ~= 0 or previous[80] ~= 0
      or envelope.raids[1] ~= 0 or previous[81] ~= 0
      or envelope.siege[1] ~= 0 or previous[87] ~= 0
      or envelope.roles ~= 0 or previous[88] ~= 0
      or envelope.siege[2] ~= 0 or previousPayment ~= 0
      or envelope.siege[3] ~= 0 or previousHarass ~= 0
      or envelope.siege[4] ~= 0 or previousMinimum ~= 0
      or envelope.siege[5] ~= 0 or previousLarge ~= 0
      or envelope.siege[6] ~= 0 or previousForceMaximum ~= 0
    if multiplayerLocked or changesPolicy or anyPolicyActive() then admission() end
    if compiled.mode == 1 then native.preflight() end
    if compiled.mode == 1 and compiled.defenseComposition == 1 then native.preflightComposition() end
    local needsCombat = targeting.policy ~= 0 or targeting.commitment ~= 0 or targeting.activation ~= 0
      or envelope.preparation ~= 0 or envelope.raids[1] ~= 0
      or envelope.siege[3] == 1
    if compiled.mode == 1 then
      for _, row in ipairs(compiled.conditions) do
        if row.requiredFacts % 2 == 1 or row.forbiddenFacts % 2 == 1 then needsCombat = true end
      end
    end
    if needsCombat then native.preflightCombat() end
    if envelope.raids[1] ~= 0 then native.preflightRaids() end
    if targeting.policy ~= 0 or targeting.commitment ~= 0 then native.preflightTargets() end
    if envelope.siege[1] == 1 then native.preflightSafePlacement() end
    if envelope.siege[2] == 1 then native.preflightSiegePayment() end
    if envelope.siege[3] == 1 then native.preflightSiegeHarassment() end
    if envelope.siege[5] == 1 then native.preflightLargerSiegeForce() end
    if envelope.roles == 1 then native.preflightEngineerRoles() end
    local words = {compiled.mode, #compiled.conditions}
    for index = 1, 8 do
      local row = compiled.conditions[index]
      for _, value in ipairs(row and {row.strength, row.requiredFacts, row.forbiddenFacts,
          row.weights[1], row.weights[2], row.weights[3], row.weights[4]} or {0,0,0,0,0,0,0}) do
        words[#words + 1] = value
      end
    end
    for strength = 1, 3 do
      for role = 1, 4 do
        words[#words + 1] = compiled.baseRows[strength] and compiled.baseRows[strength][role] or 0
      end
    end
    words[#words + 1] = compiled.defenseComposition
    words[#words + 1] = compiled.initialDefenseTicks
    words[#words + 1] = targeting.policy
    words[#words + 1] = targeting.commitment
    words[#words + 1] = targeting.activation
    for _, value in ipairs(targeting.rules) do words[#words + 1] = value end
    words[#words + 1] = envelope.preparation
    for _, value in ipairs(envelope.raids) do words[#words + 1] = value end
    words[#words + 1] = envelope.siege[1]
    words[#words + 1] = envelope.roles
    -- Native neighbours may be visited before or after the first opt-in, when
    -- provider admission starts covering every update. Keep their unused
    -- storage identical to an unvisited record for save/MP/replay identity.
    -- The provider retains authored values for subsequent partial edits.
    if not personality.active(authored) then
      for index = 1, #words do words[index] = 0 end
    end
    return {commit = function()
      if multiplayerLocked or changesPolicy or anyPolicyActive() then admission() end
      if compiled.mode == 1 then native.activate() end
      if compiled.mode == 1 and compiled.defenseComposition == 1 then native.activateComposition() end
      if needsCombat then native.activateCombat() end
      if envelope.raids[1] ~= 0 then native.activateRaids() end
      if envelope.siege[1] == 1 then native.activateSafePlacement() end
      if envelope.siege[2] == 1 then native.activateSiegePayment() end
      if envelope.siege[3] == 1 then native.activateSiegeHarassment() end
      if envelope.siege[5] == 1 then native.activateLargerSiegeForce() end
      if envelope.roles == 1 then native.activateEngineerRoles() end
      writeRecord(ai, words)
      core.writeInteger(native.siegePaymentPolicy + ai * 4, envelope.siege[2])
      core.writeInteger(native.siegeHarassPolicy + ai * 4, envelope.siege[3])
      core.writeInteger(native.siegeHarassMinimum + ai * 4, envelope.siege[4])
      core.writeInteger(native.largeSiegePolicy + ai * 4, envelope.siege[5])
      core.writeInteger(native.siegeForceMaximum + ai * 4, envelope.siege[6])
    end,
      rollback = function()
        writeRecord(ai, previous)
        core.writeInteger(native.siegePaymentPolicy + ai * 4, previousPayment)
        core.writeInteger(native.siegeHarassPolicy + ai * 4, previousHarass)
        core.writeInteger(native.siegeHarassMinimum + ai * 4, previousMinimum)
        core.writeInteger(native.largeSiegePolicy + ai * 4, previousLarge)
        core.writeInteger(native.siegeForceMaximum + ai * 4, previousForceMaximum)
      end}
  end}
end
return M
