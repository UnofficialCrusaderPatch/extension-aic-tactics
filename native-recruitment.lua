local context = require('native-context')
local M = {}
local signatures = {
  moatVacancies = '53 55 56 8B 74 24 10 57 33 C0 8D 91 8C 08 50 00 BD D5 14 00 00 BF 00 00 00 40 8D 9B 00 00 00 00 8A 1A 84 DB 74 16 0F BE DB 3B DE 75 0F 8B 5A F4 85 BC 99 60 51 16 00 75 03 83 C0 01 8A 5A 10 84 DB 74 16 0F BE DB 3B DE 75 0F 8B 5A 04 85 BC 99 60 51 16 00 75 03 83 C0 01 8A 5A 20 84 DB 74 16 0F BE DB 3B DE 75 0F 8B 5A 14 85 BC 99 60 51 16 00 75 03 83 C0 01 83 C2 30 83 ED 01 75 A2 5F 5E 5D 5B C2 04 00',
  moatRecruitment = '39 9A 5C 01 00 00 89 54 24 14 74 49 55 B9 ? ? ? ? E8 ? ? ? ? 85 C0 74 3A 0F BF 86 ? ? ? ? 85 C0 74 27 8B 96 ? ? ? ? 69 C0 ? ? ? ? 3B 90 ? ? ? ? 75 13',
  moatGroup = '55 B9 ? ? ? ? E8 ? ? ? ? 85 C0 7F 10 55 56 8B CB E8 ? ? ? ? 5F 5E 5D 5B C2 04 00 0F BF 9F ? ? ? ? 6A 01 53 55 B9 ? ? ? ? E8 ? ? ? ? 83 F8 FF 7E 58 8B',

  rangedSortie = '55 57 8B 7C 24 0C 8B E9 8B CF 69 C9 F4 39 00 00 8B 81 ? ? ? ? 85 C0 0F 84 01 01 00 00 83 C0 FF 69 C0 A4 02 00 00 53 8D 1C 28 56 8B B3 4C 01 00 00 85 F6 0F 8C E3 00 00 00 8B 81 ? ? ? ? 99 2B C2 D1 F8 03 C6 39 81 ? ? ? ? 0F 8D CA 00 00 00 83 B9 ? ? ? ? 00 0F 8E BD 00 00 00 83 B9 ? ? ? ? 00 0F 84 B0 00 00 00 8B 83 50 01 00 00 83 F8 1E 0F 84 A1 00 00 00 83 F8 1D 0F 84 98 00 00 00 83 F8 05 0F 84 8F 00 00 00 83 F8 46 7D 08 8B 89 ? ? ? ? EB 06 8B 89 ? ? ? ? 85 C9 74 78 83 F8 46 6A 00 57 51 50 B9 ? ? ? ? 7D 07 E8 ? ? ? ? EB 05 E8 ? ? ? ? 8B F0 85 F6 75 2D 8B 83 9C 00 00 00 85 C0 7E 4D 83 3D ? ? ? ? 02 75 44 69 FF 7D 0E 00 00 03 3D ? ? ? ? 5E 5B 89 04 BD ? ? ? ? 5F 5D C2 04 00 8B C6 69 C0 90 04 00 00 68 A6 00 00 00 57 8B CD 66 C7 80 ? ? ? ? 06 00 E8 ? ? ? ? 50 56 B9 ? ? ? ? E8 ? ? ? ? 5E 5B 5F 5D C2 04 00',
  meleeSortie = '55 57 8B 7C 24 0C 8B E9 8B CF 69 C9 F4 39 00 00 8B 81 ? ? ? ? 85 C0 0F 84 F4 00 00 00 83 C0 FF 69 C0 A4 02 00 00 53 8D 1C 28 8B 83 54 01 00 00 85 C0 0F 8C D8 00 00 00 39 81 ? ? ? ? 0F 8D CC 00 00 00 83 B9 ? ? ? ? 00 0F 8E BF 00 00 00 83 B9 ? ? ? ? 00 0F 84 B2 00 00 00 8B 83 58 01 00 00 83 F8 1E 0F 84 A3 00 00 00 83 F8 1D 0F 84 9A 00 00 00 83 F8 05 0F 84 91 00 00 00 83 F8 46 7D 08 8B 89 ? ? ? ? EB 06 8B 89 ? ? ? ? 85 C9 74 7A 83 F8 46 56 6A 00 57 51 50 B9 ? ? ? ? 7D 07 E8 ? ? ? ? EB 05 E8 ? ? ? ? 8B F0 85 F6 75 2D 8B 83 9C 00 00 00 85 C0 7E 4D 83 3D ? ? ? ? 02 75 44 69 FF 7D 0E 00 00 03 3D ? ? ? ? 5E 5B 89 04 BD ? ? ? ? 5F 5D C2 04 00 8B C6 69 C0 90 04 00 00 68 A7 00 00 00 57 8B CD 66 C7 80 ? ? ? ? 07 00 E8 ? ? ? ? 50 56 B9 ? ? ? ? E8 ? ? ? ? 5E 5B 5F 5D C2 04 00',
  scheduler = '57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 39 3D ? ? ? ? 0F 85 ? ? ? ?',
  recruitmentPrefix = '83 EC 14 53 56 8B 74 24 20 69 F6 F4 39 00 00 8B 86 ? ? ? ? 33 DB 3B C3 8B D1 89 54 24 10 0F 84 ? ? ? ? 32 C9 83 3D ? ? ? ? 03 57 89 5C 24 0C 8D 78 FF 75 14 83 3D ? ? ? ? 01 75 0B 83 3D ? ? ? ? 02 75 02 B1 01 8B C7 69 C0 A9 00 00 00 55 8B AE ? ? ? ? 03 E8 89 44 24 20 8B 84 AA 64 01 00 00 8B E8 F7 DD 1B ED 83 C5 02',
  opportunity = '83 86 ? ? ? ? 01 39 86 ? ? ? ? 0F 8C ? ? ? ? 39 9E ? ? ? ? 89 9E ? ? ? ? 0F 8E ? ? ? ? 39 9E ? ? ? ? 0F 84 ? ? ? ? 8B 86 ? ? ? ? 85 C0 75 44 8B C7 69 C0 A4 02 00 00 84 C9 8B 84 10 70 01 00 00',
  wallCheck = '3B 91 80 01 00 00 50 8B CF 7C 07 E8 ? ? ? ? EB 28 E8 ? ? ? ? EB 21',
  wallReset = '8B FF 3B CA 89 90 78 05 00 00 89 10 89 90 7C 05 00 00 89 90 80 05 00 00 89 90 84 05 00 00 89 90 D0 0D 00 00 89 90 D8 0D 00 00 89 50 EC 89 50 F0 89 50 3C 89 90 60 0E 00 00 89 50 40',
  wallCount = '8D 8F ? ? ? ? BB 01 00 00 00 01 19 E9 6B 01 00 00 69 FF F4 39 00 00 8D 8F ? ? ? ? BB 01 00 00 00 01 19 E9 53 01 00 00 69 FF F4 39 00 00',
}

