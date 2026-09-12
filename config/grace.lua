local M = {}
local expectedTicks = 4800
local binding, original
-- Legacy ai_recruitstate_initialtimer owns this same comparison when enabled.
-- Its surrounding native recruitment/RNG context agrees in SHC and Extreme.
local signature = '03 CB 03 CD 74 4D 0F BF 05 ? ? ? ? 99 F7 F9 B9 ? ? ? ? 89 54 24 14 E8 ? ? ? ? 81 3D ? ? ? ? ? ? ? ? 7D 08 8B 54 24 10 33 ED EB 43'

function M.resolveNative()
  if not binding then
    local site = require('native-context').find('recruitment context', signature)
    local candidate = {gameTick=core.readInteger(site + 32),
      rngState=core.readInteger(site + 17), rngValue=core.readInteger(site + 9),
      rngNext=site + 30 + core.readInteger(site + 26), initialDefenseTicks=site + 36}
    assert(candidate.rngValue == candidate.rngState + 2,
      'AIC Tactics: unsupported native RNG state layout')
    for _, value in pairs(candidate) do
      assert(type(value) == 'number' and value > 0,
        'AIC Tactics: invalid native recruitment operand')
    end
    original = {}
    for index = 0, 49 do original[index] = core.readByte(site + index) end
    binding = candidate
  end
  local result = {}
  for key, value in pairs(binding) do result[key] = value end
  return result
end

function M.validateMonths(value)
  assert(type(value) == 'number' and value == math.floor(value) and value >= 0 and value <= 30,
    'AIC Tactics: nativeInitialDefenseMonths must be an integer from 0 to 30')
  return value * 800
end

function M.preflight()
  -- The framework supplies its resolved configuration to native modules.
  -- The byte check alone cannot detect Legacy enabled with its original value.
  assert(type(configFinal) == 'table', 'AIC Tactics: resolved framework configuration is unavailable')
  for name, config in pairs(configFinal) do
    if name:match('^ucp2%-legacy%-') then
      local option = config.ai_recruitstate_initialtimer
      assert(not option or option.enabled ~= true,
        require('messages').legacyOff('ai_recruitstate_initialtimer', 'nativeInitialDefenseMonths'))
    end
  end
  local game = M.resolveNative()
  local site = game.initialDefenseTicks - 36
  for index = 0, 49 do
    if index < 36 or index >= 40 then
      assert(core.readByte(site + index) == original[index],
        'AIC Tactics: native recruitment context was modified; restart with compatible modules')
    end
  end
  assert(core.readInteger(game.initialDefenseTicks) == expectedTicks,
    'AIC Tactics: initial recruitment timer was modified; restart with compatible modules')
end

function M.configure(months)
  local ticks = M.validateMonths(months)
  M.preflight()
  -- Retain the native comparison and all its branch/RNG behaviour.
  if ticks ~= expectedTicks then core.writeInteger(binding.initialDefenseTicks, ticks) end
  expectedTicks = ticks
end

return M
