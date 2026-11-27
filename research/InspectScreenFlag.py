from ghidra.app.decompiler import DecompInterface
d=DecompInterface(); d.openProgram(currentProgram)
for rva in [0xb3860,0xb3a74,0xb3ab0,0xb3aec,0xb5240,0xb2290,0x10e850]:
 a=currentProgram.getImageBase().add(rva); f=getFunctionContaining(a);print('\nFUNC',a,f)
 if f:
  x=d.decompileFunction(f,45,monitor)
  if x.decompileCompleted():print(x.getDecompiledFunction().getC())
  else: print('FAILED',x.getErrorMessage())
d.dispose()
