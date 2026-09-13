local context = require('native-context')
local M = {}
local signatures = {
  findRecruitmentBuilding = '53 56 8B 71 08 B8 01 00 00 00 3B F0 57 7E 3C 8B 7C 24 14 8B 5C 24 10 81 C1 10 04 00 00 8D 49 00 0F B7 11 66 85 D2 74 16 66 83 FA 03 74 10 0F BF 51 06 3B D3 75 08 0F BF 51 02 3B D7 74 0F 83 C0 01 81 C1 2C 03 00 00 3B C6 7C D5 33 C0 5F 5E 5B C2 08 00',
  buildingCaller = '83 FB 05 75 08 8B 86 ? ? ? ? EB 27 83 FB 25 75 0F 6A 26 55 B9 ? ? ? ? E8 ? ? ? ? EB 13 83 FB 46 7D 08 8B 86 ? ? ? ? EB 06 8B 86 ? ? ? ? 85 C0 0F 84 0C 01 00 00',
  attackRecruitType = '8B 54 24 04 69 D2 F4 39 00 00 8B 82 ? ? ? ? 85 C0 75 03 C2 08 00 56 8B 74 24 0C 83 C0 FF 83 FE 0A 75 07 8D 46 14 5E C2 08 00 83 FE 0B 75 11 69 C0 A4 02 00 00 8B 84 08 40 02 00 00 5E C2 08 00 83 FE 0C 75 07 8D 46 3D 5E C2 08 00 83 FE 0D 75 11 69 C0 A4 02 00 00 8B 84 08 48 02 00 00 5E C2 08 00 83 FE 0E 75 07 8D 46 0F 5E C2 08 00 83 FE 0F 75 07 8D 46 F6 5E C2 08 00 83 FE 10 75 11 69 C0 A4 02 00 00 8B 84 08 5C 02 00 00 5E C2 08 00 83 FE 11 75 11 69 C0 A4 02 00 00 8B 84 08 68 02 00 00 5E C2 08 00 83 FE 12 75 11 69 C0 A4 02 00 00 8B 84 08 74 02 00 00 5E C2 08 00 83 FE 13 75 11 69 C0 A4 02 00 00 8B 84 08 7C 02 00 00 5E C2 08 00 83 FE 14 75 51 8B B2 ? ? ? ? 69 C0 A9 00 00 00 03 F0 83 BC B1 88 02 00 00 00 75 0A C7 82 ? ? ? ? 00 00 00 00 83 BA ? ? ? ? 04 7C 0A C7 82 ? ? ? ? 00 00 00 00 8B B2 ? ? ? ? 03 C6 8B 84 81 88 02 00 00 83 C6 01 89 B2 ? ? ? ? 5E C2 08 00 69 C0 A4 02 00 00 8B 84 08 88 02 00 00 5E C2 08 00',
  roleCaller = '83 F8 02 75 17 55 8B CF E8 ? ? ? ? 50 55 8B CF 89 44 24 20 E8 ? ? ? ? 8B D8 85 DB 0F 84 62 01 00 00',
  raidMaximum = '83 3D ? ? ? ? 03 56 75 49 83 3D ? ? ? ? 01 75 40 83 3D ? ? ? ? 02 75 37 8B 44 24 0C 8B 74 24 08 69 C0 F4 39 00 00 8B 90 ? ? ? ? 69 F6 A4 02 00 00 03 94 0E A4 01 00 00 B8 56 55 55 55 03 D2 03 D2 F7 EA 8B C2 C1 E8 1F 03 C2 5E C2 08 00 8B 54 24 0C 8B 74 24 08 69 D2 F4 39 00 00 8B 82 ? ? ? ? 69 F6 A4 02 00 00 03 84 0E A4 01 00 00 5E C2 08 00',
  defenseTypes = '8B 44 24 04 69 C0 90 04 00 00 0F B7 88 ? ? ? ? 66 83 F9 01 0F BF 90 ? ? ? ? 75 09 0F BF 88 ? ? ? ? EB 03 0F BF C9 8B C2 69 C0 F4 39 00 00 8B 90 ? ? ? ? 83 EA 01 83 FA 06 74 31 83 7C 24 08 00 74 2A 83 B8 ? ? ? ? 00 7E 21 83 F9 16 75 08 B8 0D 00 00 00 C2 08 00 83 F9 17 74 F3 83 F9 46 74 EE 83 F9 48 74 E9 83 F9 4C 74 E4 33 C0 39 0C 85 ? ? ? ? 74 0A 83 C0 01 83 F8 14 7C EF 33 C0 C2 08 00',
  specialDefenders = '8B 44 24 04 69 C0 90 04 00 00 0F BF 88 ? ? ? ? B8 ? ? ? ? 39 08 74 0F 83 C0 04 3D ? ? ? ? 7C F2 33 C0 C2 04 00 B8 01 00 00 00 C2 04 00',
  defenseSlots = '83 EC 14 55 8B 6C 24 1C 8B C5 69 C0 F4 39 00 00 8B 90 ? ? ? ? 56 33 F6 3B D6 75 0A 5E 33 C0 5D 83 C4 14 C2 08 00 8B 44 24 24 53 57 83 C2 FF 8B FD 69 D2 A4 02 00 00 8B 94 0A 14 01 00 00 69 FF 7D 0E 00 00 8D 1C 07 8B 1C 9D ? ? ? ? 3B DA 89 74 24 14 89 74 24 18 C7 44 24 10 E8 03 00 00 7E 11 83 F8 08 74 0A 83 F8 0A 74 05 83 F8 11 75 02 8B DA 85 DB 8B 04 85 ? ? ? ? 89 44 24 1C 7E 7E 69 ED FA 1C 00 00 03 C5 8D 0C 45 ? ? ? ? 89',
  raidTypes = '56 57 8B 7C 24 10 33 C0 69 FF 90 04 00 00 0F BF 97 ? ? ? ? 8B 0C 85 ? ? ? ? 3B D1 74 63 0F BF B7 ? ? ? ? 3B F1 74 58 8B 0C 85 ? ? ? ? 3B D1 74 3B 3B F1 74 37 8B 0C 85 ? ? ? ? 3B D1 74 31 3B F1 74 2D 8B 0C 85 ? ? ? ? 3B D1 74 27 3B F1 74 23 8B 0C 85 ? ? ? ? 3B D1 74 1D 3B F1 74 19 83 C0 05 83 F8 14 7C A6 EB 12 83 C0 01 EB 0D 83 C0 02 EB 08 83 C0 03 EB 03 83 C0 04',
  equipment = '83 EC 14 8B 44 24 18 53 56 8B F1 83 C0 EA 57 33 FF 89 BE 0C 06 00 00 8B 0C 85 ? ? ? ? C1 E0 04 39 3D ? ? ? ? 8B 90 ? ? ? ? 8B 98 ? ? ? ? 89 4C 24 0C 8B 88 ? ? ? ? 8B 80 ? ? ? ? 89 4C 24 18 89 54 24 1C 89 5C 24 10 89 44 24 14',
}

