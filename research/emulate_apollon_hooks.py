#!/usr/bin/env python3
"""Research-only, synthetic JNI emulation; this is not an Android runtime.

Requires unicorn and pyelftools. No instructions from Minecraft are executed:
its base, hooks, JNI, parts of libc and reads of game bytes are faked. Results
must be cross-checked against the original ELF; no instrumentation is installed.
"""
import struct,sys,traceback
from unicorn import *
from unicorn.arm64_const import *
from elftools.elf.elffile import ELFFile
P = sys.argv[1] if len(sys.argv) > 1 else '/home/user/research_resources/libReverseSigma.so'
f=open(P,'rb'); elf=ELFFile(f)
B=0x100000; ST=0x7000000; HP=0xa000000; TLS=0x6000000; TR=0x5000000; GAME=0x70000000
u=Uc(UC_ARCH_ARM64,UC_MODE_ARM); u.mem_map(B,0x400000); u.mem_map(ST,0x800000);u.mem_map(HP,0x4000000);u.mem_map(TLS,0x10000);u.mem_map(TR,0x10000)
for seg in elf.iter_segments():
 if seg['p_type']=='PT_LOAD':u.mem_write(B+seg['p_vaddr'],seg.data())
p64=lambda x:struct.pack('<Q',x&0xffffffffffffffff)
def w64(a,x):u.mem_write(a,p64(x))
def r64(a):return struct.unpack('<Q',u.mem_read(a,8))[0]
for sec in elf.iter_sections():
 if sec.name=='.relr.dyn':
  data=sec.data(); loc=0
  for val, in struct.iter_unpack('<Q',data):
   if not val&1:
    loc=B+val;w64(loc,r64(loc)+B);loc+=8
   else:
    for k in range(63):
     if (val>>(k+1))&1:w64(loc+8*k,r64(loc+8*k)+B)
    loc+=63*8
sym=elf.get_section_by_name('.dynsym')
for r in elf.get_section_by_name('.rela.dyn').iter_relocations():
 a=B+r['r_offset'];typ=r['r_info_type'];ad=r['r_addend'];sy=r['r_info_sym'];s=sym.get_symbol(sy) if sy else None;v=s['st_value'] if s else 0
 if typ==0x403:w64(a,B+ad)
 elif typ in (0x101,0x401,0x402):w64(a,B+v+ad if v else HP+0x1000)
# fake trampolines
names={}; u.mem_write(TR,b'\xc0\x03\x5f\xd6'*0x4000)
for i,r in enumerate(elf.get_section_by_name('.rela.plt').iter_relocations()):
 s=sym.get_symbol(r['r_info_sym']);addr=TR+i*4;names[addr]=(s.name,int(s['st_value']));w64(B+r['r_offset'],addr)
# fake java invocation table and env
w64(TLS, TLS+0x1000)
for n in range(128):w64(TLS+0x1000+8*n, TR+0xff00)
w64(TLS+0x28,0xdeadbeefabdefade)
# globals supplied by guide
u.mem_write(HP+0x2000,b'libminecraftpe.so\0');w64(B+0x357b28,HP+0x2000)
u.mem_write(HP+0x3000,b'/home/user/config.json\0');w64(B+0x357b10,HP+0x3000)
u.reg_write(UC_ARM64_REG_TPIDR_EL0,TLS);u.reg_write(UC_ARM64_REG_SP,ST+0x400000);u.reg_write(UC_ARM64_REG_X0,TLS);u.reg_write(UC_ARM64_REG_LR,TR+0xff08)
regs=[UC_ARM64_REG_X0,UC_ARM64_REG_X1,UC_ARM64_REG_X2,UC_ARM64_REG_X3]
heap=HP+0x10000
hooks=[]; last=[]; finished=False; failed=False
def ret(v=0):u.reg_write(UC_ARM64_REG_X0,v);u.reg_write(UC_ARM64_REG_PC,u.reg_read(UC_ARM64_REG_LR))
def cstr(addr):
 out=bytearray()
 for k in range(65536):
  b=u.mem_read(addr+k,1)[0]
  if b==0:return bytes(out)
  out.append(b)
 raise Exception('unterminated')