-- Discovery runs after Legacy enable: its census/check patches are deliberately
-- outside these surviving contexts. Their complete trampolines are checked by
-- legacyCounter before they are consumed or chained; Legacy retains ownership.
function M.resolve(game)
  local sites = {}
  for _, name in ipairs({'moatVacancies','moatRecruitment','moatGroup','rangedSortie','meleeSortie','scheduler','recruitmentPrefix',
      'opportunity','wallCheck','wallReset','wallCount'}) do
    sites[name] = context.find(name, signatures[name])
  end
  local function operand(site, offset, expected)
    assert(core.readInteger(site + offset) == expected,
      'AIC Tactics: incompatible native recruitment operand')
  end
  local moat=core.readInteger(sites.moatRecruitment+14)
  assert(moat>0 and core.readInteger(sites.moatGroup+2)==moat
      and core.readInteger(sites.moatGroup+43)==moat
      and context.call(sites.moatRecruitment+18,'recruitment moat vacancy')==sites.moatVacancies
      and context.call(sites.moatGroup+6,'group moat vacancy')==sites.moatVacancies,
    'AIC Tactics: inconsistent native moat owner')
  operand(sites.moatRecruitment,30,game.players+0x3120)
  operand(sites.moatRecruitment,40,game.players+0x32C4)
  operand(sites.moatRecruitment,46,game.tribeStride)
  operand(sites.moatRecruitment,52,game.tribes+0x34)
  operand(sites.moatGroup,34,game.tribes+0x5A)
  local r, m, p, o = sites.rangedSortie, sites.meleeSortie, sites.recruitmentPrefix, sites.opportunity
  operand(r,18,game.players+0x2300); operand(m,18,game.players+0x2300)
  operand(r,60,game.players+0x3940); operand(r,73,game.players+0x393C)
  operand(m,59,game.players+0x3944)
  operand(r,85,game.players+0x2B6C); operand(m,71,game.players+0x2B6C)
  operand(r,98,game.players+0x3974); operand(m,84,game.players+0x3974)
  operand(r,149,game.players+0x15C); operand(m,135,game.players+0x15C)
  operand(r,157,game.players+0x24C); operand(m,143,game.players+0x24C)
  operand(r,174,game.units); operand(m,161,game.units)
  operand(r,210,game.units+0x60C); operand(m,197,game.units+0x60C)
  operand(r,225,game.units+0x610); operand(m,212,game.units+0x610)
  operand(r,234,game.players+0x2A70); operand(m,221,game.players+0x2A70)
  operand(r,262,game.unitRecords+0x42A); operand(m,249,game.unitRecords+0x42A)
  operand(r,276,game.tribes); operand(m,263,game.tribes)
  assert(context.call(r+268,'ranged group') == game.findSortieGroup
      and context.call(m+255,'melee group') == game.findSortieGroup
      and context.call(r+280,'ranged membership') == game.addUnitToTribe
      and context.call(m+267,'melee membership') == game.addUnitToTribe,
    'AIC Tactics: incompatible native sortie group owner')
  local european = context.call(r+180,'European recruitment')
  local nonEuropean = context.call(r+187,'mercenary recruitment')
  assert(context.call(m+167,'European recruitment') == european
      and context.call(m+174,'mercenary recruitment') == nonEuropean,
    'AIC Tactics: inconsistent native recruitment owners')
  operand(p,17,game.players+0x2300); operand(p,87,game.players+0x30F4)
  local scenario=core.readInteger(p+41)
  assert(scenario>0 and core.readInteger(p+58)==scenario+0x1F2C
      and core.readInteger(p+67)==scenario+0x1F34,
    'AIC Tactics: inconsistent native scenario fields')
  for _, off in ipairs({2,9,27}) do operand(o,off,game.players+0x30FC) end
  operand(o,21,game.players+0x2B6C); operand(o,39,game.players+0x3974)
  operand(o,51,game.players+0x30F8)
  local finish = o+19+core.readInteger(o+15)
  assert(finish == o+37+core.readInteger(o+33)
      and finish == o+49+core.readInteger(o+45)
      and finish+2 == p+37+core.readInteger(p+33),
    'AIC Tactics: incompatible recruitment cleanup branches')
  local cleanup={0x5D,0x5F,0x5E,0x5B,0x83,0xC4,0x14,0xC2,0x04,0x00}
  for index,value in ipairs(cleanup) do
    assert(core.readByte(finish+index-1)==value,'AIC Tactics: incompatible recruitment cleanup ABI')
  end
  assert(context.call(sites.scheduler+3,'ranged scheduler') == r
      and context.call(sites.scheduler+11,'melee scheduler') == m
      and context.call(sites.scheduler+19,'recruitment scheduler') == p,
    'AIC Tactics: modified native recruitment scheduler')
  assert(context.call(sites.wallCheck+11,'patrol defense') == game.patrolDefense
      and context.call(sites.wallCheck+18,'wall defense') == game.wallDefense,
    'AIC Tactics: incompatible Legacy defense context')
  operand(sites.wallCount,2,game.players+0x30E8)
  operand(sites.wallCount,26,game.players+0x30EC)
  return {moat=moat,moatVacancies=sites.moatVacancies,recruitUpdate=p,rangedSortieNative=r,meleeSortieNative=m,
    recruitEuropean=european,recruitNonEuropean=nonEuropean,
    scenarioMode=core.readInteger(p+41),scenarioCustom=core.readInteger(p+58),scenarioMission=core.readInteger(p+67),
    recruitmentSites={interval=p+97,opportunity=o+49,finished=finish,
      ranged=sites.scheduler+3,melee=sites.scheduler+11,recruit=sites.scheduler+19,
      wallCheck=sites.wallCheck-6,wallReset=sites.wallReset-5,wallCount=sites.wallCount-6}}
