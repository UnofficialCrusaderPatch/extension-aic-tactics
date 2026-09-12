from pathlib import Path

import pytest
from lupa import LuaRuntime

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize('invalid', ['abi','owner','version','count','stride','address'])
def test_binding_contract_failure_does_not_write_native_memory(invalid):
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.globals().invalid = invalid
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      local layout={version=1,address=10000,characters=16,stride=676}
      local native={nativeBindingsSize=96,nativeBindings=20000}
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
      assert(not pcall(require('native-bindings').initialize,native))
      assert(writes==0 and native.game==nil)
    ''')


@pytest.mark.parametrize('invalid', ['absent','zero','duplicate','rng'])
def test_native_context_failure_has_no_pointer_fallback(invalid):
    lua = LuaRuntime()
    lua.globals().root = ROOT.as_posix()
    lua.globals().invalid = invalid
    lua.execute('''
      package.path=root..'/?.lua;'..package.path
      core={AOBScan=function()
          if invalid=='absent' then return nil end
          return invalid=='zero' and 0 or 10000
        end,
        scanForAOB=function()return invalid=='duplicate' and 20000 or nil end,
        readInteger=function()return 10000 end}
      local ok,why=pcall(require('config.grace').resolveNative)
      assert(not ok and why:find('AIC Tactics:',1,true))
    ''')
