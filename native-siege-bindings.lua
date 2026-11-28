local context = require('native-context')
local M = {}

-- The native checker is called once per footprint tile. Keep its ownership of
-- terrain/access validation and guard the occupied-engine case at that call.
function M.resolve(game)
  local callSite = context.find('siege footprint tile check',
    '8B 4C 24 10 8B 5C 24 40 6A 00 51 53 50 8B CE E8 ? ? ? ? 85 C0 B9 01 00 00 00 74 06')
  local original = context.call(callSite + 15, 'siege footprint tile check')
  -- Smarter Buildings patches the first 7 bytes; verify the body after them.
  context.verify(original + 7, 'tile placement owner',
    '53 55 56 8B F1 57 8B 7C 24 20 8B 9C BE ? ? ? ?')
  -- Build Over Workers hooks the 8-byte unitLayer read at -8 and replays it first in its stub.
  local occupancy = context.find('siege footprint unit occupancy',
    '85 C9 74 26 0F BF 44 24 28 83 C0 D3 83 F8 21 77 0E '..
    '0F B6 90 ? ? ? ? FF 24 95 ? ? ? ? 51 B9 ? ? ? ? E8 ? ? ? ?')
  local occupancyRead = occupancy - 8
  if core.readByte(occupancyRead) == 0xE9 then
    context.verify(occupancyRead + 5, 'siege footprint unit occupancy', '90 90 90')
    occupancyRead = occupancyRead + 5 + core.readInteger(occupancyRead + 1)
    assert(occupancyRead > 0, 'AIC Tactics: invalid native siege footprint unit occupancy jump')
  end
  context.verify(occupancyRead, 'siege footprint unit occupancy', '0F BF 8C 5E')
  local offset = core.readInteger(occupancyRead + 4)
  assert(offset == 0x23D7E0 and game.unitCapacity >= 2500,
    'AIC Tactics: unsupported native siege footprint layout')
  local noSpot = context.find('siege harassment no-site return',
    '85 C0 0F 84 ? ? ? ? A1 ? ? ? ? 8B 54 24 14 6A 0F 6A 03 55')
  local failed = context.find('siege harassment placement failure',
    '83 3D ? ? ? ? 00 0F 85 ? ? ? ? 8B 0D ? ? ? ? 8B C1 69 C0 2C 03 00 00')
  local noSpotBranch, failedBranch = noSpot + 2, failed + 7
  local exit = noSpotBranch + 6 + core.readInteger(noSpotBranch + 2)
  assert(failedBranch + 6 + core.readInteger(failedBranch + 2) == exit,
    'AIC Tactics: unsupported siege harassment failure flow')
  local timeout = context.find('siege harassment retry timer',
    '8B BE ? ? ? ? 85 FF 74 0F 83 C7 FF 89 BE ? ? ? ?')
  assert(core.readInteger(timeout + 2) == game.players + 0x3918
      and core.readInteger(timeout + 15) == game.players + 0x3918,
    'AIC Tactics: unsupported native siege retry timer')

  -- Resolve the attack-angle finder from its unique tent-point reservation
  -- writer. The similar tunnel-entrance placer is not this owner.
  local reservation = context.find('siege attack-angle reservation',
    'C7 80 ? ? ? ? 00 00 00 00 C7 80 ? ? ? ? 00 00 00 00 6A 0F '..
    '8B C7 C1 E0 05 89 88 ? ? ? ? 8B 88 ? ? ? ? 6A 03 52 8B 90 ? ? ? ?')
  local placer = reservation - 0x64
  -- Improved Siege Engines wraps the 5-byte prologue; verify the body after it.
  context.verify(placer + 5, 'siege attack-angle placer',
    '56 8B F3 69 F6 ? ? ? ? 0F BF 86 ? ? ? ? '..
    '69 C0 90 04 00 00 0F BF 88 ? ? ? ? 0F BF 90 ? ? ? ? '..
    '57 53 51 52 68 C8 00 00 00 B9 ? ? ? ? E8 ? ? ? ?')
  assert(core.readInteger(placer + 10) == game.tribeStride,
    'AIC Tactics: incompatible siege tribe layout')
  local finder = context.call(placer + 0x37, 'siege attack-angle finder')
  local candidate = finder + 0x1a1
  -- Improved Siege Engines replaces the 21 bytes before candidateBranch with a jump + NOPs
  -- and returns through candidateBranch (je) when accepting a spot.
  if core.readByte(candidate) == 0xE9 then
    context.verify(candidate + 5, 'siege attack-angle candidate',
      '90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 0F 84 ? ? ? ?')
  else
    context.verify(candidate, 'siege attack-angle candidate',
      '8B C8 C1 E1 05 83 B9 ? ? ? ? 00 75 ? 83 B9 ? ? ? ? 00 0F 84 ? ? ? ?')
  end
  local candidateBranch = candidate + 0x15
  assert(core.readByte(candidateBranch) == 0x0F and core.readByte(candidateBranch+1) == 0x84,
    'AIC Tactics: incompatible siege candidate branch')
  local candidateAccept = candidateBranch+6+core.readInteger(candidateBranch+2)
  context.verify(candidateAccept, 'siege candidate return', '5D 5F 5E 5B C2 10 00')
  local pointX = core.readInteger(placer + 0x90)
  assert(pointX > 0 and core.readInteger(placer + 0x87) == pointX + 4
      and core.readInteger(reservation + 2) == pointX + 0x10
      and core.readInteger(reservation + 12) == pointX + 0x14,
    'AIC Tactics: incompatible siege tent-point layout')
  context.verify(placer + 0xb3, 'siege tile-map owner',
    'B9 ? ? ? ? 66 89 BE ? ? ? ? E8 ? ? ? ?')
  local tileMap = core.readInteger(placer + 0xb4)
  assert(tileMap > 0 and game.mapRows > 0,
    'AIC Tactics: incompatible siege tile-map layout')
  local indexOffset = core.readInteger(placer + 0x49) - game.tribes
  assert(indexOffset > 0 and indexOffset < game.tribeStride
      and core.readInteger(placer + 0x5a) == game.tribes + indexOffset + 2
      and core.readInteger(placer + 0x4f) == game.tribes + 0x34,
    'AIC Tactics: incompatible siege tribe reservation layout')
  context.verify(placer + 0xbf, 'siege attack-angle placement call',
    'E8 ? ? ? ? 8B 0D ? ? ? ? 0F BF 96')
  context.verify(placer + 0xfe, 'siege attack-angle failure return',
    '5F 5E 33 C0 5B C2 08 00')
  local postPlace = placer + 0xc4
  local placedBuilding = core.readInteger(postPlace + 2)
  local placementFail = core.readInteger(failed + 2)
  assert(placedBuilding > 0 and placementFail > 0,
    'AIC Tactics: incompatible siege placement result layout')
  local buildingOwner = context.call(placer + 0xbf, 'native building placement owner')
  context.verify(buildingOwner, 'native building placement entry',
    '83 EC 08 53 55 8B 6C 24 18 56 8B F1 8B 4C 24 24')
  -- Check resources only after the native full-footprint check has accepted
  -- the site. A rejected point must not request goods through the AI market.
  local admission = buildingOwner + 0x69
  context.verify(admission, 'native building placement admission',
    '53 B9 ? ? ? ? 89 96 ? ? ? ? E8 ? ? ? ?')
  local resourceBypassAddress = core.readInteger(admission + 8)
  assert((not game.buildings or core.readInteger(admission + 2) == game.buildings)
      and resourceBypassAddress > 0
      and context.call(admission + 12, 'building type conversion') > 0,
    'AIC Tactics: incompatible native building admission owner')
  local failedPlacement = buildingOwner + 0x61
  assert(core.readByte(failedPlacement) == 0x0F
      and core.readByte(failedPlacement + 1) == 0x85,
    'AIC Tactics: incompatible native building rejection')
  local failureExit = failedPlacement + 6 + core.readInteger(failedPlacement + 2)
  local epilogue = '5F 5E 5D 5B 83 C4 08 C2 18 00'
  if core.readByte(failureExit) == 0xE9 then
    -- Build Over Workers hooks the 10-byte epilogue and replays it in its stub.
    context.verify(failureExit + 5, 'native building placement failure exit', '90 90 90 90 90')
    local stub = failureExit + 5 + core.readInteger(failureExit + 1)
    assert(stub > 0, 'AIC Tactics: invalid native building placement failure exit jump')
    local matched = false
    for delta = 0, 48 do
      if pcall(context.verify, stub + delta, 'native building placement failure exit', epilogue) then
        matched = true
        break
      end
    end
    assert(matched, 'AIC Tactics: modified native building placement failure exit')
  else
    context.verify(failureExit, 'native building placement failure exit', epilogue)
  end
  local resourceCheck = context.find('native construction resource admission',
    '83 EC 18 8B 44 24 1C 53 55 56 57 89 4C 24 20 BE 01 00 00 00 '..
    '50 B9 ? ? ? ? 89 74 24 14 C7 44 24 18 00 00 00 00 E8 ? ? ? ?')
  local gold = context.find('defensive siege extra-gold debit',
    '8B 54 24 10 8B 44 24 30 29 82 ? ? ? ? 8B 0D ? ? ? ? 8D 04 B3')
  local goldSub = gold + 8
  local goldAddress = core.readInteger(goldSub + 2)
  assert(goldAddress == game.players + 0x50c and game.gameState > 0
      and context.call(goldSub - 0x15, 'defensive siege native placement') == buildingOwner,
    'AIC Tactics: incompatible native siege resource layout')
  local direct = context.find('defensive siege direct-spawn admission',
    '8B 44 24 3C 8B 7C 24 44 50 8B 44 24 30 69 C0 2C 03 00 00 '..
    '0F BF 80 ? ? ? ? 50 8D 0C CD 04 00 00 00')
  context.verify(direct + 0x64, 'defensive siege construction metadata',
    '8B CE C1 E1 04 8B 91 ? ? ? ?')
  local metaBuildingType = core.readInteger(direct + 0x6b)
  assert(core.readInteger(metaBuildingType) == 86
      and core.readInteger(metaBuildingType + 16) == 87
      and core.readByte(direct + 0x7c) == 0xE9,
    'AIC Tactics: incompatible defensive siege resource flow')
  local directExit = direct + 0x81 + core.readInteger(direct + 0x7d)
  assert(directExit == goldSub + 0x37,
    'AIC Tactics: incompatible defensive siege retry exit')
  local siegeCommand = context.find('native siege tribe command',
    '83 EC 14 53 55 56 8B 74 24 24 8B C6 69 C0 F4 39 00 00 '..
    '8D 88 74 01 00 00 0F BF 81 ? ? ? ?')
  local groupMove = context.find('native siege group movement',
    '83 EC 34 33 C0 55 8B 6C 24 44 57 8B 7C 24 44 3B F8 '..
    '89 4C 24 08 89 44 24 18')
  assert(context.call(siegeCommand + 0x22a, 'siege tribe movement') == groupMove,
    'AIC Tactics: incompatible native siege group movement owner')
  local assaultCall = context.find('native assault siege batch scheduler',
    'E8 ? ? ? ? 6A 01 56 8B CB E8 ? ? ? ? 56 8B CB E8 ? ? ? ? 56 8B CB E8 ? ? ? ?')
  local assault = context.call(assaultCall, 'native assault siege batch')
  context.verify(assault, 'native assault siege batch',
    '83 EC 18 8B D1 8B 4C 24 1C 69 C9 F4 39 00 00 8B 81 ? ? ? ?')
  assert(core.readInteger(assault + 17) == game.players + 0x2300,
    'AIC Tactics: incompatible native assault siege player layout')
  assert(core.readInteger(assault + 0x15d) == game.players + 0x3924,
    'AIC Tactics: incompatible native assault wave layout')
  local assaultPlacementCall = assault + 0x18b
  context.verify(assaultPlacementCall, 'native assault siege construction call',
    'E8 ? ? ? ? 83 C6 01 83 C7 04 83 FE 08 0F 8C ? ? ? ?')
  local placeTent = context.call(assaultPlacementCall, 'native assault siege construction')
  -- Improved Siege Engines wraps the 5-byte prologue; verify the body after it.
  context.verify(placeTent + 5, 'native assault siege construction',
    '55 56 57 8B 6C 24 1C 8B FB 69 FF ? ? ? ?')
  assert(core.readInteger(placeTent + 16) == game.tribeStride,
    'AIC Tactics: incompatible native assault siege construction layout')
  context.verify(placeTent + 0x10e, 'native pending siege wave write',
    '69 C0 2C 03 00 00 6A 00 89 90 ? ? ? ?')
  local tribeAttackWave = core.readInteger(placeTent + 0x108) - game.tribes
  assert(tribeAttackWave == 0x2bc + game.tribeStride - 0x334
      and core.readInteger(placeTent + 0x118) == game.buildings + 0x14 + 0x2ac,
    'AIC Tactics: incompatible native siege wave storage')
  local popUnit = context.call(assault + 0xe2, 'native engineer tribe pop')
  context.verify(popUnit, 'native engineer tribe pop',
    '53 8B 5C 24 08 56 57 6A 00 53 8B F9 E8 ? ? ? ? 8B F0 85 F6')
  local getUnit = context.call(popUnit + 0xc, 'native tribe indexed membership')
  context.verify(getUnit, 'native tribe indexed membership',
    '8B 44 24 04 69 C0 ? ? ? ? 03 C8 57 33 FF 33 C0 66 83 79 40 02')
  assert(core.readInteger(getUnit + 6) == game.tribeStride,
    'AIC Tactics: incompatible native tribe membership layout')
  local unitCount = context.find('native assault siege equipment count',
    '53 56 8B 74 24 0C 33 C0 57 BA ? ? ? ? B3 03 8D 78 15 '..
    '66 83 7A FE 00 74 ? 0F BF 4A 08 3B CE 75 ? 38 9A 70 03 00 00 '..
    '75 ? 66 39 BA 9C 03 00 00 74 ? 0F B7 0A 66 83 F9 27 74 ? '..
    '66 83 F9 28 74 ? 66 83 F9 3A 74 ?')
  assert(core.readInteger(unitCount + 10) == game.unitRecords + 0x490 + 0x8e,
    'AIC Tactics: incompatible native siege equipment census')
  local harassTargeting = context.find('native siege target acquisition',
    '83 EC 10 53 55 56 57 8B 7C 24 24 8B C7 69 C0 90 04 00 00 '..
    '0F BF AC 08 ? ? ? ? 8D 34 08')
  context.verify(harassTargeting + 0x1eb, 'fire ballista target range',
    'B9 ? ? ? ? EB ?')
  local fireRange = core.readInteger(harassTargeting + 0x1ec)
  assert(fireRange >= 16 and fireRange <= 240, 'AIC Tactics: unsupported native siege target ranges')
  -- A jump table on the engine type, from the catapult (39) on, sets the range selector.
  context.verify(harassTargeting + 0x2b, 'native siege range selector',
    '83 C0 D9 33 DB 83 F8 26 BA C8 00 00 00 89 4C 24 10 89 4C 24 14 77 ? 0F B6 80 ? ? ? ? FF 24 85')
  local catapultCase = core.readInteger(core.readInteger(harassTargeting + 0x4c)
    + 4 * core.readByte(core.readInteger(harassTargeting + 0x45)))
  context.verify(catapultCase, 'native catapult range selector', 'B9 ? ? ? ?')
  local catapultUpdate = context.find('native catapult update',
    '83 EC 74 53 8B 1D ? ? ? ? 55 56 8B F3 69 F6 90 04 00 00 '..
    '0F BF 86 ? ? ? ? 57 8B F8 69 FF F4 39 00 00')
  local fireUpdate = context.find('native fire ballista update',
    '83 EC 74 53 55 56 8B 35 ? ? ? ? 69 F6 90 04 00 00 '..
    '0F BF 9E ? ? ? ? 8B C3 69 C0 F4 39 00 00')
  local function autoMoveGate(owner, offset, name)
    local site = owner + offset
    context.verify(site, name, '80 BE ? ? ? ? 03 0F 85 ? ? ? ?')
    assert(not game.unitRecords or core.readInteger(site + 2) == game.unitRecords + 0x3fe,
      'AIC Tactics: incompatible native siege movement layout')
    local exit = site + 13 + core.readInteger(site + 9)
    assert(exit > site and exit < owner + 0x2000,
      'AIC Tactics: incompatible native siege movement exit')
    return site, exit
  end
  local catapultGate, catapultExit = autoMoveGate(catapultUpdate, 0x733,
    'native catapult solo movement')
  local fireGate, fireExit = autoMoveGate(fireUpdate, 0x803,
    'native fire ballista solo movement')
  return {siegePlacementCall = callSite + 15, originalSiegeTileCheck = original,
    siegeTileOccupancyOffset = offset, siegeNoSpotBranch = noSpotBranch,
    siegeFailedBranch = failedBranch, siegeFailureExit = exit,
    siegeAngleCandidateBranch = candidateBranch, siegeAngleCandidateContinue = candidateBranch + 6,
    siegeAngleCandidateAccept = candidateAccept, siegeTileMap = tileMap,
    siegeTentPointX = pointX, siegeTribeIndexOffset = indexOffset,
    siegePlacementFail = placementFail, siegeAnglePostPlace = postPlace,
    siegeAnglePlacedBuilding = placedBuilding, siegeAngleFailureReturn = placer + 0xfe,
    siegeBuildingOwner = buildingOwner, siegeBuildingAdmission = admission,
    siegeResourceBypassAddress = resourceBypassAddress,
    siegeBuildingFailureExit = failureExit, siegeResourceCheck = resourceCheck,
    siegeGoldSub = goldSub, siegeGoldAddress = goldAddress,
    siegeDirectSpawn = direct, siegeDirectExit = directExit,
    siegeGroupMove = groupMove, siegePlaceTent = placeTent,
    popUnitFromTribe = popUnit, siegeAssaultPlacementCall = assaultPlacementCall,
    siegeAssaultBatchCall = assaultCall, siegeAssaultBatch = assault,
    tribeAttackWave = tribeAttackWave, siegeUnitCount = unitCount, siegeGetUnitForIndex = getUnit,
    siegeHarassTargeting = harassTargeting,
    siegeRangeSelection = harassTargeting + 0x1f2,
    siegeCatapultSelector = core.readInteger(catapultCase + 1),
    siegeFireRange = harassTargeting + 0x1ec,
    siegeCatapultPathGate = catapultGate,
    siegeCatapultPathExit = catapultExit, siegeFirePathGate = fireGate,
    siegeFirePathExit = fireExit}
