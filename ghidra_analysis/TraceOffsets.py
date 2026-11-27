#@category Analysis
from ghidra.app.decompiler import DecompInterface
fm=currentProgram.getFunctionManager();rm=currentProgram.getReferenceManager();li=currentProgram.getListing();d=DecompInterface();d.openProgram(currentProgram);f=open('/home/user/analysis/offset_trace.txt','w')
for a in ['0045fee0','0045fee8','0025eac0','0025eacc','001931b8']:
 f.write('\n=== '+a+' ===\n')
 for r in rm.getReferencesTo(toAddr(a)):
  fn=fm.getFunctionContaining(r.getFromAddress());f.write('%s %s %s\n'%(r.getFromAddress(),r.getReferenceType(),fn))
 if a.startswith('00') and fm.getFunctionAt(toAddr(a)):
  fn=fm.getFunctionAt(toAddr(a));res=d.decompileFunction(fn,120,monitor);f.write(res.getDecompiledFunction().getC() if res.decompileCompleted() else res.getErrorMessage())
f.close(); d.dispose()
