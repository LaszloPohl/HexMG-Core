"""Independent FUNCTION jump/parser regression; baseline remains untouched."""
import argparse,json,struct,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--solver',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
p.add_argument('--expect-failure',action='store_true');a=p.parse_args();exe=a.solver.resolve(strict=True);root=a.output.resolve();root.mkdir(parents=True,exist_ok=False)
cases=[]
for mode in ('EXTENDED','DOUBLE','AUTO'):
 for kind in ('forward','backward'):
  for value in (-3,0,7):
   if kind=='forward':
    body=['_COPY RET -999','_JMP START:','_DIVC RET P0 0','START: _ADDC V0 P0 1',
     '_GT0 V1 V0','_JEQ0 NEG: V1','_MULC RET V0 2','_JMP END:',
     'NEG: _MULC RET V0 -3','END: _ADDC RET RET .25']
    expected=(2*(value+1) if value+1>0 else -3*(value+1))+.25
   else:
    body=['_COPY V0 0','_COPY RET 0','LOOP: _ADDC V0 V0 1','_ADD RET RET V0','_COPY V1 4',
     '_JGE END: V0 V1','_JMP LOOP:','END: _ADD RET RET P0'];expected=10+value
   text=['HEXMG 1.0','.OPTIONAL HEXMG_OPT 1',f'REDUCTION_PRECISION LEVEL0 {mode}','.END OPTIONAL',
    '.FUNCTION F P=1 V=2',*body,'.END FUNCTION F','.MODEL M SUBCIRCUIT N=1 B=1',
    f'C FCB P=1 F=F(P0) B0 {value}','R R N0 GND 1','I I GND N0 1','.END MODEL M','.CREATE circuit M GND=R0',
    '.PROBE IMAGE HXIM DEF DOUBLE V 00112233445546778899AABBCCDDEEFF F47AC10B58CC4372A5670E02B2C3D479 circuit',
    '.PROBE IMAGE HXIM NODES circuit N0 B0','.RUN circuit DC INITIAL ERR=1e-9','.SAVE HXIM FILE="out.hxim" IMAGE']
   d=root/f'{mode}-{kind}-{value}';d.mkdir();(d/'input.hmg').write_text('\n'.join(text)+'\n',encoding='ascii')
   run=subprocess.run([str(exe),'input.hmg'],cwd=d,capture_output=True,timeout=10)
   (d/'stdout.txt').write_bytes(run.stdout);(d/'stderr.txt').write_bytes(run.stderr)
   actual=[]
   if run.returncode==0:actual=list(struct.unpack('<2d',(d/'out.hxim').read_bytes()[80:]))
   ok=run.returncode!=0 if a.expect_failure else len(actual)==2 and abs(actual[0]+1)<1e-9 and abs(actual[1]-expected)<1e-9
   cases.append(dict(name=d.name,exit=run.returncode,actual=actual,expected=[-1,expected],passed=ok))
(root/'summary.json').write_text(json.dumps(cases,indent=2)+'\n',encoding='utf-8')
assert all(c['passed'] for c in cases),cases
print(f'{len(cases)} FUNCTION jump cases: '+('old failure reproduced' if a.expect_failure else 'PASS, 36 independent values'))
