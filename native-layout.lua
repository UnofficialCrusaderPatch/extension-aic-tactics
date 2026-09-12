local M = {}

-- This module owns AIC's native layout binding, not allocation or a second pool.
-- All discovery uses the framework; consumers receive the result once at init.
local function unique(name, signature)
  local site = core.AOBScan(signature)
  assert(type(site) == 'number' and site > 0, 'AIC Tactics: missing native '..name)
  local second = core.scanForAOB(signature, site + 1)
  assert(second == nil or second == 0, 'AIC Tactics: ambiguous native '..name)
  return site
end

function M.resolve()
  local read = core.readInteger
  -- Whole clear-units loop: both allocation limit and record stride are native.
  local units = unique('unit pool',
    '53 55 56 8B F1 57 33 FF 89 7E 04 89 7E 08 89 7E 20 89 7E 24 89 7E 28 89 7E 2C 89 7E 30 89 7E 34 89 7E 38 89 7E 3C 89 7E 40 89 7E 44 '..
    '8D 9E 14 06 00 00 BD ? ? ? ? 53 57 68 90 04 00 00 B9 ? ? ? ? E8 ? ? ? ? 81 C3 90 04 00 00 83 ED 01 75 E4 '..
    '68 ? ? ? ? 57 68 20 74 02 00 B9 ? ? ? ? C7 06 ? ? ? ? E8 ? ? ? ? 5F 5E 5D 5B C3')
  local capacity = read(units + 51)
  assert((capacity == 2500 or capacity == 10000) and read(units + 101) == capacity,
    'AIC Tactics: unsupported native unit pool layout')

  -- Native per-player allocator owns ID order, lifecycle, UID and zeroing.
  local create = unique('army allocator',
    '53 55 8B 6C 24 0C 56 57 BF ? ? ? ? 2B FD 33 DB 3B FB 7E 49 8B C7 69 C0 ? ? ? ? 8D 44 08 40 66 39 18 74 15 83 EF 08 2D ? ? ? ? 3B FB 7F EF '..
    '5F 5E 5D 33 C0 5B C2 04 00 8B D7 69 D2 ? ? ? ? 8D 34 0A 8D 46 28 50 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 3B FB 7F 09 5F 5E 5D 33 C0 5B C2 04 00 '..
    '89 6E 2C 66 C7 46 40 02 00 8B 0D ? ? ? ? 89 4E 34 83 05 ? ? ? ? 01 8B C7 5F 66 89 5E 42 66 89 5E 44 66 89 5E 46 66 89 5E 50 66 89 5E 58 66 89 5E 5A 66 89 5E 5C 5E 5D 5B C2 04 00')
  local stride = read(create + 25)
  assert(read(create + 9) == 1250 and read(create + 42) == stride * 8
      and read(create + 63) == stride and read(create + 76) == stride
      and ((capacity == 2500 and stride == 0x334) or (capacity == 10000 and stride == 0x688)),
    'AIC Tactics: unsupported native army allocation layout')

  -- Native raid order validates a building's lifetime, then writes its ID/UID
  -- and stance. Those fields move after Extreme's expanded membership bitset.
  local order = unique('army target layout',
    '55 8B 6C 24 08 56 8B F5 69 F6 ? ? ? ? 8B 86 ? ? ? ? 0F BF 96 ? ? ? ? 57 8B 7C 24 14 81 FF BA 00 00 00 0F 84 AF 00 00 00 '..
    '83 FF 12 0F 84 A6 00 00 00 83 FF 0E 0F 84 9D 00 00 00 83 FF 0D 0F 84 94 00 00 00 85 D2 74 29 69 D2 2C 03 00 00 8B BA ? ? ? ? 3B BE ? ? ? ? 75 15 66 83 BA ? ? ? ? 00 75 0B '..
    '5F 5E B8 01 00 00 00 5D C2 08 00 69 C0 F4 39 00 00 8B 80 ? ? ? ? 50 66 C7 86 ? ? ? ? 00 00 E8 ? ? ? ? 8B F8 85 FF 74 46 8B CF 69 C9 2C 03 00 00 '..
    '8B 91 ? ? ? ? 53 8D 99 ? ? ? ? 52 57 6A 09 55 B9 ? ? ? ? E8 ? ? ? ? 8B 03 5B 89 86 ? ? ? ? 66 89 BE ? ? ? ? B8 01 00 00 00 5F 66 89 86 ? ? ? ? 5E 5D C2 08 00')
  local tribes = read(order + 16) - 0x2C
  local target = read(order + 23) - tribes
  local uid = read(order + 89) - tribes
  local stance = read(order + 210) - tribes
  local buildings = read(order + 83) - 0xD8 - 0x14
  assert(read(order + 10) == stride and uid == target + 4
      and target == stance + 24 and stance > 0x60 + math.ceil(capacity / 16) * 2
      and uid + 4 <= stride + 0x28
      and read(order + 98) == buildings + 0x14 + 0x2BE
      and read(order + 132) == tribes + target and read(order + 159) == buildings + 0x14 + 0xD8
      and read(order + 166) == buildings + 0x14 + 0xD8
      and read(order + 190) == tribes + uid and read(order + 197) == tribes + target,
    'AIC Tactics: inconsistent native army target layout')

  local path = unique('army path layout',
    '8B 44 24 04 69 C0 ? ? ? ? 8B 88 ? ? ? ? 0F BF 80 ? ? ? ? 69 C0 90 04 00 00 8B 90 ? ? ? ? 0F BF 04 55 ? ? ? ? 8B 54 24 08 0F BF 14 55 ? ? ? ? 6A 00 52 50 51 B9 ? ? ? ? E8 ? ? ? ? F7 D8 1B C0 F7 D8 C2 08 00')
  local unitState = read(order + 176)
  local records = read(path + 31) - 0xD4
  assert(read(path + 6) == stride and read(path + 12) == tribes + 0x2C
      and read(path + 19) == tribes + 0x5A and records == unitState + 0x614,
    'AIC Tactics: inconsistent native army/unit layout')

  local add = unique('army membership',
    '53 8B 5C 24 08 56 57 8B 7C 24 14 8B C7 69 C0 ? ? ? ? 66 83 44 08 5C 01 8D 34 08 8B C3 99 83 E2 0F 03 C2 8B D7 69 D2 ? ? ? ? C1 F8 04 03 D0 8D 44 51 60 '..
    '8B D3 81 E2 0F 00 00 80 79 05 4A 83 CA F0 42 66 8B 14 55 ? ? ? ? 66 09 10 66 83 7E 5A 00 75 04 66 89 5E 5A 0F B7 56 5A 8B C3 69 C0 90 04 00 00 '..
    '66 89 90 ? ? ? ? 66 89 B8 ? ? ? ? 8B 56 34 89 90 ? ? ? ? 0F B7 56 5C 57 66 89 90 ? ? ? ? E8 ? ? ? ? 5F 5E 5B C2 08 00')
  assert(read(add + 15) == stride and read(add + 40) * 2 == stride
      and read(add + 105) == records + 0x2D6 and read(add + 112) == records + 0x2D8
      and read(add + 121) == records + 0x2E4 and read(add + 133) == records + 0x2DA,
    'AIC Tactics: inconsistent native membership layout')

  local clearBuildings = unique('building pool',
    '53 56 8B D9 57 C7 03 00 00 00 00 8D 73 14 BF ? ? ? ? 56 6A 00 68 2C 03 00 00 B9 ? ? ? ? E8 ? ? ? ? 81 C6 2C 03 00 00 83 EF 01 75 E3 5F 5E C7 43 08 ? ? ? ? 5B C3')
  assert(read(clearBuildings + 15) == 2000 and read(clearBuildings + 53) == 2000,
    'AIC Tactics: unsupported native building pool layout')

  local clearEntities = unique('projectile pool',
    '53 56 8B D9 57 C7 03 00 00 00 00 8D 73 14 BF ? ? ? ? 56 6A 00 68 E8 00 00 00 B9 ? ? ? ? E8 ? ? ? ? 81 C6 E8 00 00 00 83 EF 01 75 E3 '..
    '8D B3 ? ? ? ? BF 64 00 00 00 EB 03 8D 49 00 56 6A 00 6A 64 B9 ? ? ? ? E8 ? ? ? ? 83 C6 64 83 EF 01 75 E9 5F 5E C7 43 04 ? ? ? ? C7 43 08 19 00 00 00 5B C3')
  local entityCapacity = read(clearEntities + 15)
  assert((entityCapacity == 3000 or entityCapacity == 6000)
      and read(clearEntities + 92) == entityCapacity and read(clearEntities + 50) == 0x14 + entityCapacity * 0xE8,
    'AIC Tactics: unsupported native projectile pool layout')
  local damageOwner = unique('projectile damage attribution',
    '0F B7 84 3E AA 06 00 00 0F BF D8 0F B7 85 ? ? ? ? 89 44 24 30 0F BF C0 03 C0 8B AC 00 ? ? ? ? 03 C0 3B 2C 9D ? ? ? ? 75 5B '..
    '83 7C 24 14 00 0F 84 ? ? ? ? 8B 6C 24 28 0F B7 AD ? ? ? ? 66 83 FD 14')
  local entities = read(damageOwner + 14) - 0x2C
  local teams = read(damageOwner + 30)
  assert(read(damageOwner + 39) == teams and read(damageOwner + 63) == entities + 0x2A,
    'AIC Tactics: inconsistent native projectile ownership layout')

  local result = {units=unitState, unitRecords=records, unitCapacity=capacity,
    tribes=tribes, tribeStride=stride, tribeMemberWords=math.ceil(capacity / 16),
    tribeStance=stance, tribeTargetBuilding=target, tribeTargetBuildingUID=uid,
    buildings=buildings, buildingCapacity=2000, players=read(order + 124) - 0x2BD8,
    createTribe=create, addUnitToTribe=add, tribePath=path,
    entities=entities, entityCapacity=entityCapacity, teams=teams}
  for _, value in pairs(result) do
    assert(type(value) == 'number' and value > 0 and value == math.floor(value),
      'AIC Tactics: invalid native layout operand')
  end
  return result
end

return M
