"""CONTROLLER RET/P LOAD/STORE bounds and multi-output compatibility."""
import argparse,json,struct,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--solver',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--record-old',action='store_true');a=p.parse_args()
exe=a.solver.resolve(strict=True);root=a.output.resolve();root.mkdir(parents=True,exist_ok=False);results=[]
for mode in ('EXTENDED','DOUBLE','AUTO'):
 for kind in ('valid-custom','valid-builtin','short','load-extra','store-extra','store-placeholders','missing-function'):
  function='.FUNCTION F P=2 V=1\n_ADD RET P0 P1\n_MULC P0 P0 2\n_MULC P1 P1 -1\n.END FUNCTION F\n'
  selection='FUNCTION F';load='LOAD - P0 P1';store='STORE A0 A1 -';expected=[8,6]
  if kind=='valid-builtin':selection='FUNCTION _MUL';expected=[15,3]
  if kind=='short':load='LOAD - P0';store='STORE A0';expected=[3,0]
  if kind=='load-extra':load+=' P0'
  if kind=='store-extra':store+=' A1'
  if kind=='store-placeholders':store+=' -'
  if kind=='missing-function':selection=''
  invalid=kind not in ('valid-custom','valid-builtin','short')
  text=f'''HEXMG 1.0
.OPTIONAL HEXMG_OPT 1
REDUCTION_PRECISION LEVEL0 {mode}
.END OPTIONAL
{function}.MODEL C CONTROLLER A=2 P=2
{selection}
{load}
{store}
.END MODEL C
.MODEL M SUBCIRCUIT N=1 B=2
C0 MODEL C B0 B1 3 5
R R N0 GND 1
I I GND N0 1
.END MODEL M
.CREATE circuit M GND=R0
.PROBE IMAGE HXIM DEF DOUBLE V 00112233445546778899AABBCCDDEEFF F47AC10B58CC4372A5670E02B2C3D479 circuit
.PROBE IMAGE HXIM NODES circuit B0 B1
.RUN circuit DC INITIAL ITER
.SAVE HXIM FILE="out.hxim" IMAGE
'''
  d=root/f'{mode}-{kind}';d.mkdir();(d/'input.hmg').write_text(text,encoding='ascii')
  proc=subprocess.run([str(exe),'input.hmg'],cwd=d,capture_output=True,timeout=10);log=proc.stdout+proc.stderr;(d/'solver.log').write_bytes(log)
  actual=list(struct.unpack('<2d',(d/'out.hxim').read_bytes()[80:])) if (d/'out.hxim').exists() else []
  ok=(proc.returncode==1 and not actual and b'CONTROLLER' in log and (b'line' in log)) if invalid else proc.returncode==0 and actual==expected
  results.append(dict(name=d.name,invalid=invalid,exit=proc.returncode,actual=actual,expected=expected,passed=ok))
(root/'summary.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
if not a.record_old:assert all(x['passed'] for x in results),results
print(f"{len(results)} controller list cases: {sum(x['passed'] for x in results)} PASS"+(' (old behaviour recorded)' if a.record_old else ''))