def code(uc,a,size,ud):
 global heap, finished
 if a==B+0x931b8:ret(GAME);return
 if a==B+0x15eca8:
  x=[uc.reg_read(r) for r in regs];hooks.append(x);print('HOOK',len(hooks)-1,*(hex(y) for y in x[:3]),flush=True)
  if x[2]:w64(x[2],x[0])
  ret(HP+0x1000);return
 if a==TR+0xff00:
  x1=uc.reg_read(regs[1]);
  if x1:w64(x1,TLS+0x2000)
  ret(0);return
 if a==TR+0xff08:
  finished=True;print('return',hex(uc.reg_read(regs[0])));uc.emu_stop();return
 if a not in names:return
 name,val=names[a];x=[uc.reg_read(r) for r in regs];last.append((name,x));last[:]=last[-12:]
 if name.startswith('_Zn') or name in ('malloc','calloc','realloc'):
  n=x[0] if name!='calloc' else x[0]*x[1]
  if name=='realloc':n=x[1]
  if n>0x800000:raise Exception('alloc huge '+hex(n)+' '+name)
  p=heap; heap=(heap+n+31)&~15
  if heap>HP+0x4000000:raise Exception('out of heap')
  if name=='realloc' and x[0]:uc.mem_write(p,bytes(uc.mem_read(x[0],n)))
  ret(p);return
 if name in ('memcpy','memmove','__memcpy_chk'):
  if x[2]>0x1000000:raise Exception('copy huge '+str(x))
  data = bytes(x[2]) if GAME <= x[1] < GAME+0x20000000 else bytes(uc.mem_read(x[1],x[2])); uc.mem_write(x[0],data);ret(x[0]);return
 if name in ('memset','__memset_chk'):
  if x[2]>0x1000000:raise Exception('set huge '+str(x))
  uc.mem_write(x[0],bytes([x[1]&255])*x[2]);ret(x[0]);return
 if name in ('strlen','__strlen_chk'):ret(len(cstr(x[0])));return
 if name=='__errno':ret(TLS+0x4000);return
 if name in ('strtoll_l','strtoull_l'):
  import re
  raw=cstr(x[0]); base=x[2]; txt=raw.decode('ascii','ignore'); pattern=r'^\s*[+-]?(?:0[xX])?[0-9a-fA-F]+' if base==16 else r'^\s*[+-]?[0-9]+'
  match=re.match(pattern,txt); token=match.group(0) if match else ''
  if x[1]: w64(x[1], x[0]+len(token))
  try: result=int(token,base or 0)
  except ValueError: result=0
  ret(result);return
 if name=='strcmp':ret((cstr(x[0])>cstr(x[1]))-(cstr(x[0])<cstr(x[1])));return
 if name=='memchr':
  i=bytes(uc.mem_read(x[0],x[2])).find(bytes([x[1]&255]));ret(x[0]+i if i>=0 else 0);return
 if name in ('free','_ZdlPv','_ZdaPv'):ret(0);return
 if val:uc.reg_write(UC_ARM64_REG_PC,B+val);return
 
 if not name.startswith('pthread_'): print('STUB',name,[hex(k) for k in x],flush=True)
 ret(0)
u.hook_add(UC_HOOK_CODE,code,begin=TR,end=TR+0xffff)
u.hook_add(UC_HOOK_CODE,code,begin=B+0x931b8,end=B+0x931b8)
u.hook_add(UC_HOOK_CODE,code,begin=B+0x15eca8,end=B+0x15eca8)
try:u.emu_start(B+0xb5568,0,count=40000000)
except Exception as e:
 failed=True
 print('ERROR',e,'PC',hex(u.reg_read(UC_ARM64_REG_PC)),'LR',hex(u.reg_read(UC_ARM64_REG_LR)));print('recent',last);traceback.print_exc()
finally:
 print('COUNT',len(hooks),'pc',hex(u.reg_read(UC_ARM64_REG_PC)))
 for x in [0x357ba0,0x357c38,0x35aeb0]:print('GLOBAL',hex(x),hex(r64(B+x)))

for x in range(0x357b00,0x357d00,8):
 v=r64(B+x)
 if GAME<=v<GAME+0x20000000:print('GAMEPTR',hex(x),hex(v-GAME))

if failed or not finished:
 print('Synthetic emulation did not complete', file=sys.stderr)
 sys.exit(1)
if len(hooks) != 59 or hooks[30][:3] != [GAME+0x0af5e460, B+0xabb30, B+0x35aeb0]:
 print('Unexpected registration results; investigate', file=sys.stderr)
 sys.exit(1)
