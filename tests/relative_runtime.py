"""Real CLI/HXIM convergence regressions; independent equations, no third-party modules."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--solver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    exe = args.solver.resolve(strict=True)
    root = args.output.resolve()
    root.mkdir(parents=True, exist_ok=False)
    cases = []
    ids = '00112233445546778899AABBCCDDEEFF F47AC10B58CC4372A5670E02B2C3D479'
    save = ['.SAVE HXIM FILE="out.hxim" P']

    def probe(nodes='N0'):
        return [f'.PROBE P HXIM DEF DOUBLE V {ids} circuit', f'.PROBE P HXIM NODES circuit {nodes}']

    def options(deck, value='0 1e-6 1e-12', extra=()):
        return deck[:1]+['.OPTIONAL HEXMG_OPT 1', 'CONVERGENCE RELATIVE '+value, *extra, '.END OPTIONAL']+deck[1:]

    def run(name, deck, expected=None, reject=False, iterations=None, message=None, accuracy=2e-6):
        folder = root/name
        folder.mkdir()
        (folder/'input.hmg').write_text('\n'.join(deck)+'\n', encoding='ascii')
        p = subprocess.run([str(exe), 'input.hmg'], cwd=folder, capture_output=True, timeout=15)
        (folder/'stdout.txt').write_bytes(p.stdout)
        (folder/'stderr.txt').write_bytes(p.stderr)
        trace = [tuple(map(float, pair)) for pair in re.findall(rb'Relative iter \d+ state=(\S+) balance=(\S+)', p.stdout)]
        path = folder/'out.hxim'
        values = list(struct.unpack('<'+'d'*((path.stat().st_size-80)//8), path.read_bytes()[80:])) if path.exists() else []
        ok = (p.returncode == 1 and not path.exists()) if reject else p.returncode == 0 and not p.stderr
        if expected is not None:
            ok &= len(values)==len(expected) and all(math.isclose(v,e,rel_tol=accuracy,abs_tol=1e-20) for v,e in zip(values,expected))
        if not reject and iterations is None and trace:
            ok &= max(trace[-1])<=1
        if iterations is not None:
            ok &= len(trace)==iterations
        if message:
            ok &= message.lower().encode() in p.stderr.lower()
        case = dict(name=name, passed=bool(ok), exit_code=p.returncode, values=values, trace=trace, stderr=p.stderr.decode(errors='replace'))
        cases.append(case)
        print(name, 'PASS' if ok else 'FAIL', flush=True)
        return case

    def cubic(sv=1., si=1., reference=0., count=1):
        # u=(V-reference)/sv, current=si*(u+u^3), exact solution u=10.
        lines = ['HEXMG 1.0', f'.RAILS=2 R1={reference:.17g}', '.FUNCTION law P=2 V=2',
                 '_SUB V0 P0 P1', f'_MULC V0 V0 {1/sv:.17g}', '_MUL V1 V0 V0',
                 '_MUL RET V1 V0', f'_MULC RET RET {-si:.17g}', '.END FUNCTION law',
                 f'.MODEL m SUBCIRCUIT N={count}']
        for i in range(count):
            lines += [f'F{i} FCI F=law(X0,X1) N{i} GND {si/sv:.17g}', f'I{i} I N{i} GND DC0={1010*si:.17g} DC={1010*si:.17g}']
        return lines+['.END MODEL m', '.CREATE circuit m GND=R1']+probe()

    normalized_traces=[]
    for scale in (1e-9,1.,1e9):
        c=run('scaled_'+str(scale), options(cubic(scale,scale),f'0 {scale*1e-6:.17g} {scale*1e-12:.17g}')+
              ['.RUN circuit DC INITIAL ERR=0.0001']+save,[10*scale])
        normalized_traces.append(c['trace'])
    for ref in (25.,3000.):
        c=run('ambient_'+str(ref), options(cubic(reference=ref),f'{ref} 1e-6 1e-12')+['.RUN circuit DC INITIAL']+save,[ref+10])
        normalized_traces.append(c['trace'])
    c=run('eleven_independent_nodes',options(cubic(count=11))+['.RUN circuit DC INITIAL']+save,[10.])
    normalized_traces.append(c['trace'])
    # FCI uses finite-difference derivatives, so intermediate iterates need not match across units.
    same = all(len(t)==len(normalized_traces[0]) and max(t[-1])<=1 for t in normalized_traces)
    cases.append(dict(name='unit_reference_count_invariance',passed=same))

    base=['HEXMG 1.0','.RAILS=1','.MODEL m SUBCIRCUIT N=1','R1 R N0 GND 10',
          'I1 I N0 GND DC0=0.25 DC=0.25','.END MODEL m','.CREATE circuit m GND=R0']+probe()
    run('linear',options(base)+['.RUN circuit DC INITIAL']+save,[2.5],iterations=2)
    run('zero',options([s.replace('0.25','0') for s in base])+['.RUN circuit DC INITIAL']+save,[0.],iterations=1)
    run('near_zero',options([s.replace('0.25','1e-14') for s in base])+['.RUN circuit DC INITIAL']+save,[1e-13],iterations=1)
    run('no_free_nodes',options(['HEXMG 1.0','.RAILS=2 R1=5','.MODEL m SUBCIRCUIT',
        'R1 R R1 GND 10','.END MODEL m','.CREATE circuit m GND=R0'])+['.RUN circuit DC INITIAL'],iterations=1)

    for n in (1,2,105):
        value=0.
        for _ in range(n): value-=(value**3+value-1010)/(3*value**2+1)
        run('fixed_'+str(n),options(cubic(),extra=['MAX_ITERATIONS 1'])+[f'.RUN circuit DC INITIAL ITERS={n} ERR=-1']+save,[value],iterations=n)
    for n in (1,2):
        run('limit_'+str(n),options(cubic(),extra=[f'MAX_ITERATIONS {n}'])+['.RUN circuit DC INITIAL']+save,reject=True,iterations=n,message='did not converge')
    run('invalid_finite_state',options([s.replace('DC0=1010','DC0=1e200') for s in cubic()])+['.RUN circuit DC INITIAL ITER']+save,reject=True,message='convergence')

    # Output-only O nodes are stored outside the ordinary N array.
    nested=['HEXMG 1.0','.RAILS=1','.MODEL child SUBCIRCUIT O=1','R R O0 GND 10',
            'I I O0 GND DC0=0.25 DC=0.25','.END MODEL child','.MODEL m SUBCIRCUIT',
            'SUB MODEL child NONE','.END MODEL m','.CREATE circuit m GND=R0']+probe('SUB.O0')
    run('output_node',options(nested)+['.RUN circuit DC INITIAL']+save,[2.5],iterations=2)
    run('output_node_profile',options(nested,extra=['CONVERGENCE_NODE circuit SUB.O0 25 1e-6 1e-12'])+['.RUN circuit DC INITIAL']+save,[2.5])
    run('output_node_ac',options(nested)+['.RUN circuit DC INITIAL','.RUN circuit AC F=10']+save,[0.,0.])
    # Real mixed references: N0 electrical (zero), N1 thermal (ambient 25).
    mixed=cubic(reference=25,count=2)
    mixed=[s.replace('GND','R1') if s.startswith(('F1 ','I1 ')) else s for s in mixed]
    mixed=[s.replace('GND=R1','GND=R0').replace('NODES circuit N0','NODES circuit N0 N1') for s in mixed]
    run('mixed_references',options(mixed,extra=['CONVERGENCE_NODE circuit N1 25 1e-6 1e-12'])+
        ['.RUN circuit DC INITIAL']+save,[10.,35.])
    loose=run('loose_relative',options(cubic())+['.RUN circuit DC INITIAL ERR=0.005']+save,[10.],accuracy=0.005)
    strict=run('strict_relative',options(cubic())+['.RUN circuit DC INITIAL ERR=1e-9']+save,[10.])
    cases.append(dict(name='strict_requires_more_updates',passed=len(strict['trace'])>len(loose['trace'])))
    # Neither a permissive state tolerance nor a permissive balance tolerance
    # may disable the other test. Both must satisfy their own limit.
    for label,profile in [('balance','0 1e30 1e-12'),('state','0 1e-6 1e30')]:
        run(label+'_independent_stop',options(cubic(),profile,extra=['MAX_ITERATIONS 1'])+
            ['.RUN circuit DC INITIAL']+save,reject=True,iterations=1,message='did not converge')
    run('aliased_profile',options(base,extra=['CONVERGENCE_NODE circuit N0 0 1e-6 1e-12',
        'CONVERGENCE_NODE circuit R1.X0 0 1e-6 1e-12'])+['.RUN circuit DC INITIAL']+save,reject=True,message='aliased')
    for node in ('N1','N-1','R1.X2','GND','R0','N0.STEPSTART','BG','R1.N0','N4294967296'):
        run('bad_node_'+node,options(base,extra=[f'CONVERGENCE_NODE circuit {node} 0 1e-6 1e-12'])+['.RUN circuit DC INITIAL']+save,reject=True)
    run('bad_circuit',options(base,extra=['CONVERGENCE_NODE missing N0 0 1e-6 1e-12'])+['.RUN circuit DC INITIAL']+save,reject=True)
    for value in ('nan 1e-6 1e-12','0 0 1e-12','0 -1 1e-12','0 1e-6 inf','0 1e-6 1e-12 junk','0 1e-6x 1e-12'):
        run('bad_option_'+str(len(cases)),options(base,value)+['.RUN circuit DC INITIAL']+save,reject=True)
    run('duplicate_global',options(base,extra=['CONVERGENCE RELATIVE 0 1e-6 1e-12'])+['.RUN circuit DC INITIAL']+save,reject=True)
    run('duplicate_profile',options(base,extra=['CONVERGENCE_NODE circuit N0 0 1e-6 1e-12']*2)+['.RUN circuit DC INITIAL']+save,reject=True)
    run('many_node_profiles',options(cubic(count=131),extra=[f'CONVERGENCE_NODE circuit N{i} 0 1e-6 1e-12' for i in range(131)])+
        ['.RUN circuit DC INITIAL']+save,[10.])
    for kind in ('VIB','VIN','MIB','MIN','GM'):
        third='NONE' if kind.endswith('IB') else 'N1'
        parameters='5 5 0 0 1e10' if kind.startswith('V') else ('0.1' if kind=='GM' else '1e10')
        # VIB/VIN drive 5 V through 10 ohm, with nonzero source current;
        # MIB/MIN short to ground, current source drives .5 A.
        n=1 if third=='NONE' else 2
        deck=['HEXMG 1.0','.RAILS=1',f'.MODEL m SUBCIRCUIT N={n}',f'X {kind} N0 GND {third} {parameters}',
              'R R N0 GND 10',('I I N0 GND DC0=0.25 DC=0.25' if kind.startswith('V') else 'I I N0 GND DC0=0.5 DC=0.5'),
              '.END MODEL m','.CREATE circuit m GND=R0']+probe('N0 X.O0' if third=='NONE' else 'N0 N1')
        expected=([5.,-.25] if kind.startswith('V') else ([2.5,.25] if kind=='GM' else [0.,.5]))
        run('aux_'+kind,options(deck)+['.RUN circuit DC INITIAL']+save,expected,iterations=2)

    report=dict(solver=str(exe),sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),cases=cases)
    (root/'report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    failures=[c['name'] for c in cases if not c['passed']]
    print(f'{len(cases)-len(failures)}/{len(cases)} PASS; failures={failures}')
    return bool(failures)


if __name__=='__main__':
    raise SystemExit(main())
