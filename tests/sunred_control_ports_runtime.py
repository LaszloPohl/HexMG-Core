"""A-port reports and ordered controllers are not SUNRED matrix terminals.

Independent scalar nonlinear reference, flat/tree parity in all LEVEL0 modes.
No baseline source or binary is modified.
"""
import argparse, json, math, struct, subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--solver',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();exe=a.solver.resolve(strict=True);root=a.output.resolve();root.mkdir(parents=True,exist_ok=False);results=[]
for mode in ('EXTENDED','DOUBLE','AUTO'):
 for variable in (0,7):
  for offset in (0,5):
   for tree in (False,True):
    lines=['HEXMG 1.0','.RAILS=1 R1=10','.OPTIONAL HEXMG_OPT 1',f'REDUCTION_PRECISION LEVEL0 {mode}',
     'CONVERGENCE RELATIVE 0 1e-9 1e-12','MAX_ITERATIONS 200','.END OPTIONAL',
     '.FUNCTION REPORT P=2 V=0','_ADD RET P0 P1','.END FUNCTION REPORT',
     '.FUNCTION MEAN P=2 V=1','_MULC V0 P0 .25','_MULC RET P1 .75','_ADD RET RET V0','.END FUNCTION MEAN',
     '.FUNCTION GAIN P=1 V=0','_MULC RET P0 .1','.END FUNCTION GAIN',
     '.MODEL REPORTC CONTROLLER A=2 P=1','FUNCTION REPORT','LOAD - A0 P0','STORE A1 - -','.END MODEL REPORTC',
     '.MODEL MEANC CONTROLLER A=3','FUNCTION MEAN','LOAD - A0 A1','STORE A2 - -','.END MODEL MEANC']
    for child in range(2):
     lines += [f'.MODEL C{child} SUBCIRCUIT N=0 X=1 A=1 P=1 B=1',
      f'R R X0 {"R1" if child==0 else "GND"} 1',
      f'REPORT MODEL REPORTC CTRL_LEVEL=0 X0 A0 {offset*(child+1)}',
      'CG FCB P=1 F=GAIN(P0) B0 P0','G G X0 GND B0',f'.END MODEL C{child}']
    lines += ['.SUNREDTREE T','RED 1 0 0 0 0 1','.END SUNREDTREE T',
     f'.MODEL M SUBCIRCUIT N=1 B={variable+3}'+(' SUNRED=T' if tree else ''),
     f'C0 MODEL C0 N0 B{variable} B{variable+2}',f'C1 MODEL C1 N0 B{variable+1} B{variable+2}',
     f'AVG MODEL MEANC CTRL_LEVEL=1 B{variable} B{variable+1} B{variable+2}',
     '.END MODEL M','.CREATE circuit M GND=R0',
     '.PROBE IMAGE HXIM DEF DOUBLE V 00112233445546778899AABBCCDDEEFF F47AC10B58CC4372A5670E02B2C3D479 circuit',
     f'.PROBE IMAGE HXIM NODES circuit N0 B{variable} B{variable+1} B{variable+2}',
     '.RUN circuit DC INITIAL ERR=1e-10','.SAVE HXIM FILE="out.hxim" IMAGE']
    d=root/f'{mode}-b{variable}-o{offset}-tree{int(tree)}';d.mkdir();(d/'input.hmg').write_text('\n'.join(lines)+'\n',encoding='ascii')
    proc=subprocess.run([str(exe),'input.hmg'],cwd=d,capture_output=True,timeout=20)
    (d/'stdout.txt').write_bytes(proc.stdout);(d/'stderr.txt').write_bytes(proc.stderr)
    b=2+.35*offset;v=20/(b+math.sqrt(b*b+8));expected=[v,v+offset,v+2*offset,v+1.75*offset]
    actual=[]
    if proc.returncode==0:
     payload=(d/'out.hxim').read_bytes();actual=list(struct.unpack('<4d',payload[80:]))
    ok=len(actual)==4 and all(abs(x-y)<1e-8 for x,y in zip(actual,expected))
    results.append(dict(name=d.name,passed=ok,exit=proc.returncode,expected=expected,actual=actual))
(root/'summary.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
assert all(v['passed'] for v in results),results
print(f'PASS {len(results)} control-port/SUNRED cases, {len(results)*4} independent values')
