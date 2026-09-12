"""Build installable module ZIPs and a tester bundle; no game/framework binaries."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile
import urllib.request
import re
from module_archives import archive_bytes, package_modules
from module_payload import module_payload

p = argparse.ArgumentParser()
p.add_argument('--dll', type=Path, required=True)
p.add_argument('--loader', type=Path, required=True)
p.add_argument('--map-extensions', type=Path, required=True)
p.add_argument('--protocol', type=Path, required=True)
p.add_argument('--chat', type=Path, required=True)
p.add_argument('--files', dest='files_module', type=Path, required=True)
p.add_argument('--map-base-zip', type=Path)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--module-directory', type=Path,
               help='Also write individual module ZIPs here for direct downloads')
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
for checkout in [root, a.loader, a.map_extensions, a.protocol, a.chat, a.files_module]:
    if subprocess.check_output(['git','status','--porcelain','--untracked-files=normal'],cwd=checkout,text=True).strip():
        raise SystemExit('Commit source changes before packaging: ' + str(checkout))
files = {}
def module_name(checkout, expected):
    definition = (checkout/'definition.yml').read_text(encoding='utf-8')
    name = re.search(r'^name: ([\w-]+)\s*$', definition, re.M)
    version = re.search(r'^version: (\d+\.\d+\.\d+)\s*$', definition, re.M)
    if not name or name[1] != expected or not version:
        raise SystemExit('Invalid module definition: ' + str(checkout))
    return name[1] + '-' + version[1]

module_id, payload = module_payload(root, a.dll)
module = 'ucp/modules/' + module_id + '/'
files.update({module + name: data for name, data in payload.items()})
loader = 'ucp/modules/' + module_name(a.loader, 'aicloader') + '/'
for path in sorted(a.loader.glob('*.lua')):
    files[loader+path.name] = path.read_bytes()
files[loader+'definition.yml'] = (a.loader/'definition.yml').read_bytes()
files[loader+'options.yml'] = (a.loader/'options.yml').read_bytes()
files[loader+'vanilla.json'] = (a.loader/'resources/vanilla.json').read_bytes()
for path in sorted((a.loader/'locale').glob('*')):
    if path.is_file(): files[loader+'locale/'+path.name] = path.read_bytes()
map_base_url = 'https://github.com/UnofficialCrusaderPatch/UCP3-extensions-store/releases/download/v3.0.7/map-extensions-1.0.0.zip'
map_base_sha = '68819a64c5c3c1f5f9ecd7bd072e7ffd9fabd15baad13be0b866c7bb31f3cba1'
map_base = a.map_base_zip.read_bytes() if a.map_base_zip else urllib.request.urlopen(map_base_url, timeout=30).read()
if hashlib.sha256(map_base).hexdigest() != map_base_sha:
    raise SystemExit('Map Extensions base artifact does not match its pinned store checksum')
import io
with zipfile.ZipFile(io.BytesIO(map_base)) as archive:
    map_dll = archive.read('luamemzip.dll')
map_prefix = 'ucp/modules/' + module_name(a.map_extensions, 'map-extensions') + '/'
files[map_prefix+'luamemzip.dll'] = map_dll
for path in sorted(a.map_extensions.glob('*.lua')):
    files[map_prefix+path.name] = path.read_bytes()
for path in sorted((a.map_extensions/'mapextensions').glob('*.lua')):
    files[map_prefix+'mapextensions/'+path.name] = path.read_bytes()
for relative in ['definition.yml','LICENSE']:
    files[map_prefix+relative] = (a.map_extensions/relative).read_bytes()
for path in sorted((a.map_extensions/'locale').glob('*')):
    if path.is_file(): files[map_prefix+'locale/'+path.name] = path.read_bytes()
for checkout,name in [(a.protocol,'protocol'),(a.chat,'chat'),(a.files_module,'files')]:
    prefix='ucp/modules/'+module_name(checkout, name)+'/'
    for path in sorted(checkout.rglob('*')):
        relative=path.relative_to(checkout)
        if not path.is_file() or any(part.startswith('.') or part in ('build','tests','docs') for part in relative.parts):
            continue
        if path.suffix=='.lua' or relative.parts[0]=='locale' or relative.as_posix() in ('definition.yml','options.yml','LICENSE'):
            files[prefix+relative.as_posix()]=path.read_bytes()
files['AIC-TACTICS-COMBAT.md'] = (root/'docs/combat-integration.md').read_bytes()
files['AIC-TACTICS-COMPATIBILITY.md'] = (root/'docs/compatibility-matrix.md').read_bytes()
files['AIC-TACTICS-VERIFICATION.md'] = (root/'docs/integration-verification.md').read_bytes()
files['AIC-TACTICS-INTEGRITY-TIMING.md'] = (root/'docs/integrity-performance.md').read_bytes()
files['DEPENDENCY-SOURCES.md'] = (root/'docs/dependency-sources.md').read_bytes()
for path in sorted((root/'examples').rglob('*')):
    if path.is_file(): files['examples/'+path.relative_to(root/'examples').as_posix()] = path.read_bytes()
files['AIC-TACTICS-TESTING.md'] = (root/'docs/runtime-testing.md').read_bytes()
files['AIC-TACTICS-GRACE.md'] = (root/'docs/recruitment-grace.md').read_bytes()
files['AIC-TACTICS-EQUIPMENT.md'] = (root/'docs/equipment-surplus.md').read_bytes()
files['AIC-TACTICS-MOAT.md'] = (root/'docs/defense-moat.md').read_bytes()
files['AIC-TACTICS-COMPOSITION.md'] = (root/'docs/defense-composition.md').read_bytes()
files['INSTALL.txt'] = (
    'Copy the module ZIP files from ucp/modules into your game\'s ucp/modules folder.\n'
    'Keep those module ZIPs zipped, then reopen the UCP GUI and enable AIC Tactics.\n'
    'Replace the mistakenly extracted folders of the same names with these ZIPs.\n'
    'See AIC-TACTICS-COMPATIBILITY.md for the required Legacy settings.\n'
    'Recorder is optional; its matching ZIP and UI dependencies are only needed for replay tests.\n'
    'This is an unsigned test build for an existing security-off runtime.\n'
).encode('utf-8')
files['sortie-test-aic-fragment.json'] = json.dumps({'RecruitPolicy':'WeightedRoles', **{
    'RecruitProb'+role+strength: 100 if role=='Sortie' else 0
    for strength in ['Default','Weak','Strong'] for role in ['Def','Raid','Attack','Sortie']}}, indent=2).encode()+b'\n'
manifest = {'moduleSource':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),
    'loaderSource':subprocess.check_output(['git','rev-parse','HEAD'],cwd=a.loader,text=True).strip(),
    'mapStateSource':subprocess.check_output(['git','rev-parse','HEAD'],cwd=a.map_extensions,text=True).strip(),
    'protocolSource':subprocess.check_output(['git','rev-parse','HEAD'],cwd=a.protocol,text=True).strip(),
    'chatSource':subprocess.check_output(['git','rev-parse','HEAD'],cwd=a.chat,text=True).strip(),
    'filesSource':subprocess.check_output(['git','rev-parse','HEAD'],cwd=a.files_module,text=True).strip(),
    'mapRuntimeSource':{'url':map_base_url,'sha256':map_base_sha,'dllSha256':hashlib.sha256(map_dll).hexdigest()},
    'scope':'Development AIC Tactics integration; acceptance status is recorded separately',
    'sha256':{name:hashlib.sha256(data).hexdigest() for name,data in files.items()}}
files = package_modules(files)
manifest['layout'] = 'ucp-module-zips-v1'
manifest['moduleZipSha256'] = {name:hashlib.sha256(data).hexdigest()
    for name,data in files.items() if name.startswith('ucp/modules/')}
files['test-manifest.json'] = json.dumps(manifest,indent=2).encode()+b'\n'
a.output.parent.mkdir(parents=True,exist_ok=True)
a.output.write_bytes(archive_bytes(files))
if a.module_directory:
    a.module_directory.mkdir(parents=True,exist_ok=True)
    for name,data in files.items():
        if name.startswith('ucp/modules/'):
            (a.module_directory/Path(name).name).write_bytes(data)
print(json.dumps({'zip':str(a.output),'bytes':a.output.stat().st_size,
    'sha256':hashlib.sha256(a.output.read_bytes()).hexdigest()},indent=2))
