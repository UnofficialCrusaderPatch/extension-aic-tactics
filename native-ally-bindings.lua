local context = require('native-context')
local M = {}

-- Native owners ally help reuses (src/allies.cpp). Each operand is decoded
-- from its instruction and checked against the layout already resolved.

-- The synchronized command that carries out an ally's goods answer:
-- (asked player, resource - 1, amount, asking player). Gold is handled only
-- for a human asked player; an asked AI sends goods through the branch at
-- +0xF4: goods-sent speech, resource gain and loss, sell value statistics.
local handlerSignature = '8B 54 24 08 56 57 8D 7A 01 83 FF 0F 75 75 8B 4C 24 0C 83 3C 8D ? ? ? ? FF '..
  '0F 84 ? ? ? ? 8B 74 24 18'
local giftOffset = 0xF4
local giftSignature = '53 8B 5C 24 1C 55 53 56 B9 ? ? ? ? E8 ? ? ? ? 8B 6C 24 1C 55 57 53 B9 ? ? ? ? '..
  'E8 ? ? ? ? 6A 00 55 57 56 B9 ? ? ? ? E8 ? ? ? ? 55 57 56 B9 ? ? ? ? E8 ? ? ? ? 01 04 9D ? ? ? ? '..
  '55 57 56 B9 ? ? ? ? 8D 1C B5 ? ? ? ? E8 ? ? ? ? 01 03'
-- isResourceLargerOrEqualThanMinimumGoodsRequiredAfterTrade(player, resource,
-- amount): a human always passes, an AI never sends gold, else stock >=
-- AIC MinimumGoodsRequiredAfterTrade + amount.
local enoughSignature = '8B 44 24 04 8B D0 69 D2 F4 39 00 00 8B 92 ? ? ? ? 85 D2 75 06 8D 42 01 C2 0C 00 '..
  '56 8B 74 24 0C 83 FE 0F 75 06 33 C0 5E C2 0C 00 69 D2 A4 02 00 00 69 C0 7D 0E 00 00 '..
  '8B 8C 0A 00 FE FF FF 03 4C 24 10 03 C6 33 D2 39 0C 85 ? ? ? ? 5E 0F 9D C2 8B C2 C2 0C 00'
local enoughGold, enoughGoods = 0x21, 0x2C
local enoughGoldBytes = {0x83, 0xFE, 0x0F, 0x75, 0x06}
-- requestGoods(player, resource, amount): an AI with no request delay asks
-- each living human ally with a free slot (never hops, wheat or ale; food
-- only when nearly out of it).
local requestSignature = '51 8B 54 24 08 8B C2 69 C0 F4 39 00 00 83 B8 ? ? ? ? 00 89 0C 24 0F 84 ? ? ? ? '..
  '83 B8 ? ? ? ? 00 0F 85 ? ? ? ? 53 8B 5C 24 10 83 FB 09'
-- buyRequiredGoods: a wanted good the AI can afford is bought, else asked
-- for at twice the amount.
local buySignature = '8B CD 69 C9 7D 0E 00 00 03 CE 8B 3C 8D ? ? ? ? 85 FF 74 ? 83 F8 03 7F ? '..
  '8B 54 24 14 8B 83 ? ? ? ? 69 D2 A4 02 00 00 8B 4C 24 10 3B 44 0A 18 7C ? 8B 44 24 1C '..
  '83 BB ? ? ? ? 00 7F ? 83 BB ? ? ? ? 00 74 ? 57 56 55 B9 ? ? ? ? E8 ? ? ? ? 39 83 ? ? ? ? '..
  '8B 4C 24 10 7D ? 8D 14 3F 52 56 55 E8'
-- updateAIStrengthState: the monthly strength score becomes the weak,
-- normal or strong feeling.
local strengthSignature = '8B 86 ? ? ? ? 3B C5 7F 08 89 8E ? ? ? ? EB 13 83 F8 07 7F 08 89 9E ? ? ? ? '..
  'EB 06 89 AE ? ? ? ?'
-- determineAIPlayerHelp(player, asker): a calm AI with at least its AIC
-- AttForceSupportAllyThreshold attack troops accepts a human's help request.
local helpSignature = '56 8B 74 24 08 8B C6 69 C0 F4 39 00 00 8B 90 ? ? ? ? 85 D2 74 17 83 C2 FF '..
  '83 B8 ? ? ? ? 00 74 11 8B 44 24 0C 50 56 E8 ? ? ? ? 33 C0 5E C2 08 00 8B 80 ? ? ? ? '..
  '69 D2 A4 02 00 00 3B 84 0A FC 01 00 00'
