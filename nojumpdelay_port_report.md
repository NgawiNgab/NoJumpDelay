# NoJumpDelay reverse-engineering and port report

**Status: incomplete 1:1 port.** The feature-specific component lookup and
write were recovered, and an inferred hook address matched an independent
BedrockTools tick signature in the supplied game ELF. The hook's *complete*
activation preconditions and live obfuscation variables were not verified.
The supplied project is therefore a **non-operational, fail-closed research
prototype**, not a verified 1:1 mod. It intentionally installs no detour or
patch. No device/runtime test was performed.

## 1. Decompiled and disassembled ReverseSigma logic

### Input and provenance

- `/home/user/libReverseSigma.so`: 3,473,688 bytes; SHA-256
  `c48da8d725e3515ef80e686c58a6a6fa2178ec7cce6b21d1356f53468d0c5b6f`.
- Existing project restored from the prior workspace backup, processed with
  Ghidra 11.2.1 (`-process libReverseSigma.so -noanalysis`, `MAXMEM=1400M`,
  SerialGC). Ghidra addresses below have image base `0x100000`; subtract that
  base for ReverseSigma ELF virtual addresses.
- ASCII config string `noJumpDelay` is at ELF `0x3f931`, Ghidra `0x13f931`.
  Config load writes byte `DAT_00457ee4`, save reads it, and the UI function
  takes its address. The only executable feature-path direct read in the
  recovered XREF list is `0x1ae2d8`, inside `FUN_001abb30`.

### Feature call and surrounding guards

The recovered `FUN_001abb30` is installed as a detour. It normally forwards its
one apparent pointer argument to the saved original at `DAT_0045aeb0`. Before
reaching the jump-delay call, its decompilation checks the panic flag
`DAT_00457f68`, `DAT_002d4a20`, `this+0x228`, a game attribute lookup whose
result `+0x7c` must convert to a nonzero integer, and pointers at `this+0xa08`
and `this+0x1e0`. The function also performs other mod logic before the
following block. It does **not** simply run a direct offset write on every tick.
The full surrounding code is in `ghidra_analysis/ghidra_decompiled.c`.

```c
// Faithful extraction of this branch only, not the full detour.
if ((DAT_004580d0 & 1) == 0) {
    if (DAT_00457ee4 == 1)
        FUN_001b1598((char*)this_ptr + 8, 0);
    if (DAT_00457ec0 == 1)
        ladder_fly_callback((char*)this_ptr + 8, 0x12, 1, 0);
}
original(this_ptr);
```

`DAT_004580d0` is set/reset elsewhere based on hashes of a string; the previous
binary analysis found the feedback string `Bedrock Anti Cheat Detected.` but
that alone is **not** a sufficient reimplementation of the guard. The call is
**before** the original. The prior guards and the exact value/lifetime of the
component context matter.

Exact ReverseSigma AArch64, Ghidra addresses and little-endian instruction
bytes:

```asm
001ae2c8 481500d0  adrp x8, 0x458000
001ae2cc 08414339  ldrb w8, [x8, #0xd0]
001ae2d0 68020037  tbnz w8, #0, 0x001ae31c
001ae2d4 481500b0  adrp x8, 0x457000
001ae2d8 08917b39  ldrb w8, [x8, #0xee4]
001ae2dc 1f050071  cmp w8, #1
001ae2e0 81000054  b.ne 0x001ae2f0
001ae2e4 e0031caa  mov x0, x28
001ae2e8 e1031f2a  mov w1, wzr
001ae2ec ab0c0094  bl 0x001b1598
...
001ae31c 68150090  adrp x8, 0x45a000
001ae320 e00313aa  mov x0, x19
001ae324 085947f9  ldr x8, [x8, #0xeb0]
001ae328 00013fd6  blr x8
```

### Component helper

`FUN_001b1598` takes the context at `this+8` in x0 and a 32-bit value in w1.
It performs a hash-bucket and sparse-set lookup for key `0xc52c8f78`, validates
indices/generation and pointers, and writes w1 at
`chunk + (packed & 0x7f)*0x14 + 0x10`. This is **not** the same as a direct
fixed `this+0x10` field: `+0x10` is within an entry in the resolved component
chunk. At the observed call site w1 is zero. The component's semantic name is
not recovered; identifying it as a named `jumpTicks` type would be speculation.

Ghidra's complete helper pseudocode and exact 53-instruction disassembly are in
[nojump_ghidra.txt](ghidra_analysis/nojump_ghidra.txt); the independently
transcribed C++ is [ComponentLookup.cpp](NoJumpDelayMod/src/ComponentLookup.cpp).
The prototype adds some null checks and is not claimed binary-identical. Its
host synthetic tests cover a successful zero write and several early returns.
That is not a runtime test against Minecraft.

The critical final instructions are:

```asm
001b1658 49194092  and x9, x10, #0x7f
001b165c 8a028052  mov w10, #0x14
001b1660 2821aa9b  umaddl x8, w9, w10, x8
001b1664 011100b9  str w1, [x8, #0x10]
001b1668 c0035fd6  ret
```

### Hook target recovery and uncertainty

