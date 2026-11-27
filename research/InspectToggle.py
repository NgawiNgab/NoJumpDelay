from ghidra.app.decompiler import DecompInterface
d=DecompInterface(); d.openProgram(currentProgram)
for rva in [0xf89a0,0xf8a2c,0x145abc,0x1590c4]:
    a=currentProgram.getImageBase().add(rva); f=getFunctionContaining(a)
    print('\nFUNC',a,f)
    if f:
        res=d.decompileFunction(f,30,monitor)
        if res.decompileCompleted(): print(res.getDecompiledFunction().getC())
        else: print('FAILED',res.getErrorMessage())
d.dispose()
