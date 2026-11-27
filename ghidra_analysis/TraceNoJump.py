#@category Analysis
from ghidra.app.decompiler import DecompInterface
fm=currentProgram.getFunctionManager(); listing=currentProgram.getListing(); rm=currentProgram.getReferenceManager()
f=open('/home/user/analysis/nojump_ghidra.txt','w')
d=DecompInterface();d.openProgram(currentProgram)
for target in ['001b1598','001ae2d8','001bc96c','001b5568','0025eca8']:
 a=toAddr(target); fn=fm.getFunctionContaining(a); f.write('\n=== %s in %s ===\n'%(target,fn));
 if not fn: continue
 if target in ('001b1598','0025eca8'):
  r=d.decompileFunction(fn,120,monitor);f.write(r.getDecompiledFunction().getC() if r.decompileCompleted() else r.getErrorMessage()); f.write('\n')
 if target!='001b5568':
  lo=a.subtract(80) if target in ('001ae2d8','001bc96c') else fn.getEntryPoint()
  hi=a.add(160) if target in ('001ae2d8','001bc96c') else fn.getBody().getMaxAddress()
  it=listing.getInstructions(lo,True)
  while it.hasNext():
   ins=it.next(); p=ins.getAddress()
   if p.compareTo(hi)>0: break
   b=ins.getBytes(); f.write('%s %s %s\n'%(p,''.join('%02x'%(x & 255) for x in b),ins))
for address in ['001b1598','001abb30','0045aeb0','00457ee4','00457b30']:
 f.write('\n=== refs to %s ===\n'%address)
 for ref in rm.getReferencesTo(toAddr(address)):
  fn=fm.getFunctionContaining(ref.getFromAddress()); f.write('%s %s %s\n'%(ref.getFromAddress(),ref.getReferenceType(),fn))
f.close(); d.dispose()
