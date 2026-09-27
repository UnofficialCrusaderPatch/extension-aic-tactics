"""Licensed-binary check for the UCP AOB engineer-role binding."""
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
        assert len(matches) == 1, (str(path), len(matches))
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
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      core={AOBScan=aob,readByte=read_byte,readInteger=read_integer}
      local context=require('native-context')
      local site=context.find('AI engineer role recount',
        '0F B7 90 3E 0A 00 00 66 83 FA 0A 75 1C 69 FF F4 39 00 00 BB 01 00 00 00 '..
        '01 9F ? ? ? ? 01 9F ? ? ? ? E9 ? ? ? ? 66 83 F9 1E 0F 84 ? ? ? ? '..
        '0F BF CA 83 F9 15')
      local players=core.readInteger(site+26)-0x2BA8
      local duty=context.find('native siege-construction group duty',
        'B9 ? ? ? ? 66 C7 83 ? ? ? ? 0B 00 66 C7 83 ? ? ? ? 10 04 '..
        'E8 ? ? ? ? 83 3D ? ? ? ? 00 0F 85')
      local tribes=core.readInteger(duty+8)-0x42
      sites=require('native-role-bindings').resolve(
        {units=0x1000000,unitRecords=0x1000614,players=players,tribes=tribes})
      assert(core.readByte(sites.engineerRoleSkip)==0xBB)
      assert(core.readByte(sites.engineerRoleOrdinary)==0x0F)
    ''')
    sites = lua.globals().sites
    return {'variant': path.name, 'sha256': identity['sha256'][:8],
            'hook': hex(sites['engineerRoleHook']),
            'ordinary': hex(sites['engineerRoleOrdinary']),
            'skip': hex(sites['engineerRoleSkip'])}


if __name__ == '__main__':
    identities = json.loads(FIXTURES.read_text())
    identities += json.loads((FIXTURES.parent / 'official-patches/executable-matrix.json').read_text())
    print(json.dumps([check(identity) for identity in identities], indent=2))