function M.resolve(game)
  local sites={}
  for _,name in ipairs({'findRecruitmentBuilding','buildingCaller','attackRecruitType','roleCaller',
      'raidMaximum','defenseTypes','specialDefenders','defenseSlots','raidTypes'}) do
    sites[name]=context.find(name,signatures[name])
  end
  local function operand(site,offset,value)
    assert(core.readInteger(site+offset)==value,'AIC Tactics: incompatible native AIC query operand')
  end
  local b,t,d,s,r=sites.buildingCaller,sites.attackRecruitType,sites.defenseTypes,sites.defenseSlots,sites.raidTypes
  operand(b,7,game.players+0x224);operand(b,22,game.buildings)
  operand(b,40,game.players+0x15C);operand(b,48,game.players+0x24C)
  assert(context.call(b+26,'recruitment building')==sites.findRecruitmentBuilding
      and context.call(sites.roleCaller+21,'attack role')==t,
    'AIC Tactics: inconsistent native AIC query callers')
  operand(t,12,game.players+0x2300)
  for _,offset in ipairs({218,242,252,261,271,289}) do operand(t,offset,game.players+0x3108) end
  operand(sites.raidMaximum,2,game.scenarioMode)
  operand(sites.raidMaximum,12,game.scenarioCustom)
  operand(sites.raidMaximum,21,game.scenarioMission)
  operand(sites.raidMaximum,44,game.players+0x38F4);operand(sites.raidMaximum,99,game.players+0x38F4)
  operand(d,13,game.unitRecords+0x8E);operand(d,24,game.unitRecords+0x96)
  operand(d,33,game.unitRecords+0x2CA);operand(d,52,game.players+0x2300)
  operand(d,73,game.players+0x30C0)
  operand(sites.specialDefenders,13,game.unitRecords+0x8E)
  local special=core.readInteger(sites.specialDefenders+18)
  operand(sites.specialDefenders,30,special+7*4)
  operand(s,18,game.players+0x2300);operand(s,75,game.players+0x308C)
  operand(s,142,game.players+0x310C)
  operand(r,17,game.unitRecords+0x8E);operand(r,35,game.unitRecords+0x2CA)
  local raidTypes=core.readInteger(r+24)
  for index,offset in ipairs({46,61,76,91}) do operand(r,offset,raidTypes+index*4) end
  -- The acquisition function is already resolved by its two native sortie
  -- callers. Verify its recipe consumer in place, rather than rediscover it.
  context.verify(game.recruitEuropean,'European equipment recipe',signatures.equipment)
  local recipe=core.readInteger(game.recruitEuropean+57)
  operand(game.recruitEuropean,41,recipe+4);operand(game.recruitEuropean,47,recipe+8)
  operand(game.recruitEuropean,63,recipe+12)
  local result={findRecruitmentBuilding=sites.findRecruitmentBuilding,attackRecruitType=t,
    raidMaximum=sites.raidMaximum,defenseTypes=core.readInteger(d+118),specialDefenders=special,
    defenseSlots=core.readInteger(s+121),raidTypes=raidTypes,equipmentRecipes=recipe}
  for name,address in pairs(result) do assert(address>0,'AIC Tactics: invalid native '..name) end
  game.recruitmentSites.attackType=sites.roleCaller+21
  return result
end
return M
