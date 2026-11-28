# Independent investigation: 1.26.60.4 arm64 (incomplete)

**Status: runtime hooks have been implemented, but no runtime binding, ABI,
hook behavior, parity or package has been device-validated.** Existing project research and code were not used. A unique
byte-pattern match is a candidate, not semantic or calling-convention proof.

## Inputs and reproducibility

* The supplied Drive `libReverseSigma.so` is an AArch64 ELF; SHA-256
  `c48da8d725e3515ef80e686c58a6a6fa2178ec7cce6b21d1356f53468d0c5b6f`,
  GNU build ID `59cc05d99075de296ca33243f64866354adb5910`.
* The supplied Drive `1.26.60.4` directory contained seven split ZIP parts.
  Each downloaded part matched its Drive MD5 before concatenation, and the ZIP
  extracted `libminecraftpe.so`, AArch64 ELF, SHA-256
  `4e7f97ecf1989edfaa7ca4805acc907a63bd4c56f1b1965079b2c47f63e3a7c4`,
  build ID `eb291b883dfdfaff70c4999c2eabd61885ae3e15`. Version attribution
  follows the Drive folder name, not an independently parsed game version.
* BedrockTools independent reference: QYCottage/BedrockTools commit
  `56d974c3c178c66d6f2494693cb1d121f2aef032`. Its signature definitions are
  candidates for investigation, not a guarantee of 1.26.60.4 compatibility.
* Ghidra 11.2.1, bundled JDK 21, `MAXMEM=1200M`, max 2 CPUs. A fresh Ghidra
  import and automatic analysis succeeded; Ghidra reported unhandled TLSDESC
  relocations and unresolved external libraries. Results must be interpreted
  with those limitations. Ghidra's image base is `0x100000`, so addresses below
  are Ghidra addresses, **not** game RVAs. Raw output and project are in
  `/home/user/re_workspace/decomp` and `/home/user/re_workspace/ghidra_proj`.
* `fresh-port/research/run_ghidra.sh` creates a new `SigmaFresh` project and runs both independent
  scripts. It refuses to overwrite an existing project. Each script writes to
  `/home/user/re_workspace/decomp`; it is not a complete indirect-data-flow
  analysis. No proprietary binary is committed.

## Config and data-reference trace

Direct references to each of `killAura`, `killRange`, `minKillAuraCPS`,
`maxKillAuraCPS`, `auraType`, `rotations`, `rotationsRange`, `swing`,
`hurtCheck`, `emulateNativeAttack`, `friendList`, `killAuraShortcut`,
`killItems`, `killItemsShortcut`, `antiBot` and `botList` were found in
functions `FUN_001950f4` and `FUN_001986cc`. `Kill Near Items` has two code
references in `FUN_0020e850` and a data reference. The first two functions
appear to read/write configuration; the inference is based on the decompiled
key accesses, not on external symbol names.

The config load associates `killAura` with `DAT_00457eb4`, `killItems` with
`DAT_00457eb8`, `antiBot` with `DAT_002d47d4`, `hurtCheck` with
`DAT_00457ff0`, and `auraType` with `DAT_00458030`. Reference analysis of
these globals points to `FUN_001abb30`, which also references the other
combat settings, lists and game-function pointer globals. It is a strong
candidate for the shared logic, not a verified game hook address.
`FUN_001ab7d4` and `FUN_001ae86c` also reference rotation/CPS settings.

The decompiled candidate core contains a conditional `hurtCheck` comparison
against an integer read at candidate actor pointer + `0x1b4` (skip when not
less than 2), a 6.0 distance comparison in a `killItems` branch, and indirect
calls through `DAT_00457b88` and `DAT_00457c00`. These are **observations in
Sigma**, not independently validated 1.26.60.4 class layouts or function ABIs.
`FUN_001ae86c` uses `steady_clock`, integer `1000 / CPS`, `rand()` and clamps
subsequent sampled CPS to 1–20. Its initialization has distinct behavior and
needs separate analysis before faithfully implementing edge cases.

## Preliminary executable-pattern scan

The independently written `tools/scan_signatures.py` scans only executable
ELF PT_LOAD bytes and reports all matches. Reproduction:

