"""Local precision, OPTIONAL semantics, independent DC/RC/gyrator references."""
import argparse, cmath, hashlib, json, math, struct, subprocess
from pathlib import Path
p=argparse.ArgumentParser()
for name in ('baseline','optimized','output'):p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();root=a.output.resolve();root.mkdir(parents=True,exist_ok=False)
exes={'baseline':a.baseline.resolve(strict=True),'optimized':a.optimized.resolve(strict=True)}
report={'executables':{k:hashlib.sha256(v.read_bytes()).hexdigest() for k,v in exes.items()},'cases':[]}
ids='11111111111111111111111111111111 22222222222222222222222222222222'
def block(mode):return f'.OPTIONAL HEXMG_OPT 1\nREDUCTION_PRECISION LEVEL0 {mode}\n.END OPTIONAL\n'
def run(name,deck,mode=None,expected=None,reject=False,extra='',fallback=False):
    folder=root/name;folder.mkdir();text=deck
    if mode:text=text.replace('HEXMG 1.0\n','HEXMG 1.0\n'+block(mode),1)
    text=extra+text;(folder/'input.hmg').write_text(text,encoding='ascii')
    proc=subprocess.run([str(exes['optimized' if mode or extra else 'baseline']),'input.hmg'],cwd=folder,capture_output=True,timeout=20)
    (folder/'stdout.txt').write_bytes(proc.stdout);(folder/'stderr.txt').write_bytes(proc.stderr)
    case={'name':name,'exit':proc.returncode,'pass':False};report['cases'].append(case)
    try:
        if reject:assert proc.returncode!=0 and not list(folder.glob('*.hxim')),proc.stderr
        else:
            assert proc.returncode==0,proc.stderr
            if fallback:assert b'extended fallbacks=0' not in proc.stdout and b'extended fallbacks=' in proc.stdout
            for filename,reference in (expected or {}).items():
                data=(folder/filename).read_bytes();assert data[:4]==b'HXIM'
                values=struct.unpack('<'+'d'*((len(data)-80)//8),data[80:])
                assert len(values)==len(reference) and all(math.isclose(v,r,rel_tol=1e-9,abs_tol=1e-10) for v,r in zip(values,reference)),(values,reference)
            if mode=='DOUBLE':assert b'double steps=0' not in proc.stdout
        case['pass']=True
    except Exception as error:case['error']=str(error)
    return folder
def circuit(components,nodes='N0',n=1):
    return '\n'.join(['HEXMG 1.0','.RAILS=1',f'.MODEL M SUBCIRCUIT N={n}',*components,'.END MODEL M','.CREATE circuit M GND=R0',f'.PROBE P HXIM DEF DOUBLE V {ids} circuit','.PROBE P HXIM NODES circuit '+nodes])+'\n'
thermal=(Path(__file__).parent/'fixtures/thermal18.hmg').read_text()
for mode in (None,'EXTENDED','DOUBLE','AUTO'):
    label=mode or 'baseline'
    run('thermal_'+label,thermal,mode,{'result.hxim':[-12.5,25.,87.5]*6})
    # Varying dt and C/dt, and currents after each accepted step.
    for resistance,capacity in [(10.,.01),(1000.,1e-6),(.01,10.)]:
        deck=circuit([f'R1 R N0 GND {resistance}',f'C1 C N0 GND {capacity}','I1 I N0 GND DC0=0 DC=0.25 AC=0.5 Phi=0.7'])
        deck+=f'.PROBE Q HXIM DEF DOUBLE I {ids} circuit\n.PROBE Q HXIM NODES circuit R1.X0 C1.X0\n.RUN circuit DC INITIAL\n'
        expected={};previous=0
        for i,factor in enumerate((.01,.1,1,2,.03),1):
            dt=resistance*capacity*factor;u=(.25+capacity/dt*previous)/(1/resistance+capacity/dt)
            expected[f'v{i}.hxim']=[u];expected[f'i{i}.hxim']=[-u/resistance,-(u-previous)*capacity/dt];previous=u
            deck+=f'.RUN circuit TIMESTEP DT={dt}\n.SAVE HXIM FILE="v{i}.hxim" P\n.SAVE HXIM FILE="i{i}.hxim" Q\n'
        for i,w in enumerate((0.,.01,1.,100.,10000.)):
            f=w/(2*math.pi*resistance*capacity);u=.5*cmath.exp(.7j)/(1/resistance+2j*math.pi*f*capacity)
            expected[f'a{i}.hxim']=[u.real,u.imag]
            deck+=f'.RUN circuit AC F={f}\n.SAVE HXIM FILE="a{i}.hxim" P\n'
        run(f'rc_{resistance}_{label}',deck,mode,expected)
    deck=circuit(['R1 R N0 GND 10','R2 R N0 N1 20','R3 R N1 N2 30','I1 I N2 GND DC0=.25 DC=.25'],'N0 N1 N2',3)
    deck+='.RUN circuit DC INITIAL\n.SAVE HXIM FILE="three.hxim" P\n'
    run('three_internal_'+label,deck,mode,{'three.hxim':[2.5,7.5,15.]})
    # Nonsymmetric matrix path.
    deck=circuit(['R1 R N0 GND 10','R2 R N1 GND 20','G1 GYR N0 GND N1 GND 2 3','I1 I N0 GND DC0=.25 DC=.25'],'N0 N1',2)
    deck+='.RUN circuit DC INITIAL\n.SAVE HXIM FILE="g.hxim" P\n'
    run('gyrator_'+label,deck,mode,{'g.hxim':[.25/120.1,15/120.1]})
# Catastrophic Schur cancellation: extended retains the unit conductance.
extreme='\n'.join(['HEXMG 1.0','.RAILS=1','.MODEL LEAF SUBCIRCUIT N=1 X=1','R1 R N0 X0 1e-20','R2 R N0 GND 1','.END MODEL LEAF','.MODEL TOP SUBCIRCUIT N=1','CELL MODEL LEAF N0','I1 I N0 GND DC0=1 DC=1','.END MODEL TOP','.CREATE circuit TOP GND=R0',f'.PROBE P HXIM DEF DOUBLE V {ids} circuit','.PROBE P HXIM NODES circuit N0 CELL.N0','.RUN circuit DC INITIAL','.SAVE HXIM FILE="x.hxim" P'])+'\n'
for mode in (None,'EXTENDED','DOUBLE','AUTO'):
    run('cancellation_'+str(mode),extreme,mode,{'x.hxim':[1,1]},reject=mode=='DOUBLE',fallback=mode=='AUTO')
simple=circuit(['R1 R N0 GND 10','I1 I N0 GND DC0=.25 DC=.25'])+'.RUN circuit DC INITIAL\n.SAVE HXIM FILE="s.hxim" P\n'
for i,body in enumerate(['REDUCTION_PRECISION LEVEL0 TYPO','REDUCTION_PRECISION LEVEL1 DOUBLE','THREADS 2','REDUCTION_PRECISION LEVEL0 AUTO\nREDUCTION_PRECISION LEVEL0 DOUBLE','INVENTED 1']):
    run('invalid_'+str(i),simple,reject=True,extra='.OPTIONAL HEXMG_OPT 1\n'+body+'\n.END OPTIONAL\n')
run('unknown_version',simple,expected={'s.hxim':[2.5]},extra='.OPTIONAL HEXMG_OPT 2\n.INVALID\n.END OPTIONAL\n')
run('before_header',simple,expected={'s.hxim':[2.5]},extra=block('DOUBLE'))
run('lowercase',simple,expected={'s.hxim':[2.5]},extra=block('auto').lower())
# Reader pre-scans all directives before execution, even a block inside a model.
run('model_scope',simple.replace('R1 R',block('DOUBLE')+'R1 R'),extra='; optimized test\n',expected={'s.hxim':[2.5]})
run('duplicate_blocks',simple,reject=True,extra=block('AUTO')+block('AUTO'))
run('invalid_header',simple,reject=True,extra='.OPTIONAL HEXMG_OPT 1 EXTRA\n.END OPTIONAL\n')
(root/'report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
failed=[c for c in report['cases'] if not c['pass']]
for case in failed:print(case)
print(f'{len(report["cases"])-len(failed)}/{len(report["cases"])} PASS: {root}')
raise SystemExit(bool(failed))
