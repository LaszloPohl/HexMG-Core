"""Independent high-contrast current reference and SUNRED DC/time/AC lifecycle."""
import argparse, json, math, struct, subprocess, zlib
from decimal import Decimal, localcontext
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('solver',type=Path);p.add_argument('output',type=Path)
a=p.parse_args();exe=a.solver.resolve(strict=True);root=a.output.resolve()
root.mkdir(parents=True,exist_ok=False);report=[]

def run(name,text,expected=None,reject=False,binary=False):
    folder=root/name;folder.mkdir();data=text.encode('ascii');file='input.hmgb' if binary else 'input.hmg'
    if binary:data=b'HXHB0001'+struct.pack('<Q',len(data))+zlib.compress(data,1)
    (folder/file).write_bytes(data)
    r=subprocess.run([str(exe),file],cwd=folder,capture_output=True,timeout=10)
    (folder/'solver.log').write_bytes(r.stdout+r.stderr)
    outputs={f.name:list(map(float,f.read_text().split())) for f in folder.glob('*.res')}
    ok=r.returncode!=0 and not outputs if reject else r.returncode==0
    if expected is not None:
        ok=ok and outputs.keys()==expected.keys() and all(len(outputs[k])==len(v) and
            all(math.isclose(x,y,rel_tol=1e-8,abs_tol=1e-19) for x,y in zip(outputs[k],v))
            for k,v in expected.items())
    report.append(dict(name=name,passed=ok,exit=r.returncode,values=outputs,expected=expected))
    return r.returncode,outputs

def options(leaf,tree):
    return '.OPTIONAL HEXMG_OPT 1\nREDUCTION_PRECISION LEVEL0 '+leaf+'\n'+(
        'REDUCTION_PRECISION SUNRED '+tree+'\n' if tree else '')+'.END OPTIONAL\n'

def island(g,block):
    return f'''HEXMG 1.0
{block}.RAILS=2 R0=0 R1=7
.FUNCTION DROP P=1 V=0
_MULC RET P0 {-1/g:.17g}
.END FUNCTION DROP
.MODEL BR SUBCIRCUIT X=2 N=1
GY GYR X0 X1 N0 R0 1 1
R FCI F=DROP(X0) N0 R0 0
.END MODEL BR
.SUNREDTREE T
RED 1 0 0 0 0 1
RED 1 1 0 2 0 3
RED 2 0 1 0 1 1
.END SUNREDTREE T
.MODEL M SUBCIRCUIT N=2 SUNRED=T
R MODEL BR N0 N1
A G N0 R1 1e-9
B G N1 R0 1e-9
I I N0 R0 DC0=1e-9 DC=1e-9
.END MODEL M
.CREATE circuit M GND=R0
.PROBE U0 V circuit N0
.PROBE U1 V circuit N1
.PROBE I I circuit R.X1
.RUN circuit DC INITIAL ITERS=3
.SAVE RAW MAXRESULTSPERROW=3 FILE="result.res" U0 U1 I
'''

for g in (1.,1e8):
    with localcontext() as context:
        context.prec=70;gd=Decimal(str(g));w=Decimal('1e-9')
        delta=(w*7+w)/(2*gd+w);v=gd*delta/w
        expected={'result.res':list(map(float,(v+delta,v,gd*delta)))}
    for leaf in ('DOUBLE','EXTENDED'):
        prefix=f'{g:g}-{leaf}'
        default=run(prefix+'-default',island(g,options(leaf,None)),expected if g==1 else None)
        explicit=run(prefix+'-double',island(g,options(leaf,'DOUBLE')),expected if g==1 else None)
        report.append(dict(name=prefix+'-default-parity',passed=default==explicit))
        run(prefix+'-extended',island(g,options(leaf,'EXTENDED')),expected)
        if g==1e8:run(prefix+'-hmgb',island(g,options(leaf,'EXTENDED')),expected,binary=True)

