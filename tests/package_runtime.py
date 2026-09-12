"""Build a development test ZIP; no game/framework binaries are bundled."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile
import urllib.request

p = argparse.ArgumentParser()
p.add_argument('--dll', type=Path, required=True)
p.add_argument('--loader', type=Path, required=True)
p.add_argument('--map-extensions', type=Path, required=True)
p.add_argument('--protocol', type=Path, required=True)
p.add_argument('--chat', type=Path, required=True)
p.add_argument('--map-base-zip', type=Path)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
for checkout in [root, a.loader, a.map_extensions, a.protocol, a.chat]:
    if subprocess.check_output(['git','status','--porcelain','--untracked-files=normal'],cwd=checkout,text=True).strip():
        raise SystemExit('Commit source changes before packaging: ' + str(checkout))
files = {}
module = 'ucp/modules/aic-tactics-0.0.1/'
for relative in ['definition.yml','options.yml']:
    files[module+relative] = (root/relative).read_bytes()
for path in sorted(root.glob('*.lua')):
    files[module+path.name] = path.read_bytes()
for path in sorted((root/'config').glob('*.lua')):
    files[module+'config/'+path.name] = path.read_bytes()
for path in sorted((root/'locale').glob('*.yml')):
    files[module+'locale/'+path.name] = path.read_bytes()
for path in sorted((root/'locale').glob('description-*.md')):
    files[module+'locale/'+path.name] = path.read_bytes()
for path in sorted((root/'docs').glob('configuration-*.md')):
    files[module+'docs/'+path.name] = path.read_bytes()
files[module+'aicTactics.dll'] = a.dll.read_bytes()
# The identity excludes its own generated file. Length framing makes file names
# and bytes unambiguous, independent of filesystem enumeration or ZIP metadata.
identity = hashlib.sha256()
identity_files = []
for name, data in sorted(files.items()):
    if name.startswith(module) and name != module+'build-identity.lua':
        relative = name[len(module):].encode('utf-8')
        identity_files.append(relative.decode('utf-8'))
        identity.update(len(relative).to_bytes(4,'little')); identity.update(relative)
        identity.update(len(data).to_bytes(8,'little')); identity.update(data)
files[module+'build-identity.lua'] = ("return {sha256='"+identity.hexdigest()+"', files={"
    + ','.join(json.dumps(name) for name in identity_files) + "}}\n").encode('ascii')
loader = 'ucp/modules/aicloader-1.1.3/'
for path in sorted(a.loader.glob('*.lua')):
    files[loader+path.name] = path.read_bytes()
files[loader+'definition.yml'] = (a.loader/'definition.yml').read_bytes().replace(b'version: 1.1.2', b'version: 1.1.3')
files[loader+'options.yml'] = (a.loader/'options.yml').read_bytes()
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
map_prefix = 'ucp/modules/map-extensions-1.1.0/'
files[map_prefix+'luamemzip.dll'] = map_dll
for path in sorted(a.map_extensions.glob('*.lua')):
    files[map_prefix+path.name] = path.read_bytes()
for path in sorted((a.map_extensions/'mapextensions').glob('*.lua')):
    files[map_prefix+'mapextensions/'+path.name] = path.read_bytes()
for relative in ['definition.yml','LICENSE']:
    files[map_prefix+relative] = (a.map_extensions/relative).read_bytes()
for path in sorted((a.map_extensions/'locale').glob('*')):
    if path.is_file(): files[map_prefix+'locale/'+path.name] = path.read_bytes()
for checkout,name,version in [(a.protocol,'protocol','1.1.0'),(a.chat,'chat','1.0.0')]:
    prefix='ucp/modules/'+name+'-'+version+'/'
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
files['sortie-test-aic-fragment.json'] = json.dumps({'RecruitPolicy':'WeightedRoles', **{
    'RecruitProb'+role+strength: 100 if role=='Sortie' else 0
    for strength in ['Default','Weak','Strong'] for role in ['Def','Raid','Attack','Sortie']}}, indent=2).encode()+b'\n'
manifest = {'moduleSource':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),
    'loaderSource':subprocess.check_output(['git','rev-parse','HEAD'],cwd=a.loader,text=True).strip(),
    'mapStateSource':subprocess.check_output(['git','rev-parse','HEAD'],cwd=a.map_extensions,text=True).strip(),
    'protocolSource':subprocess.check_output(['git','rev-parse','HEAD'],cwd=a.protocol,text=True).strip(),
    'chatSource':subprocess.check_output(['git','rev-parse','HEAD'],cwd=a.chat,text=True).strip(),
    'mapRuntimeSource':{'url':map_base_url,'sha256':map_base_sha,'dllSha256':hashlib.sha256(map_dll).hexdigest()},
    'scope':'Development AIC Tactics integration; acceptance status is recorded separately',
    'sha256':{name:hashlib.sha256(data).hexdigest() for name,data in files.items()}}
files['test-manifest.json'] = json.dumps(manifest,indent=2).encode()+b'\n'
a.output.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(a.output,'w',zipfile.ZIP_DEFLATED) as z:
    for name,data in sorted(files.items()):
        info=zipfile.ZipInfo(name,(2026,9,12,0,0,0)); info.compress_type=zipfile.ZIP_DEFLATED
        z.writestr(info,data)
print(json.dumps({'zip':str(a.output),'bytes':a.output.stat().st_size,
    'sha256':hashlib.sha256(a.output.read_bytes()).hexdigest()},indent=2))