```sh
python3 fresh-port/tools/scan_signatures.py \
  /home/user/re_workspace/mc_libs/1.26.60.4/libminecraftpe.so \
  /home/user/re_workspace/BedrockTools/src/core/memory/Signatures.cpp \
  NormalTick ActorManagerList GameModeAttack
```

| BedrockTools pattern | Hits | Candidate game RVA | First 16 bytes |
|---|---:|---:|---|
| NormalTick | 1 | `0x0af5e460` | `e8 0f 19 fc fd 7b 01 a9 fc 6f 02 a9 fa 67 03 a9` |
| ActorManagerList | 1 | `0x103b4754` | `fd 7b ba a9 fc 6f 01 a9 fa 67 02 a9 f8 5f 03 a9` |
| GameModeAttack | 1 | `0x1009cb98` | `e3 03 02 aa 22 00 80 52 01 00 00 14 ff 43 07 d1` |

The last match begins with a short instruction sequence preceding another
possible entry, emphasizing why pattern hits alone cannot establish the correct
ABI or hook site. Prologue matching alone cannot prove that runtime hooks are safe.

## Required before device testing

Trace the indirect game-function table and hook installation back to their
runtime targets; verify against the exact game binary with disassembly and
semantic/calling-convention analysis, including actor iteration and lifetime,
all offsets, swing, rotations, all three aura modes, CPS initialization,
friend/bot filtering and emulated native attack. Implement and test a new
LeviLauncher module only after those are established, with fail-closed in-memory
verification, native UI and persistence. Then produce an Android arm64 build,
verify every PT_LOAD alignment is at least `0x4000`, check exports and package
layout, run host sanitizers and CI, and only then publish an artifact/PR for
device testing. No claim of 1:1 equivalence is justified yet.

## Follow-up: simulated decoder and direct target disassembly

A new, independent Unicorn harness `fresh-port/research/trace_decoder.py`
executes the AArch64 `JNI_OnLoad` decoder in isolation. The harness is a
**model**, not an Android runtime: it intercepts the library-discovery path,
substitutes a game load base and JavaVM, fakes the hook installer, and stubs
many imported functions (including `strlen`) with simplified behavior. It
applies Sigma's ELF relative and symbol relocations; it does not execute real
hooks or prove all original initialization side effects. It must not be used
as a hook installer. Two successful synthetic-base runs (`0x5000000000` and
`0x6000000000`) yielded the same 58 game-relative hook targets and 28 table
values. Reproduction, with `unicorn==2.1.2` and `pyelftools==0.32` installed:

```sh
PYTHONPATH=/home/user/re_workspace/python python3 fresh-port/research/trace_decoder.py
GAME_BASE=0x6000000000 PYTHONPATH=/home/user/re_workspace/python \
  python3 fresh-port/research/trace_decoder.py
```

The modeled installer associates Sigma RVA `0xabb30` with game RVA
`0x0af5e460`, and Sigma RVA `0xab7d4` with game RVA `0x10383480`. Direct
reads from the exact game ELF give respectively these 16-byte sequences:

```text
0x0af5e460  e8 0f 19 fc fd 7b 01 a9 fc 6f 02 a9 fa 67 03 a9
0x10383480  ff 83 01 d1 fd 7b 01 a9 fa 67 02 a9 f8 5f 03 a9
```

The former independently matches the BedrockTools `NormalTick` signature.
The latter is not semantically established merely by a valid prologue.

### Table entries needed by the combat candidate

`Sigma slot` below is the binary-relative address of the modeled table cell,
not its Ghidra address. Bytes are read from the target file, not from a live
process. Names in the last column are interpretations, not recovered debug
symbols.

