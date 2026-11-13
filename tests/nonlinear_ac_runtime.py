"""AC linearization of FCI/FCIM/FCID/FCIMD and GD/GMD, with thermal feedback.

Closed form DC plus independently assembled complex equations. Repeated AC,
zero drive, scaled drive and changed DC operating point must all agree.
"""
import argparse, cmath, hashlib, json, math, subprocess
from pathlib import Path

def solve(a,b):
    a=[list(row)+[v] for row,v in zip(a,b)];n=len(b)
    for k in range(n):
        p=max(range(k,n),key=lambda i:abs(a[i][k]));a[k],a[p]=a[p],a[k]
        pivot=a[k][k]
        if abs(pivot)<1e-14:raise ValueError('Singular independent reference')
        a[k]=[v/pivot for v in a[k]]
        for i in range(n):
            if i!=k:
                f=a[i][k];a[i]=[v-f*w for v,w in zip(a[i],a[k])]
    return [row[-1] for row in a]

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--solver',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--kinds',default='FCI,FCIM,FCID,FCIMD,GD,GMD');args=ap.parse_args()
    exe=args.solver.resolve(strict=True);root=args.output.resolve();root.mkdir(parents=True,exist_ok=False);cases=[]
    law=['.FUNCTION LAW P=4 V=3','_SUB V0 P0 P1','_MULC V1 P2 .01','_MULC V2 P3 .02','_ADD V1 V1 V2','_ADDC V1 V1 2','_MUL V1 V1 V0','_POWC V2 V0 3','_MULC V2 V2 .3','_ADD RET V1 V2','_NEG RET RET','.END FUNCTION LAW']
    share=.3;h1=2;h2=3
    for kind in args.kinds.split(','):
      if kind not in ('FCI','FCIM','FCID','FCIMD','GD','GMD'):raise ValueError('Unknown component kind')
      for tree in (False,True):
        diss=kind.endswith('D');monitor='M' in kind;function=kind.startswith('FC')
        nodeports=['N0','GND']+(['N2'] if monitor else [])+(['N3','N4'] if diss else [])
        controls=''
        if function:
            if diss:controls=f' F=LAW(X0 X1 X{3 if monitor else 2} X{4 if monitor else 3})'
            else:controls=' Y=2 F=LAW(X0 X1 Y0 Y1)';nodeports+=['N3','N4']
        component='D '+kind+controls+' '+' '.join(nodeports)+' '+('.3 ' if diss else '')+('.1' if function else '2')
        components=[component,'IS I N0 GND B0 B0 B1 B2','C0 C N0 GND .02','H1 G N3 R1 2','H2 G N4 R1 3','C1 C N3 GND .05','C2 C N4 GND .08','IT I N3 GND 0 0 B3 -.6','UNUSED G N1 GND 1']
        if not monitor:components+=['UNUSEDM G N2 GND 1']
        lines=['HEXMG 1.0','.RAILS=2 R0=0 R1=25',*law]
        if tree:
            lines+=['.SUNREDTREE T']
            for i in range(1,len(components)):lines+=[f'RED {i} 0 {i-1} 0 0 {i}']
            lines+=['.END SUNREDTREE T']
        lines+=['.MODEL M SUBCIRCUIT N=5 B=4'+(' SUNRED=T' if tree else ''),*components,'.END MODEL M','.CREATE C M GND=R0','.PROBE V V C N0','.PROBE T1 V C N3','.PROBE T2 V C N4','.PROBE I I C D.X0']
        if monitor:lines+=['.PROBE MON V C N2']
        if diss:lines += [f'.PROBE Q1 I C D.X{3 if monitor else 2}',f'.PROBE Q2 I C D.X{4 if monitor else 3}']
        refs=[];dc_refs=[]
        for op,vdc in enumerate((1.,-.4,1.)):
            # T1=25+a*V*I/h1 and T2=25+(1-a)*V*I/h2.
            current=(2.85*vdc+.3*vdc**3)/(1-vdc*vdc*(.01*share/h1+.02*(1-share)/h2)) if function and diss else (2.85*vdc+.3*vdc**3 if function else 2*vdc)
            t1=25+(share*vdc*current/h1 if diss else 0);t2=25+((1-share)*vdc*current/h2 if diss else 0)
            dc_refs.append([vdc,t1,t2])
            lines +=[f'.SET C.B0 {current:.17g}','.RUN C DC INITIAL ERR=1e-11',f'.SAVE RAW MAXRESULTSPERROW=100 {"APPEND " if op else ""}FILE="dc.res" V T1 T2']
            iv=2.1+.01*t1+.02*t2+.9*vdc*vdc if function else 2
            it1=.01*vdc if function else 0;it2=.02*vdc if function else 0
            for f,amp in ((1.,1.),(10.,1.),(10.,3.),(10.,0.),(1.,1.)):
                s=2j*math.pi*f;e=amp*cmath.exp(.4j);heat=.2*amp*cmath.exp(-.6j)
                # Unknowns dV,dT1,dT2. dP=Idc*dV+Vdc*dI.
                pv=current+vdc*iv if diss else 0;pt1=vdc*it1 if diss else 0;pt2=vdc*it2 if diss else 0
                dv,dt1,dt2=solve([[iv+s*.02,it1,it2],[-share*pv,h1+s*.05-share*pt1,-share*pt2],[-(1-share)*pv,-(1-share)*pt1,h2+s*.08-(1-share)*pt2]],[e,heat,0])
                di=iv*dv+it1*dt1+it2*dt2;dp=current*dv+vdc*di
                values=[dv,dt1,dt2,-di]+([di] if monitor else [])+([-share*dp,-(1-share)*dp] if diss else [])
                refs.append([f]+[x for z in values for x in (z.real,z.imag)])
                probes='V T1 T2 I'+(' MON' if monitor else '')+(' Q1 Q2' if diss else '')
                lines +=[f'.SET C.B1 {amp}',f'.SET C.B2 .4',f'.SET C.B3 {.2*amp}',f'.RUN C AC F={f}',f'.SAVE RAW MAXRESULTSPERROW=100 {"APPEND " if len(refs)>1 else ""}FILE="ac.res" {probes}']
        lines+=['.END'];folder=root/f'{kind}_tree{int(tree)}';folder.mkdir();(folder/'input.hmg').write_text('\n'.join(lines)+'\n',encoding='ascii')
        p=subprocess.run([str(exe),'input.hmg'],cwd=folder,capture_output=True,timeout=30);(folder/'stdout.txt').write_bytes(p.stdout);(folder/'stderr.txt').write_bytes(p.stderr)
        failures=[];checks=0
        if p.returncode or p.stderr:failures.append(f'Exit {p.returncode}: {p.stderr.decode(errors="replace")}')
        for name,expected in (('ac.res',refs),('dc.res',dc_refs)):
            path=folder/name;actual=[[float(x) for x in row.split()] for row in path.read_text().splitlines() if row.strip()] if path.exists() else []
            if len(actual)!=len(expected) or any(len(a)!=len(b) for a,b in zip(actual,expected)):failures.append(name+': table shape');continue
            for row,(a,b) in enumerate(zip(actual,expected)):
                for col,(v,w) in enumerate(zip(a,b)):
                    checks+=1
                    if not math.isfinite(v) or abs(v-w)>1e-8+6e-6*abs(w):failures.append(f'{name}[{row},{col}] {v} != {w}')
        cases.append(dict(name=folder.name,passed=not failures,checks=checks,failures=failures));print(folder.name,'PASS' if not failures else 'FAIL '+str(failures[:3]),flush=True)
    out=dict(solver=str(exe),sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),passed=sum(c['passed'] for c in cases),total=len(cases),checks=sum(c['checks'] for c in cases),cases=cases)
    (root/'summary.json').write_text(json.dumps(out,indent=2)+'\n');print(f"{out['passed']}/{out['total']} cases; {out['checks']} checks");raise SystemExit(0 if out['passed']==out['total'] else 1)

if __name__=='__main__':main()
