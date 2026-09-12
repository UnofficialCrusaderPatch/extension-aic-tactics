local M = {}

function M.new(native)
  local multiplayerLocked=false
  assert(native.configurationSize == 344, 'AIC Tactics: unsupported configuration ABI')
  local function readRecord(ai)
    local result = {}
    local address = native.configuration + ai * native.configurationSize
    for index = 0, 85 do result[index + 1] = core.readInteger(address + index * 4) end
    return result
  end
  local function writeRecord(ai, words)
    local address = native.configuration + ai * native.configurationSize
    for index = 1, #words do core.writeInteger(address + (index - 1) * 4, words[index]) end
  end
  local function admission()
    assert(not multiplayerLocked and core.readInteger(native.configurationLocked) == 0 and core.readInteger(0x1FE7DA8) == 0,
      'AIC Tactics: personality changes require a fresh game process')
  end
  local function anyPolicyActive()
    for ai = 1, 16 do
      local address = native.configuration + ai * native.configurationSize
      for _, offset in ipairs({0,288,292,296,316,320}) do
        if core.readInteger(address + offset) ~= 0 then return true end
      end
    end
    return false
  end
  return {freezeMultiplayer = function() multiplayerLocked=true end,
    multiplayerLocked = function() return multiplayerLocked end,
    prepare = function(ai, authored, envelope)
    assert(ai >= 1 and ai <= 16 and ai == math.floor(ai), 'Invalid AI character')
    assert(envelope.schemaVersion == 4, 'Unsupported personality schema')
    local compiled, targeting = envelope.recruitment, envelope.targeting
    assert(compiled.schemaVersion == 3, 'Unsupported recruitment schema')
    assert(targeting.schemaVersion == 2, 'Unsupported targeting schema')
    local previous = readRecord(ai)
    local changesPolicy = compiled.mode ~= 0 or previous[1] ~= 0
      or targeting.policy ~= 0 or targeting.commitment ~= 0 or targeting.activation ~= 0
      or previous[73] ~= 0 or previous[74] ~= 0 or previous[75] ~= 0
      or envelope.preparation ~= 0 or previous[80] ~= 0
      or envelope.raids[1] ~= 0 or previous[81] ~= 0
    if multiplayerLocked or changesPolicy or anyPolicyActive() then admission() end
    if compiled.mode == 1 then native.preflight() end
    if compiled.mode == 1 and compiled.defenseComposition == 1 then native.preflightComposition() end
    local needsCombat = targeting.policy ~= 0 or targeting.commitment ~= 0 or targeting.activation ~= 0
      or envelope.preparation ~= 0 or envelope.raids[1] ~= 0
    if compiled.mode == 1 then
      for _, row in ipairs(compiled.conditions) do
        if row.requiredFacts % 2 == 1 or row.forbiddenFacts % 2 == 1 then needsCombat = true end
      end
    end
    if needsCombat then native.preflightCombat() end
    if envelope.raids[1] ~= 0 then native.preflightRaids() end
    if targeting.policy ~= 0 or targeting.commitment ~= 0 then native.preflightTargets() end
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
    return {commit = function()
      if multiplayerLocked or changesPolicy or anyPolicyActive() then admission() end
      if compiled.mode == 1 then native.activate() end
      if compiled.mode == 1 and compiled.defenseComposition == 1 then native.activateComposition() end
      if needsCombat then native.activateCombat() end
      if envelope.raids[1] ~= 0 then native.activateRaids() end
      writeRecord(ai, words)
    end,
      rollback = function() writeRecord(ai, previous) end}
  end}
end
return M
