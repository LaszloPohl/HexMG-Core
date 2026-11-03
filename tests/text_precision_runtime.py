"""Compare textual numeric results against the same binary double HXIM samples.

--expect-truncation records the old failure without changing its executable.
The strict mode requires bit-exact double round trips, including coordinates.
"""
import argparse
import hashlib
import json
import re
import struct
import subprocess
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--solver', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--expect-truncation', action='store_true')
a = p.parse_args()
exe = a.solver.resolve(strict=True)
root = a.output.resolve()
root.mkdir(parents=True, exist_ok=False)
cases = []
ids = '00112233445546778899AABBCCDDEEFF F47AC10B58CC4372A5670E02B2C3D479'
number = r'[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?'
for mode in ('EXTENDED', 'DOUBLE', 'AUTO'):
    for amplitude in (1.2345678901234567, -9.876543210987654, 1.2345678901234567e-9):
        folder = root / (mode + '-' + str(amplitude))
        folder.mkdir()
        lines = ['HEXMG 1.0', '.OPTIONAL HEXMG_OPT 1',
                 'REDUCTION_PRECISION LEVEL0 ' + mode, '.END OPTIONAL',
                 '.MODEL M SUBCIRCUIT N=1', 'R1 R N0 GND 3.456789012345678',
                 'C1 C N0 GND .0006789012345678901',
                 f'I1 I N0 GND DC0={amplitude:.17g} DC={amplitude:.17g} AC={abs(amplitude):.17g} PHI=-.7234567890123456',
                 '.END MODEL M', '.CREATE circuit M GND=R0',
                 '.PROBE U V circuit N0', '.PROBE J I circuit R1.X0',
                 f'.PROBE P HXIM DEF DOUBLE V {ids} circuit', '.PROBE P HXIM NODES circuit N0',
                 f'.PROBE Q HXIM DEF DOUBLE I {ids} circuit', '.PROBE Q HXIM NODES circuit R1.X0']
        stages = [('dc', '.RUN circuit DC INITIAL', 0),
                  ('step', '.RUN circuit TIMESTEP DT=.0012345678901234567', 2),
                  ('ac', '.RUN circuit AC F=159.15494309189535', 1)]
        for name, command, _ in stages:
            lines += [command, f'.SAVE RAW FILE="{name}.res" U J',
                      f'.SAVE FILE="{name}.txt" U J',
                      f'.SAVE HXIM FILE="{name}-v.hxim" P',
                      f'.SAVE HXIM FILE="{name}-i.hxim" Q']
        (folder / 'input.hmg').write_text('\n'.join(lines) + '\n', encoding='ascii')
        proc = subprocess.run([str(exe), 'input.hmg'], cwd=folder, capture_output=True, timeout=20)
        (folder / 'stdout.txt').write_bytes(proc.stdout)
        (folder / 'stderr.txt').write_bytes(proc.stderr)
        assert proc.returncode == 0, (folder, proc.stderr)
        for name, _, prefix in stages:
            v = (folder / (name + '-v.hxim')).read_bytes()
            i = (folder / (name + '-i.hxim')).read_bytes()
            components = 2 if name == 'ac' else 1
            values = list(struct.unpack('<' + 'd' * components, v[80:]))
            values += struct.unpack('<' + 'd' * components, i[80:])
            coordinates = list(struct.unpack('<2d', v[8:24]))[:prefix]
            expected = coordinates + values
            raw = [float(n) for n in (folder / (name + '.res')).read_text().split()]
            # Human text differs only by its header and the complex a+bi syntax.
            human = (folder / (name + '.txt')).read_text().splitlines()[1:]
            human = [float(n) for n in re.findall(number, '\n'.join(human))]
            assert len(raw) == len(expected) == len(human), (folder, name, raw, expected, human)
            different = sum(struct.pack('<d', x) != struct.pack('<d', y)
                            for data in (raw, human) for x, y in zip(data, expected))
            cases.append(dict(case=folder.name, stage=name, numbers=2*len(expected),
                              rounded=different, raw=raw, expected=expected))
            assert (different > 0) if a.expect_truncation else (different == 0), cases[-1]
report = dict(solver=str(exe), sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
              cases=cases, checked=sum(c['numbers'] for c in cases),
              rounded=sum(c['rounded'] for c in cases))
(root / 'summary.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(f'{len(cases)} text precision cases / {report["checked"]} numeric comparisons: '
      + ('old truncation reproduced' if a.expect_truncation else 'bit-exact PASS'))
