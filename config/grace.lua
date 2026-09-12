local M = {}
local expectedTicks = 4800

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
  assert(core.readInteger(0x4D34AB) == 0x7DA83D81 and core.readByte(0x4D34AF) == 0xFE
      and core.readByte(0x4D34B0) == 1 and core.readInteger(0x4D34B1) == expectedTicks
      and core.readByte(0x4D34B5) == 0x7D and core.readByte(0x4D34B6) == 8,
    'AIC Tactics: initial recruitment timer was modified; restart with compatible modules')
end

function M.configure(months)
  local ticks = M.validateMonths(months)
  M.preflight()
  -- Retain the native comparison and all its branch/RNG behaviour.
  if ticks ~= expectedTicks then core.writeInteger(0x4D34B1, ticks) end
  expectedTicks = ticks
end

return M
