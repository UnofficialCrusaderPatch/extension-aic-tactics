local M = {}

function M.new(game)
  local native = require('aicTactics.dll')
  assert(type(native) == 'table' and native.configurationSize == 352,
    'AIC Tactics: incompatible native library')
  local bindings = require('native-bindings')
  bindings.initialize(native, game)
  game = native.game
  local sites = game.recruitmentSites
  local stateReady, stateInterval, stateTarget
  function native.prepareState()
    if stateReady == nil then
      stateReady, stateInterval, stateTarget = pcall(function()
        bindings.requireBindings(game, {'AICLayout', 'Grace', 'Layout', 'Interval', 'Target'})
        return native.intervalForced(), native.legacyTargetChoice()
      end)
      if not stateReady then
        log(ERROR, 'AIC Tactics: save and multiplayer identity is unavailable: ' .. tostring(stateInterval))
      end
    end
    if stateReady then return stateInterval, stateTarget end
    return nil, stateInterval
  end
  local admissionTick
  function native.admissionTick()
    if admissionTick then return admissionTick end
    local protocol = modules.protocol
    if protocol and type(protocol.getNativeCommandInterface) == 'function' then
      local ok, command = pcall(protocol.getNativeCommandInterface, protocol)
      if ok and command and command.version == 1 and type(command.tick) == 'number' and command.tick > 0 then
        admissionTick = command.tick
        return admissionTick
      end
    end
    bindings.requireBindings(game, {'Grace'})
    admissionTick = game.gameTick
    return admissionTick
  end
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
  local siegeRecoveryInstalled = false
  local largerSiegeInstalled = false
  local engineerRoleHook
  local intervalBytes = {0x8B,0x84,0xAA,0x64,0x01,0x00,0x00,0x8B,0xE8,0xF7,0xDD,0x1B,0xED,0x83,0xC5,0x02}
  function native.preflightAssaultCrewRecovery()
    local target = require('native-context').call(game.siegeAssaultPlacementCall,
      'AI assault siege construction')
    assert(target == (siegeRecoveryInstalled and native.placeSiegeTentAndRecoverEngineers
        or game.siegePlaceTent),
      'AIC Tactics: AI assault siege construction was replaced')
  end
  function native.activateAssaultCrewRecovery()
    if siegeRecoveryInstalled then return end
    native.preflightAssaultCrewRecovery()
    local site = game.siegeAssaultPlacementCall
    core.writeCode(site, {core.callTo(native.placeSiegeTentAndRecoverEngineers)})
    siegeRecoveryInstalled = true
  end
  function native.preflightLargerSiegeForce()
    native.preflightAssaultCrewRecovery()
    local target = require('native-context').call(game.siegeAssaultBatchCall,
      'AI assault siege batch')
    assert(target == (largerSiegeInstalled and native.buildLargerSiegeForce
        or game.siegeAssaultBatch),
      'AIC Tactics: AI assault siege batch was replaced')
  end
  function native.activateLargerSiegeForce()
    if largerSiegeInstalled then return end
    native.preflightLargerSiegeForce()
    native.activateAssaultCrewRecovery()
    local site = game.siegeAssaultBatchCall
    core.writeCode(site, {core.callTo(native.buildLargerSiegeForce)})
    largerSiegeInstalled = true
  end
  function native.configureLargerSiegeForce(fallback, maximum)
    assert(type(fallback) == 'boolean',
      'AIC Tactics: largerSiegeForces must be boolean')
    assert(type(maximum) == 'number' and maximum == math.floor(maximum)
        and maximum >= 0 and maximum <= 64,
      'AIC Tactics: siegeForceMax must be 0 to 64')
    core.writeInteger(native.largeSiegeFallback, native.enableOptional('LargerSiegeForce', fallback) and 1 or 0)
    core.writeInteger(native.siegeForceMaximumFallback, maximum)
  end
  function native.preflightSafePlacement()
    native.preflightAssaultCrewRecovery()
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
    core.writeCode(game.siegePlacementCall, {core.callTo(native.checkedSiegeTile)})
    core.writeCode(game.siegeNoSpotBranch,
      {0x0F,0x84,backoff-game.siegeNoSpotBranch-6})
    core.writeCode(game.siegeFailedBranch,
      {0x0F,0x85,backoff-game.siegeFailedBranch-6})
    core.writeCode(game.siegeAngleCandidateBranch,
      {0x0F,0x84,candidate-game.siegeAngleCandidateBranch-6})
    core.writeCode(game.siegeAnglePostPlace, {core.jmpTo(failure),0x90})
    safePlacementBackoffHook = backoff
    safePlacementCandidateHook = candidate
    safePlacementFailureHook = failure
    safePlacementHook = true
    native.activateAssaultCrewRecovery()
  end
  function native.configureSafePlacement(fallback)
    assert(type(fallback) == 'boolean', 'AIC Tactics: safeSiegePlacement must be boolean')
    core.writeInteger(native.safePlacementFallback, native.enableOptional('SafePlacement', fallback) and 1 or 0)
  end
  function native.preflightSiegePayment()
    native.preflightAssaultCrewRecovery()
    local owner = game.siegeBuildingAdmission
    if siegePaymentHook then
      assert(core.readByte(owner)==0xE9
          and owner+5+core.readInteger(owner+1)==siegePaymentHook,
        'AIC Tactics: native siege resource admission was replaced')
    else
      require('native-context').verify(owner, 'native building placement admission',
        '53 B9 ? ? ? ? 89 96 ? ? ? ?')
      assert(core.readInteger(owner+8) == game.siegeResourceBypassAddress,
        'AIC Tactics: native siege resource bypass changed')
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
      bypassCheck=game.siegeResourceBypassAddress, buildings=game.buildings,
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
    core.writeCode(game.siegeBuildingAdmission, {core.jmpTo(admission),0x90})
    core.writeCode(game.siegeGoldSub, {core.jmpTo(gold),0x90})
    core.writeCode(game.siegeDirectSpawn, {core.jmpTo(direct),0x90,0x90,0x90})
    siegePaymentHook = admission
    siegeGoldHook = gold
    siegeDirectHook = direct
    native.activateAssaultCrewRecovery()
  end
  function native.configureSiegePayment(fallback)
    assert(type(fallback) == 'boolean', 'AIC Tactics: actualSiegeResourcePayment must be boolean')
    core.writeInteger(native.siegePaymentFallback, native.enableOptional('SiegePayment', fallback) and 1 or 0)
  end
  -- Rebalancer owns the siege ranges once it has applied a ranges block.
  -- Otherwise the game's own range code must be unchanged.
  local catapultRangeReported = false
  local siegeRangesFinalized = false
  local function refreshSiegeRanges(reportUnavailable)
    if not game.siegeRangeSelection then return end
    local rebalancer = modules.rebalancer
    local ranges = rebalancer and type(rebalancer.getSiegeRanges) == 'function'
      and rebalancer:getSiegeRanges()
    local range = ranges and ranges.catapult or require('native-siege-bindings').catapultRange(game)
    if not range and reportUnavailable and not catapultRangeReported then
      catapultRangeReported = true
      log(ERROR, 'AI catapult harassment is off. The catapult range could not be verified.')
    end
    core.writeInteger(native.siegeCatapultRange, range or 0)
  end
  function native.finalizeSiegeRanges()
    if not siegeHarassHooks then return end
    refreshSiegeRanges(siegeHarassHooks ~= nil)
    siegeRangesFinalized = true
  end
  function native.preflightSiegeHarassment()
    native.preflightCombat()
    local context = require('native-context')
    context.verify(game.siegeHarassTargeting, 'native siege target acquisition',
      '83 EC 10 53 55 56 57')
    context.verify(game.siegeFireRange - 1, 'fire ballista target range',
      'B9 ? ? ? ?')
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
      hooks[index] = hook
    end
    refreshSiegeRanges(siegeRangesFinalized)
    native.activateCombat()
    for index, site in ipairs({game.siegeCatapultPathGate, game.siegeFirePathGate}) do
      core.writeCode(site, {core.jmpTo(hooks[index]),0x90,0x90})
    end
    siegeHarassHooks = hooks
  end
  function native.configureSiegeHarassment(fallback, minimum)
    assert(type(fallback) == 'boolean',
      'AIC Tactics: coordinatedSiegeHarassment must be boolean')
    assert(type(minimum) == 'number' and minimum == math.floor(minimum)
        and minimum >= 0 and minimum <= 20,
      'AIC Tactics: siegeHarassMinEngines must be 0 to 20')
    core.writeInteger(native.siegeHarassFallback, native.enableOptional('SiegeHarassment', fallback) and 1 or 0)
    core.writeInteger(native.siegeHarassMinimumFallback, minimum)
  end
  local patrolCallsInstalled = false
  -- Detached patrol groups are handed back from the combat scheduler calls.
  function native.preflightAttackPatrols()
    native.preflightCombat()
    require('native-patrol-bindings').preflight(game,
      patrolCallsInstalled and native.dispatchAttackPatrols or game.attackPatrolDispatch)
  end
  function native.activateAttackPatrols()
    if patrolCallsInstalled then return end
    native.preflightAttackPatrols()
    native.activateCombat()
    require('native-patrol-bindings').install(game, native.dispatchAttackPatrols)
    patrolCallsInstalled = true
  end
  local siegeScreenInstalled = false
  function native.preflightSiegeScreen()
    require('native-patrol-bindings').preflightScreen(game,
      siegeScreenInstalled and native.screenSiegeDefenceMove or game.siegeGroupMove)
  end
  function native.activateSiegeScreen()
    if siegeScreenInstalled then return end
    native.preflightSiegeScreen()
    require('native-patrol-bindings').installScreen(game, native.screenSiegeDefenceMove)
    siegeScreenInstalled = true
  end
  function native.configureAttackPatrols(orders, formation, spacing, extra, minimum, radius, screen, forward,
      screenExtra)
    if not native.enableOptional('AttackPatrols', orders == 1) then orders = 0 end
    if not native.enableOptional('SiegeScreen', screen == 1) then screen = 0 end
    -- Extra siege-defence groups are handed back from the per-AI attack update.
    if not native.enableOptional('Combat', screenExtra > 0) then screenExtra = 0 end
    core.writeInteger(native.patrolOrdersFallback, orders)
    core.writeInteger(native.patrolFormationFallback, formation)
    core.writeInteger(native.patrolSpacingFallback, spacing)
    core.writeInteger(native.patrolExtraGroupsFallback, extra)
    core.writeInteger(native.patrolMinSizeFallback, minimum)
    core.writeInteger(native.patrolRadiusFallback, radius)
    core.writeInteger(native.siegeScreenFallback, screen)
    core.writeInteger(native.siegeScreenForwardFallback, forward)
    core.writeInteger(native.siegeScreenExtraGroupsFallback, screenExtra)
  end
  -- Lord hunt runs inside the combat owner's per-AI attack update, monthly
  -- target selection and unit census; it installs nothing of its own.
  function native.preflightLordHunt() native.preflightCombat() end
  function native.activateLordHunt() native.activateCombat() end
  function native.configureLordHunt(retarget, siege, switch, share, split)
    if not native.enableOptional('LordHunt', retarget == 1 or share > 0) then retarget, share = 0, 0 end
    core.writeInteger(native.lordHuntRetargetFallback, retarget)
    core.writeInteger(native.lordHuntSiegeFallback, siege)
    core.writeInteger(native.lordHuntSwitchFallback, switch)
    core.writeInteger(native.lordHuntShareFallback, share)
    core.writeInteger(native.lordHuntSplitFallback, split)
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
    core.writeCode(game.engineerRoleHook, {core.jmpTo(hook), 0x90, 0x90, 0x90, 0x90, 0x90})
    engineerRoleHook = hook
  end
  function native.configureEngineerRoles(fallback)
    assert(type(fallback) == 'boolean',
      'AIC Tactics: correctEngineerRoleCounting must be boolean')
    core.writeInteger(native.engineerRoleFallback, native.enableOptional('EngineerRoles', fallback) and 1 or 0)
  end
  -- Legacy ai_recruitinterval writes `mov eax, 1` over the interval load, so
  -- every AI recruits at interval 1. WeightedRoles AIs keep their own AIC
  -- intervals: the hook on the next instructions reloads theirs.
  local legacyInterval = {0xB8,0x01,0x00,0x00,0x00,0x90,0x90}
  local function intervalForced()
    local site = game.recruitmentInterval or (sites and sites.interval)
    if not site then return false end
    for index, value in ipairs(legacyInterval) do
      if core.readByte(site+index-1) ~= value then return false end
    end
    return true
  end
  local function verifyInterval()
    local first = 1
    if intervalForced() then
      first = 8
      if intervalHook then
        assert(core.readByte(sites.interval+7) == 0xE9
          and core.readInteger(sites.interval+8) == intervalHook - (sites.interval+12),
          'AIC Tactics: recruitment interval hook was replaced; restart with compatible modules')
        first = 14
      end
    end
    for index = first, #intervalBytes do
      assert(core.readByte(sites.interval+index-1) == intervalBytes[index],
        'AIC Tactics: recruitment interval was replaced; restart with compatible modules')
    end
  end
  native.intervalForced = intervalForced
  function native.activateInterval()
    if intervalHook or not intervalForced() then return end
    intervalHook = core.allocateAssembly([[
      push ecx
      mov ecx, dword [esi + playerCharacter]
      dec ecx
      cmp ecx, 1
      jb finished
      cmp ecx, 16
      ja finished
      imul ecx, ecx, configurationSize
      cmp dword [ecx + configuration], 1
      jne finished
      mov eax, dword [edx + ebp * 4 + 0x164]
finished:
      pop ecx
      mov ebp, eax
      neg ebp
      sbb ebp, ebp
      jmp resume
    ]], {playerCharacter=game.players+0x2300, configuration=native.configuration, configurationSize=native.configurationSize, resume=(sites.interval+13)})
    core.writeCode(sites.interval+7, {core.jmpTo(intervalHook), 0x90})
  end
  -- AIV Troop Behaviour owns the defence row and group count of a unit type,
  -- Improved Tunnelers its raid troop. The WeightedRoles probe reads their
  -- answers from tables; 0 keeps the game's value. ATB bounds the slots to
  -- 10 and answers alike for patrol group counts from 10 up or 0 down.
  function native.refreshOwnerAnswers()
    local troops, tunnelers = modules['aiv-troops-behaviour'], modules['improved-tunnelers']
    local function ask(module, name, ...)
      local answer = module and type(module[name]) == 'function' and module[name](module, ...)
      if type(answer) == 'number' and answer >= 0 and answer < 255 and answer % 1 == 0 then return answer end
    end
    local raids = {}
    for unitType = 0, 79 do raids[unitType + 1] = ask(tunnelers, 'raidRoleFor', unitType) or 0 end
    core.writeBytes(native.ownerRaidTypes, raids)
    local asksRows = troops and type(troops.defenseRowFor) == 'function'
    local asksGroups = troops and type(troops.defenseGroupsFor) == 'function'
    for ai = 1, 16 do
      if core.readInteger(native.configuration + ai * native.configurationSize) == 1 then
        local rows, groups = {}, {}
        for unitType = 0, 79 do rows[unitType + 1] = asksRows and ask(troops, 'defenseRowFor', ai, unitType) or 0 end
        for row = 0, 19 do
          for slots = 1, 10 do
            for patrolGroups = 0, 10 do
              local count = asksGroups and ask(troops, 'defenseGroupsFor', ai, row, slots, patrolGroups)
              groups[#groups + 1] = count and count + 1 or 0
            end
          end
        end
        core.writeBytes(native.ownerDefenseRows + ai * 80, rows)
        core.writeBytes(native.ownerDefenseGroups + ai * #groups, groups)
      end
    end
  end
  function native.preflight()
  sites = game.recruitmentSites
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
    core.writeCode(sites.wallReset, {core.jmpTo(resetHook)})
    core.writeCode(sites.wallCount, {core.jmpTo(countHook), 0x90})
    compositionHooks = {reset=resetHook, count=countHook, originalReset=reset, originalCount=count}
  end

  function native.activate()
  if installed then return end
  native.preflight()

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
  native.activateInterval()
  core.writeInteger(native.legacyWallCounts, wallCounter)
  core.writeCode(sites.opportunity, {core.jmpTo(hook), 0x90})
  core.writeCode(sites.ranged, {core.callTo(native.rangedSortie)})
  core.writeCode(sites.melee, {core.callTo(native.meleeSortie)})
  installed = true
  end
  local allyBindings = require('native-ally-bindings')
  local allyGoldHooks
  -- Relief and goods run in the per-AI update owned by the combat hooks;
  -- relief reads the combat census, goods the building census.
  function native.preflightAllyRelief() native.preflightCombat() end
  function native.activateAllyRelief() native.activateCombat() end
  function native.preflightAllyGoods()
    native.preflightCombat()
    native.preflightRaids()
    allyBindings.preflight(game, allyGoldHooks)
  end
  function native.activateAllyGoods()
    if allyGoldHooks then return end
    native.preflightAllyGoods()
    native.activateCombat()
    native.activateRaids()
    allyGoldHooks = allyBindings.install(game, native.allyGoldRequestAllowed, native.allyGoldRequestCommand)
  end
  -- The keep order installs nothing; it runs in relief's hooks.
  function native.preflightAllyReliefKeep() end
  function native.activateAllyReliefKeep() end
  local gateHook
  -- The relief's gate opening runs inside the native gate logic, per gatehouse.
  function native.preflightAllyGates()
    native.preflightAllyRelief()
    require('native-gate-bindings').preflight(game, gateHook)
  end
  function native.activateAllyGates()
    if gateHook then return end
    native.preflightAllyGates()
    native.activateAllyRelief()
    gateHook = require('native-gate-bindings').install(game, native.allyGateHeld)
  end
  function native.configureAllies(words)
    if not native.enableOptional('AllyRelief', words[1] ~= 0) then words[1] = 0 end
    if not native.enableOptional('AllyGoods', words[2] ~= 0) then words[2] = 0 end
    if not native.enableOptional('AllyGates', words[8] ~= 0) then words[8] = 0 end
    if not native.enableOptional('AllyReliefKeep', words[9] ~= 0) then words[9] = 0 end
    for index, value in ipairs(words) do core.writeInteger(native.allyFallback + (index - 1) * 4, value) end
  end
  local towerFallback, towerRegistered
  function native.preflightTowerCrew()
    local riders = modules['improved-siegeengines']
    assert(riders and type(riders.registerTowerCrewPolicy) == 'function',
      'AIC Tactics: siege tower crews require a compatible Improved Siege Engines module')
  end
  function native.activateTowerCrew()
    if towerRegistered then return end
    require('siege-tower-crew').register(modules['improved-siegeengines'], native, game, towerFallback)
    towerRegistered = true
  end
  function native.configureTowerCrew(fallback)
    towerFallback = fallback
    native.enableOptional('TowerCrew', fallback)
  end
  require('combat-native').attach(native)
  -- Another module can own a site that an optional feature patches, or the
  -- startup scan did not find it. That feature and the features that need it
  -- then stay off for this process: its preflight passes, its
  -- activation installs nothing and the match identity lists it. A feature
  -- whose installed hooks were replaced afterwards still fails.
  local off, names = {}, {}
  native.unavailableFeatures = off
  local function leaveOff(key, reason)
    off[key] = true
    log(DEBUG, '[aic-tactics] ' .. (reason or names[key]))
    log(ERROR, string.format('AIC Tactics: %s is off. The required game code could not be verified.', names[key]))
  end
  local function optional(key, name, groups, ...)
    names[key] = name
    local suffix = key == 'Recruitment' and '' or key
    local needs = {...}
    local preflight, activate, installed = native['preflight' .. suffix], native['activate' .. suffix], false
    local function needOff()
      for _, need in ipairs(needs) do
        if off[need] then return names[need] .. ' is off' end
      end
    end
    native['preflight' .. suffix] = function()
      if off[key] then return end
      local reason = needOff()
      if not reason then
        local ok, failure = pcall(function()
          bindings.requireBindings(game, groups)
          preflight()
        end)
        -- The preflight can run a need's preflight, which leaves that need off.
        reason = needOff()
        if ok and not reason then return end
        if not reason and installed then error(failure, 0) end
        reason = reason or tostring(failure)
      end
      leaveOff(key, reason)
    end
    native['activate' .. suffix] = function()
      native['preflight' .. suffix]()
      if installed then return end
      if off[key] then return end
      local ok, reason = pcall(function()
        bindings.publish(native)
        activate()
      end)
      if ok then installed = true else leaveOff(key, tostring(reason)) end
    end
  end
  local recruitment = {'AICLayout', 'Grace', 'Layout', 'Interval', 'Army', 'Groups', 'Recruitment', 'Queries'}
  local combat = {'AICLayout', 'Grace', 'Layout', 'Interval', 'Target', 'Army', 'Groups', 'Projectiles',
    'Recruitment', 'Queries', 'Market', 'Combat'}
  local siege = {'Layout', 'Army', 'Groups', 'Market', 'Siege'}
  local function plus(list, ...)
    local groups = {table.unpack(list)}
    for _, key in ipairs({...}) do groups[#groups + 1] = key end
    return groups
  end
  local patrol = plus(combat, 'Siege', 'Patrol', 'GroupMove')
  optional('Recruitment', 'weighted recruitment', recruitment)
  optional('Composition', 'defender composition', recruitment, 'Recruitment')
  optional('Combat', 'attack control', combat)
  optional('Raids', 'raid targeting', combat, 'Combat')
  optional('SafePlacement', 'safe siege placement', siege)
  optional('SiegePayment', 'siege resource payment', siege, 'SafePlacement')
  optional('SiegeHarassment', 'coordinated siege harassment', plus(siege, 'GroupMove'), 'SafePlacement',
    'Combat')
  optional('LargerSiegeForce', 'larger siege forces', {'AICLayout', table.unpack(siege)}, 'SafePlacement')
  optional('EngineerRoles', 'engineer role counting', {'Layout', 'Engineer'})
  optional('AttackPatrols', 'attack patrol orders', patrol, 'Combat')
  optional('SiegeScreen', 'siege escorts', patrol)
  optional('LordHunt', 'lord hunt', plus(combat, 'Siege', 'Hunt', 'GroupMove'), 'Combat')
  optional('AllyRelief', 'ally relief', {'AICLayout', 'Layout', 'Army', 'Groups', 'Market', 'Siege', 'Allies', 'GroupMove'},
    'Combat')
  optional('AllyGoods', 'ally goods', {'AICLayout', 'Layout', 'Market', 'Allies'}, 'Combat', 'Raids')
  optional('AllyGates', 'relief gate opening', {'Layout', 'Gate'}, 'AllyRelief')
  optional('AllyReliefKeep', 'relief on the keep', {'Layout', 'Keep'}, 'AllyRelief')
  optional('TowerCrew', 'siege tower crews', {'Grace', table.unpack(siege)})
  -- Activates a module switch and returns whether the feature is on.
  function native.enableOptional(key, wanted)
    if wanted then native['activate' .. key]() end
    return wanted and not off[key]
  end
  return native
end
return M
