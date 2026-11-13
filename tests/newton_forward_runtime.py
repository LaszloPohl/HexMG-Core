"""Forward-only Newton damping: shared correction, nested/aliased ports, RC history."""
import argparse,json,math,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--solver',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();exe=a.solver.resolve(strict=True);root=a.output.resolve();root.mkdir(parents=True,exist_ok=False);cases=[]
base='''HEXMG 1.0
.RAILS=2 R1=5
.OPTIONAL HEXMG_OPT 1
OPTIONS
.END OPTIONAL
.MODEL CHILD SUBCIRCUIT X=2 O=1
G0 G X0 R0 1
G1 G X1 R0 1
GO G O0 R0 1
IO I O0 R0 DC0=4 DC=4
.END MODEL CHILD
.MODEL ROOT SUBCIRCUIT N=2
SUB MODEL CHILD N0 N1 NONE
I0 I N0 R0 DC0=1 DC=1
I1 I N1 R0 DC0=3 DC=3
.END MODEL ROOT
.CREATE C ROOT GND=R0
.PROBE P V C N0 N1 SUB.O0
.RUN C DC INITIAL ITERS=1
.SAVE RAW FILE="result.txt" P
'''
def run(name,options,expected,deck=base,message=None):
    folder=root/name;folder.mkdir();(folder/'input.hmg').write_text(deck.replace('OPTIONS','\n'.join(options)),encoding='ascii')
    result=subprocess.run([str(exe),'input.hmg'],cwd=folder,capture_output=True,timeout=10)
    (folder/'stdout.txt').write_bytes(result.stdout);(folder/'stderr.txt').write_bytes(result.stderr)
    output=folder/'result.txt';values=list(map(float,output.read_text().split())) if output.exists() else []
    ok=(result.returncode==1 and not output.exists()) if expected is None else (
        result.returncode==0 and len(values)==len(expected) and all(math.isclose(v,e,rel_tol=1e-11,abs_tol=1e-12) for v,e in zip(values,expected)))
    if message:ok &= message.encode() in result.stderr
    cases.append(dict(name=name,passed=ok,values=values,expected=expected,exit=result.returncode));print(name,'PASS' if ok else 'FAIL')
limit='NEWTON_FORWARD_LIMIT C SUB.X0 R0 0.2'
run('unchanged',[],[1,3,4])
run('shared-correction-and-output-port',[limit],[.2,.6,.8])
run('two-updates',[limit],[.4,1.2,1.6],base.replace('ITERS=1','ITERS=2'))
run('multiple-minimum',[limit,'NEWTON_FORWARD_LIMIT C N1 R0 0.3'],[.1,.3,.4])
run('negative-unlimited',[limit],[-1,3,4],base.replace('DC0=1 DC=1','DC0=-1 DC=-1'))
run('reversed-pair',['NEWTON_FORWARD_LIMIT C R0 N0 0.2'],[-.2,.6,.8],base.replace('DC0=1 DC=1','DC0=-1 DC=-1'))
run('nonzero-fixed-rail',['NEWTON_FORWARD_LIMIT C N0 R1 0.2'],[.2,.6,.8])
run('relative',[limit,'CONVERGENCE RELATIVE 0 1e-10 1e-12'],[.2,.6,.8])
run('converged',[limit],[1,3,4],base.replace('ITERS=1','ERR=1e-10'))
run('relative-converged',[limit,'CONVERGENCE RELATIVE 0 1e-10 1e-12'],[1,3,4],base.replace('ITERS=1','ERR=1e-10'))
# Separate circuit must not inherit damping from C.
other=base.replace('.PROBE P V C N0 N1 SUB.O0','.CREATE D ROOT GND=R0\n.PROBE P V D N0 N1 SUB.O0').replace('.RUN C','.RUN D')
run('other-circuit',[limit],[1,3,4],other)
for name,line in [('zero','C N0 R0 0'),('negative','C N0 R0 -1'),('nan','C N0 R0 NAN'),('inf','C N0 R0 INF'),
    ('missing','C N0 R0'),('extra','C N0 R0 0.2 EXTRA'),('circuit','D N0 R0 .2'),('node','C N9 R0 .2'),
    ('identical','C N0 N0 .2'),('state-variable','C SUB.B0 R0 .2'),('bad-path','C MISSING.X0 R0 .2')]:
    run('reject-'+name,['NEWTON_FORWARD_LIMIT '+line],None)
run('reject-duplicate',[limit,limit],None)
rc='''HEXMG 1.0
.OPTIONAL HEXMG_OPT 1
OPTIONS
TRANSIENT_INITIAL_STATE C DEFAULT_RAILS
.END OPTIONAL
.MODEL ROOT SUBCIRCUIT N=1
G G N0 R0 1
C C N0 R0 1
I I N0 R0 DC0=1 DC=1
.END MODEL ROOT
.CREATE C ROOT GND=R0
.PROBE P V C N0
.RUN C TIMESTEP DT=1 ERR=1e-10
.RUN C TIMESTEP DT=1 ERR=1e-10
.SAVE RAW FILE="result.txt" P
'''
run('time-history',['NEWTON_FORWARD_LIMIT C N0 R0 .2'],[2,1,.75],rc)
run('time-history-relative',['NEWTON_FORWARD_LIMIT C N0 R0 .2','CONVERGENCE RELATIVE 0 1e-10 1e-12'],[2,1,.75],rc)
(root/'summary.json').write_text(json.dumps(cases,indent=2)+'\n',encoding='utf-8')
assert all(c['passed'] for c in cases),[c for c in cases if not c['passed']]
print(f'PASS {len(cases)} forward limit cases')
