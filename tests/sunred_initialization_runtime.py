"""First-use SUNRED matrices: zero-Jacobian subtrees and coupled radiation.
Runs fresh processes, compares numeric outputs, and switches DC/AC repeatedly.
The old bug is allocator-dependent; one passing old run does not disprove it.
"""
import argparse, hashlib, json, math, struct, subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--solver',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--repeats',type=int,default=40)
a=p.parse_args();exe=a.solver.resolve(strict=True);root=a.output.resolve();root.mkdir(parents=True,exist_ok=False)
fixture=(Path(__file__).parent/'fixtures/sunred_initialization_coupled.hmg').read_text()
lo,hi=25.,1000.
for _ in range(100):
 t=(lo+hi)/2;power=400/(1+.02*(t-25))+.5;surface=t-power/20
 balance=5.670374419e-8*1.3*.8*((surface+273.15)**4-298.15**4)-power
 if balance>0:hi=t
 else:lo=t
t=(lo+hi)/2;power=400/(1+.02*(t-25))+.5;expected=[t,t-power/20,20/(1+.02*(t-25))]
results=[]
for mode in ('EXTENDED','DOUBLE','AUTO'):
 for i in range(a.repeats):
  d=root/f'{mode}-{i}';d.mkdir()
  text=fixture.replace('.OPTIONAL HEXMG_OPT 1',f'.OPTIONAL HEXMG_OPT 1\nREDUCTION_PRECISION LEVEL0 {mode}')
  (d/'input.hmg').write_text(text,encoding='ascii')
  run=subprocess.run([str(exe),'input.hmg'],cwd=d,capture_output=True,timeout=20)
  actual=[]
  if (d/'result.hxim').exists():actual=list(struct.unpack('<3d',(d/'result.hxim').read_bytes()[80:]))
  ok=run.returncode==0 and len(actual)==3 and all(math.isfinite(x) and abs(x-y)<1e-7*max(1,abs(y)) for x,y in zip(actual,expected))
  results.append(dict(case=d.name,passed=ok,exit=run.returncode,actual=actual,expected=expected))
  if not ok:(d/'solver.log').write_bytes(run.stdout+run.stderr)
 # A subtree consists solely of two ideal current sources (zero Jacobian).
 for i in range(a.repeats):
  d=root/f'{mode}-zero-{i}';d.mkdir()
  text=f"""HEXMG 1.0
.OPTIONAL HEXMG_OPT 1
REDUCTION_PRECISION LEVEL0 {mode}
.END OPTIONAL
.SUNREDTREE T
RED 1 0 0 0 0 1
RED 2 0 1 0 0 2
.END SUNREDTREE T
.MODEL M SUBCIRCUIT N=1 SUNRED=T
I0 I N0 GND 1 1 2 0
I1 I N0 GND 0 0 0 0
G G N0 GND .1
.END MODEL M
.CREATE circuit M GND=R0
.PROBE U V circuit N0
"""
  refs={}
  for j in range(3):
   text+=f'.RUN circuit DC INITIAL ITER\n.SAVE RAW FILE="dc{j}.res" U\n.RUN circuit AC F=100\n.SAVE RAW FILE="ac{j}.res" U\n'
   refs[f'dc{j}.res']=[10.];refs[f'ac{j}.res']=[100.,20.,0.]
  (d/'input.hmg').write_text(text,encoding='ascii')
  run=subprocess.run([str(exe),'input.hmg'],cwd=d,capture_output=True,timeout=20);ok=run.returncode==0
  for file,reference in refs.items():
   values=[float(v) for v in (d/file).read_text().split()] if (d/file).exists() else []
   ok=ok and len(values)==len(reference) and all(math.isfinite(x) and abs(x-y)<1e-8 for x,y in zip(values,reference))
  results.append(dict(case=d.name,passed=ok,exit=run.returncode))
  if not ok:(d/'solver.log').write_bytes(run.stdout+run.stderr)
report=dict(solver=str(exe),sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),results=results)
(root/'summary.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
failed=[r['case'] for r in results if not r['passed']]
print(f'{len(results)-len(failed)}/{len(results)} SUNRED initialization runs PASS; failures={failed}')
raise SystemExit(bool(failed))