| Sigma slot | Modeled game RVA | First 16 file bytes | Supporting evidence / qualification |
|---|---:|---|---|
| `0x357b78` | `0x10b16414` | `00 3c 42 f9 cf 78 e2 17 08 14 40 f9 68 00 00 b4` | `ldr x0,[x0,#0x478]` followed by a branch to `0x103b4754`. |
| `0x357b80` | `0x10b13684` | `fd 7b bf a9 fd 03 00 91 08 00 40 f9 08 b9 42 f9` | Indirect call in Sigma; role not yet proven. |
| `0x357b88` | `0x1009cba4` | `ff 43 07 d1 fd 7b 18 a9 fc cb 00 f9 f8 5f 1a a9` | Unique match for BedrockTools `GameModeAttackInternal`. Sigma passes four arguments. |
| `0x357ba0` | `0x0fdabbfc` | `fd 7b bf a9 fd 03 00 91 09 08 40 f9 6a c2 80 52` | Called with actor and one of three pointer-table entries; component role remains to be verified. |
| `0x357ba8` | `0x0fd8e120` | `09 08 40 f9 ea b1 84 52 aa 2b bf 72 2b a1 43 a9` | Indirect classifier call; semantics not yet proven. |
| `0x357bb0` | `0x0fd881d4` | `fd 7b be a9 f3 0b 00 f9 fd 03 00 91 09 08 40 f9` | Result is used for a string field; exact component type remains unproven. |
| `0x357bf0` | `0x101c2d7c` | `09 04 40 f9 ea bb 9d 52 6a 1a b1 72 2b a1 43 a9` | Used by combat candidate; exact classifier remains unproven. |
| `0x357c00` | `0x0af60d78` | `ff c3 02 d1 fd 7b 07 a9 f7 43 00 f9 f6 57 09 a9` | Unique BedrockTools `LocalPlayerSwing` signature; Sigma passes `(player, 4, 0)`. |
| `0x357c10` | `0x09b6bbd4` | `fd 7b bd a9 f6 57 01 a9 f4 4f 02 a9 fd 03 00 91` | Used in emulated-attack branch; target semantics not fully identified. |
| `0x357c30` | `0x09b94c98` | `e8 03 00 aa 00 40 41 f9 60 00 00 b4 1f 41 01 f9` | Used in emulated-attack branch. Target code clears/deallocates a pointer at `+0x280`; this is not sufficient to call it an attack primitive. |

Modeled slots `0x357c38`, `0x357c40`, `0x357c48` point respectively to
`0x141d8f48`, `0x141d8d68`, `0x141d9e30` inside the game ELF's NOBITS `.bss`.
They are data addresses, not function prologues; their runtime values and
semantic types are unverified.

Direct target disassembly clarifies an important calling convention:
`0x10b16414` branches to `0x103b4754`, which writes a three-pointer vector
using AArch64's hidden result pointer in `x8`; `x0` is the Level argument
before the thunk. A naïve `void(vector*, Level*)` function pointer would put
its arguments in the wrong registers. The target's vector allocation and
ownership must also be matched to the game ABI before use. The BedrockTools
`ActorManagerList` signature independently matches the branch target.

The `GameModeAttack` signature at `0x1009cb98` identifies a wrapper which
moves `x2` to `x3`, sets `w2 = 1`, and branches to `0x1009cba4`; the latter
is the modeled indirect target and separately matches `GameModeAttackInternal`.
The distinction is significant when setting up arguments.

### Additional combat observations and limitations

Ghidra's `FUN_001abb30` decompilation is 1,589 lines and includes other
features as well as Kill Aura and killItems. In the player candidate path it
reads pointers at actor `+0x228` and `+0x230`, a Level pointer at `+0x1f0`,
and a game-mode candidate at `+0xa08`. It uses a position adjustment of
`1.62`, a closest-point-to-box distance calculation, a separately configured
rotation range, and reads `hurtTime` at `+0x1b4` before attack. The antiBot
flag gates a short-name/box-size check; bot-list matching occurs in surrounding
logic and must not be simplified to an unconditional claim about the full
filter. The sub-module item branch compares an FNV-1a hash (`0xf228491f`),
checks distance against `6.0`, and calls the modeled attack/swing pointers.
The entity type represented by that hash still needs an independent check.

The rotation detour reads a component pointer at actor `+0x238` and also
traverses a component registry before writing. It would be unsafe to replace
this with a direct LookVec write without establishing the registry component,
its lifetime and the detour's arguments. The game thunk above directly
corroborates `Level + 0x478`; BedrockTools independently lists `+0x228`,
`+0x238`, `+0x1f0`, and `+0x1b4`, but those references alone do not prove
every structure field's meaning or safety in the exact target build.

A separate Ghidra import of the game ELF was run with `-noanalysis`,
`MAXMEM=1200M`, and targeted disassembly/decompilation
(`fresh-port/research/GameEvidence.java`). It reported unresolved external
libraries; some decompilations also contain control-flow warnings and must not
be treated as verified complete C++ signatures. Output is in the external
workspace's `decomp/game_evidence.txt`. No device-level verification occurred.

