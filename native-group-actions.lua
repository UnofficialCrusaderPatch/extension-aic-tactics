local M = {}
local context = require('native-context')

-- Native AIC assignment and group/order boundaries. No replacement allocator,
-- membership writer, movement queue or pathfinder is introduced here.
function M.resolve(game)
  local read = core.readInteger
  local result = {}
  local sites = {}
  result.assignMoatDigger = context.find('assignMoatDigger',
    '53 8B 5C 24 08 8B C3 69 C0 90 04 00 00 0F BF 88 ? ? ? ? 56 8B F1 69 F6 F4 39 00 00 66 C7 80 ? ? ? ? '..
    '05 00 0F BF 86 ? ? ? ? 85 C0 57 74 16 8B F8 69 FF ? ? ? ? 8B 97 ? ? ? ? 3B 96 ? ? ? ? 74 26 '..
    '51 B9 ? ? ? ? E8 ? ? ? ? 8B F8 69 FF ? ? ? ? 8B 8F ? ? ? ? 66 89 86 ? ? ? ? 89 8E ? ? '..
    '? ? 50 53 B9 ? ? ? ? E8 ? ? ? ? 66 C7 87 ? ? ? ? 01 00 5F 5E 5B C2 04 00')
  result.wallDefense = context.find('wallDefense',
    '53 55 8B 6C 24 0C 8B DD 69 DB 90 04 00 00 56 0F BF B3 ? ? ? ? 8B C6 69 C0 F4 39 00 00 83 B8 ? ? ? ? '..
    '00 57 8B F9 0F 84 9B 00 00 00 83 B8 ? ? ? ? 01 75 16 8B C6 69 C0 F4 39 00 00 66 83 B8 ? ? ? ? 00 7E '..
    '04 6A 01 EB 02 6A 00 55 E8 ? ? ? ? 8B CE 69 C9 7D 0E 00 00 03 C8 83 3C 8D ? ? ? ? 00 66 C7 83 ? ? '..
    '? ? 01 00 8B CF 7F 42 55 E8 ? ? ? ? 85 C0 8B CF 74 1B 6A 00 56 E8 ? ? ? ? 50 55 B9 ? ? ? ? E8 '..
    '? ? ? ? 5F 5E 5D 5B C2 04 00 6A 01 56 E8 ? ? ? ? 50 55 B9 ? ? ? ? E8 ? ? ? ? 5F 5E 5D 5B C2 '..
    '04 00 50 56 E8 ? ? ? ? 50 55 B9 ? ? ? ? E8 ? ? ? ? 5F 5E 5D 5B C2 04 00')
  result.patrolDefense = context.find('patrolDefense',
    '56 57 8B 7C 24 0C 8B D7 69 D2 90 04 00 00 0F BF B2 ? ? ? ? 8B C6 69 C0 F4 39 00 00 8B 80 ? ? ? ? 85 '..
    'C0 74 2C 83 C0 FF 69 C0 A4 02 00 00 66 C7 82 ? ? ? ? 04 00 8B 94 08 74 01 00 00 52 56 E8 ? ? ? ? 50 '..
    '57 B9 ? ? ? ? E8 ? ? ? ? 5F 5E C2 04 00')
  result.assignRaider = context.find('assignRaider',
    '56 8B 74 24 08 8B C6 69 C0 90 04 00 00 0F BF 90 ? ? ? ? 57 8B FA 69 FF F4 39 00 00 83 BF ? ? ? ? 00 '..
    '74 20 56 52 66 C7 80 ? ? ? ? 02 00 E8 ? ? ? ? 85 C0 74 0C 50 56 B9 ? ? ? ? E8 ? ? ? ? 5F 5E '..
    'C2 04 00')
  result.assignAttacker = context.find('assignAttacker',
    '56 57 8B 7C 24 0C 8B C7 69 C0 90 04 00 00 0F BF 90 ? ? ? ? 8B F2 69 F6 F4 39 00 00 83 BE ? ? ? ? 00 '..
    '74 23 8B 74 24 10 56 57 52 66 89 B0 ? ? ? ? E8 ? ? ? ? 85 C0 74 0C 50 57 B9 ? ? ? ? E8 ? ? ? '..
    '? 5F 5E C2 08 00')
  result.findSortieGroup = context.find('findSortieGroup',
    '53 56 8B 74 24 0C 8B C6 69 C0 FA 1C 00 00 57 8B 7C 24 14 03 C7 8D 1C 45 ? ? ? ? 0F BF 03 85 C0 74 21 8B '..
    'CE 8B D0 69 C9 7D 0E 00 00 69 D2 ? ? ? ? 8B 92 ? ? ? ? 03 CF 3B 14 8D ? ? ? ? 74 2B 56 B9 ? ? '..
    '? ? E8 ? ? ? ? 69 F6 7D 0E 00 00 8B C8 69 C9 ? ? ? ? 8B 91 ? ? ? ? 03 F7 66 89 03 89 14 B5 ? '..
    '? ? ? 5F 5E 5B C2 08 00')
  result.findAttackGroup = context.find('findAttackGroup',
    '83 EC 08 53 8B 5C 24 10 8B C3 69 C0 F4 39 00 00 8B 80 ? ? ? ? 85 C0 75 07 5B 83 C4 08 C2 0C 00 55 8D 50 '..
    'FF 8B 44 24 1C 56 8B 34 85 ? ? ? ? 57 33 C0 33 ED 8D 7E F1 81 FF B1 00 00 00 89 44 24 14 89 6C 24 10 C7 '..
    '44 24 24 E8 03 00 00 77 4F 0F B6 BF ? ? ? ? FF 24 BD ? ? ? ? 69 D2 A4 02 00 00 8B 94 0A 64 02 00 00 '..
    'EB 2B')
  result.returnTribe = context.find('returnTribe',
    '8B 44 24 08 69 C0 F4 39 00 00 83 B8 ? ? ? ? 00 7E 30 8B 88 ? ? ? ? 8B 90 ? ? ? ? 56 8B 74 24 08 '..
    '6A 00 51 52 56 B9 ? ? ? ? E8 ? ? ? ? 69 F6 ? ? ? ? 66 C7 86 ? ? ? ? 02 00 5E C2 08 00')
  result.removeUnitFromTribe = context.find('removeUnitFromTribe',
    '53 55 56 8B 74 24 10 8B D9 8B CE 69 C9 90 04 00 00 66 83 B9 ? ? ? ? 00 57 8B 7C 24 18 75 39 8B EF 69 ED '..
    '? ? ? ? 8B C6 99 83 E2 0F 03 C2 C1 F8 04 03 E8 0F BF 44 6B 60 8B D6 81 E2 0F 00 00 80 79 05 4A 83 CA F0 '..
    '42 0F B7 14 55 ? ? ? ? 85 C2 0F 84 8D 00 00 00 8B C7 69 C0 ? ? ? ? 8D 2C 18 0F B7 45 5C 66 85 C0 7E '..
    '07 83 C0 FF 66 89 45 5C 66 83 7D 5C 00 7F 06 66 C7 45 40 03 00 8B C6 99 83 E2 0F 03 C2 8B D7 69 D2 ? ? ? '..
    '? C1 F8 04 03 D0 8D 44 53 60 8B D6 81 E2 0F 00 00 80 79 05 4A 83 CA F0 42 66 8B 14 55 ? ? ? ? 66 21 10 '..
    '0F BF 81 ? ? ? ? 3B C7 75 17 33 C0 66 89 81 ? ? ? ? 66 89 81 ? ? ? ? 66 89 81 ? ? ? ? 0F BF '..
    '4D 5A 3B CE 75 08 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 5F 5E 5D 5B C2 08 00')
  sites.raidOrder = context.find('raidOrder',
    '8B CF 69 C9 2C 03 00 00 8B 91 ? ? ? ? 50 8B 44 24 18 8D 99 ? ? ? ? 52 57 6A 09 50 B9 ? ? ? ? E8 '..
    '? ? ? ? 8B 0B 8B 5C 24 20 66 89 BE ? ? ? ? 89 8E ? ? ? ?')
  sites.raidMapRow = context.find('raidMapRow',
    '8D 0C 40 8B 14 8D ? ? ? ? 03 54 24 18 53 53 52 6A 23 57 B9 ? ? ? ? E8 ? ? ? ? 5B 5E 5D 5F 83 C4 '..
    '0C C2 04 00')
  do local site = result.assignMoatDigger
    assert(read(site + 16) == game.unitRecords + 0x96
      and read(site + 32) == game.unitRecords + 0x42A
      and read(site + 41) == game.players + 0x3120
      and read(site + 54) == game.tribeStride
      and read(site + 60) == game.tribes + 0x34
      and read(site + 66) == game.players + 0x32C4
      and read(site + 74) == game.tribes
      and context.call(site + 78, 'assignMoatDigger') == game.createTribe
      and read(site + 87) == game.tribeStride
      and read(site + 93) == game.tribes + 0x34
      and read(site + 100) == game.players + 0x3120
      and read(site + 106) == game.players + 0x32C4
      and read(site + 113) == game.tribes
      and context.call(site + 117, 'assignMoatDigger') == game.addUnitToTribe
      and read(site + 125) == game.tribes + game.tribeStance
      , 'AIC Tactics: inconsistent native assignMoatDigger bindings')
  end
  do local site = result.wallDefense
    assert(read(site + 18) == game.unitRecords + 0x96
      and read(site + 32) == game.players + 0x2300
      and read(site + 48) == game.players + 0x3864
      and read(site + 66) == game.players + 0x386E
      and read(site + 98) == game.players + 0x308C
      and read(site + 106) == game.unitRecords + 0x42A
      and context.call(site + 131, 'wallDefense') == result.findSortieGroup
      and read(site + 139) == game.tribes
      and context.call(site + 143, 'wallDefense') == game.addUnitToTribe
      and context.call(site + 158, 'wallDefense') == result.findSortieGroup
      and read(site + 166) == game.tribes
      and context.call(site + 170, 'wallDefense') == game.addUnitToTribe
      and read(site + 192) == game.tribes
      and context.call(site + 196, 'wallDefense') == game.addUnitToTribe
      , 'AIC Tactics: inconsistent native wallDefense bindings')
  end
  do local site = result.patrolDefense
    assert(read(site + 17) == game.unitRecords + 0x96
      and read(site + 31) == game.players + 0x2300
      and read(site + 51) == game.unitRecords + 0x42A
      and read(site + 74) == game.tribes
      and context.call(site + 78, 'patrolDefense') == game.addUnitToTribe
      , 'AIC Tactics: inconsistent native patrolDefense bindings')
  end
  do local site = result.assignRaider
    assert(read(site + 16) == game.unitRecords + 0x96
      and read(site + 31) == game.players + 0x2300
      and read(site + 43) == game.unitRecords + 0x42A
      and read(site + 61) == game.tribes
      and context.call(site + 65, 'assignRaider') == game.addUnitToTribe
      , 'AIC Tactics: inconsistent native assignRaider bindings')
  end
  do local site = result.assignAttacker
    assert(read(site + 17) == game.unitRecords + 0x96
      and read(site + 31) == game.players + 0x2300
      and read(site + 48) == game.unitRecords + 0x42A
      and context.call(site + 52, 'assignAttacker') == result.findAttackGroup
      and read(site + 64) == game.tribes
      and context.call(site + 68, 'assignAttacker') == game.addUnitToTribe
      , 'AIC Tactics: inconsistent native assignAttacker bindings')
  end
  do local site = result.findSortieGroup
    assert(read(site + 24) == game.players + 0x310C
      and read(site + 47) == game.tribeStride
      and read(site + 53) == game.tribes + 0x34
      and read(site + 62) == game.players + 0x329C
      and read(site + 70) == game.tribes
      and context.call(site + 74, 'findSortieGroup') == game.createTribe
      and read(site + 89) == game.tribeStride
      and read(site + 95) == game.tribes + 0x34
      and read(site + 107) == game.players + 0x329C
      , 'AIC Tactics: inconsistent native findSortieGroup bindings')
  end
  do local site = result.findAttackGroup
    assert(read(site + 18) == game.players + 0x2300
      , 'AIC Tactics: inconsistent native findAttackGroup bindings')
  end
  do local site = result.returnTribe
    assert(read(site + 12) == game.players + 0x1D4
      and read(site + 21) == game.players + 0x1DC
      and read(site + 27) == game.players + 0x1D8
      and read(site + 42) == game.tribes
      and read(site + 53) == game.tribeStride
      and read(site + 60) == game.tribes + game.tribeStance
      , 'AIC Tactics: inconsistent native returnTribe bindings')
  end
  do local site = result.removeUnitFromTribe
    assert(read(site + 20) == game.unitRecords + 0x2D8
      and read(site + 36) == game.tribeStride / 2
      and read(site + 93) == game.tribeStride
      and read(site + 141) == game.tribeStride / 2
      and read(site + 183) == game.unitRecords + 0x2D8
      and read(site + 196) == game.unitRecords + 0x2D6
      and read(site + 203) == game.unitRecords + 0x2D8
      and read(site + 210) == game.unitRecords + 0x2DA
      , 'AIC Tactics: inconsistent native removeUnitFromTribe bindings')
  end
  do local site = sites.raidOrder
    assert(read(site + 10) == game.buildings + 0x14 + 0xD8
      and read(site + 21) == game.buildings + 0x14 + 0xD8
      and read(site + 31) == game.units
      and read(site + 49) == game.tribes + game.tribeTargetBuilding
      and read(site + 55) == game.tribes + game.tribeTargetBuildingUID
      , 'AIC Tactics: inconsistent native raidOrder bindings')
  end
  do local site = sites.raidMapRow
    assert(read(site + 21) == game.units
      , 'AIC Tactics: inconsistent native raidMapRow bindings')
  end
  result.relayRaidOrder = context.call(sites.raidOrder + 35, 'raid order')
  local orderDispatch = context.find('group order dispatch',
    '83 EC 1C 8B 44 24 20 69 C0 ? ? ? ? 53 8B 5C 24 30 55 8D 2C 08 33 D2 56 0F BF 75 5C 89 4C 24 10 8B 4C 24 30 57 8B 7C 24 38 8D 41 FD 83 F8 23 '..
    '89 7C 24 10 89 5C 24 28 89 54 24 1C 89 54 24 24 89 54 24 20 89 6C 24 18 66 89 95 ? ? ? ? 66 89 95 ? ? ? ?')
  assert(context.call(sites.raidMapRow + 25, 'raid movement order') == result.relayRaidOrder
    and core.readByte(result.relayRaidOrder) == 0xB9 and read(result.relayRaidOrder + 1) == game.tribes
    and core.readByte(result.relayRaidOrder + 5) == 0xE9
    and result.relayRaidOrder + 10 + read(result.relayRaidOrder + 6) == orderDispatch
    and read(orderDispatch + 9) == game.tribeStride
    and read(orderDispatch + 75) == game.tribeStance + 8
    and read(orderDispatch + 82) == game.tribeStance - 0xA6,
    'AIC Tactics: unsupported native group order relay')
  result.mapRows = read(sites.raidMapRow + 6)
  result.attackGroupSlots = read(result.findAttackGroup + 45) + 10 * 4
  for _, value in pairs(result) do
    assert(type(value) == 'number' and value > 0, 'AIC Tactics: invalid native group action operand')
  end
  return result
end

return M