end

function M.legacyCounter(game, chained)
  local sites = game.recruitmentSites
  local function jump(site)
    assert(core.readByte(site)==0xE9, require('messages').legacyOn('ai_defense'))
    local target=site+5+core.readInteger(site+1)
    assert(target>0,'AIC Tactics: invalid Legacy defense hook')
    return target
  end
  local check = jump(sites.wallCheck)
  local reset, count = jump(sites.wallReset), jump(sites.wallCount)
  if chained then
    assert(reset==chained.reset and count==chained.count,
      'AIC Tactics: defense census hook was replaced; restart with compatible modules')
    reset, count = chained.originalReset, chained.originalCount
  end
  local wallCounter = core.readInteger(check+7)
  assert(wallCounter>0, 'AIC Tactics: invalid Legacy defense counter')
  local function bytes(site, expected)
    local offset=0
    for token in expected:gmatch('%S+') do
      assert(token=='?' or core.readByte(site+offset)==tonumber(token,16),
        'AIC Tactics: incompatible Legacy defense trampoline')
      offset=offset+1
    end
  end
  local function word(site, offset, value)
    assert(core.readInteger(site+offset)==value,'AIC Tactics: incompatible Legacy defense trampoline operand')
  end
  bytes(check,'8B 54 24 04 8B 14 95 ? ? ? ? E9 ? ? ? ?')
  word(check,12,sites.wallCheck+6-(check+16))
  bytes(reset,'31 C0 89 14 85 ? ? ? ? 40 83 F8 08 7E F3 B8 ? ? ? ? E9 ? ? ? ?')
  word(reset,5,wallCounter); word(reset,16,game.players+0x6560)
  word(reset,21,sites.wallReset+5-(reset+25))
  bytes(count,'89 E9 69 C9 90 04 00 00 0F B6 89 ? ? ? ? 83 F9 01 75 09 8D 0C BD ? ? ? ? FF 01 69 FF F4 39 00 00 E9 ? ? ? ?')
  word(count,11,game.unitRecords+0x42A); word(count,23,wallCounter)
  word(count,36,sites.wallCount+6-(count+40))
  return wallCounter
end
return M
