# Ghidra post-script. String RVA 0x3f931 in the supplied Apollon ELF.
# Reports direct references and decompiles the containing functions.
from ghidra.app.decompiler import DecompInterface

base = currentProgram.getImageBase()
target = base.add(0x3f931)
print('NOJUMP string:', target)
refs = getReferencesTo(target)
print('NOJUMP direct refs:', len(refs))
dec = DecompInterface()
dec.openProgram(currentProgram)
seen = set()
for ref in refs:
    addr = ref.getFromAddress()
    f = getFunctionContaining(addr)
    print('NOJUMP xref:', addr, 'function:', f)
    if f and str(f.getEntryPoint()) not in seen:
        seen.add(str(f.getEntryPoint()))
        result = dec.decompileFunction(f, 120, monitor)
        if result.decompileCompleted():
            print('NOJUMP DECOMPILE:', f.getEntryPoint())
            print(result.getDecompiledFunction().getC())
        else:
            print('NOJUMP decompilation failed:', result.getErrorMessage())
dec.dispose()
