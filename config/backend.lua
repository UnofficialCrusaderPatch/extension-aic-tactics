local personality = require('config.personality')
local lordHunt = require('config.lord-hunt')
local patrolArrays = require('config.attacks').arrays
local M = {}
local huntArrays = {'lordHuntRetarget', 'lordHuntSiege', 'lordHuntSwitch', 'lordHuntShare', 'lordHuntSplit'}
-- Per-AI ally settings: one word per setting and character (src/allies.cpp allySettings).
local allyWords = #require('config.allies').settings

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
    assert(not multiplayerLocked and core.readInteger(native.configurationLocked) == 0,
      'AIC Tactics: personality changes require a fresh game process')
    local tick = native.admissionTick()
    assert(core.readInteger(tick) == 0,
      'AIC Tactics: personality changes require a fresh game process')
  end
  local function anyPolicyActive()
    -- Engineer counting has a module switch, but no per-AIC field.
    if core.readInteger(native.safePlacementFallback) ~= 0 then return true end
    if core.readInteger(native.engineerRoleFallback) ~= 0 then return true end
    if core.readInteger(native.siegePaymentFallback) ~= 0 then return true end
    if core.readInteger(native.siegeHarassFallback) ~= 0 then return true end
    if core.readInteger(native.largeSiegeFallback) ~= 0 then return true end
    if core.readInteger(native.allyFallback) ~= 0 or core.readInteger(native.allyFallback + 4) ~= 0 then
      return true
    end
    for ai = 1, 16 do
      for word = 0, allyWords - 1 do
        if core.readInteger(native.allySettings + (ai * allyWords + word) * 4) ~= 0 then return true end
      end
      if core.readInteger(native.siegePaymentPolicy + ai * 4) ~= 0 then return true end
      if core.readInteger(native.siegeHarassPolicy + ai * 4) ~= 0
          or core.readInteger(native.siegeHarassMinimum + ai * 4) ~= 0 then return true end
      if core.readInteger(native.largeSiegePolicy + ai * 4) ~= 0
          or core.readInteger(native.siegeForceMaximum + ai * 4) ~= 0 then return true end
      for _, name in ipairs(patrolArrays) do
        if core.readInteger(native[name] + ai * 4) ~= 0 then return true end
      end
      for _, name in ipairs(huntArrays) do
        if core.readInteger(native[name] + ai * 4) ~= 0 then return true end
      end
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
    assert(envelope.schemaVersion == 10, 'Unsupported personality schema')
    local compiled, targeting = envelope.recruitment, envelope.targeting
    assert(compiled.schemaVersion == 3, 'Unsupported recruitment schema')
    assert(targeting.schemaVersion == 2, 'Unsupported targeting schema')
    local previous = readRecord(ai)
    local previousPayment = core.readInteger(native.siegePaymentPolicy + ai * 4)
    local previousHarass = core.readInteger(native.siegeHarassPolicy + ai * 4)
    local previousMinimum = core.readInteger(native.siegeHarassMinimum + ai * 4)
    local previousLarge = core.readInteger(native.largeSiegePolicy + ai * 4)
    local previousForceMaximum = core.readInteger(native.siegeForceMaximum + ai * 4)
    local previousPatrol = {}
    for index, name in ipairs(patrolArrays) do
      previousPatrol[index] = core.readInteger(native[name] + ai * 4)
    end
    local previousAllies = {}
    for word = 1, allyWords do
      previousAllies[word] = core.readInteger(native.allySettings + (ai * allyWords + word - 1) * 4)
    end
    local previousHunt = {}
    for index, name in ipairs(huntArrays) do previousHunt[index] = core.readInteger(native[name] + ai * 4) end
    local attacks, hunt, allies = envelope.attacks, envelope.lordHunt, envelope.allies
    local changesHunt = false
    for index = 1, #huntArrays do
      if hunt[index] ~= 0 or previousHunt[index] ~= 0 then changesHunt = true end
    end
    local alliesChange = false
    for word = 1, allyWords do
      if allies[word] ~= 0 or previousAllies[word] ~= 0 then alliesChange = true end
    end
    local changesPolicy = (attacks[10] or 0) ~= 0 or previous[88] ~= 0 or changesHunt or alliesChange or compiled.mode ~= 0 or previous[1] ~= 0
      or targeting.policy ~= 0 or targeting.commitment ~= 0 or targeting.activation ~= 0
      or previous[73] ~= 0 or previous[74] ~= 0 or previous[75] ~= 0
      or envelope.preparation ~= 0 or previous[80] ~= 0
      or envelope.raids[1] ~= 0 or previous[81] ~= 0
      or envelope.siege[1] ~= 0 or previous[87] ~= 0
      or envelope.siege[2] ~= 0 or previousPayment ~= 0
      or envelope.siege[3] ~= 0 or previousHarass ~= 0
      or envelope.siege[4] ~= 0 or previousMinimum ~= 0
      or envelope.siege[5] ~= 0 or previousLarge ~= 0
      or envelope.siege[6] ~= 0 or previousForceMaximum ~= 0
    for index = 1, #patrolArrays do
      changesPolicy = changesPolicy or attacks[index] ~= 0 or previousPatrol[index] ~= 0
    end
    if multiplayerLocked or changesPolicy or anyPolicyActive() then admission() end
    local recordActive = personality.recordActive(authored)
    local grace = compiled.initialDefenseTicks
    if recordActive and grace == nil then
      if not native.game.initialDefenseTicks then
        require('native-bindings').requireBindings(native.game, {'Grace'})
      end
      grace = core.readInteger(native.game.initialDefenseTicks)
    end
    if compiled.mode == 1 then native.preflight() end
    if compiled.mode == 1 and compiled.defenseComposition == 1 then native.preflightComposition() end
    local needsCombat = targeting.policy ~= 0 or targeting.commitment ~= 0 or targeting.activation ~= 0
      or envelope.preparation ~= 0 or envelope.raids[1] ~= 0
      or envelope.siege[3] == 1
      -- Extra siege-defence groups are handed back from the per-AI attack update.
      or attacks[9] > 1
    if compiled.mode == 1 then
      for _, row in ipairs(compiled.conditions) do
        if row.requiredFacts % 2 == 1 or row.forbiddenFacts % 2 == 1 then needsCombat = true end
      end
    end
    if needsCombat then native.preflightCombat() end
    if envelope.raids[1] ~= 0 then native.preflightRaids() end
    if envelope.siege[5] == 1 then
      native.preflightLargerSiegeForce()
      if native.unavailableFeatures.LargerSiegeForce and authored.SafeSiegePlacement == nil
          and envelope.siege[2] ~= 1 and envelope.siege[3] ~= 1 then
        envelope.siege[1] = 0
      end
    end
    if envelope.siege[1] == 1 then native.preflightSafePlacement() end
    if envelope.siege[2] == 1 then native.preflightSiegePayment() end
    if envelope.siege[3] == 1 then native.preflightSiegeHarassment() end
    if attacks[1] == 2 then native.preflightAttackPatrols() end
    if lordHunt.enables(hunt) then native.preflightLordHunt() end
    if attacks[7] == 2 then native.preflightSiegeScreen() end
    if allies[1] == 2 then native.preflightAllyRelief() end
    if allies[2] == 2 then native.preflightAllyGoods() end
    if allies[8] == 2 then native.preflightAllyGates() end
    if allies[9] == 2 then native.preflightAllyReliefKeep() end
    if attacks[10] == 2 then native.preflightTowerCrew() end
    -- Per-AI values of a feature that is off are stored as unset.
    local off = native.unavailableFeatures
    if off.Recruitment then compiled.mode = 0 end
    if off.Recruitment or off.Composition then compiled.defenseComposition = 0 end
    if off.TowerCrew then attacks[10] = 0 end
    if off.Combat then
      for _, row in ipairs(compiled.conditions) do
        -- A recruitment rule using threat observation requires the combat census.
        if row.requiredFacts % 2 == 1 or row.forbiddenFacts % 2 == 1 then compiled.mode = 0 end
      end
      targeting.policy, targeting.commitment, targeting.activation, envelope.preparation = 0, 0, 0, 0
    end
    if off.Raids then envelope.raids[1] = 0 end
    for index, key in pairs({'SafePlacement', 'SiegePayment', 'SiegeHarassment', [5] = 'LargerSiegeForce'}) do
      if off[key] then envelope.siege[index] = 0 end
    end
    if off.AttackPatrols then attacks[1] = 0 end
    if off.SiegeScreen then attacks[7] = 0 end
    if off.Combat or off.SiegeScreen then attacks[9] = 0 end
    if off.LordHunt then hunt[1], hunt[4] = 0, 0 end
    if off.AllyRelief then allies[1] = 0 end
    if off.AllyGoods then allies[2] = 0 end
    if off.AllyGates then allies[8] = 0 end
    if off.AllyReliefKeep then allies[9] = 0 end
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
    words[#words + 1] = grace or 0
    words[#words + 1] = targeting.policy
    words[#words + 1] = targeting.commitment
    words[#words + 1] = targeting.activation
    for _, value in ipairs(targeting.rules) do words[#words + 1] = value end
    words[#words + 1] = envelope.preparation
    for _, value in ipairs(envelope.raids) do words[#words + 1] = value end
    words[#words + 1] = envelope.siege[1]
    -- Native neighbours may be visited before or after the first opt-in, when
    -- provider admission starts covering every update. Keep their unused
    -- storage identical to an unvisited record for save/MP/replay identity.
    -- The provider retains authored values for subsequent partial edits.
    if not recordActive then
      for index = 1, #words do words[index] = 0 end
    end
    words[88] = attacks[10] or 0
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
      if attacks[1] == 2 then native.activateAttackPatrols() end
      if lordHunt.enables(hunt) then native.activateLordHunt() end
      if attacks[7] == 2 then native.activateSiegeScreen() end
      if allies[1] == 2 then native.activateAllyRelief() end
      if allies[2] == 2 then native.activateAllyGoods() end
      if allies[8] == 2 then native.activateAllyGates() end
      if attacks[10] == 2 then native.activateTowerCrew() end
      writeRecord(ai, words)
      core.writeInteger(native.siegePaymentPolicy + ai * 4, envelope.siege[2])
      core.writeInteger(native.siegeHarassPolicy + ai * 4, envelope.siege[3])
      core.writeInteger(native.siegeHarassMinimum + ai * 4, envelope.siege[4])
      core.writeInteger(native.largeSiegePolicy + ai * 4, envelope.siege[5])
      core.writeInteger(native.siegeForceMaximum + ai * 4, envelope.siege[6])
      for index, name in ipairs(patrolArrays) do core.writeInteger(native[name] + ai * 4, attacks[index]) end
      for index, name in ipairs(huntArrays) do core.writeInteger(native[name] + ai * 4, hunt[index]) end
      for word = 1, allyWords do
        core.writeInteger(native.allySettings + (ai * allyWords + word - 1) * 4, allies[word])
      end
    end,
      rollback = function()
        writeRecord(ai, previous)
        core.writeInteger(native.siegePaymentPolicy + ai * 4, previousPayment)
        core.writeInteger(native.siegeHarassPolicy + ai * 4, previousHarass)
        core.writeInteger(native.siegeHarassMinimum + ai * 4, previousMinimum)
        core.writeInteger(native.largeSiegePolicy + ai * 4, previousLarge)
        core.writeInteger(native.siegeForceMaximum + ai * 4, previousForceMaximum)
        for index, name in ipairs(patrolArrays) do
          core.writeInteger(native[name] + ai * 4, previousPatrol[index])
        end
        for index, name in ipairs(huntArrays) do core.writeInteger(native[name] + ai * 4, previousHunt[index]) end
        for word = 1, allyWords do
          core.writeInteger(native.allySettings + (ai * allyWords + word - 1) * 4, previousAllies[word])
        end
      end}
  end}
end
return M
