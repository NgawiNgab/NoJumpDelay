# List references to selected Apollon global slots; RVAs are relative to image base.
for rva in [0x357ba0, 0x357c38, 0x3580d0, 0x35aeb0, 0x357ee4, 0x1d4a20]:
    addr=currentProgram.getImageBase().add(rva)
    refs=getReferencesTo(addr)
    print('\nGLOBAL', addr, 'REFS', len(refs))
    for r in refs:
        a=r.getFromAddress(); f=getFunctionContaining(a)
        print('REF',a,'FUNC', f)
        ins=getInstructionAt(a)
        for _ in range(5):
            if ins and ins.getPrevious(): ins=ins.getPrevious()
        for _ in range(12):
            if not ins: break
            print(' ',ins.getAddress(),ins)
            ins=ins.getNext()
