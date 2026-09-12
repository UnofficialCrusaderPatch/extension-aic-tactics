"""Compare actual FASM census wrappers with the unchanged Legacy Lua port.

The added C++ calls are ABI spies here; their logic has native test-host coverage.
This is hook/register/flags evidence, not gameplay or save/load acceptance.
"""
import argparse
import hashlib
from executable_fixtures import digest as fixture_digest
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
p.add_argument('--loader',type=Path,required=True)
p.add_argument('--fasm',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--combat',action='store_true')
p.add_argument('--variant',choices=['SHC','SHCE'],default='SHC')
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
raw=a.reference.read_bytes()
reference_digest=fixture_digest(raw,a.variant)

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
def allocate(count, *_):
    global allocation
    answer=allocation;allocation+=(count+15)&~15
    assert allocation<0x3060000
    return answer
scan_count=0
scan_log=[]
def scan(pattern,start=None):
    global scan_count
    scan_count+=1
    expression=b''.join(b'.' if t=='?' else re.escape(bytes([int(t,16)])) for t in pattern.split())
    # Read current executable sections so discovery sees Legacy's installed patches.
    for section in pe.sections:
        address=base+section.VirtualAddress
        count=len(section.get_data())
        low=max(address,start or base)
        if low>=address+count:continue
        match=re.search(expression,bytes(uc.mem_read(low,address+count-low)),re.DOTALL)
        if match:
            scan_log.append((pattern,start,low+match.start()))
            return low+match.start()
    return None
g=lua.globals()
g.utils=lua.table_from({'itob':itob})
g.core=lua.table_from({'writeCode':write,'AOBScan':scan,'scanForAOB':scan,'allocateCode':allocate,'allocate':allocate,
    'calculateCodeSize':size,'readInteger':get,'readByte':lambda address:uc.mem_read(address,1)[0],
    'getRelativeAddress':lambda address,target,offset:target-address+offset})
lua.execute('core.relTo=function(target,offset)return function(address)return utils.itob(target-address+offset)end end')
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
recruitment_scan_range=[]
g.markRecruitment=lambda:recruitment_scan_range.append(len(scan_log))
g.owner=lua.execute((a.loader/'addresses.lua').read_text())
g.core.writeInteger=put
lua.execute('''
package.path=root..'/?.lua;'..package.path
modules={aicloader={getNativeAICLayout=function()
  return {version=1,address=owner.getAIStartAddress(1),characters=16,stride=676}
end}}
local recruitment=require('native-recruitment')
local resolveRecruitment=recruitment.resolve
recruitment.resolve=function(game)
  markRecruitment()
  local result=resolveRecruitment(game)
  markRecruitment()
  return result
end
local bindings=require('native-bindings')
local resolve=bindings.resolve
bindings.resolve=function()
  assert(preparedGame==nil, 'module load must resolve once')
  preparedGame=resolve()
  return preparedGame
end
configFinal={}
''')
# Execute the actual module entry point during load, before any Legacy enable.
before_allocation=allocation
before_storage=bytes(uc.mem_read(0x3051000,256))
before_contexts=len(scan_log)
module=lua.execute((Path(g.root)/'init.lua').read_text())
assert g.preparedGame is not None and allocation==before_allocation
assert bytes(uc.mem_read(0x3051000,256))==before_storage
assert lua.eval("package.loaded['aicTactics.dll']==nil")
for pattern,start,address in list(scan_log[before_contexts:]):
    assert start is None
    assert scan(pattern,address+1) is None, ('duplicate fixture context',pattern)
target_context=next(pattern for pattern,start,address in scan_log
                    if start is None and address==g.preparedGame.selectAttackTarget)
persistent_path=a.legacy.parent.parent/'persistent-state.lua'
persistent=lua.execute(persistent_path.read_text()).new() if persistent_path.exists() else None
port=lua.execute(a.legacy.read_text());port.init(port,lua.table());port.enable(port,lua.table(),persistent)
assault_path=a.legacy.with_name('ai_assaultswitch.lua')
assault=lua.execute(assault_path.read_text())
assault.init(assault,lua.table());assault.enable(assault,lua.table())
assert scan(target_context) is None, 'Legacy must reproduce the former late-resolution failure'
reset_site=port.ai_defense_reset_edit;count_site=port.ai_defense_count_edit
original={reset_site:bytes(uc.mem_read(reset_site,5)),count_site:bytes(uc.mem_read(count_site,6))}
wall_hook=port.ai_defense_check_edit+5+get(port.ai_defense_check_edit+1);wall_counts=get(wall_hook+7)
before_enable_scans=scan_count
lua.execute('''
package.loaded['aicTactics.dll']={configurationSize=344,configuration=0x3050000,
  nativeBindings=0x3051000,nativeBindingsSize=256,
  resetDefenseCensus=0x3070000,countDefenseUnit=0x3070020}
native=require('native').new(preparedGame)
native.preflightCombat();native.preflightTargets();native.preflightRaids()
''')
assert scan_count==before_enable_scans, 'Enable must consume load-phase bindings without rescanning'
assert get(0x3051000+54*4)==g.preparedGame.selectAttackTarget
# Exercise every production context against absence. Captured
# addresses only avoid repeating private-fixture scans for negative cases.
recruitment_contexts=[item for item in scan_log[recruitment_scan_range[0]:recruitment_scan_range[1]] if item[1] is None]
assert len(recruitment_contexts)==11
context_sites={pattern:address for pattern,_,address in recruitment_contexts}
resolver=lua.eval("require('native-recruitment')")
negative=0
for pattern,_,address in recruitment_contexts:
    g.core.AOBScan=lambda pat: None if pat==pattern else context_sites[pat]
    g.core.scanForAOB=lambda *args: None
    try:resolver.resolve(g.native.game)
    except Exception as error:assert 'AIC Tactics:' in str(error)
    else:raise AssertionError(('absent',pattern))
    negative+=1
g.core.AOBScan=context_sites.__getitem__;g.core.scanForAOB=lambda *_:None
# Decoded caller identity, roots, structure strides, cleanup branches and ABI.
for index,offset in [(1,14),(1,19),(1,30),(1,46),(2,43),(3,18),(3,181),
                     (3,269),(4,175),(5,20),(6,17),(7,15),(7,51),(8,12),(10,2)]:
    target=recruitment_contexts[index][2]+offset
    g.core.readInteger=lambda address: 0 if address==target else get(address)
    try:resolver.resolve(g.native.game)
    except Exception as error:assert 'AIC Tactics:' in str(error)
    else:raise AssertionError(('operand',index,offset))
    negative+=1
g.core.readInteger=get;g.core.AOBScan=scan;g.core.scanForAOB=scan
# Validate all bytes and operands of the unchanged owner's three trampolines.
legacy_negative=0
for site,length in [(wall_hook,16),(reset_site+5+get(reset_site+1),25),(count_site+5+get(count_site+1),40)]:
    for offset in range(length):
        old=bytes(uc.mem_read(site+offset,1));uc.mem_write(site+offset,bytes([old[0]^1]))
        try:resolver.legacyCounter(g.native.game)
        except Exception as error:assert 'AIC Tactics:' in str(error)
        else:raise AssertionError(('legacy byte',hex(site),offset))
        finally:uc.mem_write(site+offset,old)
        legacy_negative+=1
scans_before=scan_count
lua.execute('native.activateComposition();native.activateComposition();native.preflightComposition()')
assert scan_count==scans_before, 'Repeated preflight must not scan'
assert assembly_count==2
wrapped={site:bytes(uc.mem_read(site,len(data))) for site,data in original.items()}
registers=[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EFLAGS]
calls=[]

# Retain the real Legacy target commitment patch inside the captured function.
# Its early return restores the selector frame; noncommitted candidates replay
# the displaced loads and resume. No AIC replacement of this owner is needed.
assault_cases=0
order_root=get(assault.ai_recruitinterval_edit+0xD9+2)
target_root=get(assault.ai_assaultswitch_edit+0x15E)
for player in range(1,9):
    for order in range(8):
        for matches in (False,True):
            sp=0x308F000
            frame=struct.pack('<16I',*range(100,116))
            uc.mem_write(sp,frame);put(sp+48,0x307F000)
            initial=[0x12345678,player*0x39F4,0xCC001101,0,0x10203040,4,3,sp,0x202]
            put(order_root+initial[1],order);put(target_root+initial[1],4 if matches else 5)
            for reg,value in zip(registers,initial):uc.reg_write(reg,value)
            committed=order>=3 and matches
            end=0x307F000 if committed else assault.ai_assaultswitch_edit+8
            uc.emu_start(assault.ai_assaultswitch_edit,end,count=100)
            expected=initial.copy()
            if committed:
                expected[5],expected[4],expected[6],expected[1]=100,101,102,103
                expected[7]=sp+56
            else:
                expected[0],expected[2]=107,108
            # Legacy's comparisons intentionally change flags.
            assert [uc.reg_read(reg) for reg in registers[:-1]]==expected[:-1]
            assert uc.reg_read(UC_X86_REG_EIP)==end
            assault_cases+=1
def spy(uc,address,size,user):
    if address not in [0x3070000,0x3070020]:return
    sp=uc.reg_read(UC_X86_REG_ESP)
    calls.append(('reset',) if address==0x3070000 else ('count',get(sp+4),get(sp+8)))
    for reg in registers[:7]:uc.reg_write(reg,0xDEADBEEF)
    uc.reg_write(UC_X86_REG_EFLAGS,0x202)
    uc.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF);uc.reg_write(UC_X86_REG_ESP,sp+4)
