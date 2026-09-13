"""Run the Windows x86 adapter test with a private, hash-pinned SHC image.

No reference executable bytes are committed or distributed. No game is launched.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
import pefile

p = argparse.ArgumentParser()
p.add_argument('--reference', type=Path, required=True)
p.add_argument('--test-exe', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--compiler', type=Path, required=True)
p.add_argument('--benchmark-integrity', action='store_true')
a = p.parse_args()
raw = a.reference.read_bytes()
digest = hashlib.sha256(raw).hexdigest()
assert digest == '3bb0a8c1e72331b3a30a5aa93ed94beca0081b476b04c1960e26d5b45387ac5a', 'Unsupported reference executable'
pe = pefile.PE(data=raw)
assert pe.OPTIONAL_HEADER.ImageBase == 0x400000 and pe.OPTIONAL_HEADER.SizeOfImage == 0x2091000
a.output.mkdir(parents=True, exist_ok=True)
image = bytearray(pe.OPTIONAL_HEADER.SizeOfImage)
for section in pe.sections:
    start = section.VirtualAddress
    image[start:start + section.SizeOfRawData] = section.get_data()
flat = a.output / 'private-shc141-image.bin'
flat.write_bytes(image)
command=[str(a.test_exe.resolve()), str(flat.resolve())]
if a.benchmark_integrity: command.append('--benchmark-integrity')
result = subprocess.run(command, capture_output=True, text=True)
record = {'reference': str(a.reference.resolve()), 'sha256': digest,
          'compiler': str(a.compiler.resolve()),
          'compiler_sha256': hashlib.sha256(a.compiler.read_bytes()).hexdigest(),
          'test_exe_sha256': hashlib.sha256(a.test_exe.read_bytes()).hexdigest(),
          'exit_code': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr,
          'scope': 'x86 test process, original instructions, synthetic state; not running-game acceptance'}
root = Path(__file__).resolve().parents[1]
record['sources'] = {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in (
    'include/aic_tactics/shc141_recruitment.hpp', 'src/shc141_recruitment.cpp',
    'include/aic_tactics/shc141_groups.hpp', 'src/shc141_groups.cpp', 'tests/shc141_group_cases.cpp',
    'tests/shc141_probe_tests.cpp', 'tests/run_shc141_probe.py', 'tests/build_shc141_probe.ps1')}
if 'runtime' in a.test_exe.name:
    for folder, pattern in (('include/aic_tactics','*.hpp'),('src','*.cpp'),('tests','shc141_*cases.cpp')):
        for source in sorted((root/folder).glob(pattern)):
            record['sources'][source.relative_to(root).as_posix()] = hashlib.sha256(source.read_bytes()).hexdigest()
    record['sources']['tests/build_runtime_tests.ps1'] = hashlib.sha256((root/'tests/build_runtime_tests.ps1').read_bytes()).hexdigest()
(a.output / ('integrity-timing.json' if a.benchmark_integrity else 'probe-result.json')).write_text(json.dumps(record, indent=2) + '\n')
print(result.stdout, end='')
print(result.stderr, end='')
raise SystemExit(result.returncode)
