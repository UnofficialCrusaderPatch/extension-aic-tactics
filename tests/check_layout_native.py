"""Execute real SHC/Extreme allocation/membership with production Lua discovery.

Private licensed executable required; this is a component test, not gameplay.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

import pefile
from lupa import LuaRuntime
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

p = argparse.ArgumentParser()
p.add_argument('--reference', type=Path, required=True)
p.add_argument('--variant', choices=['SHC', 'SHCE'], required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
raw = a.reference.read_bytes()
expected = {
    'SHC': ('3bb0a8c1e72331b3a30a5aa93ed94beca0081b476b04c1960e26d5b45387ac5a',
            dict(units=0x1387F38, unitCapacity=2500, tribes=0x1667F78, tribeStride=0x334,
                 entities=0x2350314, entityCapacity=3000, players=0x115BDF8, teams=0x117D548)),
    'SHCE': ('55648e6b05d67d37a5773fe699bbb17a2d6ad4de1bb9dbded9a21caef82bd7fb',
             dict(units=0x145CA28, unitCapacity=10000, tribes=0x1F9AFC0, tribeStride=0x688,
                  entities=0x2DE3814, entityCapacity=6000, players=0x11EEA38, teams=0x1210188)),
}
digest, reference_values = expected[a.variant]
assert hashlib.sha256(raw).hexdigest() == digest
pe = pefile.PE(data=raw)
base = pe.OPTIONAL_HEADER.ImageBase
image = pe.get_memory_mapped_image()
uc = Uc(UC_ARCH_X86, UC_MODE_32)
uc.mem_map(base, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
for section in pe.sections:
    uc.mem_write(base + section.VirtualAddress, section.get_data())
scratch = 0x70000000
uc.mem_map(scratch, 0x20000)
scans = []


def scan(pattern, start=None):
    expression = b''.join(b'.' if t == '?' else re.escape(bytes([int(t, 16)])) for t in pattern.split())
    offset = max(0, (start or base) - base)
    match = re.search(expression, image[offset:], re.DOTALL)
    address = base + offset + match.start() if match else None
    scans.append((pattern, start, address))
    return address


def get(address):
    return struct.unpack('<i', uc.mem_read(address, 4))[0]


def put(address, value):
    uc.mem_write(address, struct.pack('<i', value))


def short(address, value):
    uc.mem_write(address, struct.pack('<h', value))


def resolve(read=get, find=scan, second=scan):
    lua = LuaRuntime()
    lua.globals().core = lua.table_from({'readInteger': read, 'AOBScan': find, 'scanForAOB': second})
    return dict(lua.execute((root / 'native-layout.lua').read_text()).resolve().items())


bound = resolve()
for name, value in reference_values.items():
    assert bound[name] == value, (name, hex(bound[name]), hex(value))
contexts = [item for item in scans if item[1] is None]
assert len(contexts) == 8 and len(scans) == 16
assert bound['tribeMemberWords'] == (bound['unitCapacity'] + 15) // 16

# Reject absent/ambiguous contexts and incompatible decoded layout before any
# native call. Exercise actual signatures above; reuse found sites below solely
# to isolate negative operand handling without repeating private fixture scans.
sites = {pattern: address for pattern, _, address in contexts}
negative = 0
for pattern, _, address in contexts:
    for kind in ('absent', 'ambiguous'):
        def find(pat):
            return None if kind == 'absent' and pat == pattern else sites[pat]
        def second(pat, start):
            return scratch if kind == 'ambiguous' and pat == pattern else None
        try:
            resolve(find=find, second=second)
        except Exception as error:
            assert 'AIC Tactics:' in str(error)
        else:
            raise AssertionError((kind, pattern))
        negative += 1

mutations = [(0, 51, 10001), (0, 101, 1), (1, 9, 1251), (1, 42, 0),
             (1, 63, 0), (2, 89, 0), (2, 190, 0), (3, 31, 0),
             (4, 40, 0), (4, 112, 0), (5, 53, 1), (6, 15, 6001),
             (6, 50, 0), (7, 39, 0)]
for index, offset, value in mutations:
    address = contexts[index][2] + offset
    def read(target):
        return value if target == address else get(target)
    try:
        resolve(read=read, find=sites.__getitem__, second=lambda *_: None)
    except Exception as error:
        assert 'AIC Tactics:' in str(error)
    else:
        raise AssertionError(('operand', index, offset))
    negative += 1


def call(function, owner, *args):
    sp = scratch + 0x10000
    put(sp, scratch)
    for index, value in enumerate(args):
        put(sp + 4 + index * 4, value)
    uc.reg_write(UC_X86_REG_ESP, sp)
    uc.reg_write(UC_X86_REG_ECX, owner)
    uc.emu_start(function, scratch, count=1000000)
    assert uc.reg_read(UC_X86_REG_ESP) == sp + 4 + len(args) * 4, 'native thiscall stack mismatch'
    return uc.reg_read(UC_X86_REG_EAX)


tribes, stride = bound['tribes'], bound['tribeStride']
allocation_checks = 0
for player in range(1, 9):
    for mode in range(4):
        uc.mem_write(tribes, bytes(0x28 + 1250 * stride))
        available = []
        for group in range(1250 - player, 0, -8):
            free = mode == 0 or mode == 2 and group == 1250 - player - 24 or mode == 3 and group <= 8
            short(tribes + group * stride + 0x40, 0 if free else 2)
            if free:
                available.append(group)
        created = call(bound['createTribe'], tribes, player)
        assert created == (available[0] if available else 0)
        if created:
            record = tribes + created * stride
            assert get(record + 0x2C) == player and get(record + 0x40) & 0xFFFF == 2
        allocation_checks += 1

# Actual native membership includes the final valid ID, not just a low-ID
# synthetic pool. Verify the selected bit, count, leader and unit group UID.
membership_checks = 0
uc.mem_write(tribes, bytes(0x28 + 1250 * stride))
group = call(bound['createTribe'], tribes, 1)
record = tribes + group * stride
for unit in (1, 15, 16, bound['unitCapacity'] - 1):
    unit_record = bound['unitRecords'] + unit * 0x490
    uc.mem_write(unit_record, bytes(0x490))
    short(unit_record + 0x8C, 2)
    short(unit_record + 0x8E, 22)
    short(unit_record + 0x96, 1)
    call(bound['addUnitToTribe'], tribes, unit, group)
    bits = struct.unpack('<H', uc.mem_read(record + 0x60 + (unit // 16) * 2, 2))[0]
    assert bits & (1 << (unit % 16))
    assert struct.unpack('<h', uc.mem_read(unit_record + 0x2D8, 2))[0] == group
    assert get(unit_record + 0x2E4) == get(record + 0x34)
    membership_checks += 1
assert get(record + 0x5C) & 0xFFFF == membership_checks
assert get(record + 0x5A) & 0xFFFF == 1

result = dict(variant=a.variant, referenceSHA256=digest, resolved=bound,
              uniqueContexts=len(contexts), negativeResolutionChecks=negative,
              nativeAllocationCases=allocation_checks, nativeMembershipCases=membership_checks,
              scope='Actual Lua discovery and original native allocation/membership, including high IDs. No running-game, MP or replay acceptance.')
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result))
