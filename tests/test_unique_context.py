from pathlib import Path
import pytest
from lupa.lua54 import LuaRuntime as Lua54
from lupa.luajit21 import LuaRuntime as LuaJIT

ROOT=Path(__file__).resolve().parents[1]


@pytest.mark.parametrize('runtime',[Lua54,LuaJIT])
def test_framework_unique_owner_is_used_without_silent_retry(runtime):
    lua=runtime();lua.globals().root=ROOT.as_posix()
    lua.execute('''
package.path=root..'/?.lua;'..package.path
local calls=0
core={AOBScan=function() error('legacy scan used') end,
 scanForAOB=function() error('legacy second scan used') end,
 AOBScanUnique=function(pattern,label)
 assert(pattern=='53 56' and label=='AIC Tactics: fixture');calls=calls+1;return 0x10000000 end}
local resolver=require('native-context')
assert(resolver.find('fixture','53 56')==0x10000000 and calls==1)
for _,failure in ipairs({'missing','ambiguous','inaccessible'}) do
 core.AOBScanUnique=function() error(failure) end
 local ok,reason=pcall(resolver.find,'fixture','53 56')
 assert(not ok and tostring(reason):find(failure,1,true))
end
for _,value in ipairs({0,false,-1}) do
 core.AOBScanUnique=function() return value end
 assert(not pcall(resolver.find,'fixture','53 56'))
end
''')