**Outstanding:** decode and validate all player-selection gates, friend/bot
matching behavior, all mode and CPS edge cases, registry writes, object
lifetimes, initialization assumptions and ABI/calling conventions. Until then
these are research findings, not a complete verified mapping or a 1:1 port.

The full Sigma combat candidate, rotation detour, timing routine and selected
helper decompilations can be regenerated via `CombatHelpers.java`; they are
written to `/home/user/re_workspace/decomp/combat_helpers.txt`. The distance
helper at Ghidra `001aede8` computes a Euclidean distance to a clamped box;
its Y interval is `[target_y - height, target_y]`. The rotation helper at
`001aef80` clamps against a differently oriented Y interval
`[target_y, target_y + height]` and calls `atan2f`; Ghidra's displayed return
alone is not proof of the complete return ABI. These details rule out a simple
center-to-center distance replacement as a 1:1 implementation. The game-side
analysis can be regenerated with `fresh-port/research/run_game_ghidra.sh`.

For persistence, `research/decomp/SigmaCombat-ghidra.txt` contains whitespace-normalized
Ghidra C-like output for the combat candidate, rotation detour, timing function
and selected helper routines. This is analysis output, not clean compilable
source; warnings and incomplete type recovery are preserved. The target game's
binary and Ghidra project remain outside the repository.

## Capstone verification output

The following is reproducible with `PYTHONPATH=/home/user/re_workspace/python
python3 fresh-port/tools/verify_capstone.py`. RVAs are translated through ELF
PT_LOAD `p_vaddr`/`p_offset` mappings. This is raw, limited disassembly; it
does **not** establish argument types, full function boundaries, object
provenance, return conventions beyond the visible instructions, or all the
requested target structure fields. Sigma displacements are not themselves
proof of target-game layout. Instructions after a `ret` may be the next
function. Additional dynamic/semantic validation remains required.

