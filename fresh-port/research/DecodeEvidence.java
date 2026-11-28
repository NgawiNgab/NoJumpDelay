// @category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
public class DecodeEvidence extends GhidraScript {
 public void run() throws Exception {
  try (PrintWriter w = new PrintWriter(new FileWriter("/home/user/re_workspace/decomp/decode_evidence.txt"))) {
   String[] p={"00457b78","00457b88","00457ba0","00457ba8","00457bb0","00457c00","0035aeb0","0035aea8"};
   for(String s:p) {
    w.println("GLOBAL "+s);
    ReferenceIterator it=currentProgram.getReferenceManager().getReferencesTo(toAddr(s));
    while(it.hasNext()) { Reference r=it.next(); Function f=getFunctionContaining(r.getFromAddress()); w.println(r.getFromAddress()+" "+(f == null ? "?" : f.getName()+" @ "+f.getEntryPoint())); }
   }
   DecompInterface di=new DecompInterface(); di.openProgram(currentProgram);
   Function f=getFunctionAt(toAddr("001b5568"));
   DecompileResults d=di.decompileFunction(f,300,monitor);
   w.println("\n===== JNI_OnLoad =====\n"+(d.decompileCompleted()?d.getDecompiledFunction().getC():"FAIL "+d.getErrorMessage()));
   di.dispose();
  }
 }
}
