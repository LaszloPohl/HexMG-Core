"""AC source-law regression against independent complex circuit equations.

Run with --solver EXE --output FRESH_DIRECTORY. No third-party packages.
The pre-fix executable must fail; output and failures remain available.
"""
import argparse
import cmath
import hashlib
import json
import math
from pathlib import Path
import subprocess


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--solver', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    exe = args.solver.resolve(strict=True)
    root = args.output.resolve()
    root.mkdir(parents=True, exist_ok=False)
    cases = []

    def run(name, lines, expected, absolute=2e-9):
        folder = root/name
        folder.mkdir()
        (folder/'input.hmg').write_text('\n'.join(lines+['.END'])+'\n', encoding='ascii')
        proc = subprocess.run([str(exe), 'input.hmg'], cwd=folder,
                              capture_output=True, timeout=20)
        (folder/'stdout.txt').write_bytes(proc.stdout)
        (folder/'stderr.txt').write_bytes(proc.stderr)
        failures = []
        checks = 0
        if proc.returncode or proc.stderr:
            failures.append('solver exit/stderr: '+str(proc.returncode))
        for filename, reference in expected.items():
            path = folder/filename
            actual = [[float(v) for v in row.split()] for row in path.read_text().splitlines()
                      if row.strip()] if path.exists() else []
            if len(actual) != len(reference) or any(len(a) != len(b) for a,b in zip(actual,reference)):
                failures.append(filename+': unexpected table shape')
                continue
            for row,(a,b) in enumerate(zip(actual,reference)):
                for col,(value,ref) in enumerate(zip(a,b)):
                    checks += 1
                    # RAW uses six significant digits. Source-current cancellation
                    # at G=1e9 additionally needs a microampere absolute tolerance.
                    if not math.isfinite(value) or abs(value-ref) > absolute+6e-6*abs(ref):
                        failures.append(f'{filename} [{row},{col}]: {value} != {ref}')
        cases.append(dict(name=name, passed=not failures, checks=checks, failures=failures))
        print(name, 'PASS' if not failures else 'FAIL', flush=True)

    def save(probes, append=False, filename='out.res'):
        return f'.SAVE RAW MAXRESULTSPERROW=100 {"APPEND " if append else ""}FILE="{filename}" {probes}'

    def row(f, *values):
        return [f]+[part for z in values for part in (complex(z).real, complex(z).imag)]

    frequencies = [10, 100, 159.15494309189535, 1000, 10000, 1000, 10, 10]
    for nested in (False, True):
        for g in (.25, 1., 4., 1e9):
            for kind in ('RC', 'RL'):
                r,c,l = 1000.,1e-6,.01
                source = 1.3*cmath.exp(.7j)
                start = ['HEXMG 1.0']
                cells = [f'RS R {"X0" if nested else "N0"} N1 {r}',
                         f'Z {"C" if kind=="RC" else "L"} N1 {"X1" if nested else "GND"} {c if kind=="RC" else l}']
                if nested:
                    cells = [line.replace('N1','N0') for line in cells]
                    start += ['.MODEL FILTER SUBCIRCUIT X=2 N=1', *cells, '.END MODEL FILTER']
                start += [f'.MODEL M SUBCIRCUIT N={1 if nested else 2}',f'VS V N0 GND 0 0 1.3 .7 {g}',
                          *(['F MODEL FILTER N0 GND'] if nested else cells), '.END MODEL M',
                          '.CREATE MAIN M GND=R0',f'.PROBE U V MAIN {"F.N0" if nested else "N1"}',
                          '.PROBE IP I MAIN VS.X0', '.PROBE IM I MAIN VS.X1',
                          f'.PROBE IZ I MAIN {"F." if nested else ""}Z.X0', '.RUN MAIN DC ITER']
                refs=[]
                for i,f in enumerate(frequencies):
                    s=2j*math.pi*f
                    z=1/(s*c) if kind=='RC' else s*l
                    current=source/(1/g+r+z)
                    refs.append(row(f,current*z,current,-current,-current))
                    start += [f'.RUN MAIN AC F={f:.17g}',save('U IP IM IZ', i>0)]
                run(f'{kind}_g{g}_nested{int(nested)}', start, {'out.res':refs}, 3e-6 if g==1e9 else 2e-9)

    # Resistive source law, first point, repeat, phase/amplitude/source removal.
    for g in (.25, 1., 4.):
        lines=['HEXMG 1.0','.MODEL M SUBCIRCUIT N=1 B=2',f'VS V N0 GND 0 0 B0 B1 {g}',
               'RL R N0 GND 10','.END MODEL M','.CREATE MAIN M GND=R0',
               '.PROBE U V MAIN N0','.PROBE I I MAIN VS.X0','.RUN MAIN DC ITER']
        refs=[]
        for i,(a,phase) in enumerate([(1.,0.),(1.,0.),(2.,.4),(0.,0.),(.5,-1.)]*4):
            e=a*cmath.exp(phase*1j); current=e/(1/g+10)
            lines += [f'.SET MAIN.B0 {a}',f'.SET MAIN.B1 {phase}',
                      '.RUN MAIN AC F=100',save('U I',i>0)]
            refs.append(row(100,10*current,current))
        run(f'changed_source_g{g}',lines,{'out.res':refs})

    # An independent current source is a control: it has no V-source defect.
    lines=['HEXMG 1.0','.MODEL M SUBCIRCUIT N=1','IS I N0 GND 0 0 .002 .3',
           'R1 R N0 GND 1k','C1 C N0 GND 1u','.END MODEL M','.CREATE MAIN M GND=R0',
           '.PROBE U V MAIN N0','.PROBE I I MAIN IS.X0']
    refs=[]
    for i,f in enumerate(frequencies):
        current=.002*cmath.exp(.3j)
        refs.append(row(f,current/(.001+2j*math.pi*f*1e-6),current))
        lines += [f'.RUN MAIN AC F={f:.17g}',save('U I',i>0)]
    run('current_source_implicit_dc',lines,{'out.res':refs})

    # The same branch law participates in complex-frequency (TIMECONST)
    # calculations, and in both local-matrix and SUNRED reductions.
    for tree in (False,True):
        lines=['HEXMG 1.0']
        if tree:
            lines += ['.SUNREDTREE T','RED 1 0 0 0 0 1','RED 2 0 1 0 0 2','.END SUNREDTREE T']
        lines += ['.MODEL M SUBCIRCUIT N=2'+(' SUNRED=T' if tree else ''),
                  'VS V N0 GND 0 0 1 0 4','RS R N0 N1 10','CS C N1 GND .001',
                  '.END MODEL M','.CREATE MAIN M GND=R0','.PROBE U V MAIN N1',
                  '.PROBE I I MAIN VS.X0','.RUN MAIN DC ITER']
        refs=[]; time_refs=[]
        for i,f in enumerate((1.,10.,100.,1.)):
            s=2j*math.pi*f; z=1/(s*.001); current=1/(10.25+z)
            refs.append(row(f,current*z,current))
            lines += [f'.RUN MAIN AC F={f}',save('U I',i>0)]
            s=2*math.pi*f*cmath.exp(1j*(math.pi+1.5*math.log(10)/5))
            z=1/(s*.001); current=1/(10.25+z)
            # RAW TIMECONST stores Im(response)*ln(10)/pi, not Re/Im pairs.
            time_refs.append([f,1/(2*math.pi*f)]+[v.imag*math.log(10)/math.pi for v in (current*z,current)])
            lines += [f'.RUN MAIN TIMECONST F={f} SPD=5',save('U I',i>0,'timeconst.res')]
        run(f'complex_frequency_tree{int(tree)}',lines,{'out.res':refs,'timeconst.res':time_refs})

    # DC terminal currents must not acquire another factor G when probed.
    for g in (.25,1.,4.):
        lines=['HEXMG 1.0','.MODEL M SUBCIRCUIT N=1',f'VS V N0 GND 3 7 1 0 {g}',
               'RL R N0 GND 10','.END MODEL M','.CREATE MAIN M GND=R0',
               '.PROBE U V MAIN N0','.PROBE IP I MAIN VS.X0','.PROBE IM I MAIN VS.X1',
               '.RUN MAIN DC INITIAL ITER',save('U IP IM'),'.RUN MAIN DC ITER',save('U IP IM',True)]
        run(f'dc_currents_g{g}',lines,{'out.res':[[10*e/(10+1/g),e/(10+1/g),-e/(10+1/g)] for e in (3,7)]})

    # AC must leave capacitor history and the DC operating point intact.
    # Also cover AC after a nonzero transient, and more than one AC in between.
    for interleaved in (False,True):
        lines=['HEXMG 1.0','.MODEL M SUBCIRCUIT N=2','VS V N0 GND 2 5 1 .2 4',
               'RS R N0 N1 10','CS C N1 GND .001','.END MODEL M','.CREATE MAIN M GND=R0',
               '.PROBE U V MAIN N1','.PROBE I I MAIN CS.X0',
               '.RUN MAIN DC INITIAL ITER',save('U',filename='initial.res')]
        refs=[]; ac_refs=[]; u=2.; r=10.25; dt=.001
        for i in range(8):
            if interleaved:
                for f in (10.,100.,10.):
                    z=1/(2j*math.pi*f*.001); current=cmath.exp(.2j)/(r+z)
                    ac_refs.append(row(f,current*z,-current))
                    lines += [f'.RUN MAIN AC F={f}',save('U I',len(ac_refs)>1,'ac.res')]
            u=(u*r*.001+5*dt)/(r*.001+dt)
            refs.append([(i+1)*dt,dt,u,-(5-u)/r])
            lines += ['.RUN MAIN TIMESTEP DT=.001 ITER',save('U I',i>0)]
        expected={'initial.res':[[2.]],'out.res':refs}
        if interleaved:expected['ac.res']=ac_refs
        run(f'transient_interleaved{int(interleaved)}',lines,expected)

    summary=dict(solver=str(exe),sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                 passed=sum(c['passed'] for c in cases),total=len(cases),
                 checks=sum(c['checks'] for c in cases),cases=cases)
    (root/'summary.json').write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8')
    print(f"{summary['passed']}/{summary['total']} cases; {summary['checks']} numeric checks")
    raise SystemExit(0 if all(c['passed'] for c in cases) else 1)


if __name__=='__main__':main()