uc.hook_add(UC_HOOK_CODE,spy)
cases=0
for site,end in [(reset_site,reset_site+5),(count_site,count_site+6)]:
 for player in range(1,9):
  for kind in [22,24,30,70,76]:
   for role in [1,4]:
    for flags in [0x202,0x247,0xA92]:
     results=[]
     for patch in [original,wrapped]:
      uc.mem_write(site,patch[site]);uc.ctl_remove_cache(site,site+6);calls.clear()
      initial=[0x12345678,0x98765432,0xCC001101,0,g.native.game.units,player,3,0x308F000,flags]
      frame=b'unchanged caller frame'
      uc.mem_write(initial[7],frame)
      uc.mem_write(g.native.game.unitRecords+0x42A+3*0x490,struct.pack('<h',role))
      uc.mem_write(g.native.game.unitRecords+0x8E+3*0x490,struct.pack('<h',kind))
      for owner in range(9):put(wall_counts+owner*4,7)
      for reg,value in zip(registers,initial):uc.reg_write(reg,value)
      uc.emu_start(site,end,count=300)
      results.append(([uc.reg_read(reg) for reg in registers],bytes(uc.mem_read(wall_counts,36))))
      assert bytes(uc.mem_read(initial[7],len(frame)))==frame
      assert calls==([] if patch is original else [('reset',)] if site==reset_site else [('count',player,kind)]),(hex(site),patch is original,player,kind,calls,hex(uc.reg_read(UC_X86_REG_EIP)))
     assert results[0]==results[1],(site,player,kind,role,flags,results)
     cases+=1
