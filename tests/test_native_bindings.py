from pathlib import Path

import pytest
from lupa import LuaRuntime

ROOT = Path(__file__).resolve().parents[1]


def test_entry_point_resolves_only_during_enable():
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      local game={}
      local phase='load'
      local resolves=0
      package.loaded['native-bindings']={resolve=function()
        assert(phase=='enable')
        resolves=resolves+1
        return game
      end}
      package.loaded.native={new=function(resolved)
        assert(phase=='enable' and resolved==game and resolves==1)
        error('native enable reached',0)
      end}
      ERROR, WARNING=0,1
      local messages={}
      log=function(level,message) messages[#messages+1]={level,message} end
      local available=false
      modules={aicloader={registerAICUpdateProvider=function()end},
        protocol={multiplayerAdmissionVersion=function()return 1 end},
        ['map-extensions']={requiredStateVersion=function()return 1 end,
          hasNativeSaveInterface=function()return available end,
          getNativeSaveInterface=function()error('the dependent must check availability')end}}
      local module=assert(loadfile(root..'/init.lua'))()
      assert(resolves==0)
      phase='enable'
      module:enable({})
      assert(resolves==0 and #messages==1 and messages[1][2]:find('save admission is unavailable',1,true))
      available=true
      module:enable({patrolSpacing=3, diagnostics='yes'})
      assert(resolves==1 and #messages==4)
      assert(messages[2][1]==ERROR and messages[2][2]:find('diagnostics',1,true))
      assert(messages[3][1]==WARNING and messages[3][2]:find('patrolSpacing',1,true))
      assert(messages[4][2]:find('native enable reached',1,true))
    ''')


@pytest.mark.parametrize('invalid', ['abi','owner','version','count','stride','address'])
def test_binding_contract_failure_does_not_write_native_memory(invalid):
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.globals().invalid = invalid
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      local layout={version=1,address=10000,characters=16,stride=676}
      local native={nativeBindingsSize=368,nativeBindings=20000,allyBindingsSize=56}
      local game={gameTick=30000,rngState=40000,rngValue=40002,rngNext=50000,initialDefenseTicks=60000}
      package.loaded['config.grace']={resolveNative=function()return game end}
      modules={aicloader={getNativeAICLayout=function()return layout end}}
      writes=0;core={writeInteger=function()writes=writes+1 end}
      if invalid=='abi' then native.nativeBindingsSize=0
      elseif invalid=='owner' then modules.aicloader.getNativeAICLayout=nil
      elseif invalid=='version' then layout.version=2
      elseif invalid=='count' then layout.characters=15
      elseif invalid=='stride' then layout.stride=672
      else layout.address=0 end
      local bindings=require('native-bindings')
      local game=bindings.resolve()
      if invalid=='abi' then
        assert(not pcall(bindings.initialize,native,game) and native.game==nil)
      else
        bindings.initialize(native,game)
        local ok,reason=pcall(bindings.requireBindings,game,{'AICLayout'})
        assert(not ok and type(reason)=='string')
      end
      assert(writes==0)
    ''')


@pytest.mark.parametrize('invalid', ['absent','zero','error','rng'])
def test_native_context_failure_has_no_pointer_fallback(invalid):
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.globals().invalid = invalid
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      core={AOBScan=function()
          if invalid=='error' then error('AOB not found') end
          if invalid=='absent' then return nil end
          return invalid=='zero' and 0 or 10000
        end,
        scanForAOB=function()error('unexpected uncached scan') end,
        readInteger=function()return 10000 end}
      local ok,why=pcall(require('config.grace').resolveNative)
      assert(not ok and why:find('AIC Tactics:',1,true))
    ''')
