"""Deterministic UCP module ZIPs, with module contents at the archive root."""
import io
import re
import zipfile


def archive_bytes(files):
    output = io.BytesIO()
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, (2026, 9, 12, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
    return output.getvalue()


def package_modules(files):
    """Replace staging folders with individual ZIPs; keep bundle documentation."""
    bundle, modules = {}, {}
    for name, data in files.items():
        if not name.startswith('ucp/modules/'):
            bundle[name] = data
            continue
        module, member = name[len('ucp/modules/'):].split('/', 1)
        modules.setdefault(module, {})[member] = data
    if not modules:
        raise ValueError('No modules to package')
    for module, members in sorted(modules.items()):
        if not {'definition.yml', 'init.lua'} <= members.keys():
            raise ValueError('Incomplete module: ' + module)
        definition = members['definition.yml'].decode('utf-8-sig')
        name = re.search(r'^name: ([\w-]+)\s*$', definition, re.M)
        version = re.search(r'^version: (\d+\.\d+\.\d+)\s*$', definition, re.M)
        if not name or not version or module != name[1] + '-' + version[1]:
            raise ValueError('Module filename disagrees with its definition: ' + module)
        bundle['ucp/modules/' + module + '.zip'] = archive_bytes(members)
    return bundle
