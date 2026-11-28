from pathlib import Path
import pytest
from lupa.lua54 import LuaRuntime as Lua54
from lupa.luajit21 import LuaRuntime as LuaJIT

ROOT=Path(__file__).resolve().parents[1]


@pytest.mark.parametrize('runtime',[Lua54,LuaJIT])
def test_framework_scans_once_and_rejects_unverified_sites(runtime):
    lua=runtime();lua.globals().root=ROOT.as_posix()
    lua.execute('''
package.path=root..'/?.lua;'..package.path
local calls=0
core={scanForAOB=function() error('uncached scan used') end,
 AOBScan=function(pattern)
 assert(pattern=='53 56');calls=calls+1;return 0x10000000 end}
local resolver=require('native-context')
assert(resolver.find('fixture','53 56')==0x10000000 and calls==1)
core.AOBScan=function() error('AOB not found: 53 56') end
local ok,reason=pcall(resolver.find,'fixture','53 56')
assert(not ok and tostring(reason):find('AIC Tactics: native fixture not found',1,true))
for _,value in ipairs({0,false,-1,1.5,math.huge}) do
 core.AOBScan=function() return value end
 assert(not pcall(resolver.find,'fixture','53 56'))
end
''')
