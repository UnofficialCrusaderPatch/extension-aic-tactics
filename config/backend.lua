local M = {}

function M.new(native)
  assert(native.configurationSize == 288, 'AIC Tactics: unsupported configuration ABI')
  local function readRecord(ai)
    local result = {}
    local address = native.configuration + ai * native.configurationSize
    for index = 0, 71 do result[index + 1] = core.readInteger(address + index * 4) end
    return result
  end
  local function writeRecord(ai, words)
    local address = native.configuration + ai * native.configurationSize
    for index = 1, #words do core.writeInteger(address + (index - 1) * 4, words[index]) end
  end
  local function admission()
    assert(core.readInteger(native.configurationLocked) == 0 and core.readInteger(0x1FE7DA8) == 0,
      'AIC Tactics: personality changes require a fresh game process')
  end
  return {prepare = function(ai, authored, compiled)
    assert(ai >= 1 and ai <= 16 and ai == math.floor(ai), 'Invalid AI character')
    assert(compiled.schemaVersion == 3, 'Unsupported recruitment schema')
    local previous = readRecord(ai)
    local changesPolicy = compiled.mode ~= 0 or previous[1] ~= 0
    if changesPolicy then admission() end
    if compiled.mode == 1 then native.preflight() end
    if compiled.mode == 1 and compiled.defenseComposition == 1 then native.preflightComposition() end
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
    return {commit = function()
      if changesPolicy then admission() end
      if compiled.mode == 1 then native.activate() end
      if compiled.mode == 1 and compiled.defenseComposition == 1 then native.activateComposition() end
      writeRecord(ai, words)
    end,
      rollback = function() writeRecord(ai, previous) end}
  end}
end
return M