# Exercise native thiscall owners on both actual executables, without rendering.
game=g.native.game
native_cases=0
for player in range(1,9):
    for fn in (game.rangedSortieNative,game.meleeSortieNative):
        put(game.players+player*0x39F4+0x2300,0)
        sp=0x308F000;put(sp,0x307F000);put(sp+4,player)
        initial=[0x12345678,0x98765432,0x3050000,0,0x10203040,0x50607080,0x12344321,sp,0x202]
        for reg,value in zip(registers,initial):uc.reg_write(reg,value)
        uc.emu_start(fn,0x307F000,count=100)
        assert uc.reg_read(UC_X86_REG_ESP)==sp+8
        for i in (1,4,5,6):assert uc.reg_read(registers[i])==initial[i]
        native_cases+=1
    # Three queue records: available, already occupied, owned by another player.
    for index,owner,tile,flags in [(0,player,1,0),(1,player,2,0x40000000),(15998,player%8+1,3,0)]:
        record=game.moat+0x50088C+index*16
        uc.mem_write(record,bytes([owner]));put(record-12,tile)
        put(game.moat+0x165160+tile*4,flags)
    sp=0x308F000;put(sp,0x307F000);put(sp+4,player)
    uc.reg_write(UC_X86_REG_ESP,sp);uc.reg_write(UC_X86_REG_ECX,game.moat)
    uc.emu_start(game.moatVacancies,0x307F000,count=300000)
    assert uc.reg_read(UC_X86_REG_EAX)==1 and uc.reg_read(UC_X86_REG_ESP)==sp+8
    native_cases+=1

# Install the actual recruitment bridge after the unchanged Legacy census.
native=g.native
for name,address in {'legacyWallCounts':0x306F100,'recruitOpportunity':0x3070800,
                     'rangedSortie':0x3070840,'meleeSortie':0x3070860}.items():native[name]=address
g.core.writeInteger=put
native.activate();native.activate()
assert get(native.legacyWallCounts)==wall_counts
assert scan_count==scans_before
opportunity=game.recruitmentSites.opportunity
handled=0;opportunity_calls=[]
def opportunity_spy(machine,address,size,user):
    if address!=native.recruitOpportunity:return
    sp=machine.reg_read(UC_X86_REG_ESP)
    opportunity_calls.append((get(sp+4),get(sp+8),get(sp+12)))
    for register in registers[:7]:machine.reg_write(register,0xDEADBEEF)
    machine.reg_write(UC_X86_REG_EAX,handled)
    machine.reg_write(UC_X86_REG_EFLAGS,0x202)
    machine.reg_write(UC_X86_REG_EIP,get(sp)&0xFFFFFFFF);machine.reg_write(UC_X86_REG_ESP,sp+4)
