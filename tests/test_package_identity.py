from pathlib import Path

import pytest
from lupa.lua54 import LuaRuntime as Lua54
from lupa.luajit21 import LuaRuntime as LuaJit


@pytest.mark.parametrize('runtime', [Lua54, LuaJit])
def test_identity_reads_declared_module_version_and_rejects_changed_content(runtime):
    lua = runtime()
    lua.globals().source = (Path(__file__).resolve().parents[1] / 'package-identity.lua').as_posix()
    lua.execute('''
local verify=dofile(source).verify
local path,changed=nil,false
io.open=function(name,mode)
 path=name; assert(mode=='rb')
 return {read=function()return changed and 'changed' or 'payload' end,close=function()return true end}
end
sha={sha256=function(data)
 assert(data==string.char(8,0,0,0)..'init.lua'..string.char(7,0,0,0,0,0,0,0)..'payload')
 return string.rep('a',64)
end}
local identity={module='aic-tactics-0.0.2',sha256=string.rep('a',64),files={'init.lua'}}
assert(verify(identity)==identity.sha256 and path=='ucp/modules/aic-tactics-0.0.2/init.lua')
changed=true; assert(not pcall(verify,identity))
changed=false; identity.module='../aic-tactics-0.0.2'; assert(not pcall(verify,identity))
identity.module='aic-tactics-0.0.2'; identity.files={'../init.lua'}; assert(not pcall(verify,identity))
''')
