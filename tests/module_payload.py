"""AIC runtime payload shared by local previews and the UCP store build."""
import re
from pathlib import Path


def module_payload(root: Path, dll: Path):
    definition = (root / 'definition.yml').read_text(encoding='utf-8')
    version = re.search(r'^version: (\d+\.\d+\.\d+)\s*$', definition, re.M)[1]
    module_id = 'aic-tactics-' + version
    paths = [root / name for name in ('definition.yml', 'options.yml', 'LICENSE')]
    for pattern in ('*.lua', 'config/*.lua', 'locale/*.yml',
                    'locale/description-*.md', 'docs/configuration-*.md',
                    'schema/*.json'):
        paths.extend(root.glob(pattern))
    files = {path.relative_to(root).as_posix(): path.read_bytes() for path in paths}
    files['aicTactics.dll'] = dll.read_bytes()
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
