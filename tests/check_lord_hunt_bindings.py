"""Licensed-binary check for the lord hunt bindings.

Resolves the real layout, group-action and siege owners against every local
reference executable, then runs the production lord hunt resolver on that
image. It also checks the mode-0 twin of the keep breach test, and that a
changed mode byte, area operand or wall layer operand is refused.
Usage: check_lord_hunt_bindings.py [projects-root]
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
# Same identifying context as native-combat-bindings.lua's market owner, whose
# operand is the GameState object the keep breach test runs on.
MARKET_CALLER = '8B 44 24 10 8B 15 ? ? ? ? 8D 9C 39 E5 01 00 00 50 B9 ? ? ? ? 8D 6C 32 46 E8 ? ? ? ? 6A 00'
MODE_ZERO_TWIN = '66 3B C8 74 B1 0F BF C9 6A 00 51 0F BF D0 52 56 B9 ? ? ? ? E8'


def runtime(image, base):
    def aob(pattern):
        expression = b''.join(b'.' if token == '?' else re.escape(bytes([int(token, 16)]))
                              for token in pattern.split())
        matches = list(re.finditer(expression, bytes(image), re.S))
        if len(matches) != 1:
            raise RuntimeError('pattern matched %d times' % len(matches))
        return base + matches[0].start()

    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.globals().aob = aob
    lua.globals().read_byte = lambda address: image[address - base]
    lua.globals().read_integer = lambda address: struct.unpack_from('<i', image, address - base)[0]
    lua.globals().market_caller = MARKET_CALLER
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      core={AOBScan=function(pattern) return aob(pattern) or error('AOB not found') end,readByte=read_byte,readInteger=read_integer}
      context=require('native-context')
      local layout=require('native-layout')
      game=layout.resolve()
      for key,value in pairs(layout.resolveArmy(game)) do game[key]=value end
      for key,value in pairs(require('native-group-actions').resolve(game)) do game[key]=value end
      game.gameState=core.readInteger(context.find('market caller',market_caller)+19)
      for key,value in pairs(require('native-siege-bindings').resolve(game)) do game[key]=value end
      hunt=require('native-lord-hunt-bindings')
    ''')
    return lua


def refused(lua, image, offset, value):
    saved = bytes(image)
    image[offset:offset + len(value)] = value
    result = lua.execute('return (pcall(hunt.resolve, game))')
    image[:] = saved
    return result is False


def check(identity):
    path = Path(identity['path']) if 'path' in identity else PROJECTS / identity['file']
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == identity['sha256']
    pe = pefile.PE(data=raw)
    base = pe.OPTIONAL_HEADER.ImageBase
    image = bytearray(pe.get_memory_mapped_image())
    lua = runtime(image, base)
    lua.globals().mode_zero_twin = MODE_ZERO_TWIN
    lua.execute('''
      sites=hunt.resolve(game)
      for key,value in pairs(sites) do game[key]=value end
      local keep=game.keepBreachTest
      local twin=context.find('mode-0 keep test',mode_zero_twin)
      assert(core.readInteger(twin+17)==game.areaPathFinder and context.call(twin+21,'twin')==game.areaRoute)
      assert(twin-keep==-0x2E and core.readByte(keep+106)==0x6A and core.readByte(keep+107)==2)
    ''')
    game = lua.globals().game
    keep = game['keepBreachTest']
    path_site = game['tribePath']
    wall_owner = game['wallOwnerCheck']
    checks = {
        'keepModeChanged': refused(lua, image, keep + 107 - base, b'\x00'),
        'areaOperandChanged': refused(lua, image, path_site + 51 - base, struct.pack('<i', game['areaMap'] + 2)),
        'wallLayerChanged': refused(lua, image, wall_owner + 21 - base, struct.pack('<i', game['tileWallOwner'] + 1)),
    }
    assert all(checks.values()), checks
    return {'variant': path.parent.parent.name + '/' + path.name if 'file' in identity else path.name,
            'sha256': identity['sha256'][:8], 'tribePath': hex(path_site),
            'areaMap': hex(game['areaMap']), 'areaRoute': hex(game['areaRoute']),
            'areaPathFinder': hex(game['areaPathFinder']), 'keepBreachTest': hex(keep),
            'tileMap': hex(game['siegeTileMap']), 'tileLogic': hex(game['tileLogic']),
            'tileWallOwner': hex(game['tileWallOwner']), 'gameState': hex(game['gameState']),
            'players': hex(game['players']), 'refusals': checks}


if __name__ == '__main__':
    identities = json.loads(FIXTURES.read_text())
    identities += json.loads((FIXTURES.parent / 'official-patches/executable-matrix.json').read_text())
    print(json.dumps([check(identity) for identity in identities], indent=2))
