"""Execute the actual Lua-generated FASM bridge in an x86 emulator.

Register/flags/stack and interval equivalence only, not game or MP acceptance.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
from lupa import LuaRuntime
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EFLAGS)

p=argparse.ArgumentParser()
p.add_argument('--fasm', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a=p.parse_args()
a.output.mkdir(parents=True,exist_ok=True)
root=Path(__file__).resolve().parents[1]
lua=LuaRuntime(unpack_returned_tuples=True)
lua.globals().root=root.as_posix()
lua.execute('''
package.path=root..'/?.lua;'..package.path
package.loaded['aicTactics.dll']={configurationSize=284,configuration=0x3100000}
core={AOBScan=function()return 0x4D3B41 end,
 allocateAssembly=function(text,symbols) assembly=text;bindings=symbols;return 0x3000000 end,
 writeCode=function(address,code)site=address;patch=code end}
native=require('native').new()
assert(site==nil and assembly==nil)
native.enableLegacyInterval()
assert(site==0x4D3B41 and patch[1]==0xE9 and patch[3]==0x90 and patch[4]==0x90)
''')
g=lua.globals()
source='use32\norg 0x3000000\n'
source+='\n'.join(f'{key} equ {int(value)}' for key,value in g.bindings.items())+'\n'+g.assembly
asm=a.output/'interval-bridge.asm'; binary=a.output/'interval-bridge.bin'
asm.write_text(source)
subprocess.run([str(a.fasm.resolve()),str(asm.resolve()),str(binary.resolve())],check=True)
uc=Uc(UC_ARCH_X86,UC_MODE_32)
for address,size in [(0x4D3000,0x1000),(0x115B000,0x30000),(0x2300000,0x10000),
                     (0x3000000,0x1000),(0x3100000,0x10000),(0x3200000,0x2000)]:
    uc.mem_map(address,size)
uc.mem_write(0x3000000,binary.read_bytes())
registers=[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EFLAGS]
cases=0
for player in range(1,9):
 for character in range(1,17):
  for mode in [0,1]:
   for strength in range(3):
    for interval in [-1,0,1,4,2147483647]:
     for flags in [0x202,0x247,0xA92]:
      uc.mem_write(0x115E0F8+player*0x39F4,struct.pack('<i',character+1))
      uc.mem_write(0x3100000+character*284,struct.pack('<i',mode))
      index=character*169+strength
      uc.mem_write(0x2300000+index*4+0x164,struct.pack('<i',interval))
      initial=[0x12345678,0x98765432,0xCC001101,0x2300000,player*0x39F4,
               0x10293847,index,0x3201000,flags]
      frame=b'unchanged caller stack frame'*2
      uc.mem_write(initial[7],frame)
      for register,value in zip(registers,initial):uc.reg_write(register,value)
      uc.emu_start(0x3000000,0x4D3B48,count=50)
      actual=[uc.reg_read(register) for register in registers]
      expected=initial.copy(); expected[0]=(interval if mode else 1)&0xFFFFFFFF
      assert actual==expected,(player,character,mode,strength,interval,flags,actual,expected)
      assert uc.mem_read(initial[7],len(frame))==frame
      cases+=1
result={'cases':cases,'source':'actual native.lua enableLegacyInterval assembly',
        'scope':'FASM/x86 emulation: interval output, other registers, flags and caller stack',
        'nativeExpected':'Legacy immediate interval 1','weightedExpected':'original AIC interval'}
(a.output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
