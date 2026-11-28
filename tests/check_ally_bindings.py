"""Licensed-binary check for the ally help bindings.

Resolves the real layout and the ally owners against every local reference
executable, then runs the production preflight and install of the player's
gold request on that image: both sites are redirected, preflight then
expects the hooks, and a replaced site is refused before install. The same
for the relief's gate hook. It also prints the native help owners for the
HELP-NATIVE notes.
Usage: check_ally_bindings.py [projects-root]
"""
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

import pefile
from lupa import LuaRuntime

ROOT = Path(__file__).resolve().parents[1]
PROJECTS = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT.parent
FIXTURES = PROJECTS / 'Roadmap/Investigations/Tunnelers-Engineers/binary-identities.json'
# AIC Loader's own AIC array owner (aicloader addresses.lua); its records start
# one character stride later.
AIC_ARRAY = ('? ? ? ? e8 ? ? ? ? 89 1d ? ? ? ? 83 3d ? ? ? ? 00 75 44 6a 08 b9 ? ? ? ? e8 ? ? ? ? '
             '85 c0 74 34 8b c5 2b 05')
# native-combat-bindings.lua's market caller: the game state root.
MARKET_CALLER = '8B 44 24 10 8B 15 ? ? ? ? 8D 9C 39 E5 01 00 00 50 B9 ? ? ? ? 8D 6C 32 46 E8 ? ? ? ? 6A 00'
# Native help owners documented in HELP-NATIVE.md (not bound by the module).
NATIVE_HELP = {
    'updateAIPlayerState': '53 56 8B 74 24 0C 57 8B FE 69 FF F4 39 00 00 8B 87 ? ? ? ? 85 C0 8B D9 0F 84',
    'allyCommand': '8B 44 24 10 83 F8 0A 75 23 8B 4C 24 08 8B 54 24 0C 69 C9 F4 39 00 00 33 C0',
    'shareGoldAmongTeamMembers': '83 EC 28 53 55 56 33 F6 57 8D 4E 03 BB',
    'sendUnitsToPlayerIDsKeepIfPossible': '83 EC 18 8B 44 24 1C 53 55 8B 6C 24 28 56 8B F0 69 F6 F4 39 00 00',
}


