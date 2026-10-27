"""Compare an HXIM-producing deck with/without opaque OPTIONAL blocks.

Uses separate, fresh output directories; never changes either solver/source.
The supplied deck must be self-contained and write relative *.hxim outputs.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--baseline', type=Path, required=True)
p.add_argument('--optimized', type=Path, required=True)
p.add_argument('--deck', type=Path, default=Path(__file__).parent / 'fixtures/thermal18.hmg')
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
baseline = a.baseline.resolve(strict=True)
optimized = a.optimized.resolve(strict=True)
deck = a.deck.read_bytes()
root = a.output.resolve()
root.mkdir(parents=True, exist_ok=False)
block = (b'.OPTIONAL unknown header deliberately opaque\n'
         b'.INCLUDE this-file-must-not-be-opened.hmg\n'
         b'.END MODEL invalid\n.RUN impossible\n.SAVE impossible\n'
         b'+ ignored\n.OPTIONAL another ignored opener\n.END OPTIONAL\n')
decorated = block + block.join(deck.splitlines(keepends=True))
if not decorated.endswith(b'\n'):
    decorated += b'\n'
decorated += block
report = {'baseline_sha256': hashlib.sha256(baseline.read_bytes()).hexdigest(),
          'optimized_sha256': hashlib.sha256(optimized.read_bytes()).hexdigest(),
          'deck_sha256': hashlib.sha256(deck).hexdigest(), 'cases': {}}
reference = None
for name, exe, content in [('baseline', baseline, deck),
                           ('optimized_plain', optimized, deck),
                           ('optimized_optional', optimized, decorated)]:
    folder = root / name
    folder.mkdir()
    (folder / 'input.hmg').write_bytes(content)
    completed = subprocess.run([str(exe), 'input.hmg'], cwd=folder,
                               capture_output=True, timeout=60)
    (folder / 'stdout.txt').write_bytes(completed.stdout)
    (folder / 'stderr.txt').write_bytes(completed.stderr)
    if completed.returncode or completed.stderr.strip():
        raise RuntimeError(f'{name}: {completed.returncode}: {completed.stderr!r}')
    images = {f.name: f.read_bytes() for f in folder.glob('*.hxim')}
    if not images:
        raise RuntimeError(f'{name}: no HXIM output')
    if reference is None:
        reference = images
    elif images != reference:
        raise RuntimeError(f'{name}: HXIM bytes differ')
    report['cases'][name] = {key: hashlib.sha256(value).hexdigest()
                            for key, value in images.items()}
(root / 'report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('PASS: baseline / optimized plain / OPTIONAL before every physical line: identical HXIM')
