"""Build a development test ZIP; no game/framework binaries are bundled."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--dll', type=Path, required=True)
p.add_argument('--loader', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
for checkout in [root, a.loader]:
    if subprocess.check_output(['git','status','--porcelain','--untracked-files=normal'],cwd=checkout,text=True).strip():
        raise SystemExit('Commit source changes before packaging: ' + str(checkout))
files = {}
module = 'ucp/modules/aic-tactics-0.0.1/'
for relative in ['definition.yml','init.lua','native.lua','state.lua','options.yml']:
    files[module+relative] = (root/relative).read_bytes()
for path in sorted((root/'config').glob('*.lua')):
    files[module+'config/'+path.name] = path.read_bytes()
for path in sorted((root/'locale').glob('*.yml')):
    files[module+'locale/'+path.name] = path.read_bytes()
files[module+'aicTactics.dll'] = a.dll.read_bytes()
loader = 'ucp/modules/aicloader-1.1.3/'
for path in sorted(a.loader.glob('*.lua')):
    files[loader+path.name] = path.read_bytes()
files[loader+'definition.yml'] = (a.loader/'definition.yml').read_bytes().replace(b'version: 1.1.2', b'version: 1.1.3')
files[loader+'options.yml'] = (a.loader/'options.yml').read_bytes()
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
    'scope':'Development recruitment integration; not a release or full-package acceptance',
    'sha256':{name:hashlib.sha256(data).hexdigest() for name,data in files.items()}}
files['test-manifest.json'] = json.dumps(manifest,indent=2).encode()+b'\n'
a.output.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(a.output,'w',zipfile.ZIP_DEFLATED) as z:
    for name,data in sorted(files.items()):
        info=zipfile.ZipInfo(name,(2026,9,12,0,0,0)); info.compress_type=zipfile.ZIP_DEFLATED
        z.writestr(info,data)
print(json.dumps({'zip':str(a.output),'bytes':a.output.stat().st_size,
    'sha256':hashlib.sha256(a.output.read_bytes()).hexdigest()},indent=2))
