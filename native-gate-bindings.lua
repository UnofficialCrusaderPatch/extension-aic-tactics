local context = require('native-context')
local M = {}

-- updateGateDrawBridgeOpenCloseLogic per gatehouse, after its capture part:
-- ebx the building, edi its record offset (id * 0x32C). The open/close part
-- starts with movzx eax, word [edi + enemy timer]; its exit (jl, +0x79) is the
-- shared epilogue; the native open sets the state (+0x188) to 0xB when the
-- link (+0x17D) is closed and applies it with the change call (+0x196).
local gateSignature = 'C3 0F B7 87 ? ? ? ? 66 3B C2 7E 21 8B CD 69 C9 F4 39 00 00 83 C0 FF 39 91'
local gateLength = 7

function M.resolve(game)
  local read = core.readInteger
  local site = context.find('gate open and close', gateSignature) + 1
  local records = game.buildings + 0x14
  context.verify(site + 0x79, 'gate logic exit', '0F 8C')
  context.verify(site + 0x17D, 'gate link test', '80 BF ? ? ? ? 02')
  context.verify(site + 0x188, 'gate opening', 'C6 87 ? ? ? ? 0B')
  assert(read(site + 3) == records + 0x2C0 and read(site + 0x17F) == records + 0x2A2
      and read(site + 0x18A) == records + 0x2A3,
    'AIC Tactics: inconsistent native gate logic')
  return {allyGateSite = site, allyGateTimer = records + 0x2C0,
    allyGateExit = site + 0x7F + read(site + 0x7B),
    allyGateChange = context.call(site + 0x196, 'gate change')}
end

-- `hook` is nil before install, else the installed stub.
function M.preflight(game, hook)
  local site = game.allyGateSite
  if hook then
    assert(core.readByte(site) == 0xE9 and site + 5 + core.readInteger(site + 1) == hook,
      'AIC Tactics: the native gate logic was replaced; restart with compatible modules')
    return
  end
  context.verify(site, 'gate open and close', '0F B7 87 ? ? ? ? 66 3B C2 7E 21')
  assert(core.readInteger(site + 3) == game.allyGateTimer, 'AIC Tactics: modified native gate open and close')
end

-- allyGateHeld(building) == 1 opens the gate and leaves through the exit;
-- otherwise the native part runs.
function M.install(game, held)
  local site = game.allyGateSite
  local hook = core.allocateAssembly([[
    pushfd
    pushad
    push ebx
    call gateHeld
    add esp, 4
    test eax, eax
    jnz opened
    popad
    popfd
    movzx eax, word [edi + timer]
    jmp resume
opened:
    popad
    popfd
    jmp exit
  ]], {gateHeld = held, timer = game.allyGateTimer, resume = site + gateLength,
    exit = game.allyGateExit})
  core.writeCode(site, {core.jmpTo(hook), 0x90, 0x90})
  return hook
end

return M
