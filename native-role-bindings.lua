local context = require('native-context')
local M = {}

-- Original AI role recount: role 10 has a dedicated engineer path, then
-- type 30 is skipped before the ordinary role jump table.
function M.resolve(game)
  local site = context.find('AI engineer role recount',
    '0F B7 90 3E 0A 00 00 66 83 FA 0A 75 1C 69 FF F4 39 00 00 BB 01 00 00 00 '..
    '01 9F ? ? ? ? 01 9F ? ? ? ? E9 ? ? ? ? 66 83 F9 1E 0F 84 ? ? ? ? '..
    '0F BF CA 83 F9 15')
  assert(game.unitRecords == game.units + 0x614
      and core.readInteger(site + 3) == 0xA3E
      and core.readInteger(site + 26) == game.players + 0x2BA8
      and core.readInteger(site + 32) == game.players + 0x30F0,
    'AIC Tactics: incompatible AI engineer role recount layout')
  local duty = context.find('native siege-construction group duty',
    'B9 ? ? ? ? 66 C7 83 ? ? ? ? 0B 00 66 C7 83 ? ? ? ? 10 04 '..
    'E8 ? ? ? ? 83 3D ? ? ? ? 00 0F 85')
  assert(core.readInteger(duty + 8) == game.tribes + 0x42
      and core.readInteger(duty + 17) == game.tribes + 0x50,
    'AIC Tactics: incompatible native siege-construction duty layout')
  local hook = site + 41
  local ordinary = hook + 10
  local skip = ordinary + core.readInteger(hook + 6)
  assert(core.readByte(hook) == 0x66 and core.readByte(hook + 1) == 0x83
      and core.readByte(hook + 4) == 0x0F and core.readByte(hook + 5) == 0x84
      and skip > ordinary, 'AIC Tactics: incompatible engineer role branch')
  return {engineerRoleHook = hook, engineerRoleOrdinary = ordinary,
    engineerRoleSkip = skip}
end

return M
