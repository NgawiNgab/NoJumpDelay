from ghidra.app.decompiler import DecompInterface
d=DecompInterface();d.openProgram(currentProgram);f=getFunctionContaining(currentProgram.getImageBase().add(0xb16ac));r=d.decompileFunction(f,90,monitor)
print(r.getDecompiledFunction().getC() if r.decompileCompleted() else r.getErrorMessage());d.dispose()
