local M = {}

function M.new()
  local native = require('aicTactics.dll')
  assert(type(native) == 'table' and native.configurationSize == 280,
    'AIC Tactics: incompatible native library')
  local installed = false
  local wallCounter
  function native.preflight()
  if installed then return end
  -- This adapter deliberately admits only the inspected SHC 1.41 image layout.
  -- Check every patched site before loading the DLL or making a patch.
  assert(core.AOBScan('57 8B CB E8 20 81 FF FF 57 8B CB E8 48 82 FF FF 57 8B CB E8 90 E6 FF FF') == 0x4D5438,
    'AIC Tactics: unsupported or already modified AI scheduler')
  assert(core.AOBScan('8B 86 F0 EE 15 01 85 C0 75 44 8B C7 69 C0 A4 02 00 00') == 0x4D3BA5,
    'AIC Tactics: unsupported recruitment opportunity')
  assert(core.AOBScan('8B 84 AA 64 01 00 00 8B E8 F7 DD 1B ED 83 C5 02') == 0x4D3B41,
    'AIC Tactics: turn off ucp2-legacy.ai_recruitinterval and restart; AIC intervals are authoritative')

  -- Consume the existing Legacy wall counter; do not install another counter.
  if core.readByte(0x4D3E6F) == 0xE9 then
    local target = 0x4D3E74 + core.readInteger(0x4D3E70)
    assert(core.readInteger(target) == 0x0424548B and core.readByte(target + 4) == 0x8B
      and core.readByte(target + 5) == 0x14 and core.readByte(target + 6) == 0x95,
      'AIC Tactics: unsupported wall-defense counter hook')
    wallCounter = core.readInteger(target + 7)
  else
    error('AIC Tactics: enable ucp2-legacy.ai_defense and restart')
  end
  end

  function native.activate()
  if installed then return end
  native.preflight()
  core.writeInteger(native.legacyWallCounts, wallCounter)

  local hook = core.allocateAssembly([[
    pushfd
    pushad
    mov eax, dword [esp + 76]
    mov ecx, dword [esp + 64]
    mov edx, dword [esp + 20]
    push ecx
    push eax
    push edx
    call recruitOpportunity
    add esp, 12
    test eax, eax
    jnz handled
    popad
    popfd
    mov eax, dword [esi + 0x115EEF0]
    jmp original
handled:
    popad
    popfd
    jmp finished
  ]], {recruitOpportunity=native.recruitOpportunity, original=0x4D3BAB, finished=0x4D3F16})
  core.writeCode(0x4D3BA5, {0xE9, hook - 0x4D3BAA, 0x90})
  core.writeCode(0x4D543B, {0xE8, native.rangedSortie - 0x4D5440})
  core.writeCode(0x4D5443, {0xE8, native.meleeSortie - 0x4D5448})
  installed = true
  end
  return native
end
return M
