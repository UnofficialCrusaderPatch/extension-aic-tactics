"""Package PR #1's text as an inert plugin for the real UCP GUI reader."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

ROOT = Path(__file__).resolve().parents[1]
LANGUAGES = ('ch', 'de', 'en', 'es', 'fa', 'fr', 'hu', 'ru', 'tr')
NAME = 'aic-tactics-description-preview-0.0.1.zip'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    files = {}
    for code in LANGUAGES:
        for folder, stem in [('locale', 'description'), ('docs', 'configuration')]:
            name = f'{folder}/{stem}-{code}.md'
            payload = (ROOT / name).read_bytes()
            text = payload.decode('utf-8')
            assert text.strip() and '\ufffd' not in text, name
            if folder == 'locale':
                assert not any(line.startswith('|') for line in text.splitlines()), (
                    name + ': the launcher description renderer does not support tables')
            files[name] = payload
            if folder == 'docs':
                examples = re.findall(r'```json\n(.*?)\n```', text.replace('\r\n', '\n'), re.S)
                assert len(examples) == 3, name
                for example in examples:
                    json.loads(example)
        files[f'locale/{code}.yml'] = b'{}\n'
    for name in ('definition.yml', 'config.yml', 'TESTING.md'):
        files[name] = (ROOT / 'tools/description-preview' / name).read_bytes()
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    manifest = {'sourceCommit': revision, 'scope': 'GUI descriptions only; no gameplay implementation',
                'files': {name: hashlib.sha256(data).hexdigest() for name, data in sorted(files.items())}}
    files['SOURCE.json'] = (json.dumps(manifest, indent=2) + '\n').encode('utf-8')
    args.output.mkdir(parents=True, exist_ok=True)
    archive = args.output / NAME
    with ZipFile(archive, 'w', compression=ZIP_DEFLATED) as output:
        for name, data in sorted(files.items()):
            entry = ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            entry.compress_type = ZIP_DEFLATED
            entry.external_attr = 0o644 << 16
            output.writestr(entry, data)
    with ZipFile(archive) as check:
        assert check.testzip() is None
        assert set(check.namelist()) == set(files)
        for name, payload in files.items():
            assert check.read(name) == payload
    checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
    (args.output / 'SHA256SUMS.txt').write_text(f'{checksum}  {NAME}\n', encoding='utf-8')
    (args.output / 'TESTING.md').write_bytes(files['TESTING.md'])
    print(f'{archive}: {archive.stat().st_size} bytes; SHA256 {checksum}')


if __name__ == '__main__':
    main()
