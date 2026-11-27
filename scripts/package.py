#!/usr/bin/env python3
"""Package and statically verify the Android build; no device test is implied."""
import hashlib
import json
import pathlib
import shutil
import struct
import subprocess
import sys
import zipfile

so, out = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
data = so.read_bytes()
if data[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<H',data,18)[0] != 183:
    raise SystemExit('not ELF64 little-endian AArch64')
phoff = struct.unpack_from('<Q',data,32)[0]
phentsize, phnum = struct.unpack_from('<HH',data,54)
loads = []
for n in range(phnum):
    p = phoff+n*phentsize
    if struct.unpack_from('<I',data,p)[0] == 1:
        offset,vaddr = struct.unpack_from('<QQ',data,p+8)
        align = struct.unpack_from('<Q',data,p+48)[0]
        if align < 16384 or (vaddr-offset) % 16384:
            raise SystemExit('LOAD segment is not 16 KB compatible')
        loads.append(align)
if not loads: raise SystemExit('no LOAD segments')
symbols = subprocess.check_output(['readelf','--dyn-syms','--wide',str(so)],text=True)
if not any('PLGetModRegistration' in line and 'GLOBAL' in line and 'UND' not in line for line in symbols.splitlines()):
    raise SystemExit('missing PLGetModRegistration export')
if 'JNI_OnLoad' in symbols: raise SystemExit('unexpected JNI entry point')
manifest = pathlib.Path('manifest.json').read_bytes()
m = json.loads(manifest)
if m['type'] != 'preload-native' or m['entry'] != 'libNoJumpDelay.so': raise SystemExit('invalid manifest')
shutil.copyfile(so,out/'libNoJumpDelay.so')
with zipfile.ZipFile(out/'NoJumpDelay.levipack','w',zipfile.ZIP_DEFLATED) as z:
    z.writestr('manifest.json',manifest)
    z.writestr('libNoJumpDelay.so',data)
with zipfile.ZipFile(out/'NoJumpDelay.levipack') as z:
    assert z.testzip() is None and set(z.namelist()) == {'manifest.json','libNoJumpDelay.so'}
    assert z.read('libNoJumpDelay.so') == data and z.read('manifest.json') == manifest
report={'architecture':'AArch64 ELF64 little-endian','load_alignments':loads,'page_size_checked':16384,
        'registration_export':True,'sha256':hashlib.sha256(data).hexdigest(),
        'package_sha256':hashlib.sha256((out/'NoJumpDelay.levipack').read_bytes()).hexdigest(),
        'runtime_tested':False,'behavioral_equivalence_proven':False}
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
