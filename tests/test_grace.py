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
      memory={[0x4D34AB]=0x7DA83D81,[0x4D34AF]=0xFE,[0x4D34B0]=1,
        [0x4D34B1]=4800,[0x4D34B5]=0x7D,[0x4D34B6]=8}
      writes=0
      core={readInteger=function(a)return memory[a] end,readByte=function(a)return memory[a] end,
        writeInteger=function(a,v)assert(a==0x4D34B1);memory[a]=v;writes=writes+1 end}
      grace=require('config.grace')
    ''')
    return lua


def test_native_default_and_explicit_legacy_migration():
    runtime().execute('''
      grace.configure(6);assert(writes==0)
      for months=0,30 do
        grace.configure(months);grace.preflight()
        assert(memory[0x4D34B1]==months*800)
      end
      memory[0x4D34B1]=1
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
        memory[address]=0
        assert(not pcall(grace.configure,0) and writes==0)
        memory[address]=value
      end
    ''')