```text
Game SHA-256: 4e7f97ecf1989edfaa7ca4805acc907a63bd4c56f1b1965079b2c47f63e3a7c4
Sigma SHA-256: c48da8d725e3515ef80e686c58a6a6fa2178ec7cce6b21d1356f53468d0c5b6f

=== Target game disassembly (first 64 bytes per RVA) ===

tick RVA 0xaf5e460 bytes16=e8 0f 19 fc fd 7b 01 a9 fc 6f 02 a9 fa 67 03 a9
  00af5e460  str      d8, [sp, #-0x70]!
  00af5e464  stp      x29, x30, [sp, #0x10]
  00af5e468  stp      x28, x27, [sp, #0x20]
  00af5e46c  stp      x26, x25, [sp, #0x30]
  00af5e470  stp      x24, x23, [sp, #0x40]
  00af5e474  stp      x22, x21, [sp, #0x50]
  00af5e478  stp      x20, x19, [sp, #0x60]
  00af5e47c  add      x29, sp, #0x10
  00af5e480  sub      sp, sp, #0x270
  00af5e484  mrs      x20, tpidr_el0
  00af5e488  mov      x19, x0
  00af5e48c  ldr      x8, [x20, #0x28]
  00af5e490  stur     x8, [x29, #-0x20]
  00af5e494  ldrb     w8, [x0, #0xe50]
  00af5e498  cbz      w8, #0xaf5e4f4
  00af5e49c  ldr      w8, [x19, #0xcb0]

rotation detour target RVA 0x10383480 bytes16=ff 83 01 d1 fd 7b 01 a9 fa 67 02 a9 f8 5f 03 a9
  010383480  sub      sp, sp, #0x60
  010383484  stp      x29, x30, [sp, #0x10]
  010383488  stp      x26, x25, [sp, #0x20]
  01038348c  stp      x24, x23, [sp, #0x30]
  010383490  stp      x22, x21, [sp, #0x40]
  010383494  stp      x20, x19, [sp, #0x50]
  010383498  add      x29, sp, #0x10
  01038349c  mrs      x23, tpidr_el0
  0103834a0  ldr      x8, [x23, #0x28]
  0103834a4  str      x8, [sp, #8]
  0103834a8  ldr      x9, [x0, #8]
  0103834ac  ldr      x10, [x9]
  0103834b0  tbnz     x10, #0x3f, #0x10383570
  0103834b4  ldr      x24, [x0]
  0103834b8  lsl      x8, x10, #3
  0103834bc  mov      x20, x0

level vector thunk RVA 0x10b16414 bytes16=00 3c 42 f9 cf 78 e2 17 08 14 40 f9 68 00 00 b4
  010b16414  ldr      x0, [x0, #0x478]
  010b16418  b        #0x103b4754
  010b1641c  ldr      x8, [x0, #0x28]
  010b16420  cbz      x8, #0x10b1642c
  010b16424  ldrb     w8, [x8]
  010b16428  tbz      w8, #0, #0x10b16434
  010b1642c  ldr      x0, [x0, #0x38]
  010b16430  ret
  010b16434  mov      w9, #0xc0de
  010b16438  mov      x8, xzr
  010b1643c  movk     w9, #0xdead, lsl #16
  010b16440  str      w9, [x8]
  010b16444  ldr      x0, [x0, #0x38]
  010b16448  ret
  010b1644c  stp      x29, x30, [sp, #-0x20]!
  010b16450  stp      x20, x19, [sp, #0x10]

actor manager vector RVA 0x103b4754 bytes16=fd 7b ba a9 fc 6f 01 a9 fa 67 02 a9 f8 5f 03 a9
  0103b4754  stp      x29, x30, [sp, #-0x60]!
  0103b4758  stp      x28, x27, [sp, #0x10]
  0103b475c  stp      x26, x25, [sp, #0x20]
  0103b4760  stp      x24, x23, [sp, #0x30]
  0103b4764  stp      x22, x21, [sp, #0x40]
  0103b4768  stp      x20, x19, [sp, #0x50]
  0103b476c  mov      x29, sp
  0103b4770  ldr      x24, [x0, #0x18]
  0103b4774  stp      xzr, xzr, [x8]
  0103b4778  str      xzr, [x8, #0x10]
  0103b477c  cbz      x24, #0x103b4854
  0103b4780  mov      x20, x0
  0103b4784  mov      x19, x8
  0103b4788  mov      x27, xzr
  0103b478c  mov      x26, #0x1fffffffffffffff
  0103b4790  b        #0x103b47a4

attack internal RVA 0x1009cba4 bytes16=ff 43 07 d1 fd 7b 18 a9 fc cb 00 f9 f8 5f 1a a9
  01009cba4  sub      sp, sp, #0x1d0
  01009cba8  stp      x29, x30, [sp, #0x180]
  01009cbac  str      x28, [sp, #0x190]
  01009cbb0  stp      x24, x23, [sp, #0x1a0]
  01009cbb4  stp      x22, x21, [sp, #0x1b0]
  01009cbb8  stp      x20, x19, [sp, #0x1c0]
  01009cbbc  add      x29, sp, #0x180
  01009cbc0  mrs      x24, tpidr_el0
  01009cbc4  mov      x19, x0
  01009cbc8  mov      x20, x3
  01009cbcc  ldr      x8, [x24, #0x28]
  01009cbd0  mov      w21, w2
  01009cbd4  mov      x22, x1
  01009cbd8  stur     x8, [x29, #-8]
  01009cbdc  ldr      x0, [x0, #8]
  01009cbe0  bl       #0xfd8d9a4

swing RVA 0xaf60d78 bytes16=ff c3 02 d1 fd 7b 07 a9 f7 43 00 f9 f6 57 09 a9
  00af60d78  sub      sp, sp, #0xb0
  00af60d7c  stp      x29, x30, [sp, #0x70]
  00af60d80  str      x23, [sp, #0x80]
  00af60d84  stp      x22, x21, [sp, #0x90]
  00af60d88  stp      x20, x19, [sp, #0xa0]
  00af60d8c  add      x29, sp, #0x70
  00af60d90  mrs      x23, tpidr_el0
  00af60d94  mov      w21, w2
  00af60d98  mov      w22, w1
  00af60d9c  ldr      x8, [x23, #0x28]
  00af60da0  mov      x20, x0
  00af60da4  stur     x8, [x29, #-8]
  00af60da8  bl       #0xfa887f0
  00af60dac  mov      w19, w0
  00af60db0  tbz      w0, #0, #0xaf60de8
  00af60db4  mov      x8, sp

component RVA 0xfdabbfc bytes16=fd 7b bf a9 fd 03 00 91 09 08 40 f9 6a c2 80 52
  00fdabbfc  stp      x29, x30, [sp, #-0x10]!
  00fdabc00  mov      x29, sp
  00fdabc04  ldr      x9, [x0, #0x10]
  00fdabc08  mov      w10, #0x613
  00fdabc0c  movk     w10, #0xfd3b, lsl #16
  00fdabc10  ldp      x11, x8, [x9, #0x38]
  00fdabc14  sub      x8, x8, x11
  00fdabc18  lsr      x8, x8, #3
  00fdabc1c  sub      w8, w8, #1
  00fdabc20  and      x12, x8, x10
  00fdabc24  ldr      w8, [x0, #0x18]
  00fdabc28  add      x11, x11, x12, lsl #3
  00fdabc2c  ldr      x12, [x9, #0x50]
  00fdabc30  ldr      x11, [x11]
  00fdabc34  cmn      x11, #1
  00fdabc38  b.eq     #0xfdabcc8

name candidate RVA 0xfd881d4 bytes16=fd 7b be a9 f3 0b 00 f9 fd 03 00 91 09 08 40 f9
  00fd881d4  stp      x29, x30, [sp, #-0x20]!
  00fd881d8  str      x19, [sp, #0x10]
  00fd881dc  mov      x29, sp
  00fd881e0  ldr      x9, [x0, #0x10]
  00fd881e4  mov      w10, #0x534f
  00fd881e8  movk     w10, #0xdeb6, lsl #16
  00fd881ec  ldp      x11, x8, [x9, #0x38]
  00fd881f0  sub      x8, x8, x11
  00fd881f4  lsr      x8, x8, #3
  00fd881f8  sub      w8, w8, #1
  00fd881fc  and      x12, x8, x10
  00fd88200  ldr      w8, [x0, #0x18]
  00fd88204  add      x11, x11, x12, lsl #3
  00fd88208  ldr      x12, [x9, #0x50]
  00fd8820c  ldr      x11, [x11]
  00fd88210  cmn      x11, #1

classifier A RVA 0xfd8e120 bytes16=09 08 40 f9 ea b1 84 52 aa 2b bf 72 2b a1 43 a9
  00fd8e120  ldr      x9, [x0, #0x10]
  00fd8e124  mov      w10, #0x258f
  00fd8e128  movk     w10, #0xf95d, lsl #16
  00fd8e12c  ldp      x11, x8, [x9, #0x38]
  00fd8e130  sub      x8, x8, x11
  00fd8e134  lsr      x8, x8, #3
  00fd8e138  sub      w8, w8, #1
  00fd8e13c  and      x12, x8, x10
  00fd8e140  ldr      w8, [x0, #0x18]
  00fd8e144  add      x11, x11, x12, lsl #3
  00fd8e148  ldr      x12, [x9, #0x50]
  00fd8e14c  ldr      x11, [x11]
  00fd8e150  cmn      x11, #1
  00fd8e154  b.eq     #0xfd8e1bc
  00fd8e158  add      x11, x12, x11, lsl #5
  00fd8e15c  ldr      w13, [x11, #8]

classifier B RVA 0x101c2d7c bytes16=09 04 40 f9 ea bb 9d 52 6a 1a b1 72 2b a1 43 a9
  0101c2d7c  ldr      x9, [x0, #8]
  0101c2d80  mov      w10, #0xeddf
  0101c2d84  movk     w10, #0x88d3, lsl #16
  0101c2d88  ldp      x11, x8, [x9, #0x38]
  0101c2d8c  sub      x8, x8, x11
  0101c2d90  lsr      x8, x8, #3
  0101c2d94  sub      w8, w8, #1
  0101c2d98  and      x12, x8, x10
  0101c2d9c  ldr      w8, [x0, #0x10]
  0101c2da0  add      x11, x11, x12, lsl #3
  0101c2da4  ldr      x12, [x9, #0x50]
  0101c2da8  ldr      x11, [x11]
  0101c2dac  cmn      x11, #1
  0101c2db0  b.eq     #0x101c2e38
  0101c2db4  add      x11, x12, x11, lsl #5
  0101c2db8  ldr      w13, [x11, #8]

native helper A RVA 0x9b6bbd4 bytes16=fd 7b bd a9 f6 57 01 a9 f4 4f 02 a9 fd 03 00 91
  009b6bbd4  stp      x29, x30, [sp, #-0x30]!
  009b6bbd8  stp      x22, x21, [sp, #0x10]
  009b6bbdc  stp      x20, x19, [sp, #0x20]
  009b6bbe0  mov      x29, sp
  009b6bbe4  ldr      x8, [x0]
  009b6bbe8  mov      x19, x0
  009b6bbec  ldr      x8, [x8, #0x4e0]
  009b6bbf0  blr      x8
  009b6bbf4  tbz      w0, #0, #0x9b6bc90
  009b6bbf8  ldr      x8, [x19]
  009b6bbfc  mov      x0, x19
  009b6bc00  mov      w1, #0x44
  009b6bc04  ldr      x8, [x8, #0x9b8]
  009b6bc08  blr      x8
  009b6bc0c  ldr      x8, [x19]
  009b6bc10  mov      x0, x19

native helper B RVA 0x9b94c98 bytes16=e8 03 00 aa 00 40 41 f9 60 00 00 b4 1f 41 01 f9
  009b94c98  mov      x8, x0
  009b94c9c  ldr      x0, [x0, #0x280]
  009b94ca0  cbz      x0, #0x9b94cac
  009b94ca4  str      xzr, [x8, #0x280]
  009b94ca8  b        #0xe803424
  009b94cac  ret
  009b94cb0  sub      sp, sp, #0x60
  009b94cb4  stp      x29, x30, [sp, #0x30]
  009b94cb8  stp      x22, x21, [sp, #0x40]
  009b94cbc  stp      x20, x19, [sp, #0x50]
  009b94cc0  add      x29, sp, #0x30
  009b94cc4  mrs      x22, tpidr_el0
  009b94cc8  mov      x20, x0
  009b94ccc  mov      w19, w1
  009b94cd0  ldr      x8, [x22, #0x28]
  009b94cd4  stur     x8, [x29, #-8]

=== Sigma field-displacement samples in tick/rotation detours ===
These are encoded displacements; register provenance and target-layout identity require separate proof.
0x1f0:
  no direct memory displacement in scanned ranges (may use add/address calculation)
0xa08:
  no direct memory displacement in scanned ranges (may use add/address calculation)
0xca8:
  no direct memory displacement in scanned ranges (may use add/address calculation)
0x478:
  no direct memory displacement in scanned ranges (may use add/address calculation)
0x228:
  no direct memory displacement in scanned ranges (may use add/address calculation)
0x230:
  no direct memory displacement in scanned ranges (may use add/address calculation)
0x238:
  rotation 0xab844: ldr x10, [x1, #0x238]
  rotation 0xabb08: ldr x9, [x1, #0x238]
  rotation 0xabb18: ldr x8, [x1, #0x238]
0x1b4:
  combat 0xac8c0: ldr w8, [x9, #0x1b4]
  combat 0xacac4: ldr w8, [x8, #0x1b4]
  combat 0xacbf8: ldr w8, [x8, #0x1b4]
  combat 0xacea0: ldr w8, [x8, #0x1b4]
0xb28:
  combat 0xabf44: ldrb w9, [x14, #0xb28]
  combat 0xae0b4: ldr w9, [x9, #0xb28]
0x7c:
  combat 0xabc00: ldr s0, [x0, #0x7c]
  combat 0xac20c: ldr s0, [x8, #0x7c]
  combat 0xac7cc: ldr s0, [x0, #0x7c]
  combat 0xac87c: ldr s0, [x0, #0x7c]
```

## Runtime implementation added after the investigation

The user authorized enabling the modeled tick and rotation hooks. The implementation
checks all twelve recorded prologues first, but it necessarily uses the user-supplied
function signatures and layouts; the checks do not independently validate those.
The native-attack path calls the two specified routines, including `0x09b94c98`,
which target disassembly shows clears/deallocates a pointer at `+0x280`. This is
not proof that the two calls reproduce Sigma's native attack behavior.

The live code intentionally uses the existing simplified `playerEligible`,
`selectPlayers` and CPS models: `Switch` mode, some source filter/control-flow
conditions, rotation registry updates and timing edge cases are not reproduced.
Host simulations and Android cross-compilation do not demonstrate device
compatibility, pointer lifetimes, vector ownership or 1:1 equivalence.
