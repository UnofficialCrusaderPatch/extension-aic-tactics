local context = require('native-context')
local M = {}

-- sendAITribe_0xf_toAttackPatrol: validates the AI, slot 15 and its UID and
-- leader, sets the tribe's stance, then jumps to the native destination routine.
local dispatcherSignature = '8B 4C 24 04 8B C1 69 C0 F4 39 00 00 83 B8 ? ? ? ? 00 74 52 8B D1 69 D2 F4 39 00 00 '..
  '0F BF 92 ? ? ? ? 85 D2 74 3F 8B C2 69 C9 F4 39 00 00 69 C0 ? ? ? ? 56 8B B0 ? ? ? ? '..
  '3B B1 ? ? ? ? 5E 75 21 66 83 B8 ? ? ? ? 00 74 17 66 C7 80 ? ? ? ? 01 00 89 54 24 04 B9 ? ? ? ? E9'
-- sendAttackingPatrolTribeToComputedDestination(tribe): leader-based search.
local destinationSignature = '53 8B 5C 24 08 8B C3 69 C0 ? ? ? ? 56 0F BF B0 ? ? ? ? 57 6A 01 B9 ? ? ? ? E8 ? ? ? ? '..
  '69 F6 ? ? ? ? 0F BF 8E ? ? ? ? 0F BF 96 ? ? ? ? 0F BF 86 ? ? ? ? 6A 01 6A 0C'
-- giveTribeMoveInstruction first rejects coordinates outside the playable
-- map: playable[y * 400 + x] == 0. The same 400x400 byte map gates stations.
local playableSignature = '8B C5 69 C0 90 01 00 00 80 BC 38 ? ? ? ? 00'
-- The leader-based tile search the destination routine calls (radius 12, 30,
-- 50) reads the AI's distance layer: layers[index[player]][tile] is a tile's
-- walking distance from the attacked castle.
local layerSignature = '8B 4C 24 24 0F B6 B8 ? ? ? ? 69 C9 BC 77 01 00 8B 89 ? ? ? ? 69 C9 10 3A 01 00 '..
  '83 7C 24 2C 00 0F B6 AC 01 ? ? ? ?'
local searchCalls = {0x49, 0x75, 0xA1}
local layerRead = 0x170
-- Siege-defence rally (states 3-4): for slots 190 and 191, once every member
-- reached its point and the leader is not fighting, the next point of the
-- tribe's own rally list is ordered with giveTribeMoveInstruction.
local rallySignature = '83 EC 0C 8B 44 24 10 69 C0 F4 39 00 00 83 B8 ? ? ? ? 00 89 4C 24 08 0F 84 31 01 00 00 '..
  '53 55 33 C9 8D 80 ? ? ? ? 56 89 4C 24 0C 89 44 24 10 57 0F BF 38 85 FF 0F 84 EF 00 00 00 8B 44 24 20 8B F7 '..
  '69 C0 7D 0E 00 00 69 F6 ? ? ? ? 03 C1 8B 8E ? ? ? ? 3B 0C 85 ? ? ? ? 0F 85 C8 00 00 00 57 B9 ? ? ? ? '..
  '66 C7 86 ? ? ? ? 02 00 E8 ? ? ? ? 85 C0 0F 84 AC 00 00 00 8B 4C 24 18 57 66 C7 86 ? ? ? ? 00 00 E8 ? ? ? ? '..
  '0F B7 96 ? ? ? ? 0F BF 8E ? ? ? ? 0F BF C2 83 C1 01 3B C8 7C 02 33 C9 8B C7 69 C0 ? ? ? ? 03 C1 03 C0 '..
  '0F BF 9C 00 ? ? ? ? 0F BF AC 00 ? ? ? ? 03 C0 66 85 D2 0F BF 86 ? ? ? ? 66 89 8E ? ? ? ? 7E 4E '..
  '69 C0 90 04 00 00 66 83 FA 01 7F 25 0F BF 88 ? ? ? ? 0F BF 90 ? ? ? ? 55 53 51 52 B9 ? ? ? ? E8 ? ? ? ? '..
  '83 3D ? ? ? ? 05 EB 08 66 83 B8 ? ? ? ? 60 7E 13 6A 00 6A 00 6A 00 55 53 57 B9 ? ? ? ? E8 ? ? ? ?'
local rallyMove = 0x127
-- The native callers pass the player and the AI state object.
local callerPrefix = {0x56, 0x8B, 0xCB}
local scanLength = 0xA00

