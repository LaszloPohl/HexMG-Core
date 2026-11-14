"""Supplied and live operating points; AC must preserve subsequent RC history."""
import argparse, cmath, json, math, subprocess
from pathlib import Path

p=argparse.ArgumentParser();p.add_argument('--solver',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();exe=a.solver.resolve(strict=True);root=a.output.resolve();root.mkdir(parents=True,exist_ok=False);cases=[]
base='''HEXMG 1.0
.FUNCTION F P=1 V=1
_MUL RET P0 P0
_ADD RET RET P0
_NEG RET RET
.END FUNCTION F
.FUNCTION DF P=1 V=0
_MULC RET P0 -2
_SUBC RET RET 1
.END FUNCTION DF
.OPTIONAL HEXMG_OPT 1
FUNCTION_DERIVATIVE F P0 DF
MAX_ITERATIONS 300
OPTIONS
.END OPTIONAL
TREE
.MODEL M SUBCIRCUIT N=1 TREEMODEL
F FCI F=F(X0) N0 R0 0
I I N0 R0 9 9 1 0
C C N0 R0 .2
.END MODEL M
.CREATE C M GND=R0
.PROBE V V C N0
.PROBE CI I C C.X0
COMMANDS
.END
'''
def deck(commands,options='',tree=False):
    return base.replace('OPTIONS',options).replace('COMMANDS',commands).replace('TREEMODEL','SUNRED=T' if tree else '').replace('TREE','.SUNREDTREE T\nRED 1 0 0 0 0 1\nRED 2 0 1 0 0 2\n.END SUNREDTREE T' if tree else '')
def ac(v,f=1):
    z=1/(1+2*v+2j*math.pi*f*.2);return [f,z.real,z.imag]
def run(name,text,expected=None,message=None):
    folder=root/name;folder.mkdir();(folder/'input.hmg').write_text(text,encoding='ascii')
    r=subprocess.run([str(exe),'input.hmg'],cwd=folder,capture_output=True,timeout=15)
    (folder/'stdout.txt').write_bytes(r.stdout);(folder/'stderr.txt').write_bytes(r.stderr)
    values={};failures=[];checks=0
    if expected is None:
        checks+=1
        if r.returncode!=1 or (folder/'result.txt').exists():failures.append('Invalid input not rejected before output')
        if message and message.encode() not in r.stderr:failures.append('Expected error message missing')
    else:
        if r.returncode or r.stderr:failures.append(f'Exit {r.returncode}: '+r.stderr.decode(errors='replace'))
        for name,want in expected.items():
            q=folder/name;got=list(map(float,q.read_text().split())) if q.exists() else [];values[name]=got
            if len(got)!=len(want):failures.append(f'{name}: {len(got)} != {len(want)} values');continue
            for i,(x,y) in enumerate(zip(got,want)):
                checks+=1
                if not math.isfinite(x) or abs(x-y)>1e-10+2e-8*abs(y):failures.append(f'{name}[{i}]: {x} != {y}')
    cases.append(dict(name=folder.name,passed=not failures,checks=checks,failures=failures,values=values));print(folder.name,'PASS' if not failures else 'FAIL '+str(failures[:2]),flush=True)
option='SMALL_SIGNAL_OPERATING_POINT C'
save='.SAVE RAW FILE="result.txt" V'
guard='FUNCTION_RANGE F P0 0 10\nFUNCTION_RANGE F RET -100 0'
for tree in (False,True):
    suffix='-tree' if tree else '-direct'
    for selected in (False,True):
        options=(option+'\n' if selected else '')+guard
        run('loaded'+str(selected)+suffix,deck('.SET C.N0 2\n.RUN C AC F=1 ERR=1e-11\n'+save,options,tree),{'result.txt':ac(2 if selected else (math.sqrt(37)-1)/2)})
        run('live-dc'+str(selected)+suffix,deck('.RUN C DC INITIAL ERR=1e-11\n.RUN C AC F=1 ERR=1e-11\n'+save,options,tree),{'result.txt':ac((math.sqrt(37)-1)/2)})
    run('changed-supplied'+suffix,deck('.SET C.N0 2\n.RUN C AC F=1 ERR=1e-11\n'+save+'\n.SET C.N0 1\n.RUN C AC F=2\n.SAVE RAW APPEND FILE="result.txt" V',option+'\n'+guard,tree),{'result.txt':ac(2)+ac(1,2)})
    # Legacy converter junctions use an artificial first-iteration secant.
    # Loading a physical state does not run DC and leaves the counter at zero;
    # AC must nevertheless evaluate and differentiate the physical branch.
    startup=deck('.SET C.N0 2\n.RUN C AC F=1\n'+save,option,tree)
    startup=startup.replace('_MUL RET P0 P0','_C_CURR_ITER V0\n_COPY RET 0\n_JNEQ PHYSICAL: V0 RET\n_MULC RET P0 -100\n_RETURN\nPHYSICAL: _MUL RET P0 P0')
    startup=startup.replace('.FUNCTION DF P=1 V=0','.FUNCTION DF P=1 V=1').replace('_MULC RET P0 -2','_C_CURR_ITER V0\n_COPY RET 0\n_JNEQ PHYSICALD: V0 RET\n_COPY RET -100\n_RETURN\nPHYSICALD: _MULC RET P0 -2')
    run('loaded-startup-secant'+suffix,startup,{'result.txt':ac(2)})
    # Backward Euler: V^2 + (C/dt)V = I + (C/dt)Vold.
    v1=(math.sqrt(45)-3)/2;v2=(math.sqrt(45+8*v1)-3)/2
    commands='.RUN C TIMESTEP INITIAL DT=.1 ERR=1e-11\n.SAVE RAW FILE="time.txt" V CI\n.RUN C AC F=1 ERR=1e-11\n'+save+'\n.RUN C TIMESTEP DT=.1 ERR=1e-11\n.SAVE RAW APPEND FILE="time.txt" V CI'
    run('live-time'+suffix,deck(commands,'TRANSIENT_INITIAL_STATE C DEFAULT_RAILS\n'+option+'\n'+guard,tree),{'result.txt':ac(v1),'time.txt':[.1,.1,v1,-2*v1,.2,.1,v2,-2*(v2-v1)]})
    # Starting AC in C must not suppress the initial DC of unrelated D.
    commands='.CREATE D M GND=R0\n.PROBE VD V D N0\n.RUN C DC INITIAL ERR=1e-11\n.RUN D AC F=1 ERR=1e-11\n.SAVE RAW FILE="result.txt" VD'
    run('separate-circuit'+suffix,deck(commands,'',tree),{'result.txt':ac((math.sqrt(37)-1)/2)})
    # AC cannot change the capacitor current available for a following DC observer.
    # The repeated time-step values above also check unchanged step-start voltages.
run('guarded-ac-amplitude',deck('.SET C.N0 2\n.RUN C AC F=1 ERR=1e-11\n'+save,option+'\n'+guard).replace('9 9 1 0','9 9 999 0'),{'result.txt':[1]+[x*999 for x in ac(2)[1:]]})
for name,options,command,message in [
    ('bad-state',option+'\n'+guard,'.SET C.N0 -1','FUNCTION_RANGE'),
    ('bad-circuit','SMALL_SIGNAL_OPERATING_POINT MISSING','','SMALL_SIGNAL_OPERATING_POINT'),
    ('duplicate',option+'\n'+option,'','HEXMG_OPT'),
    ('missing-argument','SMALL_SIGNAL_OPERATING_POINT','','HEXMG_OPT'),
    ('extra-argument',option+' EXTRA','','HEXMG_OPT'),
    ('bad-derivative',option+'\n'+guard,'.SET C.N0 2','FUNCTION_GUARD')]:
    text=deck(command+'\n.RUN C AC F=1 ERR=1e-11\n'+save,options)
    if name=='bad-derivative':text=text.replace('_MULC RET P0 -2','_COPY RET 1e308\n_MUL RET RET RET')
    run(name,text,message=message)
out=dict(solver=str(exe),passed=sum(c['passed'] for c in cases),total=len(cases),checks=sum(c['checks'] for c in cases),cases=cases)
(root/'summary.json').write_text(json.dumps(out,indent=2)+'\n');print(f"{out['passed']}/{out['total']}; {out['checks']} checks");raise SystemExit(0 if out['passed']==out['total'] else 1)
