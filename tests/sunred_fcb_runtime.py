"""FCB is evaluated each iteration but must not enter the SUNRED matrix."""
import argparse, json, struct, subprocess
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--solver',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args(); exe=a.solver.resolve(strict=True); root=a.output.resolve()
root.mkdir(parents=True,exist_ok=False); results=[]
for variable in (0,3):
 for position in range(3):
  for tree in (False,True):
   lines=['H G N0 R1 2',f'L G N0 GND B{variable}']
   lines.insert(position,f'F FCB A=1 F=VALUE(A1) B{variable} N0')
   leaves=[i for i,line in enumerate(lines) if not line.startswith('F ')]
   deck=['HEXMG 1.0','.RAILS=1 R1=10',
    '.FUNCTION VALUE P=1 V=0','_MULC RET P0 0','_ADDC RET RET 2','.END FUNCTION VALUE',
    '.SUNREDTREE T',f'RED 1 0 0 {leaves[0]} 0 {leaves[1]}','.END SUNREDTREE T',
    f'.MODEL M SUBCIRCUIT N=1 B={variable+1}'+(' SUNRED=T' if tree else ''),
    *lines,'.END MODEL M','.CREATE circuit M GND=R0',
    '.PROBE IMAGE HXIM DEF DOUBLE V 00112233445546778899AABBCCDDEEFF F47AC10B58CC4372A5670E02B2C3D479 circuit',
    '.PROBE IMAGE HXIM NODES circuit N0', '.RUN circuit DC INITIAL ITERS=4',
    '.SAVE HXIM FILE="out.hxim" IMAGE']
   name=f'b{variable}-position{position}-tree{int(tree)}'; d=root/name; d.mkdir()
   (d/'input.hmg').write_text('\n'.join(deck)+'\n',encoding='ascii')
   proc=subprocess.run([str(exe),'input.hmg'],cwd=d,capture_output=True,timeout=20)
   (d/'stdout.txt').write_bytes(proc.stdout); (d/'stderr.txt').write_bytes(proc.stderr)
   value=None
   if proc.returncode==0:
    data=(d/'out.hxim').read_bytes()
    if len(data)==88:value=struct.unpack('<d',data[80:])[0]
   ok=value is not None and abs(value-5)<1e-12
   results.append(dict(name=name,passed=ok,exit=proc.returncode,value=value))
(root/'summary.json').write_text(json.dumps(results,indent=2)+'\n')
assert all(r['passed'] for r in results),results
print(f'PASS {len(results)} FCB/SUNRED runtime cases')
