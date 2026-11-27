# Inspect Apollon's directly observed helper and UI call site (Ghidra image base 0x100000).
from ghidra.app.decompiler import DecompInterface
base=currentProgram.getImageBase()
dec=DecompInterface(); dec.openProgram(currentProgram)
for rva in [0xb1598,0xa0958]:
    addr=base.add(rva); f=getFunctionContaining(addr)
    print('FUNCTION', addr, f)
    if rva == 0xb1598 and f:
        result=dec.decompileFunction(f,90,monitor)
        if result.decompileCompleted(): print(result.getDecompiledFunction().getC())
        else: print('FAILED', result.getErrorMessage())
for start,end in [(0xb1598,0xb166c),(0xa44e0,0xa4590)]:
    inst=getInstructionAt(base.add(start)); endaddr=base.add(end)
    while inst and inst.getAddress().compareTo(endaddr)<0:
        print('INSN', inst.getAddress(), inst)
        inst=inst.getNext()
dec.dispose()
