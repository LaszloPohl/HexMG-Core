"""Initialization laws must not certify convergence of a different physical law."""
from pathlib import Path
import argparse, json, math, subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--solver',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();exe=a.solver.resolve(strict=True);a.output.mkdir(parents=True,exist_ok=False)
reports=[]
for relative in (False,True):
    for offset in (False,True):
        for exact in (False,True):
            name=f'relative{int(relative)}-offset{int(offset)}-exact{int(exact)}'
            folder=a.output/name;folder.mkdir()
            initial='_COPY RET -1' if offset else '_MULC RET P0 -0.1'
            options='CONVERGENCE RELATIVE 0 1e-9 1e-12' if relative else ''
            deck=f'''HEXMG 1.0
.FUNCTION F P=1 V=2
_C_CURR_ITER V0
_COPY V1 0
_JEQ INITIAL: V0 V1
_MUL RET P0 P0
_NEG RET RET
_RETURN
INITIAL: {initial}
.END FUNCTION F
.MODEL M SUBCIRCUIT N=1
F FCI F=F(X0) N0 R0 0
{'G G N0 R0 1' if offset else ''}
I I N0 R0 DC0=1 DC=1
.END MODEL M
.OPTIONAL HEXMG_OPT 1
MAX_ITERATIONS 30
{options}
.END OPTIONAL
.CREATE circuit M GND=R0
.PROBE U V circuit N0
.RUN circuit DC INITIAL {'ITERS=1' if exact else 'ERR=1e-7'}
.SAVE RAW FILE="result.res" U
'''
            (folder/'input.hmg').write_text(deck,encoding='ascii')
            result=subprocess.run([str(exe),'input.hmg'],cwd=folder,capture_output=True,timeout=10)
            text=(result.stdout+result.stderr).decode(errors='replace');(folder/'solver.log').write_text(text)
            values=[float(x) for x in (folder/'result.res').read_text().split()] if (folder/'result.res').exists() else []
            expected=(0 if offset else 10) if exact else ((math.sqrt(5)-1)/2 if offset else 1)
            passed=result.returncode==0 and len(values)==1 and math.isclose(values[0],expected,rel_tol=1e-8,abs_tol=1e-10)
            if relative and not exact:passed=passed and 'Relative iter 1 state=0 balance=0' not in text
            reports.append(dict(name=name,passed=passed,actual=values,expected=expected,exitCode=result.returncode))
            print(name,'PASS' if passed else 'FAIL',flush=True)
(a.output/'summary.json').write_text(json.dumps(reports,indent=2))
assert all(r['passed'] for r in reports),reports
print(len(reports),'initialization-law checks PASS')
