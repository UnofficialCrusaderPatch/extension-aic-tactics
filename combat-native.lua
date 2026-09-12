local M = {}

function M.attach(native)
  local installed, targetingInstalled, raidsInstalled = false, false, false
  local raidSites = {
    {0x422EE2, '8B CF 89 5E 04'},
    {0x422F56, '66 83 B8 E6 00 00 00 43'},
    {0x42331B, '8B 46 10 B9 D0 07 00 00'},
  }
  local sites = {
    {0x5798EF, '8B C7 A3 C8 0F EE 00 39 3E'},
    {0x579940, '8B C5 69 C0 90 04 00 00 8B 8C 30 E8 06 00 00'},
    {0x579DDC, 'B9 00 03 35 02 E8 3A 78 E8 FF'},
    {0x531220, '51 53 8B 5C 24 0C 55 56 57'},
    {0x531920, '83 EC 14 8B 44 24 18 53 55'},
    {0x532460, '8B 44 24 04 53 33 DB 3B C3'},
    {0x4D4A62, '89 AF 68 F7 15 01 C7 87 9C E9 15 01 01 00 00 00'},
    {0x4CDD47, '89 AE 98 F6 15 01 69 C0 A4 02 00 00'},
    {0x4D40F2, '0F B7 86 42 02 00 00 66 85 C0 74 0F'},
  }
  local calls = {{0x4D54A8,0x4D49E0}, {0x4D5570,0x4D4680}, {0x4D547B,0x4D2A70},
    {0x4D4B5C,0x4CEA50}, {0x4D4F59,0x4CEA50}, {0x4D4FAF,0x4CEA50},
    {0x4D544B,0x4D3AE0}, {0x4D3DA5,0x4CC250}}

  function native.preflightTargets()
    assert(type(configFinal) == 'table', 'AIC Tactics: resolved framework configuration is unavailable')
    for name, config in pairs(configFinal) do
      if name:match('^ucp2%-legacy%-') then
        assert(not config.ai_attacktarget or config.ai_attacktarget.enabled ~= true,
          require('messages').legacyOff('ai_attacktarget', 'nativeTargetPolicy'))
      end
    end
    if not targetingInstalled then
      assert(core.AOBScan('83 F8 04 75 33 83 3C BD 10 DE 91 01 FF') == 0x4D47B2,
        'AIC Tactics: native target choice is already modified; restart with compatible modules')
    end
  end

  function native.preflightCombat()
    if installed then return end
    for _, site in ipairs(sites) do
      -- Several native damage prologues occur more than once in the image.
      -- Validate the admitted 1.41 owner itself instead of taking the first match.
      local offset = 0
      for byte in site[2]:gmatch('%x%x') do
        assert(core.readByte(site[1] + offset) == tonumber(byte, 16),
          string.format('AIC Tactics: unsupported or modified combat owner at 0x%X', site[1]))
        offset = offset + 1
      end
    end
    for _, site in ipairs(calls) do
      assert(core.readByte(site[1]) == 0xE8 and site[1] + 5 + core.readInteger(site[1] + 1) == site[2],
        string.format('AIC Tactics: unsupported AI scheduler call at 0x%X', site[1]))
    end
  end

  function native.preflightRaids()
    if raidsInstalled then return end
    for _, site in ipairs(raidSites) do
      local offset = 0
      for byte in site[2]:gmatch('%x%x') do
        assert(core.readByte(site[1] + offset) == tonumber(byte, 16),
          string.format('AIC Tactics: unsupported building census at 0x%X', site[1]))
        offset = offset + 1
      end
    end
  end

  local function jump(site, target, length)
    local code = {0xE9, target - site - 5}
    for index = 6, length do code[#code + 1] = 0x90 end
    core.writeCode(site, code)
  end
  local function call(site, target) core.writeCode(site, {0xE8, target - site - 5}) end
  local function detour(site, length, entry, originalAddress)
    local code = {}
    for index = 0, length - 1 do code[#code + 1] = core.readByte(site + index) end
    local trampoline = core.allocateCode(length + 5)
    core.writeCode(trampoline, code)
    core.writeCode(trampoline + length, {0xE9, site - trampoline - 5})
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
    ]], {resetCensus=native.resetRaidBuildingCensus, resume=0x422EE7})
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
    ]], {countBuilding=native.countRaidBuilding, resume=0x422F5E})
    local complete = core.allocateAssembly([[
      pushfd
      pushad
      call completeCensus
      popad
      popfd
      mov eax, dword [esi + 0x10]
      mov ecx, 2000
      jmp resume
    ]], {completeCensus=native.completeRaidBuildingCensus, resume=0x423323})
    jump(0x422EE2, reset, 5)
    jump(0x422F56, count, 8)
    jump(0x42331B, complete, 8)
    raidsInstalled = true
  end

  function native.enableNativeTargetPolicy(value)
    local choices = {Native = 0, Nearest = 1, Richest = 2, Weakest = 3}
    assert(choices[value] ~= nil, 'AIC Tactics: nativeTargetPolicy must be Native, Nearest, Richest or Weakest')
    if value == 'Native' then return end
    native.preflightTargets()
    assert(not targetingInstalled, 'AIC Tactics: native target policy is already configured')
    core.writeInteger(native.legacyTargetPolicy, choices[value])
    local hook = core.allocateAssembly([[
      cmp dword [policy], 1
      je nearest
      cmp dword [policy], 2
      je richest
      cmp dword [policy], 3
      je weakest
      cmp eax, 4
      jne notPlayer
      jmp player
    ]], {policy=native.legacyTargetPolicy, nearest=0x4D47C5, richest=0x4D47F3,
      weakest=0x4D4806, notPlayer=0x4D47EA, player=0x4D47B7})
    jump(0x4D47B2, hook, 5)
    targetingInstalled = true
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
      mov dword [0xEE0FC8], eax
      jmp resume
    ]], {resetCensus=native.resetCombatCensus, resume=0x5798F6})
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
    ]], {countUnit=native.countCombatUnit, resume=0x579948})
    local complete = core.allocateAssembly([[
      pushfd
      pushad
      call completeCensus
      popad
      popfd
      mov ecx, 0x2350300
      jmp resume
    ]], {completeCensus=native.completeCombatCensus, resume=0x579DE1})
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
      mov dword [edi + 0x115F768], ebp
      jmp resume
    refused:
      popad
      popfd
      jmp waiting
    ]], {commitTarget=native.commitOpponent, resume=0x4D4A68, waiting=0x4D4AB5})
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
      mov dword [esi + 0x115F698], ebp
      jmp resume
    preserve:
      popad
      popfd
      jmp resume
    ]], {preserveRequirement=native.preserveRandomWaveRequirement, resume=0x4CDD4D})
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
    ]], {reserved=native.isReserveUnit, resume=0x4D40F9, nextUnit=0x4D4117})
    -- Every admission above precedes mutations. Original damage bodies stay in
    -- their existing owners; the trampolines only replay displaced prologues.
    jump(0x5798EF, reset, 7)
    jump(0x579940, count, 8)
    jump(0x579DDC, complete, 5)
    jump(0x4D4A62, launch, 6)
    jump(0x4CDD47, randomRequirement, 6)
    jump(0x4D40F2, tunnelers, 7)
    detour(0x531220, 6, native.observedUnitDamage, native.originalUnitDamage)
    detour(0x531920, 7, native.observedEntityDamage, native.originalEntityDamage)
    detour(0x532460, 5, native.observedFireDamage, native.originalFireDamage)
    call(0x4D54A8, native.updateOffensiveArmy)
    call(0x4D5570, native.selectOpponent)
    call(0x4D547B, native.updateOffensiveRaids)
    call(0x4D4B5C, native.returnFromAttack)
    call(0x4D4F59, native.returnFromAttack)
    call(0x4D4FAF, native.returnFromAttack)
    call(0x4D544B, native.recruitWithReserve)
    call(0x4D3DA5, native.reserveRecruitType)
    installed = true
  end
end

return M
