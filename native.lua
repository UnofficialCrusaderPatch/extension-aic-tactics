local M = {}

function M.new()
  local native = require('aicTactics.dll')
  assert(type(native) == 'table' and native.configurationSize == 344,
    'AIC Tactics: incompatible native library')
  local installed = false
  local wallCounter
  local intervalHook
  local compositionHooks
  local intervalSignature = '8B 84 AA 64 01 00 00 8B E8 F7 DD 1B ED 83 C5 02'
  local function verifyInterval()
    if intervalHook then
      assert(core.readByte(0x4D3B41) == 0xE9
        and core.readInteger(0x4D3B42) == intervalHook - 0x4D3B46,
        'AIC Tactics: recruitment interval hook was replaced; restart with compatible modules')
    else
      assert(core.AOBScan(intervalSignature) == 0x4D3B41,
        require('messages').legacyOff('ai_recruitinterval', 'legacyRecruitInterval'))
    end
  end
  function native.enableLegacyInterval()
    if intervalHook then return end
    verifyInterval()
    intervalHook = core.allocateAssembly([[
      pushfd
      push ecx
      mov eax, dword [edx + ebp * 4 + 0x164]
      mov ecx, dword [esi + 0x115E0F8]
      dec ecx
      cmp ecx, 1
      jb legacy
      cmp ecx, 16
      ja legacy
      imul ecx, ecx, configurationSize
      cmp dword [ecx + configuration], 1
      je finished
legacy:
      mov eax, 1
finished:
      pop ecx
      popfd
      jmp resume
    ]], {configuration=native.configuration, configurationSize=native.configurationSize, resume=0x4D3B48})
    core.writeCode(0x4D3B41, {0xE9, intervalHook - 0x4D3B46, 0x90, 0x90})
  end
  function native.preflight()
  require('config.grace').preflight()
  if installed then return end
  -- This adapter deliberately admits only the inspected SHC 1.41 image layout.
  -- Check every patched site before loading the DLL or making a patch.
  assert(core.AOBScan('57 8B CB E8 20 81 FF FF 57 8B CB E8 48 82 FF FF') == 0x4D5438,
    'AIC Tactics: unsupported or already modified AI scheduler')
  assert(core.AOBScan('8B 86 F0 EE 15 01 85 C0 75 44 8B C7 69 C0 A4 02 00 00') == 0x4D3BA5,
    'AIC Tactics: unsupported recruitment opportunity')
  assert(core.AOBScan('53 55 56 8B 74 24 10 57 33 C0 8D 91 8C 08 50 00') == 0x500180
      and core.AOBScan('53 8B 5C 24 08 8B C3 69 C0 90 04 00 00 0F BF 88 E2 85 38 01') == 0x4CC840,
    'AIC Tactics: unsupported or modified native moat recruitment owner')
  verifyInterval()

  -- Consume the existing Legacy wall counter; do not install another counter.
  if core.readByte(0x4D3E6F) == 0xE9 then
    local target = 0x4D3E74 + core.readInteger(0x4D3E70)
    assert(core.readInteger(target) == 0x0424548B and core.readByte(target + 4) == 0x8B
      and core.readByte(target + 5) == 0x14 and core.readByte(target + 6) == 0x95,
      'AIC Tactics: unsupported wall-defense counter hook')
    wallCounter = core.readInteger(target + 7)
  else
    error(require('messages').legacyOn('ai_defense'))
  end
  end

  local function branchTarget(site)
    assert(core.readByte(site) == 0xE9, 'AIC Tactics: PreserveSlots requires the Legacy ai_defense census')
    return site + 5 + core.readInteger(site + 1)
  end
  function native.preflightComposition()
    native.preflight()
    if compositionHooks then
      assert(branchTarget(0x579879) == compositionHooks.reset and branchTarget(0x579A7C) == compositionHooks.count,
        'AIC Tactics: defense census hook was replaced; restart with compatible modules')
      return
    end
    local reset, count = branchTarget(0x579879), branchTarget(0x579A7C)
    assert(core.readInteger(reset) == 0x1489C031 and core.readByte(reset + 4) == 0x85
      and core.readInteger(reset + 5) == wallCounter,
      'AIC Tactics: unsupported Legacy defense census reset')
    assert(core.readInteger(count) % 4294967296 == 0xC969E989 and core.readInteger(count + 4) == 0x490
      and core.readByte(count + 8) == 0x0F and core.readByte(count + 9) == 0xB6
      and core.readByte(count + 10) == 0x89 and core.readInteger(count + 11) == 0x1388976
      and core.readInteger(count + 23) == wallCounter,
      'AIC Tactics: unsupported Legacy defense census counter')
  end
  function native.activateComposition()
    if compositionHooks then return end
    native.preflightComposition()
    local reset, count = branchTarget(0x579879), branchTarget(0x579A7C)
    local resetHook = core.allocateAssembly([[
      pushfd
      pushad
      call resetDefenseCensus
      popad
      popfd
      jmp original
    ]], {resetDefenseCensus=native.resetDefenseCensus, original=reset})
    local countHook = core.allocateAssembly([[
      pushfd
      pushad
      imul ecx, ebp, 0x490
      movsx ecx, word [ecx + 0x13885DA]
      push ecx
      push edi
      call countDefenseUnit
      add esp, 8
      popad
      popfd
      jmp original
    ]], {countDefenseUnit=native.countDefenseUnit, original=count})
    core.writeCode(0x579879, {0xE9, resetHook - 0x57987E})
    core.writeCode(0x579A7C, {0xE9, countHook - 0x579A81, 0x90})
    compositionHooks = {reset=resetHook, count=countHook}
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
  require('combat-native').attach(native)
  return native
end
return M