`JNI_OnLoad` calls the hook wrapper `FUN_0025eca8` at `0x1bc980` with x1 =
`FUN_001abb30`, x2 = `&DAT_0045aeb0`, x0 = resolved module base + decoded
32-bit value. A focused Unicorn execution of the obfuscated decode block
`0x1bc660..0x1bc980` with the two globals returned by `FUN_0025eac0/eacc` set
to **zero** (their initial BSS values) produced offset `0x0af5e460`. Script:
[decode_hook.py](ghidra_analysis/decode_hook.py). This is conditional on those
globals being zero at the actual hook installation; that runtime assumption
has not been independently confirmed. The original uses a `/proc/self/maps`
resolver `FUN_001931b8` for its base; the module-name setup is obfuscated.

## 2. Supplied game-library verification

All seven Google Drive parts were downloaded through the connector and MD5s
matched Drive metadata. The reassembled ZIP contained a single
`libminecraftpe.so` (342,231,392 bytes). It is at
`/home/user/mc_1_26_60/libminecraftpe.so`.

- SHA-256: `4e7f97ecf1989edfaa7ca4805acc907a63bd4c56f1b1965079b2c47f63e3a7c4`
- ELF64 little-endian AArch64 ET_DYN; build ID
  `eb291b883dfdfaff70c4999c2eabd61885ae3e15`.
- Embedded version string: `1.26.60.04_RC3`. The precise variant, not merely
  the marketing version, matters.
- At ELF VA/file offset `0x0af5e460` the **original**, unpatched first 32 bytes
  are:
  `e8 0f 19 fc fd 7b 01 a9 fc 6f 02 a9 fa 67 03 a9 f8 5f 04 a9 f6 57 05 a9 f4 4f 06 a9 fd 43 00 91`.
- Prologue:

```asm
0af5e460  str d8, [sp, #-0x70]!
0af5e464  stp x29, x30, [sp, #0x10]
0af5e468  stp x28, x27, [sp, #0x20]
0af5e46c  stp x26, x25, [sp, #0x30]
0af5e470  stp x24, x23, [sp, #0x40]
0af5e474  stp x22, x21, [sp, #0x50]
0af5e478  stp x20, x19, [sp, #0x60]
0af5e47c  add x29, sp, #0x10
0af5e480  sub sp, sp, #0x270
0af5e484  mrs x20, tpidr_el0
0af5e488  mov x19, x0
0af5e48c  ldr x8, [x20, #0x28]
0af5e490  stur x8, [x29, #-0x20]
0af5e494  ldrb w8, [x0, #0xe50]
```

The BedrockTools `NormalTick` byte signature uniquely matched `0x0af5e460` in
the supplied ELF. This is independent corroboration of a tick-like hook
location, **not** proof that the obfuscation globals have their assumed runtime
values or that the two game binaries correspond exactly. `readelf -sW` did not
recover a game `LocalPlayer` / `normalTick` export at that address. No game
instruction bytes are patched by the recovered feature branch; an inline hook
framework may alter target prologue bytes at runtime, but those hook-engine
bytes were not measured and are not a NoJumpDelay NOP patch. No full Ghidra
auto-analysis was run on the 342 MB game library.

## 3. Port source, loader and build

- Source: `/home/user/NoJumpDelayMod/`; repository copy:
  [NoJumpDelayMod](NoJumpDelayMod/). CMake, C++ source, manifest and synthetic
  tests are included. The lifecycle exposes `PLGetModRegistration` via the
  `PL_REGISTER_MOD` macro; it logs and returns false from `enable`.
- Reference: QYCottage/BedrockTools commit
  `56d974c3c178c66d6f2494693cb1d121f2aef032` uses xmake, **not** CMake; its
  entry is `PL_REGISTER_MOD`, and its hook helper uses
  `pl::memory::hook/unhook`. The referenced preloader-android source was
  inspected at commit `cc08b40a31471d3e2dedb267f5f17b28da2b0604`. The
  `preload-native` manifest follows BedrockTools' package script. The CMake
  project requires a matching external Preloader SDK and `libpreloader.so`.
- Host test: `g++ -std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined`
  passed the synthetic test in `tests/component_lookup_test.cpp`.
- No Android NDK, Android CMake or matching preloader library was present; the
  Android `.so` was **not compiled or verified**. No compiled `.so` path exists.
  Android build instructions are in the project's README and remain untested.

**Remaining work before an operational port:** establish actual runtime values
of the decode globals; recover and validate every outer guard including
attribute resolution and server state; confirm the real hook calling convention
and lifecycle against the intended launcher; verify the component ID/layout on
the target game in an authorized runtime; add build-ID and byte checks and
safe hook removal; then build with matching NDK and Preloader and test on device.

## Persistence and storage

The first step reused the project **backup** because `/home/user` from the prior
turn was absent in this sandbox. Ghidra and the project were restored under
`/home/user`; TMPDIR, JVM temp and Ghidra cache were set under
`/home/user/analysis`. Only workspace files are guaranteed to survive sandbox
replacement; the large proprietary game library and Ghidra install are not
included in the repository snapshot. The report and source have workspace copies.

The GitHub PR omits the downloaded ReverseSigma binary and the generated Ghidra
project archive from version control. CI uploads research source and evidence
only; it does not claim to produce an operational Android mod.