-- AIVState: the castle plans' owner, its plan stride and step layout.
local aivSignature = '83 3D ? ? ? ? 0A 75 0C 6A 00 B9 ? ? ? ? E8 ? ? ? ? 5B C3'
local aivPlanSignature = '69 FF 98 6D 00 00 83 BF ? ? ? ? 00 0F 84'
local aivStepSignature = '69 C0 22 09 00 00 03 C2 8D 14 40 8B 44 24 6C 80 7C 90 38 00'
-- sendUnitsToPlayerIDsKeepIfPossible(player, asker), the native help order:
-- reads the helper's camp, then per attack tribe sends it to the asker's
-- campfire (+0x13B) or onto the asker's keep (+0x142).
local helpSendSignature = '83 EC 18 8B 44 24 1C 53 55 8B 6C 24 28 56 8B F0 69 F6 F4 39 00 00'
local helpSendKeep = 0x142
-- sendUnitsToKeep(tribe, player): the group move to the player's keep
-- building x + 3, y + 4.
local sendToKeepSignature = '8B 44 24 08 69 C0 F4 39 00 00 8B 80 ? ? ? ? 85 C0 7E 2D 69 C0 2C 03 00 00 '..
  '0F BF 88 ? ? ? ? 0F BF 90 ? ? ? ? 8B 44 24 04 6A 00 83 C1 04 51 83 C2 03 52 50 B9'

function M.resolve(game)
  local read = core.readInteger
  local players = game.players
  local handler = context.find('ally goods answer', handlerSignature)
  context.verify(handler + giftOffset, 'ally goods gift', giftSignature)
  local gift = handler + giftOffset
  local aiState = read(gift + 0x09)
  local sellPrice = context.call(gift + 0x3A, 'goods sell value')
  assert(aiState == game.aicRecords - 676
      and read(gift + 0x1A) == game.buildings and read(gift + 0x29) == game.buildings
      and read(gift + 0x36) == game.gameState and read(gift + 0x4A) == game.gameState
      and context.call(gift + 0x55, 'goods sell value') == sellPrice,
    'AIC Tactics: inconsistent native ally goods answer')
  local enough = context.find('trade minimum test', enoughSignature)
  assert(read(enough + 0x0E) == players + 0x2300 and read(enough + 0x4A) == players + 0x4D0,
    'AIC Tactics: inconsistent native trade minimum test')
  local request = context.find('goods request', requestSignature)
  assert(read(request + 0x0F) == players + 0x2300 and read(request + 0x1F) == players + 0x3898
      and read(request + 0x65) == players + 0x209C
      and read(request + 0x9A) == players + 0x39F4 + 0x3824
      and read(request + 0xC0) == game.units,
    'AIC Tactics: inconsistent native goods request')
  local buy = context.find('purchase of wanted goods', buySignature)
  assert(read(buy + 0x0D) == players + 0x2A70 and read(buy + 0x4E) == game.gameState
      and read(buy + 0x59) == players + 0x4D0 + 15 * 4
      and context.call(buy + 0x69, 'goods request') == request,
    'AIC Tactics: inconsistent native purchase of wanted goods')
  local strength = context.find('AI strength feeling', strengthSignature)
  assert(read(strength + 0x02) == players + 0x2B74 and read(strength + 0x0C) == players + 0x30F4
      and read(strength + 0x19) == players + 0x30F4 and read(strength + 0x21) == players + 0x30F4,
    'AIC Tactics: inconsistent native AI strength feeling')
  local help = context.find('AI help answer', helpSignature)
  assert(read(help + 0x0F) == players + 0x2300 and read(help + 0x1C) == players + 0x3820
      and read(help + 0x36) == players + 0x30F0,
    'AIC Tactics: inconsistent native AI help answer')
  local aiv = context.find('AI castle plans', aivSignature)
  local aivState = read(aiv + 0x0C)
  local plan = context.find('AI castle plan stride', aivPlanSignature)
  context.find('AI castle plan steps', aivStepSignature)
  assert(aivState > 0 and read(plan + 0x08) == aivState + 0x14,
    'AIC Tactics: inconsistent native AI castle plans')
  return {allyAnswerHandler = handler, allyEnoughTest = enough,
    allyResourceGain = context.call(gift + 0x1E, 'resource gain'),
    allyResourceLoss = context.call(gift + 0x2D, 'resource loss'),
    allySellPrice = sellPrice, allyBuyPrice = context.call(buy + 0x52, 'goods buy price'),
    allyGoodsReceived = read(gift + 0x42), allyGoodsSent = read(gift + 0x51),
    allyRequestGoods = request, allySentSpeech = context.call(gift + 0x0D, 'goods sent speech'),
    allyLordAlive = context.call(request + 0xC4, 'lord alive'), allyAIState = aiState,
    allyAIVState = aivState, allyDetermineHelp = help}