rc='''HEXMG 1.0
OPTIONS
.SUNREDTREE T
RED 1 0 0 0 0 1
RED 1 1 0 2 0 3
RED 2 0 1 0 1 1
.END SUNREDTREE T
.MODEL M SUBCIRCUIT N=1 SUNRED=T
R G N0 GND .1
C C N0 GND .01
I I N0 GND DC0=0 DC=1 AC=2
Z I N0 GND DC0=0 DC=0 AC=0
.END MODEL M
.CREATE circuit M GND=R0
.PROBE U V circuit N0
.RUN circuit DC INITIAL
.SAVE RAW FILE="dc.res" U
.RUN circuit TIMESTEP DT=.1
.SAVE RAW FILE="step1.res" U
.RUN circuit TIMESTEP DT=.1
.SAVE RAW FILE="step2.res" U
.RUN circuit AC F=10
.SAVE RAW FILE="ac.res" U
.RUN circuit DC INITIAL
.SAVE RAW FILE="dc2.res" U
.RUN circuit AC F=10
.SAVE RAW FILE="ac2.res" U
'''
u=2/complex(.1,2*math.pi*10*.01)
expected={'dc.res':[0.],'step1.res':[.1,.1,5.],'step2.res':[.2,.1,7.5],
          'ac.res':[10.,u.real,u.imag],'dc2.res':[0.],'ac2.res':[10.,u.real,u.imag]}
for leaf in ('DOUBLE','EXTENDED'):
    double=run('rc-'+leaf+'-double',rc.replace('OPTIONS',options(leaf,'DOUBLE')),expected)
    wide=run('rc-'+leaf+'-extended',rc.replace('OPTIONS',options(leaf,'EXTENDED')),expected)
    report.append(dict(name='rc-'+leaf+'-AC-parity',passed=double[1].get('ac.res')==wide[1].get('ac.res')))
# Nonlinear radiation + Joule coupling. Bound Newton temperature changes;
# the raw historical fixture overshoots its conductivity domain even with
# the pre-change solver, so it is not used as a successful reference here.
fixture=(Path(__file__).parent/'fixtures/sunred_initialization_coupled.hmg').read_text()
limits=''.join(f'NEWTON_FORWARD_LIMIT circuit {x} {y} 5\n' for n in ('CELL.N0','CELL.N4') for x,y in ((n,'R0'),('R0',n)))
lo,hi=25.,1000.
for _ in range(100):
    t=(lo+hi)/2;power=400/(1+.02*(t-25))+.5;surface=t-power/20
    if 5.670374419e-8*1.3*.8*((surface+273.15)**4-298.15**4)>power:hi=t
    else:lo=t
t=(lo+hi)/2;power=400/(1+.02*(t-25))+.5
fixture=fixture.replace('.PROBE IMAGE HXIM DEF DOUBLE V 00112233445546778899AABBCCDDEEFF F47AC10B58CC4372A5670E02B2C3D479 circuit\n.PROBE IMAGE HXIM NODES circuit CELL.N0 CELL.N4 CELL.N3',
    '.PROBE T V circuit CELL.N0\n.PROBE S V circuit CELL.N4\n.PROBE I V circuit CELL.N3')
fixture=fixture.replace('.SAVE HXIM FILE="result.hxim" IMAGE','.SAVE RAW MAXRESULTSPERROW=3 FILE="thermal.res" T S I')
for mode in ('DOUBLE','EXTENDED'):
    text=fixture.replace('.OPTIONAL HEXMG_OPT 1','.OPTIONAL HEXMG_OPT 1\nREDUCTION_PRECISION SUNRED '+mode+'\n'+limits)
    run('bounded-nonlinear-'+mode,text,{'thermal.res':[t,t-power/20,20/(1+.02*(t-25))]})
for n,bad in enumerate(('REDUCTION_PRECISION SUNRED AUTO','REDUCTION_PRECISION SUNRED',
        'REDUCTION_PRECISION SUNRED EXTENDED EXTRA','REDUCTION_PRECISION SUNRED TYPO',
        'REDUCTION_PRECISION SUNRED DOUBLE\nREDUCTION_PRECISION SUNRED EXTENDED')):
    run('invalid-'+str(n),rc.replace('OPTIONS','.OPTIONAL HEXMG_OPT 1\n'+bad+'\n.END OPTIONAL'),reject=True)
run('duplicate-blocks',rc.replace('OPTIONS',options('EXTENDED','DOUBLE')+
    '.OPTIONAL HEXMG_OPT 1\nREDUCTION_PRECISION SUNRED DOUBLE\n.END OPTIONAL\n'),reject=True)
(root/'summary.json').write_text(json.dumps(report,indent=2))
failed=[r['name'] for r in report if not r['passed']]
print(f'{len(report)-len(failed)}/{len(report)} SUNRED precision checks PASS; failed={failed}')
raise SystemExit(bool(failed))