def runtime(image, base):
    def aob(pattern):
        expression = b''.join(b'.' if token == '?' else re.escape(bytes([int(token, 16)]))
                              for token in pattern.split())
        matches = list(re.finditer(expression, bytes(image), re.S))
        if len(matches) != 1:
            raise RuntimeError('pattern matched %d times: %s' % (len(matches), pattern[:40]))
        return base + matches[0].start()

    def write_code(address, values):
        offset = address - base
        image[offset:offset + 5] = bytes([values[1]]) + struct.pack('<i', values[2])
        for index in range(3, len(values) + 1):
            image[offset + 2 + index] = values[index]

    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.globals().aob = aob
    lua.globals().read_byte = lambda address: image[address - base]
    lua.globals().read_integer = lambda address: struct.unpack_from('<i', image, address - base)[0]
    lua.globals().write_code = write_code
    lua.globals().aic_array = AIC_ARRAY
    lua.globals().market_caller = MARKET_CALLER
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      core={AOBScan=function(pattern) return aob(pattern) or error('AOB not found') end,readByte=read_byte,readInteger=read_integer,
        jmpTo=function(target)return target end,
        writeCode=function(address, values)
          table.insert(values, 1, 0xE9)
          values[2]=values[2]-address-5
          write_code(address, values)
        end,
        allocateAssembly=function(text, symbols)
          allocated=(allocated or 0x30000000)+0x1000
          stubs[#stubs+1]={address=allocated,text=text,symbols=symbols}
          return allocated
        end}
      stubs={}
      context=require('native-context')
      game=require('native-layout').resolve()
      game.aicRecords=core.readInteger(context.find('AIC array',aic_array))+676
      game.gameState=core.readInteger(context.find('market caller',market_caller)+19)
      allies=require('native-ally-bindings')
      gates=require('native-gate-bindings')
    ''')
    return lua


def check(identity):
    path = Path(identity['path']) if 'path' in identity else PROJECTS / identity['file']
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == identity['sha256']
    pe = pefile.PE(data=raw)
    base = pe.OPTIONAL_HEADER.ImageBase
    image = bytearray(pe.get_memory_mapped_image())
    lua = runtime(image, base)
    lua.execute('''
      sites=allies.resolve(game)
      for key,value in pairs(allies.resolveKeep(game)) do sites[key]=value end
      for key,value in pairs(gates.resolve(game)) do sites[key]=value end
      for key,value in pairs(sites) do game[key]=value end
      for _,name in ipairs(allies.names) do assert(type(game[name])=='number' and game[name]>0, name) end
      allies.preflight(game, nil)
      gates.preflight(game, nil)
    ''')
    sites = lua.globals().sites
    game = lua.globals().game
    native_help = {}
    for name, pattern in NATIVE_HELP.items():
        native_help[name] = hex(lua.eval('context.find')(name, pattern))
    result = {'variant': path.parent.parent.name + '/' + path.name if 'file' in identity else path.name,
              'sha256': identity['sha256'][:8]}
    for key in ('allyAnswerHandler', 'allyEnoughTest', 'allyRequestGoods', 'allyDetermineHelp',
                'allyResourceGain', 'allyResourceLoss', 'allySellPrice', 'allyBuyPrice',
                'allySentSpeech', 'allyLordAlive', 'allyGoodsReceived', 'allyGoodsSent',
                'allyAIState', 'allyAIVState', 'allySendToKeep', 'allyGateSite', 'allyGateExit',
                'allyGateChange'):
        result[key] = hex(sites[key])
    result['players'] = hex(game['players'])
    result['nativeHelp'] = native_help

    # A site another module replaced is refused before install.
    saved = bytes(image)
    test = sites['allyEnoughTest'] + 0x21
    image[test - base] = 0xE9
    assert lua.execute('return (pcall(allies.preflight, game, nil))') is False
    image[:] = saved
    handler = sites['allyAnswerHandler']
    image[handler - base] = 0xE9
    assert lua.execute('return (pcall(allies.preflight, game, nil))') is False
    image[:] = saved
    # Install redirects both sites; afterwards only the hooks are accepted.
    lua.execute('''
      hooks=allies.install(game, 0x31000000, 0x31000100)
      assert(#stubs==2 and hooks.test==stubs[1].address and hooks.command==stubs[2].address)
      assert(stubs[1].symbols.allowedTest==0x31000000 and stubs[2].symbols.commandHandler==0x31000100)
      assert(stubs[1].symbols.goodsBranch==game.allyEnoughTest+0x2C)
      assert(stubs[2].symbols.resumeHandler==game.allyAnswerHandler+6)
      local test=game.allyEnoughTest+0x21
      assert(core.readByte(test)==0xE9 and test+5+core.readInteger(test+1)==hooks.test)
      assert(core.readByte(game.allyAnswerHandler)==0xE9
        and game.allyAnswerHandler+5+core.readInteger(game.allyAnswerHandler+1)==hooks.command)
      assert(core.readByte(game.allyAnswerHandler+5)==0x90)
      allies.preflight(game, hooks)
      assert(not pcall(allies.preflight, game, nil))
    ''')
    image[:] = saved
    # The gate hook: refused when replaced, then only the installed hook passes.
    gate = sites['allyGateSite']
    image[gate - base] = 0xE9
    assert lua.execute('return (pcall(gates.preflight, game, nil))') is False
    image[:] = saved
    image[gate + 3 - base] ^= 0xFF
    assert lua.execute('return (pcall(gates.preflight, game, nil))') is False
    image[:] = saved
    lua.execute('''
      stubs={}
      local hook=gates.install(game, 0x31000200)
      assert(#stubs==1 and hook==stubs[1].address and stubs[1].symbols.gateHeld==0x31000200)
      assert(stubs[1].symbols.resume==game.allyGateSite+7 and stubs[1].symbols.exit==game.allyGateExit)
      assert(stubs[1].symbols.timer==game.buildings+0x14+0x2C0)
      assert(core.readByte(game.allyGateSite)==0xE9
        and game.allyGateSite+5+core.readInteger(game.allyGateSite+1)==hook)
      gates.preflight(game, hook)
      assert(not pcall(gates.preflight, game, nil))
    ''')
    image[:] = saved
    return result


if __name__ == '__main__':
    identities = json.loads(FIXTURES.read_text())
    identities += json.loads((FIXTURES.parent / 'official-patches/executable-matrix.json').read_text())
    print(json.dumps([check(identity) for identity in identities], indent=2))
