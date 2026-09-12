from pathlib import Path
import pytest
from lupa import LuaRuntime

ROOT = Path(__file__).resolve().parents[1]


def runtime():
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      configFinal={['ucp2-legacy-2.15.1']={ai_recruitstate_initialtimer={enabled=false}}}
      -- Deliberately moved instruction/data addresses: this is a Lua contract
      -- fixture. Real SHC/Extreme contexts are checked separately.
      site=0x18000000
      memory={}
      writes=0
      core={readInteger=function(a)return memory[a] end,readByte=function(a)return memory[a] end,
        AOBScan=function() return site end,scanForAOB=function() return nil end,
        writeInteger=function(a,v)assert(a==site+36);memory[a]=v;writes=writes+1 end}
      for i=0,49 do memory[site+i]=i end
      memory[site+9]=0x19000002;memory[site+17]=0x19000000
      memory[site+26]=0x200;memory[site+32]=0x19000100;memory[site+36]=4800
      grace=require('config.grace')
      grace.resolveNative()
    ''')
    return lua


def test_native_default_and_explicit_legacy_migration():
    runtime().execute('''
      grace.configure(6);assert(writes==0)
      for months=0,30 do
        grace.configure(months);grace.preflight()
        assert(memory[site+36]==months*800)
      end
      memory[site+36]=1
      assert(not pcall(grace.preflight))
    ''')


@pytest.mark.parametrize('value', ['-1', '31', '0.1', "'6'", 'false', '0/0'])
def test_bad_migration_value_writes_nothing(value):
    runtime().execute(f'assert(not pcall(grace.configure,{value}) and writes==0)')


def test_legacy_enabled_even_at_original_value_rejected():
    runtime().execute('''
      configFinal['ucp2-legacy-2.15.1'].ai_recruitstate_initialtimer.enabled=true
      assert(not pcall(grace.configure,6) and writes==0)
      configFinal=nil
      assert(not pcall(grace.preflight) and writes==0)
    ''')


def test_foreign_code_rejected_before_migration():
    runtime().execute('''
      for address,value in pairs(memory) do
        if address<site+37 or address>=site+40 then
          memory[address]=value+1
          assert(not pcall(grace.configure,0) and writes==0)
          memory[address]=value
        end
      end
    ''')
