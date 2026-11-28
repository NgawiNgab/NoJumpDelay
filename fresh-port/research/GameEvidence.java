// @category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.*;
public class GameEvidence extends GhidraScript {
 public void run() throws Exception {
  long[] rvas={0x0af5e460L,0x10383480L,0x10b16414L,0x103b4754L,0x1009cba4L,0xfdabbfcL,0xfd8e120L,0xfd881d4L,0xaf60d78L,0x9b6bbd4L,0x9b94c98L};
  try(PrintWriter w=new PrintWriter(new FileWriter("/home/user/re_workspace/decomp/game_evidence.txt"))) {
   w.println("image base "+currentProgram.getImageBase());
   DecompInterface di=new DecompInterface();di.openProgram(currentProgram);
   for(long rva:rvas) {
    Address addr=currentProgram.getImageBase().add(rva);
    w.println("===== RVA "+Long.toHexString(rva)+" Ghidra "+addr+" ====="); w.flush();
    try {
     if(!disassemble(addr)) w.println("disassembly reported failure");
     Function f=getFunctionAt(addr); if(f==null)f=createFunction(addr,null);
     if(f==null){w.println("function creation failed");continue;}
     DecompileResults d=di.decompileFunction(f,120,monitor);
     w.println(d.decompileCompleted()?d.getDecompiledFunction().getC():"decompilation failed "+d.getErrorMessage());
    }catch(Exception ex){w.println("ERROR "+ex);}
    w.flush();
   }
   di.dispose();
  }
 }
}
