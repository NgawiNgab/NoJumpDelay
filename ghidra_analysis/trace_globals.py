#@category Analysis
import re
fm=currentProgram.getFunctionManager(); rm=currentProgram.getReferenceManager()
f=open('/home/user/analysis/global_xrefs.tsv','w'); f.write('key\tglobal\tfrom\tfunction\n')
for line in open('/home/user/analysis/key_globals.tsv').readlines()[1:]:
    a=line.strip().split('\t')
    if len(a)!=2: continue
    key,glob=a
    addr=toAddr(glob.replace('DAT_',''))
    for ref in rm.getReferencesTo(addr):
        src=ref.getFromAddress(); fn=fm.getFunctionContaining(src)
        f.write('%s\t%s\t%s\t%s\n'%(key,glob,src,fn.getEntryPoint() if fn else ''))
f.close()
print('Global references exported')
