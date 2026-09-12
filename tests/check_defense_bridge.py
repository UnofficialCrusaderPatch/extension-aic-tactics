"""Compare actual FASM census wrappers with the unchanged Legacy Lua port.

The added C++ calls are ABI spies here; their logic has native test-host coverage.
This is hook/register/flags evidence, not gameplay or save/load acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import pefile
from lupa import LuaRuntime, lua_type
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EFLAGS, UC_X86_REG_EIP)

p=argparse.ArgumentParser()
p.add_argument('--reference',type=Path,required=True)
p.add_argument('--legacy',type=Path,required=True)
p.add_argument('--fasm',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--combat',action='store_true')
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
raw=a.reference.read_bytes()
assert hashlib.sha256(raw).hexdigest()=='3bb0a8c1e72331b3a30a5aa93ed94beca0081b476b04c1960e26d5b45387ac5a'
pe=pefile.PE(data=raw);uc=Uc(UC_ARCH_X86,UC_MODE_32)
base=pe.OPTIONAL_HEADER.ImageBase
uc.mem_map(base,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
for section in pe.sections:uc.mem_write(base+section.VirtualAddress,section.get_data())
uc.mem_map(0x3000000,0x100000)
lua=LuaRuntime(unpack_returned_tuples=True)
allocation=0x3020000
def get(address):return struct.unpack('<i',uc.mem_read(address,4))[0]
def put(address,value):uc.mem_write(address,struct.pack('<I',value&0xFFFFFFFF))
def itob(value):return lua.table_from(struct.pack('<I',int(value)&0xFFFFFFFF))
def size(code):
    return sum(size(v) if lua_type(v)=='table' else 4 if lua_type(v)=='function' else
        1 if 0<=v<=255 else 4 for v in code.values())
def compile_at(address,code):
    output=bytearray()
    for value in code.values():
        if lua_type(value)=='function':value=value(address+len(output),len(output)+1,lua.table())
        if lua_type(value)=='table':output.extend(compile_at(address+len(output),value))
        elif 0<=value<=255:output.append(int(value))
        else:output.extend(struct.pack('<I',int(value)&0xFFFFFFFF))
    return bytes(output)
def write(address,code):uc.mem_write(address,compile_at(address,code))
def allocate(count):
    global allocation
    answer=allocation;allocation+=(count+15)&~15
    assert allocation<0x3060000
    return answer
def scan(pattern):
    expression=b''.join(b'.' if t=='?' else re.escape(bytes([int(t,16)])) for t in pattern.split())
    matches=list(re.finditer(expression,raw,re.DOTALL));assert len(matches)==1,(pattern,len(matches))
    return base+pe.get_rva_from_offset(matches[0].start())
g=lua.globals()
g.utils=lua.table_from({'itob':itob})
g.core=lua.table_from({'writeCode':write,'AOBScan':scan,'allocateCode':allocate,'allocate':allocate,
    'calculateCodeSize':size,'readInteger':get,'readByte':lambda address:uc.mem_read(address,1)[0],
    'getRelativeAddress':lambda address,target,offset:target-address+offset})
lua.execute('core.relTo=function(target,offset)return function(address)return utils.itob(target-address+offset)end end')
port=lua.execute(a.legacy.read_text());port.init(port,lua.table());port.enable(port,lua.table())
original={site:bytes(uc.mem_read(site,6 if site==0x579A7C else 5)) for site in [0x579879,0x579A7C]}
wall_hook=0x4D3E74+get(0x4D3E70);wall_counts=get(wall_hook+7)
assembly_count=0
def assemble(source,symbols):
    global assembly_count
    address=allocate(0x100);assembly_count+=1
    asm=a.output/f'census-{assembly_count}.asm';binary=asm.with_suffix('.bin')
    asm.write_text(f'use32\norg {address}\n'+''.join(f'{k} equ {int(v)}\n' for k,v in symbols.items())+source)
    subprocess.run([str(a.fasm.resolve()),str(asm.resolve()),str(binary.resolve())],check=True)
    code=binary.read_bytes();assert len(code)<=0x100
    uc.mem_write(address,code);return address
g.core.allocateAssembly=assemble
g.root=Path(__file__).resolve().parents[1].as_posix()
lua.execute('''
package.path=root..'/?.lua;'..package.path
package.loaded['native-bindings']={initialize=function()end} -- defense bridge only
package.loaded['config.grace']={preflight=function()end}
configFinal={}
package.loaded['aicTactics.dll']={configurationSize=344,configuration=0x3050000,
  resetDefenseCensus=0x3070000,countDefenseUnit=0x3070020}
native=require('native').new()
native.activateComposition();native.activateComposition()
native.preflightComposition()
''')
assert assembly_count==2
wrapped={site:bytes(uc.mem_read(site,len(data))) for site,data in original.items()}
registers=[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EFLAGS]
calls=[]
def spy(uc,address,size,user):
    if address not in [0x3070000,0x3070020]:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    calls.append(('reset',) if address==0x3070000 else ('count',get(sp+4),get(sp+8)))
    for reg in registers[:7]:uc.reg_write(reg,0xDEADBEEF)
    uc.reg_write(UC_X86_REG_EFLAGS,0x202)
    uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF);uc.reg_write(UC_X86_REG_ESP,sp+4)
uc.hook_add(UC_HOOK_CODE,spy)
cases=0
for site,end in [(0x579879,0x57987E),(0x579A7C,0x579A82)]:
 for player in range(1,9):
  for kind in [22,24,30,70,76]:
   for role in [1,4]:
    for flags in [0x202,0x247,0xA92]:
     results=[]
     for patch in [original,wrapped]:
      uc.mem_write(site,patch[site]);uc.ctl_remove_cache(site,site+6);calls.clear()
      initial=[0x12345678,0x98765432,0xCC001101,0,0x1387F38,player,3,0x308F000,flags]
      frame=b'unchanged caller frame'
      uc.mem_write(initial[7],frame)
      uc.mem_write(0x1388976+3*0x490,struct.pack('<h',role))
      uc.mem_write(0x13885DA+3*0x490,struct.pack('<h',kind))
      for owner in range(9):put(wall_counts+owner*4,7)
      for reg,value in zip(registers,initial):uc.reg_write(reg,value)
      uc.emu_start(site,end,count=300)
      results.append(([uc.reg_read(reg) for reg in registers],bytes(uc.mem_read(wall_counts,36))))
      assert bytes(uc.mem_read(initial[7],len(frame)))==frame
      assert calls==([] if patch is original else [('reset',)] if site==0x579879 else [('count',player,kind)]),(hex(site),patch is original,player,kind,calls,hex(uc.reg_read(UC_X86_REG_EIP)))
     assert results[0]==results[1],(site,player,kind,role,flags,results)
     cases+=1
result={'cases':cases,'source':'actual native.lua wrappers and unchanged Legacy ai_defense.lua',
    'legacySHA256':hashlib.sha256(a.legacy.read_bytes()).hexdigest(),
    'scope':'Original census effects, registers, flags, stack and added callback arguments; C++ calls replaced by ABI spies'}
(a.output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))

if a.combat:
    # Reuse the same reference image, actual Lua assembler and register oracle.
    names=['resetCombatCensus','countCombatUnit','completeCombatCensus','commitOpponent',
        'preserveRandomWaveRequirement','isReserveUnit','observedUnitDamage','observedEntityDamage',
        'observedFireDamage','updateOffensiveArmy','selectOpponent','updateOffensiveRaids',
        'returnFromAttack','recruitWithReserve','reserveRecruitType','resetRaidBuildingCensus',
        'countRaidBuilding','completeRaidBuildingCensus']
    native=g.native
    functions={name:0x3070100+i*32 for i,name in enumerate(names)}
    for name,address in functions.items(): native[name]=address
    for index,name in enumerate(['originalUnitDamage','originalEntityDamage','originalFireDamage','legacyTargetPolicy']):
        native[name]=0x306F000+index*4
    g.core.writeInteger=put
    sites=[(0x5798EF,7,'resetCombatCensus',None),(0x579940,8,'countCombatUnit',UC_X86_REG_EBP),
        (0x579DDC,5,'completeCombatCensus',None),(0x422EE2,5,'resetRaidBuildingCensus',None),
        (0x422F56,8,'countRaidBuilding',UC_X86_REG_EDX),(0x42331B,8,'completeRaidBuildingCensus',None),
        (0x4D4A62,6,'commitOpponent',UC_X86_REG_ESI),(0x4CDD47,6,'preserveRandomWaveRequirement',UC_X86_REG_ESI),
        (0x4D40F2,7,'isReserveUnit',UC_X86_REG_EDI)]
    original_combat={site:bytes(uc.mem_read(site,length)) for site,length,_,_ in sites}
    native.activateCombat(); native.activateRaids(); native.activateCombat(); native.activateRaids()
    wrapped_combat={site:bytes(uc.mem_read(site,length)) for site,length,_,_ in sites}
    returns={'commitOpponent':1,'preserveRandomWaveRequirement':0,'isReserveUnit':0}
    combat_calls=[]
    inverse={address:name for name,address in functions.items()}
    def combat_spy(machine,address,size,user):
        if address not in inverse:return
        name=inverse[address];sp=machine.reg_read(UC_X86_REG_ESP)
        args=(get(sp+4),) if name in ['countCombatUnit','countRaidBuilding','commitOpponent',
            'preserveRandomWaveRequirement','isReserveUnit'] else ()
        combat_calls.append((name,)+args)
        for register in registers[:7]:machine.reg_write(register,0xDEADBEEF)
        machine.reg_write(UC_X86_REG_EAX,returns.get(name,0))
        machine.reg_write(UC_X86_REG_EFLAGS,0x202)
        machine.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF);machine.reg_write(UC_X86_REG_ESP,sp+4)
    uc.hook_add(UC_HOOK_CODE,combat_spy)
    combat_cases=0
    for site,length,name,arg in sites:
        for player in range(1,9):
            for flags in [0x202,0x247,0xA92]:
                initial=[0x1387F38,0x98765432,0xCC001101,player,0x1387F38,player*0x39F4,3,0x308F000,flags]
                if name=='commitOpponent':initial[4]=player
                if name=='preserveRandomWaveRequirement':initial[4]=player*0x39F4
                locations=[0xEE0FC8,initial[4]+4,initial[5]+0x115F768,initial[4]+0x115F698]
                locations=[address for address in locations if 0x400000<=address<0x2491000]
                results=[]
                for patch in [original_combat,wrapped_combat]:
                    uc.mem_write(site,patch[site]);uc.ctl_remove_cache(site,site+length)
                    combat_calls.clear()
                    for address in locations:put(address,123)
                    uc.mem_write(initial[7],b'caller frame')
                    for register,value in zip(registers,initial):uc.reg_write(register,value)
                    uc.emu_start(site,site+length,count=200)
                    results.append(([uc.reg_read(register) for register in registers],[get(address) for address in locations]))
                    assert bytes(uc.mem_read(initial[7],12))==b'caller frame'
                    expected=[] if patch is original_combat else [(name,)+((initial[registers.index(arg)],) if arg else ())]
                    assert combat_calls==expected,(name,combat_calls,expected)
                assert results[0]==results[1],(hex(site),player,flags,results)
                combat_cases+=1
                skips={'commitOpponent':(0,0x4D4AB5),'preserveRandomWaveRequirement':(1,site+length),
                    'isReserveUnit':(1,0x4D4117)}
                if name in skips:
                    previous=returns[name];returns[name],end=skips[name]
                    uc.mem_write(site,wrapped_combat[site]);uc.ctl_remove_cache(site,site+length)
                    for address in locations:put(address,123)
                    for register,value in zip(registers,initial):uc.reg_write(register,value)
                    combat_calls.clear();uc.emu_start(site,end,count=200)
                    assert [uc.reg_read(register) for register in registers]==initial,(name,'skip registers')
                    assert all(get(address)==123 for address in locations),(name,'skip wrote displaced store')
                    assert combat_calls==[(name,initial[registers.index(arg)])]
                    returns[name]=previous;combat_cases+=1
    # Damage wrappers call these original prologues through relocated trampolines.
    for site,length,original_name in [(0x531220,6,'originalUnitDamage'),
        (0x531920,7,'originalEntityDamage'),(0x532460,5,'originalFireDamage')]:
        displaced=pe.get_data(site-base,length)
        trampoline=get(native[original_name])
        patched=bytes(uc.mem_read(site,length))
        for flags in [0x202,0x247,0xA92]:
            results=[]
            for entry in [site,trampoline]:
                uc.mem_write(site,displaced);uc.ctl_remove_cache(site,site+length)
                initial=[0x12345678,0x98765432,0x1387F38,3,0x11223344,0x55667788,0xABCD,0x308F000,flags]
                uc.mem_write(initial[7]-64,bytes(range(128)))
                for register,value in zip(registers,initial):uc.reg_write(register,value)
                uc.emu_start(entry,site+length,count=30)
                results.append(([uc.reg_read(register) for register in registers],bytes(uc.mem_read(initial[7]-64,128))))
            assert results[0]==results[1],(original_name,flags)
            combat_cases+=1
        uc.mem_write(site,patched);uc.ctl_remove_cache(site,site+length)
    result={'cases':combat_cases,'source':'actual combat-native.lua wrappers',
        'scope':'Original displaced effects, callback arguments, registers, flags and stack; C++ ABI spies'}
    (a.output/'combat-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
