local M = {}

function M.new(game)
  local native = require('aicTactics.dll')
  assert(type(native) == 'table' and native.configurationSize == 344,
    'AIC Tactics: incompatible native library')
  require('native-bindings').initialize(native, game)
  game = native.game
  local sites = game.recruitmentSites
  local installed = false
  local wallCounter
  local intervalHook
  local compositionHooks
  local intervalBytes = {0x8B,0x84,0xAA,0x64,0x01,0x00,0x00,0x8B,0xE8,0xF7,0xDD,0x1B,0xED,0x83,0xC5,0x02}
  local function verifyInterval()
    if intervalHook then
      assert(core.readByte(sites.interval) == 0xE9
        and core.readInteger((sites.interval+1)) == intervalHook - (sites.interval+5),
        'AIC Tactics: recruitment interval hook was replaced; restart with compatible modules')
    else
      for index,value in ipairs(intervalBytes) do
        assert(core.readByte(sites.interval+index-1)==value,
          require('messages').legacyOff('ai_recruitinterval', 'legacyRecruitInterval'))
      end
    end
  end
  function native.enableLegacyInterval()
    if intervalHook then return end
    verifyInterval()
    intervalHook = core.allocateAssembly([[
      pushfd
      push ecx
      mov eax, dword [edx + ebp * 4 + 0x164]
      mov ecx, dword [esi + playerCharacter]
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
    ]], {playerCharacter=game.players+0x2300, configuration=native.configuration, configurationSize=native.configurationSize, resume=(sites.interval+7)})
    core.writeCode(sites.interval, {0xE9, intervalHook - (sites.interval+5), 0x90, 0x90})
  end
  function native.preflight()
  require('config.grace').preflight()
  if installed then return end
  verifyInterval()
  assert(require('native-context').call(sites.ranged,'ranged scheduler') == game.rangedSortieNative
      and require('native-context').call(sites.melee,'melee scheduler') == game.meleeSortieNative,
    'AIC Tactics: recruitment scheduler was replaced')
  assert(core.readByte(sites.opportunity)==0x8B and core.readByte(sites.opportunity+1)==0x86
      and core.readInteger(sites.opportunity+2)==game.players+0x30F8,
    'AIC Tactics: recruitment opportunity was replaced')
  wallCounter = require('native-recruitment').legacyCounter(game, compositionHooks)
  end

  local function branchTarget(site)
    assert(core.readByte(site) == 0xE9, 'AIC Tactics: PreserveSlots requires the Legacy ai_defense census')
    return site + 5 + core.readInteger(site + 1)
  end
  function native.preflightComposition()
    native.preflight()
    if compositionHooks then
      assert(branchTarget(sites.wallReset) == compositionHooks.reset and branchTarget(sites.wallCount) == compositionHooks.count,
        'AIC Tactics: defense census hook was replaced; restart with compatible modules')
      return
    end
    require('native-recruitment').legacyCounter(game, compositionHooks)
  end
  function native.activateComposition()
    if compositionHooks then return end
    native.preflightComposition()
    local reset, count = branchTarget(sites.wallReset), branchTarget(sites.wallCount)
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
      movsx ecx, word [ecx + unitType]
      push ecx
      push edi
      call countDefenseUnit
      add esp, 8
      popad
      popfd
      jmp original
    ]], {unitType=game.unitRecords+0x8E, countDefenseUnit=native.countDefenseUnit, original=count})
    core.writeCode(sites.wallReset, {0xE9, resetHook - (sites.wallReset+5)})
    core.writeCode(sites.wallCount, {0xE9, countHook - (sites.wallCount+5), 0x90})
    compositionHooks = {reset=resetHook, count=countHook, originalReset=reset, originalCount=count}
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
    mov eax, dword [esi + spendingMode]
    jmp original
handled:
    popad
    popfd
    jmp finished
  ]], {spendingMode=game.players+0x30F8, recruitOpportunity=native.recruitOpportunity, original=(sites.opportunity+6), finished=sites.finished})
  core.writeCode(sites.opportunity, {0xE9, hook - (sites.opportunity+5), 0x90})
  core.writeCode(sites.ranged, {0xE8, native.rangedSortie - (sites.ranged+5)})
  core.writeCode(sites.melee, {0xE8, native.meleeSortie - (sites.melee+5)})
  installed = true
  end
  require('combat-native').attach(native)
  return native
end
return M
