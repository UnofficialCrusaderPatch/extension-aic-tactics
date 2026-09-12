"""Reference-byte compatibility and actual calendar instructions; no game launch."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import pefile
from lupa import LuaRuntime, lua_type
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

p = argparse.ArgumentParser()
p.add_argument('--reference', type=Path, required=True)
p.add_argument('--legacy', type=Path, required=True)
p.add_argument('--loader', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--variant', choices=('SHC','SHCE'), default='SHC')
a = p.parse_args()
raw = a.reference.read_bytes()
fixtures = {
    'SHC': ('3bb0a8c1e72331b3a30a5aa93ed94beca0081b476b04c1960e26d5b45387ac5a',
        dict(gameTick=0x1FE7DA8,rngState=0x1A279C0,rngValue=0x1A279C2,rngNext=0x46A7D0,
             initialDefenseTicks=0x4D34B1,aicRecords=0x23FC8E8+676)),
    'SHCE': ('55648e6b05d67d37a5773fe699bbb17a2d6ad4de1bb9dbded9a21caef82bd7fb',
        dict(gameTick=0x2A7B2A8,rngState=0x24BAEC0,rngValue=0x24BAEC2,rngNext=0x46A9F0)),
}
expected_hash, bindings = fixtures[a.variant]
assert hashlib.sha256(raw).hexdigest() == expected_hash
pe = pefile.PE(data=raw)
uc = Uc(UC_ARCH_X86, UC_MODE_32)
base = pe.OPTIONAL_HEADER.ImageBase
uc.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
for section in pe.sections:
    uc.mem_write(base + section.VirtualAddress, section.get_data())
uc.mem_map(0x3000000, 0x10000)


def get(address):
    return struct.unpack('<i', uc.mem_read(address, 4))[0]


def put(address, value):
    uc.mem_write(address, struct.pack('<i', value))


image = pe.get_memory_mapped_image()
scan_count = 0
def scan(pattern, start=None):
    global scan_count
    scan_count += 1
    expression = b''.join(b'.' if t == '?' else re.escape(bytes([int(t, 16)])) for t in pattern.split())
    offset = max(0, (start or base) - base)
    match = re.search(expression, image[offset:], re.DOTALL)
    return base + offset + match.start() if match else None


lua = LuaRuntime()
g = lua.globals()
g.root = Path(__file__).resolve().parents[1].as_posix()
g.core = lua.table_from({'readInteger': get, 'writeInteger': put,
    'readByte': lambda address: uc.mem_read(address, 1)[0], 'AOBScan': scan, 'scanForAOB': scan})
g.utils = lua.table_from({'itob': lambda v: lua.table_from(struct.pack('<i', v))})
def flatten(code):
    return b''.join(flatten(value) if lua_type(value) == 'table' else bytes([value])
                    for value in code.values())


g.core.writeCode = lambda address, code: uc.mem_write(address, flatten(code))
lua.execute("package.path=root..'/?.lua;'..package.path;configFinal={}")
port = lua.execute(a.legacy.read_text())
context = scan('03 CB 03 CD 74 4D')
for months in range(31):
    port.init(port, lua.table_from({'sliderValue': months}))
    port.enable(port, lua.table())
    expected = bytes(uc.mem_read(context, 0xA3))
    put(context+36, 4800)
    lua.execute("package.loaded['config.grace']=nil;grace=require('config.grace')")
    g.grace.configure(months)
    assert bytes(uc.mem_read(context, len(expected))) == expected
    resolved = g.grace.resolveNative()
    for key,value in bindings.items():
        if key != 'aicRecords':
            assert resolved[key] == value, (key,resolved[key],value)

# Use Loader's real discovery implementation, not a second AIC table scan.
owner = lua.execute((a.loader/'addresses.lua').read_text())
g.owner = owner
g.native = lua.table_from({'nativeBindings': 0x3001000, 'nativeBindingsSize':256})
lua.execute('''
  modules={aicloader={getNativeAICLayout=function()
    return {version=1,address=owner.getAIStartAddress(1),characters=16,stride=676}
  end}}
  require('native-bindings').initialize(native)
''')
bound = g.native.game
for index,key in enumerate(('gameTick','rngState','rngValue','rngNext','initialDefenseTicks','aicRecords')):
    assert get(0x3001000+index*4) == bound[key]
    if key in bindings:
        assert bound[key] == bindings[key]
scans_before = scan_count
for months in range(31):
    g.grace.configure(months)
assert scan_count == scans_before

if a.variant == 'SHCE':
    result = {'variant':a.variant,'referenceSHA256':expected_hash,'legacyMonthsCompared':31,
        'resolved':{key:hex(bound[key]) for key in ('gameTick','rngState','rngValue','rngNext','initialDefenseTicks','aicRecords')},
        'repeatPreflightScans':0,
        'scope':'Actual Lua discovery, Loader storage metadata, native binding ABI writes and unchanged Legacy timer bytes; no calendar or running-game acceptance'}
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
    raise SystemExit(0)

# Original game tick -> 200-step scheduler -> four quarters -> one month.
assert pe.get_data(0x45CC19-base, 18) == bytes.fromhex('8386a41809000181bea4180900c80000007c')
assert pe.get_data(0x45CE58-base, 7) == bytes.fromhex('8305a87dfe0101')


def spy(uc, address, size, user):
    if address != 0x4AF6E0:
        return
    # Scenario calendar-pause predicate false; it is unrelated to conversion.
    sp = uc.reg_read(UC_X86_REG_ESP)
    uc.reg_write(UC_X86_REG_EAX, 0)
    uc.reg_write(UC_X86_REG_EIP, get(sp))
    uc.reg_write(UC_X86_REG_ESP, sp + 4)


uc.hook_add(UC_HOOK_CODE, spy)
game = 0x112B0B8
put(0x1FE7DE0, 0)
for offset in [0x519F8, 0x519FC, 0x51A00]:
    put(game + offset, 0)
for tick in range(1, 9601):
    put(0x3008000, 0x3000000)
    put(0x3008004, int(tick % 200 == 0))
    put(0x3008008, tick % 200)
    uc.reg_write(UC_X86_REG_ECX, game)
    uc.reg_write(UC_X86_REG_ESP, 0x3008000)
    uc.emu_start(0x456670, 0x3000000, count=150)
    assert get(game + 0x519F8) == (tick // 200) % 4
    assert get(game + 0x519FC) == (tick // 800) % 12
    assert get(game + 0x51A00) == tick // 9600

result = {'legacyMonthsCompared': 31, 'calendarTicksExecuted': 9600,
    'legacySHA256': hashlib.sha256(a.legacy.read_bytes()).hexdigest(),
    'scope': 'Actual Lua patch bytes and native calendar conversion with unpaused scenario predicate; no running-game acceptance'}
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result))
