local M = {}

function M.new(game)
  local native = require('aicTactics.dll')
  assert(type(native) == 'table' and native.configurationSize == 352,
    'AIC Tactics: incompatible native library')
  require('native-bindings').initialize(native, game)
  game = native.game
  local sites = game.recruitmentSites
  local installed = false
  local wallCounter
  local intervalHook
  local compositionHooks
  local safePlacementHook
  local safePlacementBackoffHook
  local safePlacementCandidateHook
  local safePlacementFailureHook
  local siegePaymentHook
  local siegeGoldHook
  local siegeDirectHook
  local siegeHarassHooks
  local engineerRoleHook
  local intervalBytes = {0x8B,0x84,0xAA,0x64,0x01,0x00,0x00,0x8B,0xE8,0xF7,0xDD,0x1B,0xED,0x83,0xC5,0x02}
  function native.preflightSafePlacement()
    local target = require('native-context').call(game.siegePlacementCall, 'siege footprint tile check')
    assert(target == (safePlacementHook and native.checkedSiegeTile or game.originalSiegeTileCheck),
      'AIC Tactics: siege footprint check was replaced; restart with compatible modules')
    for _, branch in ipairs({{game.siegeNoSpotBranch,0x84},{game.siegeFailedBranch,0x85}}) do
      local site, opcode = branch[1], branch[2]
      assert(core.readByte(site)==0x0F and core.readByte(site+1)==opcode,
        'AIC Tactics: siege retry branch was replaced')
      local destination = site+6+core.readInteger(site+2)
      assert(destination == (safePlacementBackoffHook or game.siegeFailureExit),
        'AIC Tactics: siege retry branch was replaced')
    end
    local candidate = game.siegeAngleCandidateBranch
    assert(core.readByte(candidate)==0x0F and core.readByte(candidate+1)==0x84,
      'AIC Tactics: siege candidate branch was replaced')
    assert(candidate+6+core.readInteger(candidate+2)
        == (safePlacementCandidateHook or game.siegeAngleCandidateAccept),
      'AIC Tactics: siege candidate branch was replaced')
    local post = game.siegeAnglePostPlace
    if safePlacementFailureHook then
      assert(core.readByte(post)==0xE9 and post+5+core.readInteger(post+1)==safePlacementFailureHook,
        'AIC Tactics: siege placement result was replaced')
    else
      assert(core.readByte(post)==0x8B and core.readByte(post+1)==0x0D
          and core.readInteger(post+2)==game.siegeAnglePlacedBuilding,
        'AIC Tactics: siege placement result was replaced')
    end
  end
  function native.activateSafePlacement()
    if safePlacementHook then return end
    native.preflightSafePlacement()
    local backoff = core.allocateAssembly([[
      pushfd
      pushad
      mov ecx, dword [esp + 0x38]
      push ecx
      call policyEnabled
      add esp, 4
      test eax, eax
      jz done
      mov ecx, dword [esp + 0x38]
      imul ecx, ecx, 0x39F4
      mov dword [ecx + timeout], 8
done:
      popad
      popfd
      jmp finished
    ]], {policyEnabled=native.siegePlacementPolicyEnabled,
      timeout=game.players+0x3918, finished=game.siegeFailureExit})
    local candidate = core.allocateAssembly([[
      pushfd
      pushad
      mov ecx, dword [esp + 0x44]
      push eax
      push ecx
      call allowed
      add esp, 8
      test eax, eax
      jz rejected
      popad
      popfd
      jmp accepted
rejected:
      popad
      popfd
      jmp continueSearch
    ]], {allowed=native.siegeTentCandidateAllowed,
      accepted=game.siegeAngleCandidateAccept,
      continueSearch=game.siegeAngleCandidateContinue})
    local failure = core.allocateAssembly([[
      pushfd
      pushad
      push edi
      push ebx
      call failedTent
      add esp, 8
      test eax, eax
      jnz failed
      popad
      popfd
      mov ecx, dword [placedBuilding]
      jmp resume
failed:
      popad
      popfd
      jmp failedReturn
    ]], {failedTent=native.failedSiegeTent,
      placedBuilding=game.siegeAnglePlacedBuilding,
      resume=game.siegeAnglePostPlace+6,
      failedReturn=game.siegeAngleFailureReturn})
    core.writeCode(game.siegePlacementCall,
      {0xE8, native.checkedSiegeTile - game.siegePlacementCall - 5})
    core.writeCode(game.siegeNoSpotBranch,
      {0x0F,0x84,backoff-game.siegeNoSpotBranch-6})
    core.writeCode(game.siegeFailedBranch,
      {0x0F,0x85,backoff-game.siegeFailedBranch-6})
    core.writeCode(game.siegeAngleCandidateBranch,
      {0x0F,0x84,candidate-game.siegeAngleCandidateBranch-6})
    core.writeCode(game.siegeAnglePostPlace,
      {0xE9,failure-game.siegeAnglePostPlace-5,0x90})
    safePlacementBackoffHook = backoff
    safePlacementCandidateHook = candidate
    safePlacementFailureHook = failure
    safePlacementHook = true
  end
  function native.configureSafePlacement(fallback)
    assert(type(fallback) == 'boolean', 'AIC Tactics: safeSiegePlacement must be boolean')
    core.writeInteger(native.safePlacementFallback, fallback and 1 or 0)
    if fallback then native.activateSafePlacement() end
  end
  function native.preflightSiegePayment()
    local owner = game.siegeBuildingAdmission
    if siegePaymentHook then
      assert(core.readByte(owner)==0xE9
          and owner+5+core.readInteger(owner+1)==siegePaymentHook,
        'AIC Tactics: native siege resource admission was replaced')
    else
      require('native-context').verify(owner, 'native building placement admission',
        '53 B9 ? ? ? ? 89 96 30 49 55 00')
    end
    local gold = game.siegeGoldSub
    if siegeGoldHook then
      assert(core.readByte(gold)==0xE9
          and gold+5+core.readInteger(gold+1)==siegeGoldHook,
        'AIC Tactics: defensive siege debit was replaced')
    else
      assert(core.readByte(gold)==0x29 and core.readByte(gold+1)==0x82
          and core.readInteger(gold+2)==game.siegeGoldAddress,
        'AIC Tactics: defensive siege debit was replaced')
    end
    local direct = game.siegeDirectSpawn
    if siegeDirectHook then
      assert(core.readByte(direct)==0xE9
          and direct+5+core.readInteger(direct+1)==siegeDirectHook,
        'AIC Tactics: defensive siege spawn was replaced')
    else
      require('native-context').verify(direct, 'defensive siege spawn',
        '8B 44 24 3C 8B 7C 24 44')
    end
  end
  function native.activateSiegePayment()
    if siegePaymentHook then return end
    native.preflightSiegePayment()
    local admission = core.allocateAssembly([[
      pushfd
      cmp dword [esi + bypassCheck], 0
      jnz bypass
      popfd
      pushfd
      pushad
      mov eax, dword [esp + 0x4c]
      mov edx, dword [esp + 0x40]
      push eax
      push edx
      call checkResources
      add esp, 8
      test eax, eax
      jnz checkedAllowed
      popad
      popfd
      mov dword [placementFail], 1
      mov dword [placedBuilding], 0
      jmp failedExit
checkedAllowed:
      popad
      popfd
      jmp allowed
bypass:
      popfd
allowed:
      push ebx
      mov ecx, buildings
      jmp resume
    ]], {checkResources=native.siegeResourceAdmission,
      bypassCheck=0x554930, buildings=game.buildings,
      placementFail=game.siegePlacementFail,
      placedBuilding=game.siegeAnglePlacedBuilding,
      failedExit=game.siegeBuildingFailureExit,
      resume=game.siegeBuildingAdmission+6})
    local gold = core.allocateAssembly([[
      pushfd
      pushad
      push edx
      call paymentEnabled
      add esp, 4
      test eax, eax
      jnz skip
      popad
      popfd
      sub dword [edx + goldAddress], eax
      jmp resume
skip:
      popad
      popfd
      jmp resume
    ]], {paymentEnabled=native.siegePaymentEnabledForGoldOffset,
      goldAddress=game.siegeGoldAddress,
      resume=game.siegeGoldSub+6})
    local direct = core.allocateAssembly([[
      pushfd
      pushad
      mov eax, esi
      add eax, 210
      mov edx, dword [esp + 0x68]
      push eax
      push edx
      call checkResources
      add esp, 8
      test eax, eax
      jz defer
      popad
      popfd
      mov eax, dword [esp + 0x3c]
      mov edi, dword [esp + 0x44]
      jmp resume
defer:
      popad
      popfd
      jmp finished
    ]], {checkResources=native.siegeResourceAdmission,
      resume=game.siegeDirectSpawn+8,
      finished=game.siegeDirectExit})
    core.writeCode(game.siegeBuildingAdmission,
      {0xE9,admission-game.siegeBuildingAdmission-5,0x90})
    core.writeCode(game.siegeGoldSub,
      {0xE9,gold-game.siegeGoldSub-5,0x90})
    core.writeCode(game.siegeDirectSpawn,
      {0xE9,direct-game.siegeDirectSpawn-5,0x90,0x90,0x90})
    siegePaymentHook = admission
    siegeGoldHook = gold
    siegeDirectHook = direct
  end
  function native.configureSiegePayment(fallback)
    assert(type(fallback) == 'boolean', 'AIC Tactics: actualSiegeResourcePayment must be boolean')
    core.writeInteger(native.siegePaymentFallback, fallback and 1 or 0)
    if fallback then native.activateSiegePayment() end
  end
  function native.preflightSiegeHarassment()
    for index, site in ipairs({game.siegeCatapultPathGate, game.siegeFirePathGate}) do
      if siegeHarassHooks then
        assert(core.readByte(site) == 0xE9
            and site + 5 + core.readInteger(site + 1) == siegeHarassHooks[index],
          'AIC Tactics: native siege harassment movement was replaced')
      else
        require('native-context').verify(site, 'native siege harassment movement',
          '80 BE ? ? ? ? 03')
      end
    end
  end
  function native.activateSiegeHarassment()
    if siegeHarassHooks then return end
    native.preflightSiegeHarassment()
    native.activateCombat()
    local hooks = {}
    for index, site in ipairs({game.siegeCatapultPathGate, game.siegeFirePathGate}) do
      local exit = index == 1 and game.siegeCatapultPathExit or game.siegeFirePathExit
      local hook = core.allocateAssembly([[
        pushfd
        pushad
        push esi
        call suppress
        add esp, 4
        test eax, eax
        jnz coordinated
        popad
        popfd
        cmp byte [esi + harassMode], 3
        jmp resume
coordinated:
        popad
        popfd
        jmp nativeExit
      ]], {suppress=native.suppressNativeSiegeHarassMove,
        harassMode=game.unitRecords+0x3fe, resume=site+7, nativeExit=exit})
      core.writeCode(site, {0xE9,hook-site-5,0x90,0x90})
      hooks[index] = hook
    end
    siegeHarassHooks = hooks
  end
  function native.configureSiegeHarassment(fallback, minimum)
    assert(type(fallback) == 'boolean',
      'AIC Tactics: coordinatedSiegeHarassment must be boolean')
    assert(type(minimum) == 'number' and minimum == math.floor(minimum)
        and minimum >= 0 and minimum <= 20,
      'AIC Tactics: siegeHarassMinEngines must be 0 to 20')
    core.writeInteger(native.siegeHarassFallback, fallback and 1 or 0)
    core.writeInteger(native.siegeHarassMinimumFallback, minimum)
    if fallback then native.activateSiegeHarassment() end
  end
  function native.preflightEngineerRoles()
    if engineerRoleHook then return end
    require('native-context').verify(game.engineerRoleHook, 'AI engineer role recount',
      '66 83 F9 1E 0F 84 ? ? ? ?')
    assert(game.engineerRoleOrdinary == game.engineerRoleHook + 10
        and game.engineerRoleSkip == game.engineerRoleOrdinary
          + core.readInteger(game.engineerRoleHook + 6),
      'AIC Tactics: engineer role recount was replaced')
  end
  function native.activateEngineerRoles()
    if engineerRoleHook then return end
    native.preflightEngineerRoles()
    local hook = core.allocateAssembly([[
      cmp cx, 30
      jne ordinary
      pushfd
      pushad
      push ebp
      push edi
      call eligible
      add esp, 8
      test eax, eax
      jz excluded
      popad
      popfd
      jmp ordinary
    excluded:
      popad
      popfd
      jmp skip
    ]], {eligible=native.countableEngineerRole,
      ordinary=game.engineerRoleOrdinary, skip=game.engineerRoleSkip})
    core.writeCode(game.engineerRoleHook,
      {0xE9, hook - game.engineerRoleHook - 5, 0x90, 0x90, 0x90, 0x90, 0x90})
    engineerRoleHook = hook
  end
  function native.configureEngineerRoles(fallback)
    assert(type(fallback) == 'boolean', 'AIC Tactics: correctEngineerRoleCounting must be boolean')
    core.writeInteger(native.engineerRoleFallback, fallback and 1 or 0)
    if fallback then native.activateEngineerRoles() end
  end
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
