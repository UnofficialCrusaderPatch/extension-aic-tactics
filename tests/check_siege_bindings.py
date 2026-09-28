"""Local licensed-binary check for the framework AOB siege binding owner."""
import hashlib
import json
import re
import struct
from pathlib import Path

import pefile
from lupa import LuaRuntime


ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT.parent / 'Roadmap/Investigations/Tunnelers-Engineers/binary-identities.json'


def check(identity):
    path = Path(identity['path']) if 'path' in identity else ROOT.parent / identity['file']
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == identity['sha256']
    pe = pefile.PE(data=raw)
    image = pe.get_memory_mapped_image()
    base = pe.OPTIONAL_HEADER.ImageBase

    def aob(pattern):
        expression = b''.join(b'.' if token == '?' else re.escape(bytes([int(token, 16)]))
                              for token in pattern.split())
        matches = list(re.finditer(expression, image, re.S))
        assert len(matches) == 1, (str(path), pattern[:48], len(matches))
        return base + matches[0].start()

    def read_byte(address):
        return image[address - base]

    def read_integer(address):
        return struct.unpack_from('<i', image, address - base)[0]

    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.globals().aob = aob
    lua.globals().read_byte = read_byte
    lua.globals().read_integer = read_integer
    lua.globals().players = 0x11EEA38 if 'Extreme' in path.name else 0x115BDF8
    lua.globals().tribe_stride = 0x688 if 'Extreme' in path.name else 0x334
    lua.globals().tribes = 0x1F9AFC0 if 'Extreme' in path.name else 0x1667F78
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      core={AOBScan=aob,readByte=read_byte,readInteger=read_integer}
      sites=require('native-siege-bindings').resolve({unitCapacity=2500,players=players,
        tribes=tribes,tribeStride=tribe_stride,mapRows=1})
      assert(sites.siegeTileOccupancyOffset==0x23D7E0)
      assert(core.readByte(sites.siegePlacementCall)==0xE8)
    ''')
    return {'variant': path.name, 'sha256': identity['sha256'][:8],
            'placementCall': hex(lua.globals().sites['siegePlacementCall']),
            'tileCheck': hex(lua.globals().sites['originalSiegeTileCheck']),
            'noSiteBranch': hex(lua.globals().sites['siegeNoSpotBranch']),
            'failedBranch': hex(lua.globals().sites['siegeFailedBranch']),
            'angleCandidateBranch': hex(lua.globals().sites['siegeAngleCandidateBranch']),
            'anglePostPlace': hex(lua.globals().sites['siegeAnglePostPlace']),
            'placementFail': hex(lua.globals().sites['siegePlacementFail']),
            'tentPointX': hex(lua.globals().sites['siegeTentPointX'])}


if __name__ == '__main__':
    identities = json.loads(FIXTURES.read_text())
    identities += json.loads((FIXTURES.parent / 'official-patches/executable-matrix.json').read_text())
    print(json.dumps([check(identity) for identity in identities], indent=2))
