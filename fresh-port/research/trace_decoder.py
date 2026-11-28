#!/usr/bin/env python3
"""Research-only emulation of Sigma's offset-decoding path, not execution in game."""
import os, sys
from unicorn import *
from unicorn.arm64_const import *
from elftools.elf.elffile import ELFFile
import struct, collections, json
BASE=0x10000000; GAME=int(os.environ.get('GAME_BASE', '0x5000000000'), 0); STUB=0x20000000; VM=0x21000000; TLS=0x22000000; STACK=0x23000000; HEAP=0x25000000
u=Uc(UC_ARCH_ARM64,UC_MODE_ARM); u.mem_map(BASE,0x400000);u.mem_map(STUB,0x100000);u.mem_map(VM,0x10000);u.mem_map(TLS,0x10000);u.mem_map(STACK,0x400000);u.mem_map(HEAP,0x4000000)
def read64(a): return struct.unpack('<Q',u.mem_read(a,8))[0]
def write64(a,v): u.mem_write(a,struct.pack('<Q',v & ((1<<64)-1)))
def r(n): return u.reg_read(n)
def w(n,v): u.reg_write(n,v)
with open('/home/user/re_workspace/targets/libReverseSigma.so','rb') as f:
 e=ELFFile(f)
 for seg in e.iter_segments():
  if seg['p_type']=='PT_LOAD':u.mem_write(BASE+seg['p_vaddr'],seg.data())
 # ELF RELR relative relocations
 relr=e.get_section_by_name('.relr.dyn').data();cursor=0
 for (v,) in struct.iter_unpack('<Q',relr):
  if not v&1:
   cursor=v;write64(BASE+cursor,read64(BASE+cursor)+BASE);cursor+=8
  else:
   for b in range(1,64):
    if v>>b&1:write64(BASE+cursor+(b-1)*8,read64(BASE+cursor+(b-1)*8)+BASE)
   cursor+=63*8
 # Assign a distinct trampoline to each external symbol
 stubs={}; symbols={}
 def stub(sym):
  if sym not in symbols:
   addr=STUB+(len(symbols)+1)*16;symbols[sym]=addr;stubs[addr]=sym
  return symbols[sym]
 for secname in ('.rela.dyn','.rela.plt'):
  s=e.get_section_by_name(secname); symtab=e.get_section(s['sh_link'])
  for rel in s.iter_relocations():
   typ=rel['r_info_type']; sy=symtab.get_symbol(rel['r_info_sym']); val=sy['st_value'];name=sy.name
   if typ in (257,1025,1026):
    target=(BASE+val if sy['st_shndx']!='SHN_UNDEF' else stub(name))+rel['r_addend']
    write64(BASE+rel['r_offset'],target)
 # fake JavaVM and vtable, GetEnv callback
 write64(VM,VM+0x100);write64(VM+0x100+0x30,stub('GetEnv'))
# minimal stubs; unimplemented symbols must be reported rather than treated as valid Android behavior
heap=HEAP; counts=collections.Counter(); hooks=[]
def ret(v=0):w(UC_ARM64_REG_X0,v);w(UC_ARM64_REG_PC,r(UC_ARM64_REG_LR))
def cstr(p):
 try:return bytes(u.mem_read(p,256)).split(b'\0',1)[0].decode('utf-8','replace')
 except UcError:return '<invalid>'
def external(uc,addr,size,user):
 global heap
 n=stubs.get(addr,'?'); counts[n]+=1
 a,b,c=(r(UC_ARM64_REG_X0),r(UC_ARM64_REG_X1),r(UC_ARM64_REG_X2))
 if n=='GetEnv': write64(b,VM+0x300);ret(0)
 elif n in ('malloc','_Znwm','_Znam','calloc'):
  size=(a*b if n=='calloc' else a);p=heap;heap+=(size+31)&~15
  if heap>=HEAP+0x4000000:raise Exception('heap exhausted')
  ret(p)
 elif n in ('memcpy','memmove','__memcpy_chk'):
  if c:
   if GAME <= b and b+c <= GAME+0x136869b0:
    # The first PT_LOAD has identical RVA and file offset. Refuse to
    # translate any other range without accounting for its ELF mapping.
    with open('/home/user/re_workspace/mc_libs/1.26.60.4/libminecraftpe.so','rb') as mf:
     mf.seek(b-GAME); src=mf.read(c)
   else: src=bytes(u.mem_read(b,c))
   u.mem_write(a,src)
  ret(a)
 elif n in ('memset','__memset_chk'):
  if c:u.mem_write(a,bytes([b&255])*c)
  ret(a)
 elif n=='dlopen':ret(1)
 elif n=='dlsym':ret(0x7fff01234000)
 elif n=='__stack_chk_fail': raise Exception('stack canary failed')
 else:ret(0)
u.hook_add(UC_HOOK_CODE,external,begin=STUB,end=STUB+0xfffff)
def intercept(uc,addr,size,user):
 off=addr-BASE
 if off==0x15e184:
  u.mem_write(r(UC_ARM64_REG_X8),b'\0'*128);ret(0)
 elif off==0x931b8:ret(GAME)
 elif off==0x15ead8:ret(0)
 elif off==0x15eca8:
  a,b,c=(r(UC_ARM64_REG_X0),r(UC_ARM64_REG_X1),r(UC_ARM64_REG_X2))
  hooks.append((a-GAME,b-BASE,c-BASE)); print('HOOK',*[hex(x) for x in hooks[-1]],flush=True);ret(1)
for p in (0x15e184,0x931b8,0x15ead8,0x15eca8):u.hook_add(UC_HOOK_CODE,intercept,begin=BASE+p,end=BASE+p)
u.reg_write(UC_ARM64_REG_TPIDR_EL0,TLS);w(UC_ARM64_REG_SP,STACK+0x3ff000);w(UC_ARM64_REG_X0,VM);w(UC_ARM64_REG_LR,STUB)
try:
 u.emu_start(BASE+0xb5568,STUB,count=150000000)
 print('END PC',hex(r(UC_ARM64_REG_PC)),'X0',hex(r(UC_ARM64_REG_X0)))
 completed = r(UC_ARM64_REG_PC) == STUB and r(UC_ARM64_REG_X0) == 0x10006
except Exception as ex:
 completed = False
 print('ERROR',ex,'PC',hex(r(UC_ARM64_REG_PC)),stubs.get(r(UC_ARM64_REG_PC)), 'LR',hex(r(UC_ARM64_REG_LR)), 'args',*[hex(r(x)) for x in (UC_ARM64_REG_X0,UC_ARM64_REG_X1,UC_ARM64_REG_X2)])
print('stubs',counts.most_common(50))
print('hook_count',len(hooks))
for a in range(0x357b70,0x357c50,8):
 v=read64(BASE+a); print(hex(a),hex(v-GAME) if v else 0)
with open('/home/user/re_workspace/decomp/emulation-'+hex(GAME)+'.json','w') as f:json.dump({'completed':completed,'game_base':hex(GAME),'hooks':hooks,'stub_counts':dict(counts),'function_table':{hex(a):hex(read64(BASE+a)) for a in range(0x357b70,0x357c50,8)}},f,indent=2)
if not completed:
 sys.exit(1)
