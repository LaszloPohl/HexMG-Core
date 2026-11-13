"""Mixed-field initial state: independent RC equations, restart and syntax guards."""
import argparse, json, math, struct, subprocess
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('--solver',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args();exe=a.solver.resolve(strict=True);root=a.output.resolve();root.mkdir(parents=True,exist_ok=False)
ids='00112233445546778899AABBCCDDEEFF F47AC10B58CC4372A5670E02B2C3D479'
cases=[]

def run(name,option,commands,expected=None):
    directory=root/name;directory.mkdir()
    deck=['HEXMG 1.0','.RAILS=3 R0=0 R1=25 R2=45','.OPTIONAL HEXMG_OPT 1',
          'CONVERGENCE RELATIVE 0 1e-10 1e-12','MAX_ITERATIONS 40',*option,'.END OPTIONAL',
          '.MODEL CHILD SUBCIRCUIT X=2 N=1',
          '.DEFAULTRAILRANGE R1 N0 N0',
          'CT C N0 R1 1','GT G N0 R1 1','IT I N0 R1 1 1 0 0',
          'CE C X0 R0 1','GE G X0 R0 1','IE I X0 R0 2 2 0 0',
          'CF C X1 R1 1','.END MODEL CHILD',
          '.MODEL ROOT SUBCIRCUIT N=1','INNER MODEL CHILD N0 R2','.END MODEL ROOT',
          '.CREATE CIRCUIT ROOT GND=R0',
          f'.PROBE P HXIM DEF DOUBLE V {ids} CIRCUIT',
          '.PROBE P HXIM NODES CIRCUIT N0 INNER.N0 INNER.X1',*commands]
    (directory/'input.hmg').write_text('\n'.join(deck)+'\n',encoding='ascii')
    proc=subprocess.run([str(exe),'input.hmg'],cwd=directory,capture_output=True,timeout=15)
    (directory/'stdout.txt').write_bytes(proc.stdout);(directory/'stderr.txt').write_bytes(proc.stderr)
    actual=[]
    if expected is not None:
        for i in range(len(expected)):
            file=directory/f'{i}.hxim'
            actual.append(list(struct.unpack('<3d',file.read_bytes()[80:])) if file.exists() else [])
        ok=proc.returncode==0 and not proc.stderr and len(actual)==len(expected) and all(
            len(x)==3 and all(math.isclose(v,w,rel_tol=1e-10,abs_tol=1e-10) for v,w in zip(x,y)) for x,y in zip(actual,expected))
    else: ok=proc.returncode==1 and not list(directory.glob('*.hxim'))
    cases.append(dict(name=name,passed=bool(ok),exit=proc.returncode,actual=actual,expected=expected))
    print(name,'PASS' if ok else 'FAIL',flush=True)

steps=[]
for i in range(2):steps += ['.RUN CIRCUIT TIMESTEP DT=1 ERR=1e-10',f'.SAVE HXIM FILE="{i}.hxim" P']
steps += ['.RUN CIRCUIT TIMESTEP INITIAL DT=1 ERR=1e-10','.SAVE HXIM FILE="2.hxim" P',
          '.RAILS R1=30','.RUN CIRCUIT TIMESTEP INITIAL DT=1 ERR=1e-10','.SAVE HXIM FILE="3.hxim" P']
run('mixed-and-restart',['TRANSIENT_INITIAL_STATE CIRCUIT DEFAULT_RAILS'],steps,
    [[1,25.5,45],[1.5,25.75,45],[1,25.5,45],[1,30.5,45]])
# The legacy instruction intentionally remains uniform, including electrical
# N0 in this diagnostic mixed deck. This documents why it is thermal-only.
run('legacy-unchanged',['TRANSIENT_INITIAL_TEMPERATURE CIRCUIT 25'],steps[:2],[[13.5,25.5,45]])
run('legacy-default',['TRANSIENT_INITIAL_STATE CIRCUIT DEFAULT_RAILS'],
    ['.RUN CIRCUIT DC INITIAL ERR=1e-10',*steps[:2]],[[1,25.5,45]])
for name,options in [
    ('bad-mode',['TRANSIENT_INITIAL_STATE CIRCUIT 25']),
    ('extra',['TRANSIENT_INITIAL_STATE CIRCUIT DEFAULT_RAILS 25']),
    ('unknown',['TRANSIENT_INITIAL_STATE MISSING DEFAULT_RAILS']),
    ('duplicate',['TRANSIENT_INITIAL_STATE CIRCUIT DEFAULT_RAILS']*2),
    ('mixed-duplicate',['TRANSIENT_INITIAL_TEMPERATURE CIRCUIT 25','TRANSIENT_INITIAL_STATE CIRCUIT DEFAULT_RAILS']),
    ('mixed-duplicate-reverse',['TRANSIENT_INITIAL_STATE CIRCUIT DEFAULT_RAILS','TRANSIENT_INITIAL_TEMPERATURE CIRCUIT 25'])]:
    run(name,options,steps[:2])
(root/'summary.json').write_text(json.dumps(cases,indent=2)+'\n',encoding='utf-8')
assert all(c['passed'] for c in cases),cases
print(f'PASS {len(cases)} initialization cases')
