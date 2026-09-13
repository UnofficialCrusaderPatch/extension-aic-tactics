import hashlib
from pathlib import Path

from lupa.lua54 import LuaRuntime

from module_payload import module_payload


def test_packaged_identity_reads_exact_payload_including_locales_and_dll(tmp_path):
    root = Path(__file__).resolve().parents[1]
    dll = tmp_path / 'runtime.dll'
    dll.write_bytes(b'MZ\0binary\xff')
    module, files = module_payload(root, dll)
    assert len([name for name in files if name.startswith('locale/description-')]) == 9
    assert files['aicTactics.dll'] == dll.read_bytes()
    lua = LuaRuntime(encoding=None)
    lua.globals()[b'payload'] = lua.table_from({name.encode(): data for name, data in files.items()})
    lua.globals()[b'prefix'] = ('ucp/modules/' + module + '/').encode()
    lua.globals()[b'digest'] = lambda data: hashlib.sha256(data).hexdigest().encode()
    lua.execute(b'''
      io.open=function(path, mode)
        assert(mode=='rb' and path:sub(1,#prefix)==prefix)
        local data=assert(payload[path:sub(#prefix+1)])
        return {read=function()return data end,close=function()return true end}
      end
      sha={sha256=function(data)return digest(data)end}
      local verify=assert(load(payload['package-identity.lua']))().verify
      local identity=assert(load(payload['build-identity.lua']))()
      assert(verify(identity)==identity.sha256)
      payload['aicTactics.dll']='changed runtime'
      assert(not pcall(verify,identity))
    ''')
