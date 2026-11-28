local M = {}

function M.attach(native)
  local installed, raidsInstalled = false, false
  local game = native.game
  local sites, originals, calls = game.combatSites, game.combatOriginals, game.combatCalls
  local function loadSites()
    sites, originals, calls = game.combatSites, game.combatOriginals, game.combatCalls
  end
  local context = require('native-context')

  function native.preflightCombat()
    if installed then return end
    loadSites()
    for _, name in ipairs({'unitReset','unitCount','unitComplete','unitDamage','entityDamage','fireDamage',
        'launch','randomWave','tunnelers'}) do
      context.verify(sites[name],name,originals[name])
    end
    for name, target in pairs(calls) do
      assert(context.call(sites[name],name)==target, 'AIC Tactics: native combat scheduler was replaced')
    end
  end

  function native.preflightRaids()
    if raidsInstalled then return end
    loadSites()
    for _, name in ipairs({'buildingReset','buildingCount','buildingComplete'}) do
      context.verify(sites[name],name,originals[name])
    end
  end

  local function jump(site, target, length)
    local code = {core.jmpTo(target)}
    for index = 6, length do code[#code + 1] = 0x90 end
    core.writeCode(site, code)
  end
  local function call(site, target) core.writeCode(site, {core.callTo(target)}) end
  local function detour(site, length, entry, originalAddress)
    local code = {}
    for index = 0, length - 1 do code[#code + 1] = core.readByte(site + index) end
    local trampoline = core.allocateCode(length + 5)
    core.writeCode(trampoline, code)
    core.writeCode(trampoline + length, {core.jmpTo(site + length)})
    core.writeInteger(originalAddress, trampoline)
    jump(site, entry, length)
  end

  function native.activateRaids()
    if raidsInstalled then return end
    native.preflightRaids()
    local reset = core.allocateAssembly([[
      pushfd
      pushad
      call resetCensus
      popad
      popfd
      mov ecx, edi
      mov dword [esi + 4], ebx
      jmp resume
    ]], {resetCensus=native.resetRaidBuildingCensus, resume=(sites.buildingReset+5)})
    local count = core.allocateAssembly([[
      pushfd
      pushad
      push edx
      call countBuilding
      add esp, 4
      popad
      popfd
      cmp word [eax + 0xE6], 0x43
      jmp resume
    ]], {countBuilding=native.countRaidBuilding, resume=(sites.buildingCount+8)})
    local complete = core.allocateAssembly([[
      pushfd
      pushad
      call completeCensus
      popad
      popfd
      mov eax, dword [esi + 0x10]
      mov ecx, 2000
      jmp resume
    ]], {completeCensus=native.completeRaidBuildingCensus, resume=(sites.buildingComplete+8)})
    jump(sites.buildingReset, reset, 5)
    jump(sites.buildingCount, count, 8)
    jump(sites.buildingComplete, complete, 8)
    raidsInstalled = true
  end

  -- Legacy ai_attacktarget's short jump at the native target choice. Its
  -- choice is the native selection that Inherit policies and fallbacks use.
  function native.legacyTargetChoice()
    local site = game.targetSelection and game.targetSelection + 306 or sites and sites.targetChoice
    if not site or core.readByte(site) ~= 0xEB or core.readByte(site + 2) ~= 0x90 then return 0 end
    return ({[0x11] = 1, [0x3F] = 2, [0x52] = 3})[core.readByte(site + 1)] or 0
  end

  function native.activateCombat()
    if installed then return end
    native.preflightCombat()
    local reset = core.allocateAssembly([[
      pushfd
      pushad
      call resetCensus
      popad
      popfd
      mov eax, edi
      mov dword [censusID], eax
      jmp resume
    ]], {censusID=sites.censusID, resetCensus=native.resetCombatCensus, resume=(sites.unitReset+7)})
    local count = core.allocateAssembly([[
      pushfd
      pushad
      push ebp
      call countUnit
      add esp, 4
      popad
      popfd
      mov eax, ebp
      imul eax, eax, 0x490
      jmp resume
    ]], {countUnit=native.countCombatUnit, resume=(sites.unitCount+8)})
    local complete = core.allocateAssembly([[
      pushfd
      pushad
      call completeCensus
      popad
      popfd
      mov ecx, entityState
      jmp resume
    ]], {entityState=game.entities-0x14, completeCensus=native.completeCombatCensus, resume=(sites.unitComplete+5)})
    local launch = core.allocateAssembly([[
      pushfd
      pushad
      push esi
      call commitTarget
      add esp, 4
      test eax, eax
      jz refused
      popad
      popfd
      mov dword [edi + attackDuration], ebp
      jmp resume
    refused:
      popad
      popfd
      jmp waiting
    ]], {attackDuration=game.players+0x3970, commitTarget=native.commitOpponent, resume=(sites.launch+6), waiting=sites.launchWait})
    local randomRequirement = core.allocateAssembly([[
      pushfd
      pushad
      push esi
      call preserveRequirement
      add esp, 4
      test eax, eax
      jnz preserve
      popad
      popfd
      mov dword [esi + randomWaveSize], ebp
      jmp resume
    preserve:
      popad
      popfd
      jmp resume
    ]], {randomWaveSize=game.players+0x38A0, preserveRequirement=native.preserveRandomWaveRequirement, resume=(sites.randomWave+6)})
    local tunnelers = core.allocateAssembly([[
      pushfd
      pushad
      push edi
      call reserved
      add esp, 4
      test eax, eax
      jnz skip
      popad
      popfd
      movzx eax, word [esi + 0x242]
      jmp resume
    skip:
      popad
      popfd
      jmp nextUnit
    ]], {reserved=native.isReserveUnit, resume=(sites.tunnelers+7), nextUnit=sites.nextTunneler})
    -- Every admission above precedes mutations. Original damage bodies stay in
    -- their existing owners; the trampolines only replay displaced prologues.
    core.writeInteger(native.legacyTargetPolicy, native.legacyTargetChoice())
    jump(sites.unitReset, reset, 7)
    jump(sites.unitCount, count, 8)
    jump(sites.unitComplete, complete, 5)
    jump(sites.launch, launch, 6)
    jump(sites.randomWave, randomRequirement, 6)
    jump(sites.tunnelers, tunnelers, 7)
    detour(sites.unitDamage, 6, native.observedUnitDamage, native.originalUnitDamage)
    detour(sites.entityDamage, 7, native.observedEntityDamage, native.originalEntityDamage)
    detour(sites.fireDamage, 5, native.observedFireDamage, native.originalFireDamage)
    call(sites.callAttack, native.updateOffensiveArmy)
    call(sites.callTarget, native.selectOpponent)
    call(sites.callRaid, native.updateOffensiveRaids)
    call(sites.callReturn1, native.returnFromAttack)
    call(sites.callReturn2, native.returnFromAttack)
    call(sites.callReturn3, native.returnFromAttack)
    call(sites.callRecruit, native.recruitWithReserve)
    call(sites.callRecruitType, native.reserveRecruitType)
    installed = true
  end
end

return M
