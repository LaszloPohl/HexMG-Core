"""FUNCTION_RANGE real parser/runtime checks. Fresh output directory required."""
import argparse,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--solver',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
exe=a.solver.resolve(strict=True);root=a.output.resolve();root.mkdir(parents=True,exist_ok=False);results=[]
def deck(options=('FUNCTION_RANGE F P0 10 30','FUNCTION_RANGE F V0 10 30','FUNCTION_RANGE F RET 0 *'),t=25,body=None):
 return ['HEXMG 1.0','.OPTIONAL HEXMG_OPT 1',*options,'.END OPTIONAL',f'.RAILS=2 R1={t} R2=25','.FUNCTION F P=1 V=1',*(body or ['_ADDC V0 P0 0','_COPY RET 0']),'.END FUNCTION F','.MODEL M SUBCIRCUIT N=1 B=1 X=0','.DEFAULTRAILRANGE R2 N0 N0','H G N0 R1 1','L G N0 GND B0','C FCB A=1 F=F(A1) B0 R1','.END MODEL M','.CREATE circuit M GND=R0','.PROBE IMAGE HXIM DEF DOUBLE V 00112233445546778899AABBCCDDEEFF F47AC10B58CC4372A5670E02B2C3D479 circuit','.PROBE IMAGE HXIM NODES circuit N0','.RUN circuit DC INITIAL ITERS=3','.SAVE HXIM FILE="out.hxim" IMAGE']
def run(name,lines,error=None):
 d=root/name;d.mkdir();(d/'input.hmg').write_text('\n'.join(lines)+'\n',encoding='ascii')
 r=subprocess.run([str(exe),'input.hmg'],cwd=d,capture_output=True,timeout=20)
 (d/'stdout.txt').write_bytes(r.stdout);(d/'stderr.txt').write_bytes(r.stderr)
 ok=(r.returncode==0 and (d/'out.hxim').exists()) if error is None else (r.returncode!=0 and not (d/'out.hxim').exists() and error.lower() in r.stderr.decode(errors='replace').lower())
 results.append({'name':name,'passed':ok,'exit':r.returncode});print(name,'PASS' if ok else 'FAIL',flush=True)
run('valid_zero',deck())
for t in (10,30):run('endpoint'+str(t),deck(t=t))
for t in (9,31):run('outside'+str(t),deck(t=t), 'FUNCTION_RANGE')
run('signed_finite_result',deck(('FUNCTION_RANGE F RET * *',),body=['_COPY RET -0.25']),None)
for name,body in [('nan',['_COPY RET 0','_DIV RET RET RET']),('inf',['_COPY RET 1e300','_MUL RET RET RET']),('unset',['_COPY V0 25']),('early',['_RETURN'])]:
 run(name,deck(('FUNCTION_RANGE F RET * *',),body=body),'FUNCTION_RANGE')
run('unset_local',deck(('FUNCTION_RANGE F V0 * *',),body=['_COPY RET 0']),'FUNCTION_RANGE')
run('negative_rejected',deck(('FUNCTION_RANGE F RET 0 *',),body=['_COPY RET -0.25']),'FUNCTION_RANGE')
run('computed_outside',deck(('FUNCTION_RANGE F V0 200 2000',),body=['_COPY V0 2001','_COPY RET 0']),'FUNCTION_RANGE')
run('parameter_input_not_mutated',deck(('FUNCTION_RANGE F P0 10 30',),body=['_COPY P0 100','_COPY RET 0']))
for text in ['FUNCTION_RANGE UNKNOWN RET * *','FUNCTION_RANGE F P1 * *','FUNCTION_RANGE F V1 * *','FUNCTION_RANGE F P-1 * *','FUNCTION_RANGE F RET 30 10','FUNCTION_RANGE F RET NAN *','FUNCTION_RANGE F RET * INF','FUNCTION_RANGE F X0 * *','FUNCTION_RANGE F RET * * extra','FUNCTION_RANGE F RET *','FUNCTION_RANGE F P0x * *']:
 run('parse'+str(len(results)),deck((text,)),'function' if 'UNKNOWN' in text or ' P1 ' in text or ' V1 ' in text else 'invalid')
run('duplicate',deck(('FUNCTION_RANGE F P0 * *','FUNCTION_RANGE F P00 * *')),'repeated')
late=deck();block=late[1:6];del late[1:6];index=late.index('.END FUNCTION F')+1;late[index:index]=block
run('late_optional',late)
run('old_and_new_guard',deck(('FUNCTION_GUARD F P0 V0 10 30','FUNCTION_RANGE F RET 0 *')))
step=deck(t=40,options=('FUNCTION_RANGE F P0 0 30',));step[step.index('C FCB A=1 F=F(A1) B0 R1')]='C FCB A=1 F=F(A1) B0 N0'
run('reject_after_update_before_save',step,'FUNCTION_RANGE')
nested=deck(('FUNCTION_RANGE F P0 10 30','FUNCTION_RANGE F RET 0 *'),body=['_ADDC RET P0 0'])
i=nested.index('.MODEL M SUBCIRCUIT N=1 B=1 X=0')
nested[i:i]=['.FUNCTION OUTER P=1 V=0','_ADDC RET P0 0','F RET RET','.END FUNCTION OUTER']
nested[nested.index('C FCB A=1 F=F(A1) B0 R1')]='C FCB A=1 F=OUTER(A1) B0 R1'
run('nested_in_place_parameter',nested)
transient=deck(options=('FUNCTION_RANGE F P0 * 25',))
transient[transient.index('C FCB A=1 F=F(A1) B0 R1')]='C FCB A=1 F=F(A1) B0 N0'
i=transient.index('.END MODEL M');transient[i:i]=['P I N0 GND DC0=0 DC=.1','CAP C N0 GND .2']
i=transient.index('.SAVE HXIM FILE="out.hxim" IMAGE');transient[i:i]=['.RUN circuit TIMESTEP DT=1 ITERS=3']
run('timestep_invalid_before_save',transient,'FUNCTION_RANGE')
(root/'summary.json').write_text(json.dumps(results,indent=2)+'\n')
assert all(x['passed'] for x in results),[x['name'] for x in results if not x['passed']]
print('PASS',len(results),'runtime cases')
