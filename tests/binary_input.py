"""Byte-equivalent HMG/HMGB execution; no physics or baseline source changes."""
import argparse, subprocess, struct, zlib
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('solver',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
exe=a.solver.resolve();root=a.output.resolve();root.mkdir(parents=True,exist_ok=False)
def binary(data): return b'HXHB0001'+struct.pack('<Q',len(data))+zlib.compress(data,1)
ids='11111111111111111111111111111111 22222222222222222222222222222222'
base=f'''HEXMG 1.0
.OPTIONAL HEXMG_OPT 1
REDUCTION_PRECISION LEVEL0 EXTENDED
.END OPTIONAL
.RAILS=1
.MODEL M SUBCIRCUIT N=1
R1 R N0 GND 10
C1 C N0 GND .01
I1 I N0 GND DC0=0 DC=.25 AC=.5 Phi=.7
.END MODEL M
.CREATE circuit M GND=R0
.PROBE P HXIM DEF DOUBLE V {ids} circuit
.PROBE P HXIM NODES circuit N0
.RUN circuit DC INITIAL
.SAVE HXIM FILE="dc.hxim" P
.RUN circuit TIMESTEP DT=.01
.SAVE HXIM FILE="time.hxim" P
.RUN circuit AC F=100
.SAVE HXIM FILE="ac.hxim" P
'''.encode()
thermal=(Path(__file__).parent/'fixtures/thermal18.hmg').read_bytes()
decks={'rc':base,'thermal':thermal,'crlf':base.replace(b'\n',b'\r\n')}
checks=0
for name,deck in decks.items():
 outputs=[]
 for kind in ('hmg','hmgb'):
  d=root/(name+'-'+kind);d.mkdir();(d/('input.'+kind)).write_bytes(binary(deck) if kind=='hmgb' else deck)
  r=subprocess.run([str(exe),'input.'+kind],cwd=d,capture_output=True,timeout=30);(d/'log.txt').write_bytes(r.stdout+r.stderr)
  assert r.returncode==0,(name,kind,r.stderr);outputs.append({f.name:f.read_bytes() for f in d.glob('*.hxim')});checks+=1
 assert outputs[0] and outputs[0]==outputs[1],name;checks+=1
valid=binary(base)
for name,data in {'short':valid[:12],'version':b'HXHB0002'+valid[8:],'size':valid[:8]+struct.pack('<Q',2**40)+valid[16:],'broken':valid[:-2],'trailing':valid+b'!','checksum':valid[:-1]+bytes([valid[-1]^1])}.items():
 d=root/name;d.mkdir();(d/'input.hmgb').write_bytes(data)
 r=subprocess.run([str(exe),'input.hmgb'],cwd=d,capture_output=True,timeout=10);(d/'log.txt').write_bytes(r.stdout+r.stderr)
 assert r.returncode!=0 and not list(d.glob('*.hxim')),name;checks+=1
print(f'PASS {checks} binary-input checks; DC/time/AC and thermal HXIM byte-identical')
