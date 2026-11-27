# Trace references to Apollon's config state discovered via FindNoJumpDelay.py.
from ghidra.app.decompiler import DecompInterface
state = currentProgram.getImageBase().add(0x357ee4)
refs = getReferencesTo(state)
print('STATE', state, 'REFS', len(refs))
dec = DecompInterface()
dec.openProgram(currentProgram)
seen = set()
for r in refs:
    a = r.getFromAddress()
    f = getFunctionContaining(a)
    print('REF', a, 'FUNC', f)
    inst = getInstructionAt(a)
    for x in range(4):
        if inst and inst.getPrevious(): inst = inst.getPrevious()
    for x in range(12):
        if not inst: break
        print('  ', inst.getAddress(), inst)
        inst = inst.getNext()
    if f and str(f.getEntryPoint()) not in seen:
        seen.add(str(f.getEntryPoint()))
        # Configuration save/load were already decompiled by FindNoJumpDelay.
        if str(f.getEntryPoint()) in ('001950f4','001986cc'): continue
        result = dec.decompileFunction(f, 90, monitor)
        if result.decompileCompleted():
            print('DECOMPILE', f.getEntryPoint())
            print(result.getDecompiledFunction().getC())
        else: print('FAILED', result.getErrorMessage())
dec.dispose()
