local context = require('native-context')
local M = {}
local signatures = {
  targetSelection = '83 EC 20 8B 54 24 24 53 8B DA 69 DB F4 39 00 00 8B 83 ? ? ? ? 55 33 ED 3B C5 89 4C 24 20 0F 84 ? ? ? ? 83 C0 FF 89 44 24 1C 8B 83 ? ? ? ? 89 44 24 18 8B 83 ? ? ? ? 52 C7 44 24 0C 10 27 00 00 89 6C 24 10 C7 44 24 14 40 42 0F 00 89 44 24 18 E8 ? ? ? ? 8B 83 ? ? ? ? 83 F8 02 74 05 83 F8 01 75 1B 8B 8B ? ? ? ? 51 B9 ? ? ? ? E8 ? ? ? ? 85 C0 75 06 89 AB ? ? ? ? 8B 83 ? ? ? ? 83 F8 02 75 0E 89 AB ? ? ? ? 5D 5B 83 C4 20 C2 04 00 83 F8 01 75 1F 8B 93 ? ? ? ? 52 B9 ? ? ? ? E8 ? ? ? ? 85 C0 0F 85 ? ? ? ? 89 AB ? ? ? ? 56 57 BF 01 00 00 00 BE ? ? ? ? 8B FF 8B 0C BD ? ? ? ? 8B 54 24 34 3B 0C 95 ? ? ? ? 0F 84 ? ? ? ? 57 B9 ? ? ? ? E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B 44 24 1C 8B 4C 24 20 8B 96 D0 FC FF FF 50 8B 86 CC FC FF FF 51 52 50 B9 ? ? ? ? E8 ? ? ? ? 8B 4C 24 24 8B 54 24 28 69 C9 A4 02 00 00 8B 84 11 A0 02 00 00 83 F8 04 75 33 83 3C BD ? ? ? ? FF 0F 84 ? ? ? ? A1 ? ? ? ? 3B 44 24 10 7F 77 89 44 24 10 EB 6F 8B 83 ? ? ? ? 5D 89 83 ? ? ? ? 5B 83 C4 20 C2 04 00 83 F8 02 74 D6 85 C0 75 0E 8B 06 3B 44 24 14 7C 4C 89 44 24 14 EB 44 83 F8 01 75 41 8B 86 74 1C 00 00 8B 0E 8D 04 80 03 86 DC 33 00 00 89 44 24 2C B8 1F 85 EB 51 F7 E9 8B 4C 24 2C C1 FA 05 8B C2 C1 E8 1F 03 CA 03 C1 8B 0D ? ? ? ? 8D 04 48 3B 44 24 18 7F 06 89 44 24 18 8B EF 83 C7 01 81 C6 F4 39 00 00 83 FF 09 0F 8C ? ? ? ? 85 ED 75 74 BE 01 00 00 00 BF ? ? ? ? EB 07 8D A4 24 00 00 00 00 8B 14 B5 ? ? ? ? 8B 44 24 34 3B 14 85 ? ? ? ? 74 3B 56 B9 ? ? ? ? E8 ? ? ? ? 85 C0 74 2C 8B 4C 24 1C 8B 54 24 20 8B 47 04 51 8B 0F 52 50 51 B9 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 3B 44 24 10 7F 06 89 44 24 10 8B EE 83 C6 01 81 C7 F4 39 00 00 83 FE 09 7C A3 85 ED 74 0C 89 AB ? ? ? ? 89 AB ? ? ? ? 5F 5E 5D 5B 83 C4 20 C2 04 00 CC',
  nervousness = '53 55 56 57 8B 7C 24 14 8B 14 BD ? ? ? ? 8B F7 69 F6 F4 39 00 00 8B 86 ? ? ? ? 83 F8 32 8B E9 BB 01 00 00 00 7D 05 8D 4B EB EB 4B 83 F8 64 7D 04 33 C9 EB 42 3D C8 00 00 00 7D 07 B9 14 00 00 00 EB 34 3D 90 01 00 00 7D 07 B9 64 00 00 00 EB 26 3D 58 02 00 00 7D 07 B9 C8 00 00 00 EB 18 33 C9 3D 20 03 00 00 0F 9D C1 2B CB 81 E1 38 FF FF FF 81 C1 F4 01 00 00 03 C8 3B D1 7E 0F 5F 33 C0 89 86 ? ? ? ? 5E 5D 5B C2 04 00 83 BE ? ? ? ? 00 75 27 8B C7 69 C0 7D 0E 00 00 03 05 ? ? ? ? 39 14 85 ? ? ? ? 7E 08 57 8B CD E8 ? ? ? ? 57 8B CD E8 ? ? ? ? 5F 89 9E ? ? ? ? 5E 5D 5B C2 04 00',
  attackUpdate = '53 56 8B 74 24 0C 57 8B FE 69 FF F4 39 00 00 8B 87 ? ? ? ? 85 C0 8B D9 0F 84 ? ? ? ? 83 C0 FF 89 44 24 10 8B 87 ? ? ? ? 83 F8 01 74 37 83 F8 02 75 32 8B 87 ? ? ? ? 50 B9 ? ? ? ? E8 ? ? ? ? 85 C0 74 1D 56 8B CB E8 ? ? ? ? 8B 8F ? ? ? ? 51 56 8B CB E8 ? ? ? ? 5F 5E 5B C2 04 00 55 8B AF ? ? ? ? 85 ED 56 8B CB 75 6B E8 ? ? ? ? 85 C0 56 8B CB 74 53 89 AF ? ? ? ? C7 87 ? ? ? ? 01 00 00 00 89 AF ? ? ? ? 89 AF ? ? ? ? E8 ? ? ? ? 56 8B CB E8 ? ? ? ? 56 8B CB E8 ? ? ? ? 8B 87 ? ? ? ? 69 C0 F4 39 00 00 8B D6 C1 E2 05 83 84 10 ? ? ? ? 01 8D 84 10 ? ? ? ? EB 40 E8 ? ? ? ? 5D 5F 5E 5B C2 04 00 E8 ? ? ? ? 85 C0 74 05 83 FD 09 75 1A',
  raidUpdate = '83 EC 14 53 55 8B 6C 24 20 69 ED F4 39 00 00 8B 85 ? ? ? ? 8B D9 33 C9 3B C1 89 5C 24 18 0F 84 ? ? ? ? 83 85 ? ? ? ? 01 8D 50 FF 8B 85 ? ? ? ? 89 54 24 14 69 D2 A4 02 00 00 3B 84 1A F0 01 00 00 89 4C 24 08 7C 06 89 8D ? ? ? ? 39 8D ? ? ? ? 75 08 89 8D ? ? ? ? EB 13 8B 85 ? ? ? ? 3B C1 7E 09 83 C0 FF 89 85 ? ? ? ? 8D 85 ? ? ? ?',
  returnAttack = '83 EC 0C 53 8B 5C 24 14 69 DB F4 39 00 00 83 BB ? ? ? ? 00 0F 84 D2 00 00 00 83 BB ? ? ? ? 00 0F 84 C5 00 00 00 55 56 B8 ? ? ? ? 57 89 44 24 10 8B 10 8B 48 FC 33 FF 85 D2 89 4C 24 14 89 54 24 18 0F 8E 8D 00 00 00 8B 6C 24 20 69 ED FA 1C 00 00 03 E9 8D 2C 6D ? ? ? ? EB 04 8B 4C 24 14 0F BF 75 00 85 F6 74 5C 8B 54 24 20 69 D2 7D 0E 00 00 8B C6 69 C0 ? ? ? ? 03 D7 03 D1 8B 88 ? ? ? ? 3B 0C 95 ? ? ? ? 75 37 8B 8B ? ? ? ? 6A 00 6A 00 6A 00 66 C7 80 ? ? ? ? 01 00 8B 83 ? ? ? ? 50 51 56 B9 ? ? ? ? E8 ? ? ? ? 85 C0 75 0B 56 B9 ? ? ? ? E8 ? ? ? ? 83 C7 01 83 C5 02 3B 7C 24 18 7C 8C 8B 44 24 10 83 C0 08 3D ? ? ? ? 89 44 24 10 0F 8C 4A FF FF FF 5F 5E 5D 5B 83 C4 0C C2 04 00',
  noTroops = '8B 4C 24 04 69 C9 F4 39 00 00 56 8B B1 ? ? ? ? 03 B1 ? ? ? ? 57 03 B1 ? ? ? ? 33 FF 83 B9 ? ? ? ? 06 75 3E 03 B1 ? ? ? ? 83 B9 ? ? ? ? 10 7E 13 8B 81 ? ? ? ? 99 83 E2 07 03 C2 C1 F8 03 03 F0 EB 06 03 B1 ? ? ? ? 8B 81 ? ? ? ? 03 81 ? ? ? ? 03 81 ? ? ? ? 03 F0 EB 43 8B 81 ? ? ? ? 99 2B C2 8B F8 D1 FF 83 B9 ? ? ? ? 0C 7E 13 8B 81 ? ? ? ? 99 83 E2 07 03 C2 C1 F8 03 03 F0 EB 06 03 B1 ? ? ? ? 8B 91 ? ? ? ? 03 91 ? ? ? ? 03 91 ? ? ? ? 03 F2 83 FE 01 7F 0A 5F B8 01 00 00 00 5E C2 04 00 83 FE 04 7F 07 8D 04 36 3B F8 7F EA 5F 33 C0 5E C2 04 00',
  marketPrice = '8B 44 24 04 8B 8C C1 ? ? ? ? B8 67 66 66 66 F7 E9 D1 FA 8B C2 C1 E8 1F 03 C2 C2 04 00',
  marketCaller = '8B 44 24 10 8B 15 ? ? ? ? 8D 9C 39 E5 01 00 00 50 B9 ? ? ? ? 8D 6C 32 46 E8 ? ? ? ? 6A 00',
  combatValue = '8B 4C 24 04 8D 41 FB 83 F8 48 77 66 0F B6 80 ? ? ? ? FF 24 85 ? ? ? ? A1 ? ? ? ? C2 04 00 A1 ? ? ? ? C2 04 00 A1 ? ? ? ? C2 04 00 A1 ? ? ? ? C2 04 00 A1 ? ? ? ? C2 04 00 A1 ? ? ? ? C2 04 00 A1 ? ? ? ? C2 04 00 A1 ? ? ? ? C2 04 00 A1 ? ? ? ? C2 04 00 A1 ? ? ? ? C2 04 00 A1 ? ? ? ? C2 04 00 83 F9 16 7C 0F 83 F9 1E 7F 0A 8B 04 8D ? ? ? ? C2 04 00 33 C0 C2 04 00',
  valueCaller = '0F BF 90 A2 06 00 00 8B CF 69 C9 F4 39 00 00 8D A9 ? ? ? ? 52 B9 ? ? ? ? E8 ? ? ? ? 01 45 00 8B 2D ? ? ? ? 8B C5 69 C0 90 04 00 00',
  unitDamage = '51 53 8B 5C 24 0C 55 56 57 8B 7C 24 1C 69 FF 90 04 00 00 8B EB 69 ED 90 04 00 00 8B F1 0F BF 84 37 EC 08 00 00 0F B7 8C 2E A2 06 00 00 0F BF 94 37 A2 06 00 00 89 44 24 1C 0F BF C1 8D 04 80 C1 E0 04 03 C2 66 83 F9 37 8B 04 85 ? ? ? ? 89 54 24 18 C7 44 24 10 00 00 00 00 74 10 0F BF 8D ? ? ? ? 51 50 8B CE E8 ? ? ? ?',
  entityDamage = '83 EC 14 8B 44 24 18 53 55 8B 6C 24 24 56 69 ED E8 00 00 00 8B F0 69 F6 90 04 00 00 33 D2 57 8B F9 0F BF 8C 3E EC 08 00 00 89 54 24 10 89 54 24 14 89 54 24 1C 0F B7 94 3E A2 06 00 00 0F BF DA 89 4C 24 18 0F BF 8D ? ? ? ? 89 5C 24 20 33 DB 3B C3 89 6C 24 28 89 4C 24 2C 7E 71 3B C8 74 6D 0F B7 85 ? ? ? ? 66 3D 09 00 74 60 66 3D 22 00 74 5A 66 3D 16 00 0F 85 78 01 00 00',
  fireDamage = '8B 44 24 04 53 33 DB 3B C3 7F 06 33 C0 5B C2 0C 00 69 C0 90 04 00 00 66 39 9C 08 B4 08 00 00 56 8D 34 08 74 07 5E 33 C0 5B C2 0C 00 55 57 0F B7 BE A2 06 00 00 66 83 FF 37 75 07 B8 19 00 00 00 EB 21 66 83 FF 35 75 09 BD 01 00 00 00 8B C5 EB 17 66 8B C7 66 2D 4C 00 66 F7 D8 1B C0 83 E0 5A 83 C0 0A BD 01 00 00 00 39 5C 24 1C 74 05 99 2B C2 D1 F8 8B 4C 24 18 3B CB 74 19 0F BF 96 AA 06 00 00 3B CA 74',
  unitCensus = '8B C7 A3 ? ? ? ? 39 3E 0F 8E ? ? ? ? 8B FF 69 C0 90 04 00 00 66 83 BC 30 A0 06 00 00 00 0F 84 ? ? ? ? 83 46 04 01 8B 2D ? ? ? ? 8B D5 69 D2 90 04 00 00 66 83 BC 32 A4 06 00 00 00 74 0E 55 8B CE E8 ? ? ? ? 8B 2D ? ? ? ? 8B C5 69 C0 90 04 00 00 8B 8C 30 E8 06 00 00 8A 94 30 E4 09 00 00 08 91 ? ? ? ? 03 C6 66 83 B8 B8 08 00 00 00 0F 84 ? ? ? ? F6 05 ? ? ? ? 3F 0F BF B8 AA 06 00 00 75 49 0F BF 90 A2 06 00 00',
  censusEnd = 'B9 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? BB 01 00 00 00 8B D3 89 15 ? ? ? ? 33 ED 39 1E 0F 8E ? ? ? ?',
  buildingCensus = '66 89 19 81 C1 F4 39 00 00 81 F9 ? ? ? ? 7C EF 8B CF 89 5E 04 89 0D ? ? ? ? 39 7E 08 0F 8E ? ? ? ? 55 EB 07 8D A4 24 00 00 00 00 8B D1 69 D2 2C 03 00 00 66 39 9C 32 E4 00 00 00 0F 84 ? ? ? ? 01 3E 8B 15 ? ? ? ? 8B C2 69 C0 2C 03 00 00 0F B7 8C 30 E4 00 00 00 03 C6 66 3B CF 75 0E 66 C7 80 E4 00 00 00 02 00 E9 ? ? ? ? 66 83 F9 03 75 0D 52 8B CE E8 ? ? ? ? E9 ? ? ? ? 66 83 B8 E6 00 00 00 43 75 0E 0F BF 88 EA 00 00 00 01 3C 8D ? ? ? ?',
  buildingEnd = '8B 0D ? ? ? ? 03 CF 89 0D ? ? ? ? 3B 4E 08 0F 8C ? ? ? ? 5D 8B 46 10 B9 D0 07 00 00 2B 0E 3B C3 89 9E 24 E0 18 00 89 9E 74 E0 18 00 89 4E 0C 7E 06 83 C0 FF 89 46 10 5F 5E 5B C3',
  waveReadiness = '83 EC 08 55 56 8B 74 24 14 69 F6 F4 39 00 00 8B 86 ? ? ? ? 33 ED 3B C5 89 4C 24 0C 75 0A 5E 33 C0 5D 83 C4 08 C2 04 00 83 BE ? ? ? ? 01 75 0D 5E B8 01 00 00 00 5D 83 C4 08 C2 04 00 8B 96 ? ? ? ? 69 D2 F4 39 00 00 83 C0 FF 83 BA ? ? ? ? 64 7F 06 89 AE ? ? ? ? 69 C0 A4 02 00 00 8B 84 08 F4 01 00 00 8B 8E ? ? ? ? 03 86 ? ? ? ? 2B 8E ? ? ? ? 89 6C 24 08 3B C8 7C 9B 3B AE ? ? ? ? 53 57',
  tunnelers = '83 BE 0A 02 00 00 00 75 36 66 83 7E F8 05 75 2F 66 83 BE 94 03 00 00 0F 75 25 0F B7 86 42 02 00 00 66 85 C0 74 0F 0F BF C8 51 57 B9 ? ? ? ? E8 ? ? ? ? 6A 0F 57 8B CB E8 ? ? ? ? 83 C7 01 81 C6 90 04 00 00',
  attackScheduler = '83 3D ? ? ? ? 00 75 08 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? 39 3D ? ? ? ? 74 0A 6A 00 57 8B',
  targetScheduler = '6A 01 57 8B CB E8 ? ? ? ? 57 8B CB E8 ? ? ? ? B8 01 00 00 00 39 86 F8 0D 00 00 75 08 89 86 74 16 00 00 EB',
  returnCall1 = '56 8B CB E8 ? ? ? ? 56 8B CB E8 ? ? ? ? 56 8B CB E8 ? ? ? ? 56 8B CB E8 ? ? ? ? EB 0F 83 F8 07 7D 0A C7 87 ? ? ? ? 07 00 00 00',
  returnCall2 = '56 8B CB E8 ? ? ? ? 56 8B CB E8 ? ? ? ? 5D 5F 5E 5B C2 04 00 83 F8 08 75 61',
  returnCall3 = '83 F8 08 75 61 56 8B CB C7 87 ? ? ? ? 09 00 00 00 89 AF ? ? ? ? E8 ? ? ? ? 8B 97 ? ? ? ? 56 52 B9 ? ? ? ? E8 ? ? ? ? 56 8B CB E8 ? ? ? ? 56 8B CB E8 ? ? ? ? 56 8B CB E8 ? ? ? ? 56 8B CB E8 ? ? ? ?',
}

