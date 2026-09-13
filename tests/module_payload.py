"""AIC runtime payload shared by local previews and the UCP store build."""
import hashlib
import json
import re
from pathlib import Path


def module_payload(root: Path, dll: Path):
    definition = (root / 'definition.yml').read_text(encoding='utf-8')
    version = re.search(r'^version: (\d+\.\d+\.\d+)\s*$', definition, re.M)[1]
    module_id = 'aic-tactics-' + version
    paths = [root / name for name in ('definition.yml', 'options.yml', 'LICENSE')]
    for pattern in ('*.lua', 'config/*.lua', 'locale/*.yml',
                    'locale/description-*.md', 'docs/configuration-*.md'):
        paths.extend(root.glob(pattern))
    files = {path.relative_to(root).as_posix(): path.read_bytes() for path in paths}
    files['aicTactics.dll'] = dll.read_bytes()
    identity = hashlib.sha256()
    for name, data in sorted(files.items()):
        encoded = name.encode('utf-8')
        identity.update(len(encoded).to_bytes(4, 'little'))
        identity.update(encoded)
        identity.update(len(data).to_bytes(8, 'little'))
        identity.update(data)
    files['build-identity.lua'] = (
        "return {module='" + module_id + "', sha256='" + identity.hexdigest()
        + "', files={" + ','.join(json.dumps(name) for name in sorted(files))
        + "}}\n").encode('ascii')
    return module_id, files


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--dll', type=Path, required=True)
    parser.add_argument('--stage', type=Path, required=True)
    args = parser.parse_args()
    _, payload = module_payload(Path(__file__).resolve().parents[1], args.dll)
    # The caller supplies a fresh staging directory; never silently retain files.
    args.stage.mkdir(parents=True, exist_ok=False)
    for name, data in payload.items():
        target = args.stage / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
