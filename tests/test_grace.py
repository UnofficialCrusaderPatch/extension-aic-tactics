from pathlib import Path
from lupa import LuaRuntime

ROOT = Path(__file__).resolve().parents[1]


def runtime():
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      -- Deliberately moved instruction/data addresses: this is a Lua contract
      -- fixture. Real SHC/Extreme contexts are checked separately.
      site=0x18000000
      memory={}
      writes=0
      core={readInteger=function(a)return memory[a] end,readByte=function(a)return memory[a] end,
        AOBScan=function() return site end,scanForAOB=function() return nil end,
        writeCodeInteger=function(a,v)assert(a==site+36);memory[a]=v;writes=writes+1 end}
      for i=0,49 do memory[site+i]=i end
      memory[site+9]=0x19000002;memory[site+17]=0x19000000
      memory[site+26]=0x200;memory[site+32]=0x19000100;memory[site+36]=4800
      grace=require('config.grace')
      grace.resolveNative()
    ''')
    return lua


def test_legacy_timer_is_read_and_never_written():
    runtime().execute('''
      for months=0,30 do
        memory[site+36]=months*800
        grace.preflight()
      end
      assert(writes==0)
    ''')


def test_foreign_code_rejected():
    runtime().execute('''
      for address,value in pairs(memory) do
        if address<site+36 or address>=site+40 then
          memory[address]=value+1
          assert(not pcall(grace.preflight))
          memory[address]=value
        end
      end
    ''')
