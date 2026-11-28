// @category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.Function;
import ghidra.program.model.address.Address;
import java.io.*;
public class CombatHelpers extends GhidraScript {
 public void run() throws Exception {
  String[] entries = {"001abb30", "001ab7d4", "001ae86c", "001aecd0", "001aede8", "001aef80", "001af07c", "001afa60"};
  try (PrintWriter w = new PrintWriter(new FileWriter("/home/user/re_workspace/decomp/combat_helpers.txt"))) {
   DecompInterface di = new DecompInterface(); di.openProgram(currentProgram);
   for (String entry : entries) {
    Address a=toAddr(entry); Function f=getFunctionAt(a);
    w.println("===== " + entry + " =====");
    if (f == null) { w.println("no function"); continue; }
    DecompileResults d=di.decompileFunction(f,120,monitor);
    w.println(d.decompileCompleted()? d.getDecompiledFunction().getC():"DECOMPILATION FAILED: "+d.getErrorMessage());
    w.flush();
   }
   di.dispose();
  }
 }
}
