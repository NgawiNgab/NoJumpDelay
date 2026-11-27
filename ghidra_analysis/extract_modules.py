#@category Analysis
# Export cross-references to all printable strings in the target's rodata and decompile
# string-rich functions. Addresses are Ghidra addresses; image base is recorded.
from ghidra.app.decompiler import DecompInterface
from java.io import File
from collections import defaultdict
import re
out='/home/user/analysis/'
base=currentProgram.getImageBase()
fm=currentProgram.getFunctionManager()
rm=currentProgram.getReferenceManager()
f=open(out+'ghidra_xrefs.tsv','w')
f.write('string_file_offset\tghidra_address\txref_from\tfunction_entry\tfunction_name\tstring\n')
counts=defaultdict(int)
for line in open(out+'strings.txt'):
    m=re.match(r'^\s*([0-9a-f]+)\s+(.*)$',line.rstrip('\n'))
    if not m: continue
    off=int(m.group(1),16)
    if off<0x37c40 or off>=0x57ef4: continue
    s=m.group(2)
    addr=base.add(off)
    refs=rm.getReferencesTo(addr)
    for ref in refs:
        src=ref.getFromAddress(); fn=fm.getFunctionContaining(src)
        if fn:
            entry=fn.getEntryPoint(); name=fn.getName(); counts[str(entry)]+=1
        else: entry=''; name=''
        f.write('%x\t%s\t%s\t%s\t%s\t%s\n'%(off,addr,src,entry,name,s.replace('\t',' ')))
f.close()
f=open(out+'ghidra_functions.tsv','w'); f.write('entry\tname\trodata_xrefs\n')
funcs=fm.getFunctions(True)
while funcs.hasNext():
    fn=funcs.next(); entry=str(fn.getEntryPoint())
    f.write('%s\t%s\t%d\n'%(entry,fn.getName(),counts.get(entry,0)))
f.close()
f=open(out+'ghidra_metadata.txt','w'); f.write('image_base=%s\nfunctions=%d\n'%(base,fm.getFunctionCount())); f.close()
# Decompile the most string-rich functions and JNI entry points.
dec=DecompInterface(); dec.openProgram(currentProgram)
selected=set([a for a,c in sorted(counts.items(),key=lambda x:x[1],reverse=True)[:70]])
funcs=fm.getFunctions(True)
while funcs.hasNext():
    fn=funcs.next()
    if fn.getName().startswith('Java_com_subroutineZEFF') or fn.getName()=='JNI_OnLoad': selected.add(str(fn.getEntryPoint()))
f=open(out+'ghidra_decompiled.c','w')
for addr in sorted(selected):
    if monitor.isCancelled(): break
    fn=fm.getFunctionAt(toAddr(addr))
    if fn is None: continue
    f.write('\n\n/* FUNCTION %s %s string_refs=%d */\n'%(addr,fn.getName(),counts.get(addr,0))); f.flush()
    res=dec.decompileFunction(fn,90,monitor)
    if res.decompileCompleted(): f.write(res.getDecompiledFunction().getC())
    else: f.write('/* failed: %s */\n'%res.getErrorMessage())
f.close(); dec.dispose()
print('Extraction complete: '+out)