function M.resolve(game)
  local sites={}
  for _,name in ipairs({'targetSelection','nervousness','attackUpdate','raidUpdate','returnAttack','noTroops',
      'marketCaller','combatValue','valueCaller','unitDamage','entityDamage','fireDamage',
      'unitCensus','censusEnd','buildingCensus','buildingEnd','waveReadiness','tunnelers',
      'attackScheduler','targetScheduler','returnCall1','returnCall2','returnCall3'}) do
    sites[name]=context.find(name,signatures[name])
  end
  -- Buy/sell price bodies share their arithmetic. The native caller identifies
  -- the required price; do not select the first matching arithmetic function.
  sites.marketPrice=context.call(sites.marketCaller+27,'market price')
  context.verify(sites.marketPrice,'market price',signatures.marketPrice)
  local function operand(name,offset,expected)
    assert(core.readInteger(sites[name]+offset)==expected,
      'AIC Tactics: incompatible native '..name..' operand')
  end
  operand('targetSelection',18,game.players+0x2300)
  operand('targetSelection',46,game.players+0x1D8)
  operand('targetSelection',56,game.players+0x1DC)
  operand('targetSelection',92,game.players+0x384C)
  operand('targetSelection',108,game.players+0x38E4)
  operand('targetSelection',114,game.units)
  operand('targetSelection',129,game.players+0x384C)
  operand('targetSelection',135,game.players+0x384C)
  operand('targetSelection',146,game.players+0x2BD8)
  operand('targetSelection',165,game.players+0x3850)
  operand('targetSelection',171,game.units)
  operand('targetSelection',190,game.players+0x384C)
  operand('targetSelection',202,game.players+0x3F00)
  operand('targetSelection',211,game.teams)
  operand('targetSelection',222,game.teams)
  operand('targetSelection',234,game.units)
  operand('targetSelection',344,game.players+0x3850)
  operand('targetSelection',351,game.players+0x2BD8)
  operand('targetSelection',483,game.players+0x3BCC)
  operand('targetSelection',499,game.teams)
  operand('targetSelection',510,game.teams)
  operand('targetSelection',518,game.units)
  operand('targetSelection',595,game.players+0x2BD4)
  operand('targetSelection',601,game.players+0x2BD8)
  operand('nervousness',25,game.players+0x38E0)
  operand('nervousness',132,game.players+0x3820)
  operand('nervousness',144,game.players+0x3820)
  operand('nervousness',168,game.players+0x38B8)
  operand('nervousness',193,game.players+0x3820)
  operand('attackUpdate',17,game.players+0x2300)
  operand('attackUpdate',40,game.players+0x384C)
  operand('attackUpdate',56,game.players+0x38E4)
  operand('attackUpdate',62,game.units)
  operand('attackUpdate',85,game.players+0x38E4)
  operand('attackUpdate',107,game.players+0x2BA4)
  operand('attackUpdate',132,game.players+0x3970)
  operand('attackUpdate',138,game.players+0x2BA4)
  operand('attackUpdate',148,game.players+0x3854)
  operand('attackUpdate',154,game.players+0x3930)
  operand('attackUpdate',181,game.players+0x2BD8)
  operand('attackUpdate',199,game.players+0x2BDC)
  operand('attackUpdate',207,game.players+0x2BDC)
  operand('raidUpdate',17,game.players+0x2300)
  operand('raidUpdate',39,game.players+0x2B78)
  operand('raidUpdate',49,game.players+0x2B78)
  operand('raidUpdate',78,game.players+0x2B78)
  operand('raidUpdate',84,game.players+0x30EC)
  operand('raidUpdate',92,game.players+0x390C)
  operand('raidUpdate',100,game.players+0x390C)
  operand('raidUpdate',113,game.players+0x390C)
  operand('raidUpdate',119,game.players+0x3274)
  operand('returnAttack',16,game.players+0x2300)
  operand('returnAttack',29,game.players+0x2B7C)
  operand('returnAttack',90,game.players+0x310C)
  operand('returnAttack',139,game.players+0x329C)
  operand('returnAttack',147,game.players+0x2B80)
  operand('returnAttack',168,game.players+0x2B84)
  operand('returnAttack',176,game.tribes)
  operand('returnAttack',191,game.tribes)
  operand('noTroops',13,game.players+0x2BC0)
  operand('noTroops',19,game.players+0x2BB4)
  operand('noTroops',26,game.players+0x2BB0)
  operand('noTroops',34,game.players+0x2BA4)
  operand('noTroops',43,game.players+0x2BAC)
  operand('noTroops',49,game.players+0x3970)
  operand('noTroops',58,game.players+0x2BC4)
  operand('noTroops',77,game.players+0x2BC4)
  operand('noTroops',83,game.players+0x2BD0)
  operand('noTroops',89,game.players+0x2BCC)
  operand('noTroops',95,game.players+0x2BC8)
  operand('noTroops',105,game.players+0x2BAC)
  operand('noTroops',118,game.players+0x3970)
  operand('noTroops',127,game.players+0x2BC4)
  operand('noTroops',146,game.players+0x2BC4)
  operand('noTroops',152,game.players+0x2BD0)
  operand('noTroops',158,game.players+0x2BCC)
  operand('noTroops',164,game.players+0x2BC8)
  operand('valueCaller',17,game.players+0x38E8)
  operand('unitCensus',127,game.gameTick)
  operand('waveReadiness',17,game.players+0x2300)
  operand('waveReadiness',43,game.players+0x384C)
  operand('waveReadiness',65,game.players+0x2BD8)
  operand('waveReadiness',80,game.players+0x38E8)
  operand('waveReadiness',89,game.players+0x38A0)
  operand('waveReadiness',108,game.players+0x30F0)
  operand('waveReadiness',114,game.players+0x38A0)
  operand('waveReadiness',120,game.players+0x2BA8)
  operand('waveReadiness',134,game.players+0x3908)
  operand('tunnelers',44,game.tribes)
  operand('returnCall1',41,game.players+0x2BA4)
  operand('returnCall3',10,game.players+0x2BA4)
  operand('returnCall3',20,game.players+0x3970)
  operand('returnCall3',31,game.players+0x3924)
  operand('returnCall3',38,game.buildings)
  operand('returnAttack',122,game.tribeStride)
  operand('returnAttack',132,game.tribes+0x34)
  operand('returnAttack',160,game.tribes+game.tribeStance)
  operand('unitDamage',96,game.unitRecords+0x96)
  operand('entityDamage',71,game.entities+0xA2)
  operand('entityDamage',100,game.entities+0x2A)
  operand('censusEnd',1,game.entities-0x14);operand('censusEnd',11,game.entities-0x14)
  local censusID=core.readInteger(sites.unitCensus+3)
  operand('unitCensus',44,censusID);operand('unitCensus',77,censusID)
  operand('censusEnd',29,censusID);operand('valueCaller',37,censusID)
  assert(censusID>0 and sites.unitCensus+15+core.readInteger(sites.unitCensus+11)==sites.censusEnd,
    'AIC Tactics: inconsistent unit census lifecycle')
  local buildingID=core.readInteger(sites.buildingCensus+24)
  operand('buildingCensus',73,buildingID);operand('buildingEnd',2,buildingID);operand('buildingEnd',10,buildingID)
  assert(buildingID>0 and sites.buildingCensus+37+core.readInteger(sites.buildingCensus+33)==sites.buildingEnd+24
      and sites.buildingEnd+23+core.readInteger(sites.buildingEnd+19)==sites.buildingCensus+47,
    'AIC Tactics: inconsistent building census lifecycle')
  local gameState=core.readInteger(sites.marketCaller+19)
  assert(gameState>0 and gameState+core.readInteger(sites.marketPrice+7)==game.players+0x211DC,
    'AIC Tactics: incompatible native market layout')
  assert(context.call(sites.valueCaller+27,'unit combat value')==sites.combatValue
      and context.call(sites.targetSelection+85,'nervousness')==sites.nervousness
      and context.call(sites.attackUpdate+118,'wave readiness')==sites.waveReadiness
      and context.call(sites.attackUpdate+225,'remaining army')==sites.noTroops
      and context.call(sites.tunnelers+48,'tunneler membership')==game.removeUnitFromTribe
      and context.call(sites.tunnelers+58,'tunneler assignment')==game.assignAttacker,
    'AIC Tactics: inconsistent native combat owners')
  operand('returnAttack',220,core.readInteger(sites.returnAttack+43)+11*8)
  local distance=core.readInteger(sites.targetSelection+276)
  operand('targetSelection',326,distance+12);operand('targetSelection',436,distance+12)
  operand('targetSelection',549,distance);operand('targetSelection',559,distance+12)
  assert(context.call(sites.targetSelection+280,'target distance')
      ==context.call(sites.targetSelection+553,'fallback target distance'),
    'AIC Tactics: inconsistent native distance owner')
  local alive=context.call(sites.targetSelection+118,'target survival')
  for _,offset in ipairs({175,238,522}) do
    assert(context.call(sites.targetSelection+offset,'target survival')==alive,
      'AIC Tactics: inconsistent native target survival owner')
  end
  assert(context.call(sites.attackUpdate+66,'request target survival')==alive,
    'AIC Tactics: inconsistent native request target owner')
  assert(distance>0 and core.readInteger(sites.valueCaller+23)>0,
    'AIC Tactics: invalid native combat service root')
  local value=sites.combatValue
  operand('combatValue',15,value+0xBC);operand('combatValue',22,value+0x8C)
  local weights=core.readInteger(value+27)
  assert(weights>0,'AIC Tactics: invalid native combat-value table')
  for index,offset in ipairs({35,43,51,59,67,75,83,91,99,107}) do operand('combatValue',offset,weights+index*4) end
  operand('combatValue',127,weights-0x7C)
  local cases={0x22,0x1A,0x6A,0x2A,0x32,0x3A,0x42,0x4A,0x52,0x5A,0x62,0x72}
  for index,offset in ipairs(cases) do
    assert(core.readInteger(value+0x8C+(index-1)*4)==value+offset,
      'AIC Tactics: incompatible native combat-value switch')
  end
  for index=0,72 do assert(core.readByte(value+0xBC+index)<#cases,
    'AIC Tactics: incompatible native combat-value dispatch index') end
  local hooks={
    buildingReset=sites.buildingCensus+17,buildingCount=sites.buildingCensus+133,buildingComplete=sites.buildingEnd+24,
    unitReset=sites.unitCensus,unitCount=sites.unitCensus+81,unitComplete=sites.censusEnd,
    unitDamage=sites.unitDamage,entityDamage=sites.entityDamage,fireDamage=sites.fireDamage,
    launch=sites.attackUpdate+130,launchWait=sites.attackUpdate+213,randomWave=sites.waveReadiness+87,
    tunnelers=sites.tunnelers+26,nextTunneler=sites.tunnelers+63,
    targetChoice=sites.targetSelection+306,nearest=sites.targetSelection+325,richest=sites.targetSelection+371,
    weakest=sites.targetSelection+390,notPlayer=sites.targetSelection+362,player=sites.targetSelection+311,
    callAttack=sites.attackScheduler+28,callTarget=sites.targetScheduler+13,
    callRaid=game.recruitmentSites.ranged+64,callReturn1=sites.returnCall1+27,
    callReturn2=sites.returnCall2+11,callReturn3=sites.returnCall3+74,
    callRecruit=game.recruitmentSites.recruit,callRecruitType=game.recruitmentSites.attackType,
    censusID=censusID}
  local calls={callAttack=sites.attackUpdate,callTarget=sites.targetSelection,callRaid=sites.raidUpdate,
    callReturn1=sites.returnAttack,callReturn2=sites.returnAttack,callReturn3=sites.returnAttack,
    callRecruit=game.recruitUpdate,callRecruitType=game.attackRecruitType}
  for name,target in pairs(calls) do
    assert(context.call(hooks[name],name)==target,'AIC Tactics: modified native combat scheduler')
  end
  local lengths={buildingReset=5,buildingCount=8,buildingComplete=8,unitReset=7,unitCount=8,unitComplete=5,
    unitDamage=6,entityDamage=7,fireDamage=5,launch=6,randomWave=6,tunnelers=7,targetChoice=5}
  local original={}
  for name,length in pairs(lengths) do
    local bytes={}
    for offset=0,length-1 do bytes[#bytes+1]=string.format('%02X',core.readByte(hooks[name]+offset)) end
    original[name]=table.concat(bytes,' ')
  end
  return {selectAttackTarget=sites.targetSelection,computeNervousness=sites.nervousness,
    updateAIPlayerState=sites.attackUpdate,returnAttack=sites.returnAttack,
    hasNoTroopsOrAllDiggers=sites.noTroops,updateRaids=sites.raidUpdate,
    combatValue=sites.combatValue,troopValues=core.readInteger(sites.valueCaller+23),
    marketPrice=sites.marketPrice,gameState=gameState,
    combatSites=hooks,combatOriginals=original,combatCalls=calls}
end
return M
