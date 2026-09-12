"""Execute real SHC/Extreme AIC queries with production Lua discovery.

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
p.add_argument('--combat-output',type=Path)
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



lua=LuaRuntime()
g=lua.globals();g.root=root.as_posix()
lua.execute("package.path=root..'/?.lua;'..package.path")
g.core=lua.table_from({'readInteger':get,'readByte':lambda address:uc.mem_read(address,1)[0],
    'AOBScan':scan,'scanForAOB':scan})
lua.execute("""
game=require('native-layout').resolve()
for k,v in pairs(require('native-group-actions').resolve(game)) do game[k]=v end
for k,v in pairs(require('native-recruitment').resolve(game)) do game[k]=v end
""")
start=len(scans)
owner=lua.eval("(require('native-aic-queries'))")
bound=owner.resolve(g.game)
contexts=[item for item in scans[start:] if item[1] is None]
assert len(contexts)==9
sites={pattern:address for pattern,_,address in contexts}
negative=0
for pattern,_,address in contexts:
    for kind in ('absent','ambiguous'):
        g.core.AOBScan=lambda pat:None if kind=='absent' and pat==pattern else sites[pat]
        g.core.scanForAOB=lambda pat,start:scratch if kind=='ambiguous' and pat==pattern else None
        try:owner.resolve(g.game)
        except Exception as error:assert 'AIC Tactics:' in str(error)
        else:raise AssertionError((kind,pattern))
        negative+=1
g.core.AOBScan=sites.__getitem__;g.core.scanForAOB=lambda *_:None
for index,offset in [(1,22),(1,27),(2,12),(2,289),(3,22),(4,2),(4,44),
                     (5,13),(5,118),(6,30),(7,142),(8,46)]:
    target=contexts[index][2]+offset
    g.core.readInteger=lambda address:0 if address==target else get(address)
    try:owner.resolve(g.game)
    except Exception as error:assert 'AIC Tactics:' in str(error)
    else:raise AssertionError(('operand',index,offset))
    negative+=1
for offset in (41,47,57,63):
    target=g.game.recruitEuropean+offset
    g.core.readInteger=lambda address:0 if address==target else get(address)
    try:owner.resolve(g.game)
    except Exception as error:assert 'AIC Tactics:' in str(error)
    else:raise AssertionError(('equipment',offset))
    negative+=1
g.core.readInteger=get
old=uc.mem_read(g.game.recruitEuropean,1)
uc.mem_write(g.game.recruitEuropean,b'\x90')
try:owner.resolve(g.game)
except Exception as error:assert 'AIC Tactics:' in str(error)
else:raise AssertionError('occupied acquisition owner admitted')
uc.mem_write(g.game.recruitEuropean,bytes(old));negative+=1

def call(function,owner,*args):
    sp=scratch+0x18000;put(sp,scratch)
    for index,value in enumerate(args):put(sp+4+index*4,value)
    uc.reg_write(UC_X86_REG_ESP,sp);uc.reg_write(UC_X86_REG_ECX,owner)
    uc.emu_start(function,scratch,count=200000)
    assert uc.reg_read(UC_X86_REG_ESP)==sp+4+len(args)*4,'native thiscall stack mismatch'
    return uc.reg_read(UC_X86_REG_EAX)

aic=scratch+0x1000
role_cases=0
role_fields={11:0x240,13:0x248,16:0x25C,17:0x268,18:0x274,19:0x27C}
constants={10:30,12:73,14:29,15:5}
for player in range(1,9):
    for character in range(16):
        put(g.game.players+player*0x39F4+0x2300,character+1)
        for role,offset in role_fields.items():put(aic+character*676+offset,22+(role+character)%7)
        for slot in range(4):put(aic+character*676+0x288+slot*4,22+slot)
        for role in range(10,20):
            result=call(bound.attackRecruitType,aic,player,role)
            assert result==constants.get(role,22+(role+character)%7)
            role_cases+=1
        cursor=g.game.players+player*0x39F4+0x3108
        for initial,expected in [(0,22),(1,23),(2,24),(3,25),(4,22)]:
            put(cursor,initial)
            assert call(bound.attackRecruitType,aic,player,20)==expected
            assert get(cursor)==(initial%4)+1
            role_cases+=1

raid_cases=0
for player in range(1,9):
    for character in range(16):
        for amount in (0,1,2,3,11):
            put(aic+character*676+0x1A4,amount)
            put(g.game.players+player*0x39F4+0x38F4,2)
            for scenario in (False,True):
                for field,value in [('scenarioMode',3),('scenarioCustom',1),('scenarioMission',2)]:
                    put(g.game[field],value if scenario else 0)
                assert call(bound.raidMaximum,aic,character,player)==((amount+2)*4//3 if scenario else amount+2)
                raid_cases+=1

building_cases=0
buildings=g.game.buildings
put(buildings+8,2000)
for index in (1,2,1999):
    record=buildings+0x14+index*0x32C
    short(record+0xD0,2);short(record+0xD2,38);short(record+0xD6,1)
assert call(bound.findRecruitmentBuilding,buildings,1,38)==1;building_cases+=1
short(buildings+0x14+0x32C+0xD0,3)
assert call(bound.findRecruitmentBuilding,buildings,1,38)==2;building_cases+=1
short(buildings+0x14+2*0x32C+0xD0,0)
assert call(bound.findRecruitmentBuilding,buildings,1,38)==1999;building_cases+=1
assert call(bound.findRecruitmentBuilding,buildings,2,38)==0;building_cases+=1
assert call(bound.findRecruitmentBuilding,buildings,1,37)==0;building_cases+=1

result={'variant':a.variant,'referenceSHA256':digest,'contexts':9,'negativeCases':negative,
    'nativeRoleCases':role_cases,'nativeRaidMaximumCases':raid_cases,'nativeBuildingCases':building_cases,
    'resolved':dict(bound.items()),'scope':'Actual native query instructions, side effects and thiscall ABI; no active-game acceptance'}
a.output.parent.mkdir(parents=True,exist_ok=True)
a.output.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))

if a.combat_output:
    for k,v in bound.items():g.game[k]=v
    g.core.AOBScan=scan;g.core.scanForAOB=scan
    clock=lua.eval("(require('config.grace'))").resolveNative()
    g.game.gameTick=clock.gameTick
    start=len(scans)
    combat_owner=lua.eval("(require('native-combat-bindings'))")
    combat=combat_owner.resolve(g.game)
    contexts=[item for item in scans[start:] if item[1] is None]
    assert len(contexts)==23
    sites={pattern:address for pattern,_,address in contexts}
    negative=0
    for pattern,_,address in contexts:
        for kind in ('absent','ambiguous'):
            g.core.AOBScan=lambda pat:None if kind=='absent' and pat==pattern else sites[pat]
            g.core.scanForAOB=lambda pat,start:scratch if kind=='ambiguous' and pat==pattern else None
            try:combat_owner.resolve(g.game)
            except Exception as error:assert 'AIC Tactics:' in str(error)
            else:raise AssertionError(('combat',kind,pattern))
            negative+=1
    g.core.AOBScan=sites.__getitem__;g.core.scanForAOB=lambda *_:None
    for index,offset in [(0,18),(0,86),(0,326),(1,25),(2,132),(2,226),(3,84),
            (4,122),(4,160),(5,49),(6,19),(6,28),(7,22),(7,27),(7,127),(8,28),
            (9,96),(10,71),(12,3),(12,11),(12,127),(13,1),(14,33),(15,19),
            (16,89),(17,49),(18,29),(19,14),(20,28),(21,12),(22,75)]:
        target=contexts[index][2]+offset
        g.core.readInteger=lambda address:0 if address==target else get(address)
        try:combat_owner.resolve(g.game)
        except Exception as error:assert 'AIC Tactics:' in str(error)
        else:raise AssertionError(('combat operand',index,offset))
        negative+=1
    for target,value in [(combat.marketPrice+7,0),(combat.combatValue+0x8C,0)]:
        g.core.readInteger=lambda address:value if address==target else get(address)
        try:combat_owner.resolve(g.game)
        except Exception as error:assert 'AIC Tactics:' in str(error)
        else:raise AssertionError(('market/switch',target))
        negative+=1
    g.core.readInteger=get
    for target,value in [(combat.marketPrice,0x90),(combat.combatValue+0xBC,12)]:
        g.core.readByte=lambda address:value if address==target else uc.mem_read(address,1)[0]
        try:combat_owner.resolve(g.game)
        except Exception as error:assert 'AIC Tactics:' in str(error)
        else:raise AssertionError(('owner/index',target))
        negative+=1
    g.core.readByte=lambda address:uc.mem_read(address,1)[0]
    # Original caller-identified price owner: the field moves in Extreme.
    price_cases=0
    for resource in range(1,26):
        for amount in (-17,-1,0,1,4,5,9,123456):
            put(combat.gameState+get(combat.marketPrice+7)+resource*8,amount)
            assert call(combat.marketPrice,combat.gameState,resource)==int(amount/5)&0xFFFFFFFF
            price_cases+=1
    weights=[call(combat.combatValue,combat.troopValues,kind) for kind in range(80)]
    assert weights[0]==0 and weights[22]>0
    lifecycle_cases=0
    for player in range(1,9):
        put(g.game.players+player*0x39F4+0x2300,0)
        for fn in (combat.selectAttackTarget,combat.updateAIPlayerState,combat.updateRaids,combat.returnAttack):
            call(fn,aic,player);lifecycle_cases+=1
        put(get(combat.computeNervousness+11)+player*4,10000)
        put(g.game.players+player*0x39F4+0x38E0,50)
        put(g.game.players+player*0x39F4+0x3820,1)
        call(combat.computeNervousness,aic,player)
        assert get(g.game.players+player*0x39F4+0x3820)==0
        lifecycle_cases+=1
    result={'variant':a.variant,'referenceSHA256':digest,'contexts':23,'negativeCases':negative,
        'nativePriceCases':price_cases,'nativeCombatValues':weights,'nativeLifecycleCases':lifecycle_cases,
        'scope':'Actual native query instructions and thiscall ABI; lifecycle early returns, not active games'}
    a.combat_output.parent.mkdir(parents=True,exist_ok=True)
    a.combat_output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
