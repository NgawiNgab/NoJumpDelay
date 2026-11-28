// Locate exact configuration strings, print cross-references and decompile callers.
// @category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

public class KillAuraGlobals extends GhidraScript {
  public void run() throws Exception {
    String[] globals = {"00457eb4", "002d47a4", "002d47b0", "002d47b4", "00458030", "002d47d0", "002d47a8", "002d47cc", "00457ff0", "00457ee0", "00458040", "00457f84", "00457eb8", "00457f94", "002d47d4", "00458060"};
    File out = new File("/home/user/re_workspace/decomp/killaura_globals.txt");
    try (PrintWriter w = new PrintWriter(new FileWriter(out))) {
      DecompInterface di = new DecompInterface();
      di.openProgram(currentProgram);
      Set<Address> functions = new TreeSet<>();
      for (String hex : globals) {
        Address target = toAddr(hex);
        w.println("GLOBAL " + hex);
        ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(target);
        while (refs.hasNext()) {
          Reference r = refs.next();
          Function f = getFunctionContaining(r.getFromAddress());
          w.println("  XREF " + r.getFromAddress() + " " + (f == null ? "no-function" : f.getName()+" @ "+f.getEntryPoint()));
          if (f != null && !f.getEntryPoint().equals(toAddr("001950f4")) && !f.getEntryPoint().equals(toAddr("001986cc"))) functions.add(f.getEntryPoint());
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
