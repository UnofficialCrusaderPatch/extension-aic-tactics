local context = require('native-context')
local M = {}

-- The native checker is called once per footprint tile. Keep its ownership of
-- terrain/access validation and guard the occupied-engine case at that call.
function M.resolve(game)
  local callSite = context.find('siege footprint tile check',
    '8B 4C 24 10 8B 5C 24 40 6A 00 51 53 50 8B CE E8 ? ? ? ? 85 C0 B9 01 00 00 00 74 06')
  local original = context.call(callSite + 15, 'siege footprint tile check')
  context.verify(original, 'tile placement owner',
    '83 EC 0C 8B 54 24 18 53 55 56 8B F1 57 8B 7C 24 20 8B 9C BE ? ? ? ?')
  local occupancy = context.find('siege footprint unit occupancy',
    '0F BF 8C 5E ? ? ? ? 85 C9 74 26 0F BF 44 24 28 83 C0 D3 83 F8 21 77 0E '..
    '0F B6 90 ? ? ? ? FF 24 95 ? ? ? ? 51 B9 ? ? ? ? E8 ? ? ? ?')
  local offset = core.readInteger(occupancy + 4)
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
  context.verify(placer, 'siege attack-angle placer',
    '53 8B 5C 24 08 56 8B F3 69 F6 ? ? ? ? 0F BF 86 ? ? ? ? '..
    '69 C0 90 04 00 00 0F BF 88 ? ? ? ? 0F BF 90 ? ? ? ? '..
    '57 53 51 52 68 C8 00 00 00 B9 ? ? ? ? E8 ? ? ? ?')
  assert(core.readInteger(placer + 10) == game.tribeStride,
    'AIC Tactics: incompatible siege tribe layout')
  local finder = context.call(placer + 0x37, 'siege attack-angle finder')
  local candidate = finder + 0x1a1
  context.verify(candidate, 'siege attack-angle candidate',
    '8B C8 C1 E1 05 83 B9 ? ? ? ? 00 75 ? 83 B9 ? ? ? ? 00 0F 84 ? ? ? ?')
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
    '53 B9 ? ? ? ? 89 96 30 49 55 00 E8 ? ? ? ?')
  assert((not game.buildings or core.readInteger(admission + 2) == game.buildings)
      and context.call(admission + 12, 'building type conversion') > 0,
    'AIC Tactics: incompatible native building admission owner')
  local failedPlacement = buildingOwner + 0x61
  assert(core.readByte(failedPlacement) == 0x0F
      and core.readByte(failedPlacement + 1) == 0x85,
    'AIC Tactics: incompatible native building rejection')
  local failureExit = failedPlacement + 6 + core.readInteger(failedPlacement + 2)
  context.verify(failureExit, 'native building placement failure exit',
    '5F 5E 5D 5B 83 C4 08 C2 18 00')
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
  return {siegePlacementCall = callSite + 15, originalSiegeTileCheck = original,
    siegeTileOccupancyOffset = offset, siegeNoSpotBranch = noSpotBranch,
    siegeFailedBranch = failedBranch, siegeFailureExit = exit,
    siegeAngleCandidateBranch = candidateBranch, siegeAngleCandidateContinue = candidateBranch + 6,
    siegeAngleCandidateAccept = candidateAccept, siegeTileMap = tileMap,
    siegeTentPointX = pointX, siegeTribeIndexOffset = indexOffset,
    siegePlacementFail = placementFail, siegeAnglePostPlace = postPlace,
    siegeAnglePlacedBuilding = placedBuilding, siegeAngleFailureReturn = placer + 0xfe,
    siegeBuildingOwner = buildingOwner, siegeBuildingAdmission = admission,
    siegeBuildingFailureExit = failureExit, siegeResourceCheck = resourceCheck,
    siegeGoldSub = goldSub, siegeGoldAddress = goldAddress,
    siegeDirectSpawn = direct, siegeDirectExit = directExit}
end

return M