end

function M.resolveGroupMove(game)
  -- TribesState clear(tribe): resets unit +0x42C of each living member. The
  -- group move skips members with it set; the AI calls this before its own
  -- group moves.
  local clearMove = context.find('group move flags',
    '53 8B 5C 24 08 8B C3 69 C0 ? ? ? ? 55 56 57 8B F9 33 F6 66 39 74 38 5C 8D 6C 38 5C '..
    '7E 38 90 56 53 8B CF E8 ? ? ? ? 69 C0 90 04 00 00 83 C6 01 66 83 B8 ? ? ? ? 02 75 13 66 83 B8 ? ? ? ? 00 '..
    '75 09 66 C7 80 ? ? ? ? 00 00 0F BF 4D 00 3B F1 7C C9 5F 5E 5D 5B C2 04 00')
  assert(core.readInteger(clearMove + 0x09) == game.tribeStride
      and core.readInteger(clearMove + 0x35) == game.unitRecords + 0x8C
      and core.readInteger(clearMove + 0x3F) == game.unitRecords + 0x2A0
      and core.readInteger(clearMove + 0x49) == game.unitRecords + 0x42C,
    'AIC Tactics: inconsistent native group move flags')
  return {clearGroupMoveFlags = clearMove}
end

-- The selection computes add + (selector - 3 ~= 0 and mask or 0): 83 for the
-- trebuchet (3) and 73 for the catapult (2) in the unmodified game.
-- Returns nil when another module changed that code.
function M.catapultRange(game)
  local site = game.siegeRangeSelection
  if not pcall(context.verify, site, 'catapult target range', '83 E9 03 F7 D9 1B C9 83 E1 ? 83 C1 ?') then
    return nil
  end
  local function signed(byte) return byte < 0x80 and byte or byte - 0x100 end
  local range = signed(core.readByte(site + 12))
    + (game.siegeCatapultSelector ~= 3 and signed(core.readByte(site + 9)) or 0)
  if range >= 16 and range <= 240 then return range end
end

return M
