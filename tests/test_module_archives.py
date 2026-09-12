import io
import zipfile

import pytest

from module_archives import package_modules


def test_module_zip_has_root_manifest_and_preserves_binary_and_locale_bytes():
    members = {'definition.yml': b'name: aic-tactics\nversion: 0.0.1\n',
               'init.lua': b'return {}', 'aicTactics.dll': b'MZ\x00\xff',
               'locale/description-de.md': 'Ziele w\u00e4hlen'.encode()}
    source = {'ucp/modules/aic-tactics-0.0.1/' + n: d for n, d in members.items()}
    source['README.md'] = b'Keep module ZIPs zipped.'
    bundle = package_modules(source)
    assert set(bundle) == {'README.md', 'ucp/modules/aic-tactics-0.0.1.zip'}
    with zipfile.ZipFile(io.BytesIO(bundle['ucp/modules/aic-tactics-0.0.1.zip'])) as z:
        assert z.testzip() is None
        assert {n: z.read(n) for n in z.namelist()} == members
    assert package_modules(dict(reversed(list(source.items())))) == bundle


def test_dependency_archives_are_separate_and_incomplete_modules_fail():
    source = {}
    for module in ('aic-tactics-0.0.1', 'aicloader-1.1.3'):
        source['ucp/modules/' + module + '/definition.yml'] = module.encode()
        source['ucp/modules/' + module + '/init.lua'] = b'return {}'
    assert len(package_modules(source)) == 2
    del source['ucp/modules/aicloader-1.1.3/init.lua']
    with pytest.raises(ValueError, match='aicloader-1.1.3'):
        package_modules(source)
    with pytest.raises(ValueError, match='No modules'):
        package_modules({'README.md': b'help'})