uc.hook_add(UC_HOOK_CODE,opportunity_spy)
opportunity_cases=0
for player in range(1,9):
    for flags in (0x202,0x247,0xA92):
        for handled in (0,1):
            initial=[0x12345678,0x98765432,0xCC001101,0x3050000,player*0x39F4,5,6,0x308F000,flags]
            frame=bytes(range(64));uc.mem_write(initial[7],frame)
            put(initial[7]+40,player);put(initial[7]+28,2)
            frame=bytes(uc.mem_read(initial[7],64))
            put(game.players+player*0x39F4+0x30F8,2)
            for register,value in zip(registers,initial):uc.reg_write(register,value)
            opportunity_calls.clear()
            end=game.recruitmentSites.finished if handled else opportunity+6
            uc.emu_start(opportunity,end,count=100)
            expected=initial.copy()
            if not handled:expected[0]=2
            assert [uc.reg_read(reg) for reg in registers]==expected
            assert bytes(uc.mem_read(initial[7],64))==frame
            assert opportunity_calls==[(initial[3],player,2)]
            opportunity_cases+=1

result={'variant':a.variant,'cases':cases,'contexts':11,'negativeResolutionCases':negative,
    'loadBeforeLegacyEnable':True,'enableScans':0,
    'legacyAssaultSwitchSHA256':hashlib.sha256(assault_path.read_bytes()).hexdigest(),
    'legacyAssaultSwitchCases':assault_cases,
    'negativeLegacyTrampolineCases':legacy_negative,'repeatPreflightScans':0,
    'nativeSortieAndMoatCases':native_cases,'opportunityBridgeCases':opportunity_cases,'source':'actual native.lua wrappers and unchanged Legacy ai_defense.lua',
    'legacySHA256':hashlib.sha256(a.legacy.read_bytes()).hexdigest(),
    'scope':'Original census effects, registers, flags, stack and added callback arguments; C++ calls replaced by ABI spies'}
(a.output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))