end

-- The relief's keep order; without it the relief stands at the entrance.
function M.resolveKeep(game)
  local read = core.readInteger
  local players = game.players
  local helpSend = context.find('native help order', helpSendSignature)
  local sendToKeep = context.call(helpSend + helpSendKeep, 'send to keep')
  context.verify(sendToKeep, 'send to keep', sendToKeepSignature)
  assert(read(helpSend + 0x18) == players + 0x1DC and read(helpSend + 0x1F) == players + 0x1D8
      and read(sendToKeep + 0x0C) == players + 0x94
      and read(sendToKeep + 0x1D) == game.buildings + 0x14 + 0xF0
      and read(sendToKeep + 0x24) == game.buildings + 0x14 + 0xEE
      and read(sendToKeep + 0x38) == game.tribes,
    'AIC Tactics: inconsistent native help order')
  return {allySendToKeep = sendToKeep}
end

-- Order of AllyBindings in include/aic_tactics/allies.hpp.
M.names = {'allyResourceGain', 'allyResourceLoss', 'allySellPrice', 'allyBuyPrice',
  'allyGoodsReceived', 'allyGoodsSent', 'allyEnoughTest', 'allyRequestGoods', 'allySentSpeech',
  'allyLordAlive', 'allyAIState', 'allyAIVState', 'allySendToKeep', 'allyGateChange'}

-- The player's gold request to an AI ally: the ally screen's test and the
-- command handler. `hooks` is nil before install, else the installed stubs.
function M.preflight(game, hooks)
  local test = game.allyEnoughTest + enoughGold
  local handler = game.allyAnswerHandler
  if hooks then
    assert(core.readByte(test) == 0xE9 and test + 5 + core.readInteger(test + 1) == hooks.test
        and core.readByte(handler) == 0xE9 and handler + 5 + core.readInteger(handler + 1) == hooks.command,
      'AIC Tactics: the ally gold request was replaced; restart with compatible modules')
    return
  end
  for index, byte in ipairs(enoughGoldBytes) do
    assert(core.readByte(test + index - 1) == byte,
      'AIC Tactics: the ally gold request was replaced; restart with compatible modules')
  end
  context.verify(handler, 'ally goods answer', handlerSignature)
end

function M.install(game, allowed, command)
  local test = game.allyEnoughTest + enoughGold
  local handler = game.allyAnswerHandler
  -- esi holds the resource; the routine's own esi is pushed. Gold for an AI
  -- asks allyGoldRequestAllowed(player, amount).
  local testHook = core.allocateAssembly([[
    cmp esi, 15
    jne goods
    mov eax, dword [esp + 0x10]
    push eax
    mov eax, dword [esp + 0x0C]
    push eax
    call allowedTest
    add esp, 8
    pop esi
    ret 0x0C
goods:
    jmp goodsBranch
  ]], {allowedTest = allowed, goodsBranch = game.allyEnoughTest + enoughGoods})
  local commandHook = core.allocateAssembly([[
    pushfd
    pushad
    push dword [esp + 0x34]
    push dword [esp + 0x34]
    push dword [esp + 0x34]
    push dword [esp + 0x34]
    call commandHandler
    add esp, 16
    test eax, eax
    jnz handled
    popad
    popfd
    mov edx, dword [esp + 8]
    push esi
    push edi
    jmp resumeHandler
handled:
    popad
    popfd
    ret
  ]], {commandHandler = command, resumeHandler = handler + 6})
  core.writeCode(test, {core.jmpTo(testHook)})
  core.writeCode(handler, {core.jmpTo(commandHook), 0x90})
  return {test = testHook, command = commandHook}
end

return M
