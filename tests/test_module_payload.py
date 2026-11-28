import json
from pathlib import Path

from module_payload import module_payload


def test_payload_includes_locales_schema_and_dll(tmp_path):
    root = Path(__file__).resolve().parents[1]
    dll = tmp_path / 'runtime.dll'
    dll.write_bytes(b'MZ\0binary\xff')
    _, files = module_payload(root, dll)
    assert len([name for name in files if name.startswith('locale/description-')]) == 9
    assert len(json.loads(files['schema/aic-tactics.schema.json'])['properties']) == 49
    assert 'siege-tower-crew.lua' in files
    assert files['aicTactics.dll'] == dll.read_bytes()
