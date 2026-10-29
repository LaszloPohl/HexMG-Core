"""Physical-state guards: real CLI tests, including zero-current invalid laws."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--solver', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    exe = args.solver.resolve(strict=True)
    root = args.output.resolve()
    root.mkdir(parents=True, exist_ok=False)
    cases = []
    ids = '00112233445546778899AABBCCDDEEFF F47AC10B58CC4372A5670E02B2C3D479'
    save = ['.SAVE HXIM FILE="out.hxim" IMAGE']

    def options(lines):
        return ['.OPTIONAL HEXMG_OPT 1', *lines, '.END OPTIONAL']

    def deck(t=25, k=2, upper='*', lower='-273.15', law=None, delta=0, extra=()):
        return ['HEXMG 1.0']+options([f'FUNCTION_GUARD THK0 P0 V2 {lower} {upper}', *extra])+[
            f'.RAILS=3 R1={t:.17g} R2={t+delta:.17g}', '.FUNCTION THK0 P=3 V=3',
            *(law or [f'_COPY RET {k:.17g}']), '_ADDC V2 RET 0', '_SUB V0 P1 P0',
            '_MUL RET RET V0', '_MUL RET RET P2', '.END FUNCTION THK0',
            '.MODEL M SUBCIRCUIT N=0 X=0', 'F FCI P=1 F=THK0(X0 X1 P1) R1 R2 0 1',
            '.END MODEL M', '.CREATE circuit M GND=R0',
            f'.PROBE IMAGE HXIM DEF DOUBLE I {ids} circuit', '.PROBE IMAGE HXIM NODES circuit F.X0',
            '.RUN circuit DC INITIAL']+save

    def run(name, lines, expected=None, error=None):
        folder = root/name
        folder.mkdir()
        (folder/'input.hmg').write_text('\n'.join(lines)+'\n', encoding='ascii')
        p = subprocess.run([str(exe), 'input.hmg'], cwd=folder, capture_output=True, timeout=20)
        (folder/'stdout.txt').write_bytes(p.stdout)
        (folder/'stderr.txt').write_bytes(p.stderr)
        path = folder/'out.hxim'
        values = []
        if path.exists():
            data = path.read_bytes()
            values = list(struct.unpack('<'+'d'*((len(data)-80)//8), data[80:]))
        ok = p.returncode == 1 and not path.exists() and error.lower().encode() in p.stderr.lower() if error else p.returncode == 0
        if expected is not None:
            ok &= len(values)==1 and math.isclose(values[0],expected,rel_tol=1e-7,abs_tol=1e-9)
        cases.append(dict(name=name, passed=bool(ok), exit_code=p.returncode, values=values, stderr=p.stderr.decode(errors='replace')))
        print(name, 'PASS' if ok else 'FAIL', flush=True)

    run('zero_flow_valid', deck(), 0)
    run('positive_flow', deck(delta=10), 20)
    run('negative_flow', deck(delta=-10), -20)
    for k in (0, -2):
        for d in (0, 10):
            run(f'bad_k_{k}_delta_{d}', deck(k=k, delta=d), error='finite and positive')
    for t in (10., 25.):
        run('endpoint_'+str(t), deck(t=t, lower='10', upper='25'), 0)
    for t in (math.nextafter(10., -math.inf), math.nextafter(25., math.inf)):
        run('outside_'+str(t), deck(t=t, lower='10', upper='25'), error='temperature outside')
    run('absolute_zero', deck(t=-273.15), 0)
    run('below_absolute_zero', deck(t=-274), error='temperature outside')
    run('unbounded', deck(t=-300, lower='*'), 0)
    run('reciprocal_singularity', deck(t=0, law=['_COPY RET 0', '_INV RET RET']), error='finite and positive')
    run('overflow', deck(law=['_COPY RET 1e300', '_MUL RET RET RET']), error='finite and positive')
    run('underflow', deck(law=['_COPY RET 1e-300', '_MUL RET RET RET']), error='finite and positive')
    run('unassigned_local', [s for s in deck() if s != '_ADDC V2 RET 0'], error='finite and positive')
    # The guard runs on explicit function return as well as fall-through.
    early=deck(k=-1)
    early.insert(early.index('_SUB V0 P1 P0'), '_RETURN')
    run('early_return', early, error='finite and positive')
    for option in ('FUNCTION_GUARD MISSING P0 V2 * *', 'FUNCTION_GUARD THK0 P3 V2 * *',
                   'FUNCTION_GUARD THK0 P0 V99 * *', 'FUNCTION_GUARD THK0 V0 V2 * *',
                   'FUNCTION_GUARD THK0 P0 V2 10 0', 'FUNCTION_GUARD THK0 P0 V2 NAN *',
                   'FUNCTION_GUARD THK0 P0 V2 * INF', 'FUNCTION_GUARD THK0 P0 V2 * * extra'):
        lines=deck();lines[2]=option
        run('invalid_'+str(len(cases)), lines, error='function' if 'MISSING' in option or 'P3' in option or 'V99' in option else 'invalid')
    run('duplicate',deck(extra=['FUNCTION_GUARD THK0 P0 V2 * *']),error='repeated')
    implicit=deck(law=['_COPY RET 2','_ADD RET RET 1'])
    implicit[2]='FUNCTION_GUARD THK0 P0 V3 * *'
    run('implicit_constant_not_a_local',implicit,error='Invalid parameter/local index')
    # OPTIONAL may follow the function, too.
    after=deck();block=after[1:4];del after[1:4];after[after.index('.END FUNCTION THK0')+1:after.index('.END FUNCTION THK0')+1]=block
    run('late_option',after,0)
    two=deck();at=two.index('.END MODEL M');two.insert(at,'F2 FCI P=1 F=THK0(X0 X1 P1) R0 R0 0 1')
    two[2]='FUNCTION_GUARD THK0 P0 V2 10 30'
    run('second_instance',two,error='component[1]')

    def newton(law, power, upper='*', relative=True):
        lines=['HEXMG 1.0']+options([f'FUNCTION_GUARD THK0 P0 V2 0 {upper}', 'MAX_ITERATIONS 10']+
            (['CONVERGENCE RELATIVE 0 1e-6 1e-12'] if relative else []))+[
            '.RAILS=1', '.FUNCTION THK0 P=3 V=3', *law, '_ADDC V2 RET 0',
            '_SUB V0 P1 P0', '_MUL RET RET V0', '_MUL RET RET P2', '.END FUNCTION THK0',
            '.MODEL M SUBCIRCUIT N=1', 'F FCI P=1 F=THK0(X0 X1 P1) N0 GND 0 1',
            f'I I N0 GND DC0={power} DC={power}', '.END MODEL M', '.CREATE circuit M GND=R0',
            f'.PROBE IMAGE HXIM DEF DOUBLE V {ids} circuit', '.PROBE IMAGE HXIM NODES circuit N0',
            '.RUN circuit DC INITIAL ERR=1e-8']+save
        return lines
    for relative in (False, True):
        run('root_on_upper_'+str(relative), newton(['_COPY RET 2'],200,'100',relative),100)
        run('later_invalid_'+str(relative),newton(['_MULC RET P0 -0.01','_ADDC RET RET 2'],600,relative=relative),error='finite and positive')
        run('later_outside_'+str(relative),newton(['_COPY RET 2'],200,'90',relative),error='temperature outside')
    run('nonlinear_root',newton(['_MULC RET P0 .01','_ADDC RET RET 2'],300),100)
    # Synthetic derivative overflows even though the physical value is finite.
    derivative=newton(['_COPY RET 2'],0)
    derivative.insert(derivative.index('_ADDC V2 RET 0')+1,'_MULC RET P0 1e308')
    derivative.insert(derivative.index('_ADDC V2 RET 0')+2,'_MULC RET RET 1e308')
    # Keep this artificial law's return instead of converting it to heat flow.
    derivative=[s for s in derivative if s not in ('_SUB V0 P1 P0','_MUL RET RET V0','_MUL RET RET P2')]
    run('nonfinite_derivative',derivative,error='nonfinite numerical derivative')
    overflowing=deck(k=1e308,delta=10)
    run('nonfinite_current',overflowing,error='nonfinite function result')
    for relative in (False,True):
        transient=newton(['_COPY RET 2'],200,'100',relative)
        transient[-2]='.RUN circuit TIMESTEP DT=1'
        run('timestep_endpoint_'+str(relative),transient,100)
        # Valid initial DC; the DC excitation switches on for the time step.
        invalid=newton(['_COPY RET 2'],200,'90',relative)
        invalid=[s.replace('DC0=200 DC=200','DC0=0 DC=200') for s in invalid]
        invalid[-2]='.RUN circuit TIMESTEP DT=1'
        run('timestep_invalid_'+str(relative),invalid,error='temperature outside')
    # Nested custom calls retain the guard and the caller instance context.
    nested=deck(k=-1)
    pos=nested.index('.END FUNCTION THK0')+1
    nested[pos:pos]=['.FUNCTION WRAP P=3 V=0','THK0 RET P0 P1 P2','.END FUNCTION WRAP']
    nested=[s.replace('F=THK0(', 'F=WRAP(') for s in nested]
    run('nested_invalid',nested,error='finite and positive')
    run('guarded_ac', [s.replace('.RUN circuit DC INITIAL','.RUN circuit AC F=1') for s in deck()], error='DC/time steps only')
    result={'solver':str(exe),'sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'passed':sum(c['passed'] for c in cases),'total':len(cases),'cases':cases}
    (root/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(f"{result['passed']}/{result['total']} PASS")
    return 0 if all(c['passed'] for c in cases) else 1


if __name__ == '__main__':
    raise SystemExit(main())