function M.resolve(game)
  local read = core.readInteger
  local site = context.find('attack patrol dispatch', dispatcherSignature)
  assert(read(site + 0x0E) == game.players + 0x2300
      and read(site + 0x20) == game.players + 0x310C + 15 * 2
      and read(site + 0x32) == game.tribeStride
      and read(site + 0x39) == game.tribes + 0x34
      and read(site + 0x3F) == game.players + 0x329C + 15 * 4
      and read(site + 0x49) == game.tribes + 0x5A
      and read(site + 0x53) - game.tribes == game.tribeStance,
    'AIC Tactics: inconsistent native attack patrol dispatch')
  local troopValueState = read(site + 0x5E)
  local destination = site + 0x67 + read(site + 0x63)
  context.verify(destination, 'attack patrol destination', destinationSignature)
  assert(read(destination + 9) == game.tribeStride
      and read(destination + 17) == game.tribes + 0x5A
      and read(destination + 36) == 0x490
      and read(destination + 43) == game.unitRecords + 0x96
      and read(destination + 50) == game.unitRecords + 0xC6
      and read(destination + 57) == game.unitRecords + 0xC4
      and troopValueState > 0,
    'AIC Tactics: inconsistent native attack patrol destination')
  local move = game.siegeGroupMove
  context.verify(move + 0x5B, 'group move playable map check', playableSignature)
  local playable = read(move + 0x66)
  assert(playable > 0 and read(move + 0xC1) == game.mapRows,
    'AIC Tactics: inconsistent native group move map layout')
  -- Every native caller lies inside the AI state update. Require all three.
  local owner = game.updateAIPlayerState
  local function callsTo(target)
    local found = {}
    for offset = 3, scanLength - 5 do
      local address = owner + offset
      if core.readByte(address) == 0xE8 and address + 5 + read(address + 1) == target then
        found[#found + 1] = address
      end
    end
    return found
  end
  local calls = callsTo(site)
  assert(#calls == 3, 'AIC Tactics: native attack patrol callers changed')
  for _, address in ipairs(calls) do
    for index, byte in ipairs(callerPrefix) do
      assert(core.readByte(address - 4 + index) == byte, 'AIC Tactics: unexpected native attack patrol caller')
    end
  end
  local search = context.call(destination + searchCalls[1], 'patrol tile search')
  for _, offset in ipairs(searchCalls) do
    assert(context.call(destination + offset, 'patrol tile search') == search,
      'AIC Tactics: inconsistent native patrol tile search')
  end
  context.verify(search + 0x41, 'patrol tile search map check', '80 BC 29 ? ? ? ? 00')
  context.verify(search + layerRead, 'patrol distance layer', layerSignature)
  local layerIndex, layers = read(search + layerRead + 0x13), read(search + layerRead + 0x26)
  assert(read(search + 0x44) == playable and layerIndex > 0 and layers > 0,
    'AIC Tactics: inconsistent native patrol distance layer')
  local rally = context.find('siege defence rally', rallySignature)
  local rallyCount, rallyIndex, rallyPoints = read(rally + 0x96), read(rally + 0x9D), read(rally + 0xBD)
  assert(read(rally + 0x0F) == game.players + 0x2300
      and read(rally + 0x24) == game.players + 0x310C + 190 * 2
      and read(rally + 0x5A) == game.players + 0x329C + 190 * 4
      and read(rally + 0x4B) == game.tribeStride and read(rally + 0xB1) * 4 == game.tribeStride
      and read(rally + 0x53) == game.tribes + 0x34
      and read(rally + 0x66) == game.tribes and read(rally + 0x123) == game.tribes
      and read(rally + 0x6D) == game.tribes + game.tribeStance
      and rallyIndex == rallyCount - 2 and read(rally + 0xD8) == rallyIndex
      and rallyPoints == rallyIndex - 10 * 4 and read(rally + 0xC5) == rallyPoints + 2
      and read(rally + 0xD1) == game.tribes + 0x5A
      and read(rally + 0xED) == game.unitRecords + 0xC6 and read(rally + 0xF4) == game.unitRecords + 0xC4
      and read(rally + 0x112) == game.unitRecords + 0x2A6
      and context.call(rally + rallyMove, 'siege defence rally move') == game.siegeGroupMove,
    'AIC Tactics: inconsistent native siege defence rally')
  -- Its only callers are the two in attack states 3 and 4, each directly before
  -- the attack patrol call.
  local rallyCalls = callsTo(rally)
  assert(#rallyCalls == 2, 'AIC Tactics: native siege defence rally callers changed')
  for _, address in ipairs(rallyCalls) do
    assert(address + 8 == calls[1] or address + 8 == calls[2],
      'AIC Tactics: unexpected native siege defence rally caller')
  end
  return {attackPatrolDispatch = site, attackPatrolDestination = destination,
    troopValueState = troopValueState, playableTiles = playable, attackPatrolCalls = calls,
    distanceLayers = layers, distanceLayerIndex = layerIndex,
    tribeRallyPoints = rallyPoints - game.tribes, tribeRallyCount = rallyCount - game.tribes,
    siegeDefenceRallyMove = rally + rallyMove}
end

-- `expected` is the current target of the native callers: the game's
-- dispatcher, or the module handler once installed.
function M.preflight(game, expected)
  for _, site in ipairs(game.attackPatrolCalls) do
    assert(context.call(site, 'attack patrol order') == expected,
      'AIC Tactics: native attack patrol orders were replaced; restart with compatible modules')
  end
end

function M.install(game, handler)
  for _, site in ipairs(game.attackPatrolCalls) do core.writeCode(site, {core.callTo(handler)}) end
end

-- `expected` is the current target of the rally's move order: the game's group
-- move, or the siege screen handler once installed.
function M.preflightScreen(game, expected)
  assert(context.call(game.siegeDefenceRallyMove, 'siege defence rally move') == expected,
    'AIC Tactics: the native siege defence rally order was replaced; restart with compatible modules')
end

function M.installScreen(game, handler)
  local site = game.siegeDefenceRallyMove
  core.writeCode(site, {core.callTo(handler)})
end

return M
