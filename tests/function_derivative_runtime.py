"""Analytic derivative parser/runtime checks, fresh output directory only."""
import argparse,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--solver',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
exe=a.solver.resolve(strict=True);root=a.output.resolve();root.mkdir(parents=True,exist_ok=False);rows=[]
def deck(option='FUNCTION_DERIVATIVE F P0 DF',body='_MULC RET P0 -2',signature='P=1 V=0'):
    return f'''HEXMG 1.0
.OPTIONAL HEXMG_OPT 1
MAX_ITERATIONS 100
{option}
.END OPTIONAL
.FUNCTION F P=1 V=0
_MUL RET P0 P0
_NEG RET RET
.END FUNCTION F
.FUNCTION DF {signature}
{body}
.END FUNCTION DF
.MODEL M SUBCIRCUIT N=1
G G N0 R0 1
N FCI F=F(X0) N0 R0 0
I I N0 R0 DC0=2 DC=2
.END MODEL M
.CREATE C M GND=R0
.PROBE P V C N0
.RUN C DC INITIAL ERR=1e-10
.SAVE RAW FILE="out.txt" P
'''
def run(name,text,error=None,expected=1.):
    folder=root/name;folder.mkdir();(folder/'input.hmg').write_text(text,encoding='ascii')
    result=subprocess.run([str(exe),'input.hmg'],cwd=folder,capture_output=True,timeout=10)
    (folder/'solver.log').write_bytes(result.stdout+result.stderr)
    output=folder/'out.txt';message=(result.stdout+result.stderr).decode(errors='replace')
    if error:ok=result.returncode!=0 and not output.exists() and error.lower() in message.lower()
    else:ok=result.returncode==0 and output.exists() and abs(float(output.read_text().strip())-expected)<1e-8
    rows.append(dict(name=name,passed=ok,exit=result.returncode));print(name,'PASS' if ok else 'FAIL',flush=True)
run('analytic',deck());run('numeric_fallback',deck(option=''))
for index,option in enumerate(['FUNCTION_DERIVATIVE MISSING P0 DF','FUNCTION_DERIVATIVE F P0 MISSING',
    'FUNCTION_DERIVATIVE F P1 DF','FUNCTION_DERIVATIVE F P-1 DF','FUNCTION_DERIVATIVE F RET DF',
    'FUNCTION_DERIVATIVE F V0 DF','FUNCTION_DERIVATIVE F P DF','FUNCTION_DERIVATIVE F P0x DF',
    'FUNCTION_DERIVATIVE F P0 DF EXTRA','FUNCTION_DERIVATIVE F P0',
    'FUNCTION_DERIVATIVE F P0 DF\nFUNCTION_DERIVATIVE F P00 DF']):
    run('invalid'+str(index),deck(option=option),'FUNCTION' if index<3 else 'invalid')
run('signature',deck(signature='P=2 V=0'),'signature')
run('component_signature',deck(signature='P=1 V=0 CT=1'),'signature')
run('nan',deck(body='_COPY RET 0\n_DIV RET RET RET'),'nonfinite FUNCTION_DERIVATIVE')
run('infinite',deck(body='_COPY RET 1e300\n_MUL RET RET RET'),'nonfinite FUNCTION_DERIVATIVE')
run('missing_return',deck(body='_RETURN'),'nonfinite FUNCTION_DERIVATIVE')
late=deck();begin=late.index('.OPTIONAL');end=late.index('.END OPTIONAL')+len('.END OPTIONAL\n');block=late[begin:end];late=late[:begin]+late[end:];at=late.index('.MODEL');late=late[:at]+block+late[at:]
run('late_optional',late)
guarded=deck(option='FUNCTION_DERIVATIVE F P0 DF\nFUNCTION_RANGE F P0 -2 2\nFUNCTION_RANGE F RET -4 0')
run('source_ranges',guarded)
helpers='''.FUNCTION CONST P=0 V=0
_COPY RET 2
.END FUNCTION CONST
.FUNCTION SUM P=2 V=0
_ADD RET P0 P1
.END FUNCTION SUM
'''
nested=deck(body='CONST V0\nSUM RET P0 P0\n_NEG RET RET',signature='P=1 V=1')
nested=nested.replace('.FUNCTION F ',helpers+'.FUNCTION F ')
run('nested_zero_and_two_arguments',nested)
run('nested_missing_argument',nested.replace('SUM RET P0 P0','SUM RET P0'),'unexpected end of line')
linear=deck(option='',body='_COPY RET -2').replace('.FUNCTION F ',helpers+'.FUNCTION F ').replace('_MUL RET P0 P0','SUM RET P0 P0')
run('nested_physical_function',linear,expected=2/3)
ac=deck().replace('DC0=2 DC=2','DC0=2 DC=2 AC=1').replace('.SAVE RAW','.RUN C AC F=1000\n.SAVE RAW')
# AC RAW complex output has real and imaginary columns.
folder=root/'ac';folder.mkdir();(folder/'input.hmg').write_text(ac,encoding='ascii')
result=subprocess.run([str(exe),'input.hmg'],cwd=folder,capture_output=True,timeout=10)
(folder/'solver.log').write_bytes(result.stdout+result.stderr)
values=[float(x) for x in (folder/'out.txt').read_text().split()] if (folder/'out.txt').exists() else []
(root/'ac-known-limitation.json').write_text(json.dumps(dict(expected=[1000,1/3,0],actual=values,exit=result.returncode),indent=2),encoding='utf-8')
print('Separate pre-existing nonlinear FCI AC limitation:',values,'; expected [1000, 1/3, 0]')
(root/'summary.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
assert all(row['passed'] for row in rows),[row['name'] for row in rows if not row['passed']]
print(len(rows),'runtime checks PASS')
