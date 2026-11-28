// Locate exact configuration strings, print cross-references and decompile callers.
// @category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

public class KillAuraXrefs extends GhidraScript {
  public void run() throws Exception {
    String[] keys = {"killAura", "killRange", "minKillAuraCPS", "maxKillAuraCPS", "auraType", "rotations", "rotationsRange", "swing", "hurtCheck", "emulateNativeAttack", "friendList", "killAuraShortcut", "killItems", "killItemsShortcut", "antiBot", "botList", "Kill Near Items"};
    File out = new File("/home/user/re_workspace/decomp/killaura_xrefs.txt");
    try (PrintWriter w = new PrintWriter(new FileWriter(out))) {
      DecompInterface di = new DecompInterface();
      di.openProgram(currentProgram);
      Set<Address> functions = new TreeSet<>();
      for (String key : keys) {
        byte[] bytes = (key + "\0").getBytes("UTF-8");
        Address pos = currentProgram.getMinAddress();
        while (pos != null && !monitor.isCancelled()) {
          Address hit = currentProgram.getMemory().findBytes(pos, bytes, null, true, monitor);
          if (hit == null) break;
          w.println("KEY " + key + " @ " + hit);
          ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(hit);
          int n=0;
          while (refs.hasNext()) {
            Reference r = refs.next(); n++;
            Function f = getFunctionContaining(r.getFromAddress());
            w.println("  XREF " + r.getFromAddress() + " " + (f == null ? "no-function" : f.getName()+" @ "+f.getEntryPoint()));
            if (f != null) functions.add(f.getEntryPoint());
          }
          if (n == 0) w.println("  NO DIRECT XREF");
          pos = hit.next();
        }
      }
      for (Address addr : functions) {
        Function f = getFunctionAt(addr);
        w.println("\n===== " + f.getName() + " @ " + addr + " =====");
        DecompileResults result = di.decompileFunction(f, 120, monitor);
        if (result.decompileCompleted()) w.println(result.getDecompiledFunction().getC());
        else w.println("DECOMPILATION FAILED: " + result.getErrorMessage());
      }
      di.dispose();
    }
    println("Wrote " + out);
  }
}
