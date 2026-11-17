"""FCI control columns must not report fictitious subcircuit terminal currents."""
from pathlib import Path
import argparse, json, math, subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--solver',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();exe=a.solver.resolve(strict=True);a.output.mkdir(parents=True,exist_ok=False)
reports=[]
for sign in (-1,1):
    folder=a.output/str(sign);folder.mkdir()
    deck=f'''HEXMG 1.0
.FUNCTION F P=1 V=0
_MULC RET P0 2
.END FUNCTION F
.MODEL A SUBCIRCUIT X=2
G G X0 R0 1
F FCI Y=1 F=F(Y0) X0 R0 X1 0
.END MODEL A
.MODEL M SUBCIRCUIT N=2
CTRL I N1 R0 DC0={3*sign} DC={3*sign} AC={sign}
GC G N1 R0 1
A MODEL A N0 N1
.END MODEL M
.CREATE circuit M GND=R0
.PROBE V V circuit N0
.PROBE C V circuit N1
.PROBE I I circuit A.X1
.RUN circuit DC INITIAL ERR=1e-10
.SAVE RAW MAXRESULTSPERROW=3 FILE="dc.res" V C I
.RUN circuit TIMESTEP DT=.01 ERR=1e-10
.SAVE RAW MAXRESULTSPERROW=3 FILE="time.res" V C I
.RUN circuit AC F=100
.SAVE RAW MAXRESULTSPERROW=3 FILE="ac.res" V C I
'''
    (folder/'input.hmg').write_text(deck,encoding='ascii')
    result=subprocess.run([str(exe),'input.hmg'],cwd=folder,capture_output=True,timeout=10)
    (folder/'solver.log').write_bytes(result.stdout+result.stderr)
    for phase,expected in [('dc',[6*sign,3*sign,0]),('time',[.01,.01,6*sign,3*sign,0]),('ac',[100,2*sign,0,sign,0,0,0])]:
        f=folder/(phase+'.res');actual=[float(x) for x in f.read_text().split()] if f.exists() else []
        passed=result.returncode==0 and len(actual)==len(expected) and all(math.isclose(x,y,rel_tol=2e-7,abs_tol=1e-10) for x,y in zip(actual,expected))
        reports.append(dict(sign=sign,phase=phase,passed=passed,actual=actual,expected=expected));print(sign,phase,'PASS' if passed else 'FAIL')
(a.output/'summary.json').write_text(json.dumps(reports,indent=2))
assert all(r['passed'] for r in reports),reports
print(len(reports),'FCI control-current checks PASS')