if a.combat:
    # Every occupied patch byte and redirected scheduler call must be rejected
    # before allocating/writing a combat hook.
    hooks=g.native.game.combatSites
    occupied_cases=0
    for name,pattern in g.native.game.combatOriginals.items():
        preflight=g.native.preflightTargets if name=='targetChoice' else g.native.preflightRaids if name.startswith('building') else g.native.preflightCombat
        for offset in range(len(pattern.split())):
            address=hooks[name]+offset;old=bytes(uc.mem_read(address,1))
            uc.mem_write(address,bytes([old[0]^1]))
            before=assembly_count
            try:preflight()
            except Exception as error:assert 'AIC Tactics:' in str(error)
            else:raise AssertionError(('occupied',name,offset))
            finally:uc.mem_write(address,old)
            assert assembly_count==before
            occupied_cases+=1
    for name,target in g.native.game.combatCalls.items():
        address=hooks[name]+1;old=get(address);put(address,old+1)
        try:g.native.preflightCombat()
        except Exception as error:assert 'AIC Tactics:' in str(error)
        else:raise AssertionError(('redirected',name))
        finally:put(address,old)
        occupied_cases+=1
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
    hooks=native.game.combatSites
    sites=[(hooks.unitReset,7,'resetCombatCensus',None),(hooks.unitCount,8,'countCombatUnit',UC_X86_REG_EBP),
        (hooks.unitComplete,5,'completeCombatCensus',None),(hooks.buildingReset,5,'resetRaidBuildingCensus',None),
        (hooks.buildingCount,8,'countRaidBuilding',UC_X86_REG_EDX),(hooks.buildingComplete,8,'completeRaidBuildingCensus',None),
        (hooks.launch,6,'commitOpponent',UC_X86_REG_ESI),(hooks.randomWave,6,'preserveRandomWaveRequirement',UC_X86_REG_ESI),
        (hooks.tunnelers,7,'isReserveUnit',UC_X86_REG_EDI)]
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
                initial=[game.units,0x98765432,0xCC001101,player,game.units,player*0x39F4,3,0x308F000,flags]
                if name in ['resetRaidBuildingCensus','countRaidBuilding','completeRaidBuildingCensus']:
                    initial[0]=game.buildings;initial[4]=game.buildings
                if name=='commitOpponent':initial[4]=player
                if name=='preserveRandomWaveRequirement':initial[4]=player*0x39F4
                locations=[hooks.censusID,initial[4]+4,initial[5]+game.players+0x3970,initial[4]+game.players+0x38A0]
                locations=[address for address in locations if base<=address<base+pe.OPTIONAL_HEADER.SizeOfImage]
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
                skips={'commitOpponent':(0,hooks.launchWait),'preserveRandomWaveRequirement':(1,site+length),
                    'isReserveUnit':(1,hooks.nextTunneler)}
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
    for site,length,original_name in [(hooks.unitDamage,6,'originalUnitDamage'),
        (hooks.entityDamage,7,'originalEntityDamage'),(hooks.fireDamage,5,'originalFireDamage')]:
        displaced=pe.get_data(site-base,length)
        trampoline=get(native[original_name])
        patched=bytes(uc.mem_read(site,length))
        for flags in [0x202,0x247,0xA92]:
            results=[]
            for entry in [site,trampoline]:
                uc.mem_write(site,displaced);uc.ctl_remove_cache(site,site+length)
                initial=[0x12345678,0x98765432,game.units,3,0x11223344,0x55667788,0xABCD,0x308F000,flags]
                uc.mem_write(initial[7]-64,bytes(range(128)))
                for register,value in zip(registers,initial):uc.reg_write(register,value)
                uc.emu_start(entry,site+length,count=30)
                results.append(([uc.reg_read(register) for register in registers],bytes(uc.mem_read(initial[7]-64,128))))
            assert results[0]==results[1],(original_name,flags)
            combat_cases+=1
        uc.mem_write(site,patched);uc.ctl_remove_cache(site,site+length)
    result={'variant':a.variant,'cases':combat_cases,'occupiedHookCases':occupied_cases,
        'source':'actual combat-native.lua wrappers',
        'scope':'Original displaced effects, callback arguments, registers, flags and stack; C++ ABI spies'}
    (a.output/'combat-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))

    # Compare explicit replacement policies to the real, unchanged Legacy port
    # through the native selection loop's comparison and stack updates.
    target_port=lua.execute(a.legacy.with_name('ai_attacktarget.lua').read_text())
    choice_site=hooks.targetChoice
    original_choice=pe.get_data(choice_site-base,5)
    policy_cases=0
    for choice in ('Nearest','Richest','Weakest'):
        uc.mem_write(choice_site,original_choice)
        target_port.init(target_port,lua.table_from({'choice':choice.lower()}))
        target_port.enable(target_port,lua.table())
        legacy_patch=bytes(uc.mem_read(choice_site,5))
        uc.mem_write(choice_site,original_choice)
        candidate=lua.table_from({'game':game,'legacyTargetPolicy':0x306F100})
        lua.eval("require('combat-native').attach")(candidate)
        candidate.enableNativeTargetPolicy(choice)
        replacement=bytes(uc.mem_read(choice_site,5))
        for configured in range(5):
            for flags in (0x202,0x247,0xA92):
                results=[]
                for patch in (legacy_patch,replacement):
                    uc.mem_write(choice_site,patch);uc.ctl_remove_cache(choice_site,choice_site+5)
                    initial=[configured,0x98765432,0x12345678,0x10203040,
                        game.players+0x39F4+0x50C,2,0,0x308F000,flags]
                    frame=bytearray(64)
                    uc.mem_write(initial[7],bytes(frame));put(initial[7]+0x10,10000)
                    put(initial[7]+0x18,1000000)
                    put(initial[4],300)
                    for register,value in zip(registers,initial):uc.reg_write(register,value)
                    uc.emu_start(choice_site,game.selectAttackTarget+0x1C7,count=100)
                    results.append(([uc.reg_read(register) for register in registers],
                        bytes(uc.mem_read(initial[7],64))))
                assert results[0]==results[1],(choice,configured,flags,results)
                policy_cases+=1
        uc.mem_write(choice_site,original_choice);uc.ctl_remove_cache(choice_site,choice_site+5)
    policy_result={'variant':a.variant,'cases':policy_cases,
        'legacySHA256':hashlib.sha256(a.legacy.with_name('ai_attacktarget.lua').read_bytes()).hexdigest(),
        'scope':'Actual Legacy target-policy patch versus actual AIC FASM/native selection loop; no active-game acceptance'}
    (a.output/'target-policy-result.json').write_text(json.dumps(policy_result,indent=2)+'\n')
    print(json.dumps(policy_result))
