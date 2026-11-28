"""Licensed-binary check for the attack patrol and siege-defence bindings.

Resolves the real layout, group-action and patrol owners against every local
reference executable, then runs the production preflight and install on that
image: install redirects all three patrol callers and the siege-defence rally's
move order, and a replaced site is refused. The distance layer and rally list
operands are checked against the instructions that read them.
Usage: check_patrol_bindings.py [projects-root]
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
STATE_UPDATE = '53 56 8B 74 24 0C 57 8B FE 69 FF F4 39 00 00 8B 87 ? ? ? ? 85 C0 8B D9 0F 84'
# Same identifying context as native-siege-bindings.lua's group movement owner.
GROUP_MOVE = '83 EC 34 33 C0 55 8B 6C 24 44 57 8B 7C 24 44 3B F8 89 4C 24 08 89 44 24 18'


def runtime(image, base):
    def aob(pattern):
        expression = b''.join(b'.' if token == '?' else re.escape(bytes([int(token, 16)]))
                              for token in pattern.split())
        matches = list(re.finditer(expression, bytes(image), re.S))
        if len(matches) != 1:
            raise RuntimeError('pattern matched %d times' % len(matches))
        return base + matches[0].start()

    def write_code(address, values):
        # The module only writes E8 rel32 here: one opcode byte, then a dword.
        offset = address - base
        image[offset:offset + 5] = bytes([values[1]]) + struct.pack('<i', values[2])

    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.globals().aob = aob
    lua.globals().read_byte = lambda address: image[address - base]
    lua.globals().read_integer = lambda address: struct.unpack_from('<i', image, address - base)[0]
    lua.globals().write_code = write_code
    lua.globals().state_update = STATE_UPDATE
    lua.globals().group_move = GROUP_MOVE
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      core={AOBScan=function(pattern) return aob(pattern) or error('AOB not found') end,readByte=read_byte,readInteger=read_integer,
        callTo=function(target)return target end,
        writeCode=function(address, values) write_code(address, {0xE8, values[1]-address-5}) end}
      context=require('native-context')
      local layout=require('native-layout')
      game=layout.resolve()
      for key,value in pairs(layout.resolveArmy(game)) do game[key]=value end
      for key,value in pairs(require('native-group-actions').resolve(game)) do game[key]=value end
      game.updateAIPlayerState=context.find('AI state update',state_update)
      game.siegeGroupMove=context.find('group move',group_move)
      patrol=require('native-patrol-bindings')
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
      sites=patrol.resolve(game)
      for key,value in pairs(sites) do game[key]=value end
      assert(#game.attackPatrolCalls==3)
      for _,site in ipairs(game.attackPatrolCalls) do
        assert(context.call(site,'patrol caller')==game.attackPatrolDispatch)
        assert(site-game.updateAIPlayerState<0xA00)
      end
      patrol.preflight(game, game.attackPatrolDispatch)
    ''')
    sites = lua.globals().sites
    game = lua.globals().game
    calls = [sites['attackPatrolCalls'][index] for index in (1, 2, 3)]
    rally_move = sites['siegeDefenceRallyMove']
    rally = rally_move - 0x127

    def u32(address):
        return struct.unpack_from('<I', image, address - base)[0]

    # The rally reads slots 190/191, the tribe's own rally list (ten points, then
    # the current index and the count) and the leader's nearest-enemy distance.
    assert u32(rally + 0x24) == game['players'] + 0x310C + 190 * 2
    assert sites['tribeRallyCount'] == sites['tribeRallyPoints'] + 10 * 4 + 2
    assert sites['tribeRallyPoints'] > 0x60 + game['tribeMemberWords'] * 2
    assert sites['tribeRallyCount'] + 2 <= game['tribeStride']
    # Its two callers sit directly before the patrol calls of states 3 and 4.
    for call in calls[:2]:
        site = call - 8
        assert image[site - base] == 0xE8 and site + 5 + struct.unpack_from('<i', image, site + 1 - base)[0] == rally
    # The distance layers are 80400-byte tile maps indexed per player at stride 0x177BC.
    search = sites['attackPatrolDestination'] + 0x49
    search = search + 5 + struct.unpack_from('<i', image, search + 1 - base)[0]
    assert u32(search + 0x170 + 0x13) == sites['distanceLayerIndex']
    assert u32(search + 0x170 + 0x26) == sites['distanceLayers']
    result = {'variant': path.parent.parent.name + '/' + path.name if 'file' in identity else path.name,
              'sha256': identity['sha256'][:8], 'dispatch': hex(sites['attackPatrolDispatch']),
              'destination': hex(sites['attackPatrolDestination']),
              'troopValueState': hex(sites['troopValueState']),
              'patrolStance': hex(game['tribeStance']),
              'playableTiles': hex(sites['playableTiles']),
              'tribeStride': hex(game['tribeStride']),
              'callOffsets': [hex(call - game['updateAIPlayerState']) for call in calls],
              'distanceLayers': hex(sites['distanceLayers']),
              'distanceLayerIndex': hex(sites['distanceLayerIndex']),
              'siegeDefenceRally': hex(rally),
              'rallyPoints': hex(sites['tribeRallyPoints']), 'rallyCount': hex(sites['tribeRallyCount'])}

    # A caller that no longer calls the expected target is refused.
    saved = bytes(image)
    offset = calls[1] - base
    image[offset + 1:offset + 5] = struct.pack('<i', 0x1000)
    assert lua.execute('return (pcall(patrol.preflight, game, game.attackPatrolDispatch))') is False
    image[:] = saved
    # Install redirects all three callers; afterwards the handler is expected.
    lua.execute('''
      local handler = game.updateAIPlayerState + 0x40000
      patrol.install(game, handler)
      for _, site in ipairs(game.attackPatrolCalls) do
        assert(context.call(site, 'redirected caller') == handler)
      end
      patrol.preflight(game, handler)
      assert(not pcall(patrol.preflight, game, game.attackPatrolDispatch))
    ''')
    image[:] = saved
    # The siege screen: an unchanged rally move passes, a replaced one is refused,
    # and install redirects exactly that call.
    lua.execute('patrol.preflightScreen(game, game.siegeGroupMove)')
    offset = rally_move - base
    image[offset + 1:offset + 5] = struct.pack('<i', 0x1000)
    assert lua.execute('return (pcall(patrol.preflightScreen, game, game.siegeGroupMove))') is False
    image[:] = saved
    lua.execute('''
      local handler = game.updateAIPlayerState + 0x50000
      patrol.installScreen(game, handler)
      assert(context.call(game.siegeDefenceRallyMove, 'redirected rally move') == handler)
      patrol.preflightScreen(game, handler)
      assert(not pcall(patrol.preflightScreen, game, game.siegeGroupMove))
    ''')
    image[:] = saved
    # A changed rally fight check is not resolved.
    image[rally + 0x116 - base] = 0x61
    assert lua.execute('return (pcall(patrol.resolve, game))') is False
    image[:] = saved
    return result


if __name__ == '__main__':
    identities = json.loads(FIXTURES.read_text())
    identities += json.loads((FIXTURES.parent / 'official-patches/executable-matrix.json').read_text())
    print(json.dumps([check(identity) for identity in identities], indent=2))
