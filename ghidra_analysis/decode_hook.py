from unicorn import *
from unicorn.arm64_const import *
from elftools.elf.elffile import ELFFile
from struct import pack
p='/home/user/libReverseSigma.so'
with open(p,'rb') as f:
 e=ELFFile(f);u=Uc(UC_ARCH_ARM64,UC_MODE_ARM);u.mem_map(0x100000,0x380000)
 for ph in e.iter_segments():
  if ph['p_type']=='PT_LOAD':
   f.seek(ph['p_offset']);u.mem_write(0x100000+ph['p_vaddr'],f.read(ph['p_filesz']))
 u.mem_map(0x800000,0x20000);u.reg_write(UC_ARM64_REG_SP,0x808000);u.reg_write(UC_ARM64_REG_X29,0x809000)
 u.mem_write(0x457b30,pack('<Q',0x10000000))
 u.mem_write(0x45fee0,pack('<Q',0));u.mem_write(0x45fee8,pack('<Q',0))
 def h(uc,pc,size,_):
  if pc==0x1bc980:
   print('target =',hex(uc.reg_read(UC_ARM64_REG_X0)),'offset =',hex(uc.reg_read(UC_ARM64_REG_X0)-0x10000000))
   print('decoded buffer:',bytes(uc.mem_read(0x809000-0xa0,21)).hex());uc.emu_stop()
 u.hook_add(UC_HOOK_CODE,h)
 try:u.emu_start(0x1bc660,0x1bc984,count=100000)
 except UcError as ex: print('failed',ex,'PC',hex(u.reg_read(UC_ARM64_REG_PC)))
