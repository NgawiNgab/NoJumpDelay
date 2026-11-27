# No Jump Delay port: research and progress

Status: **PARTIALLY COMPLETE — RE in progress**. Hook registration, the component identity, and the immediate UI toggle were independently corroborated; full 1:1 behavioral and ABI verification remains incomplete. No native implementation, package, CI result, or PR has been produced. Do not infer a safe hook, complete guard behavior, or final ABI from the partial findings below. Phases 3–4 have not been started.

## Repository and access

- The session checkout is `NgawiNgab/NoJumpDelay`, branch `arena/b6a002cd-nojumpdelay`, at base commit `3cda3e340343ee5366ed36032a9970a4fa3b8db4` (`main`). It was clean before these research files were added. The session branch cannot be replaced or renamed.
- The user has corrected the PR target to **`NgawiNgab/NoJumpDelay`**. `gh api repos/NgawiNgab/NoJumpDelay` reports push access. The previous repository mismatch is no longer a blocker; do not query the former target.

## Inputs and setup

Downloaded via the Google Drive connector (not committed):

| Input | Local path | Evidence |
| --- | --- | --- |
| Apollon binary | `/home/user/research_resources/libReverseSigma.so` | 3,473,688 bytes; SHA-256 `c48da8d725e3515ef80e686c58a6a6fa2178ec7cce6b21d1356f53468d0c5b6f` (matches guide) |
| Guide | `/home/user/research_resources/ApollonClient-1.26.60-deobfuscation-method.md` | 11,693 bytes |
| Minecraft | `/home/user/binaries/libminecraftpe.so` | 342,231,392 bytes; SHA-256 `4e7f97ecf1989edfaa7ca4805acc907a63bd4c56f1b1965079b2c47f63e3a7c4`; `readelf -h`: ELF64 little-endian AArch64 `ET_DYN` |
| Ghidra + JDK | `/home/user/tools/ghidra_11.2.1_PUBLIC` and `/home/user/tools/jdk-21.0.5+11` | Extracted from connector-supplied archive; headless import and Apollon analysis succeeded |

The Minecraft file was reassembled in numeric order from `libminecraftpe.zip.1` through `.7` and extracted with `unzip`. The Ghidra tar parts were concatenated in lexical order and successfully extracted. The connector reported `file_transfer_failed` for Ghidra part 06 even though a 53,968,906-byte file appeared on disk; the whole tar extraction nevertheless completed successfully. Its part-06 SHA-256 was `d6c619784ce52338c4da989a777246194f9c324843521e84e6a1c93ecf3c13e4`; no independent expected hash was supplied. Other parts returned successful transfers.

`unzip -t` reported no errors in the reassembled Minecraft archive. `analyzeHeadless` initially failed with `Java runtime not found`; adding the bundled JDK's `bin` to `PATH` fixed it. Its launcher was configured to honor `MAXMEM=1400M`, `-XX:+UseSerialGC`, and to place Java/Ghidra temporary and cache directories under `/home/user/ghidra_projects`. The first import was stopped after observing its empty cache directory defaulted to `/var/tmp`; the second used the corrected location. The successful Apollon import reported unsupported `R_AARCH64_TLSDESC` relocations; its analysis log also contains an invalid PNG and VarnodeContext out-of-address-spaces errors. Do not treat all decompilation as verified runtime behavior. No Ghidra auto-analysis was run on the Minecraft binary.

The guide describes a `scripts/deob_shadowhook.py` emulator and a hook table, but that script is absent from this initial checkout. A separate synthetic emulator, `research/emulate_apollon_hooks.py`, was subsequently written following the guide. It independently reproduced the 59 hook registrations under emulated assumptions; see the continuation findings below. Ghidra string/global XREFs and AArch64 Capstone scanning/disassembly provide independent supporting evidence. This emulation is not an Android execution test.

## Continuation: independently reproduced findings

The user corrected the GitHub target to the current repository and asked to reuse existing files. No resource was redownloaded. All offsets below are ELF RVAs, not Ghidra image-base addresses.

### Hook registration and additional pointers

- Read the entire supplied guide. Its table states hook #30 is `game+0x0af5e460 -> so+0x0abb30`. It **does not supply a complete verified AArch64 signature** for that hook.
- `research/emulate_apollon_hooks.py` maps Apollon at `0x100000`, applies RELA/RELR relocations, uses GOT-indexed unique PLT trampolines, emulated TLS/JNI and a fabricated game base `0x70000000`, and intercepts Apollon's hook-install routine `so+0x15eca8`. The resulting hook #30 is `(0x7af5e460, 0x1abb30, 0x45aeb0)`, i.e. game RVA `0x0af5e460`, callback RVA `0x0abb30`, original-pointer slot RVA `0x35aeb0`. All 59 registration calls matched the guide's table. These are **synthetic emulator observations**, not a real-device hook test. The loader's static call site at `so+0xbc970` passes the callback and original slot to this routine.
- The first emulator attempt failed because a synthetic game-address read was unmapped. A second attempt faked game-byte reads but failed on an unstubbed `__errno`. Both were corrected; a later run returned `0x10006` from JNI. Replacing the original crude zero-return `strtoll_l` stub with one that parses text did not change the hook table or the pointers relevant here. Other imports (notably locale, thread synchronization and game bytes) remain simulated, so the results do not establish device behavior or safety.
- In the completed synthetic run, `so+0x357ba0` held `game+0x0fdabbfc`, and `so+0x357c38` held `game+0x141d8f48`. The former game function accepts its first argument in `x0`, accesses the actor's context at `+0x10`/`+0x18`, and passes the `x1` argument into a component/attribute lookup at `game+0x10177180`. The second address lies in game `.bss`. Game initialization at `0x1018e97c`–`0x1018e9a0` builds it using the string `minecraft:health` at `0x24cc02d`; game `0x101a9ad0` reads `s0` from attribute instance `+0x7c`. Thus this is the **health attribute**, but safe runtime initialization and missing-component behavior still need verification: the `0xfdabbfc` lookup takes an error path at `0xfdabcc8` when its required component is absent. Do not call it blindly.

### UI, default and guards

- `so+0xf89a0` initializes an encrypted label from bytes at `so+0x38b10` and `so+0x393a8` plus `0xa5ae`; `so+0xf8a2c` decrypts it with `so+0x38230`, alternating bytes from `so+0x38bf0`, and `d7a5`. The result is exactly `No Jump Delay` followed by a NUL byte. At `so+0xa456c`–`0xa457c`, this label is passed to a UI control with `x1 = so+0x357ee4`. The control at `so+0x145abc` toggles that byte with XOR 1 when activated (`so+0x145c00`–`0x145c0c`). This proves a boolean UI control, not the absence of all indirect configuration/trigger behavior. The byte's BSS initial value is false and the only direct configuration/UI state writes identified are load and toggle; the effective runtime default before all constructors/configuration have been traced remains **UNRESOLVED**.
- The callback does not simply reset the component unconditionally. It first gates on `so+0x357f68`, `so+0x1d4a20`, non-null `player+0x228`, the result of the `game+0xfdabbfc` call with the pointer at `game+0x141d8f48`, the float at result `+0x7c`, and non-null values loaded at `player+0xa08` and `player+0x1e0`. `so+0xabc04` uses `fcvtps w9, s0` (round **toward positive infinity**), then requires `w9 != 0`; the earlier Ghidra C cast is not a precise rounding description. Another condition before the reset is `(so+0x3580d0 & 1) == 0`. The callback calls the original after this operation.
- The `0x3580d0` guard is updated by callback `so+0xb16ac`, associated by the guide and synthetic emulation with `game+0x09b82744`. This callback calls the game function at `0x09b8f504` to obtain a screen-name string (the game function uses the AArch64 indirect-result register `x8`), FNV-1a hashes it, and sets the guard for five hashes: `0x9a28e264`, `0xe9722343`, `0x18590063`, `0x4a4bc599`, `0x7abeda05`. Another global gate `so+0x1d4a20` is set for a larger list of screen hashes, and also has a separate early path controlled by `so+0x3580c0`. Exact screen names, lifecycle, and the original screen callback's complete ABI are **UNRESOLVED**. The guide's shorthand description of #36 as a sound hook is not sufficient to describe these observed screen-related guards. Omitting these guards would not be a proven 1:1 port.

### Minecraft component and target evidence

- The game FDE table lists a function start at `0x0af5e460` and the next at `0x0af5f580`. In `.rela.dyn`, relocation at file offset `0x11c4168` is `R_AARCH64_RELATIVE`: slot `game+0x139f6f68` points to `game+0x0af5e460`. The containing primary vtable starts at `game+0x139f6ea0`; its RTTI at `game+0x139f7ec0` references the name `11LocalPlayer` at `game+0x2e3bbdb`. Thus it is virtual slot **25** / byte displacement `0xc8` from this vtable address point. The game function prologue copies `x0` to `x19`, and Apollon's callback supplies `x0` to the original. As additional non-authoritative evidence, BedrockTools commit `56d974c3c178c66d6f2494693cb1d121f2aef032` labels a wildcard signature `NormalTick` and uses `void (*)(void*)`; that pattern matches exactly once in this game ELF, at `0x0af5e460`. This supports an inference of `LocalPlayer::normalTick`, not definitive proof of source signature or runtime safety. Direct BL scan found no direct `BL` to this entry; it is referenced by relocation. Caller-side verification is still needed.
- The first 16 game bytes are `e8 0f 19 fc fd 7b 01 a9 fc 6f 02 a9 fa 67 03 a9`: four stack stores with no PC-relative instructions. This is evidence only that **these instructions** do not need PC-relative relocation; it does not prove an arbitrary hook framework's trampoline, thread synchronization, or patch installation is safe.
- Game `0x0c0e89c4` constructs type information containing hash `0xc52c8f78` and the 16-byte string at `0x25a32ba`, `MobJumpComponent`. Thus the helper's hash identifies **MobJumpComponent**, not necessarily the separately named `JumpTicksComponent` also present in game strings. Game `0x10191fd8` reads a 32-bit field at component `+0x10`; game `0x10192184` writes `w1` to the same field. Both use stride `0x14` after the hashed sparse/dense lookup. At game `0x0f4290cc`, the getter's return is checked for zero; if zero, the routine calls `0x0fa86250`, then calls the setter with `w1=10`. This strongly corroborates the field as a jump cooldown counter, although a source-level field name is not available. Apollon helper `so+0xb1598` independently uses the same hash, sparse lookup, stride and `str w1,[x8,#0x10]`, with `w1=0` at the observed call site.
- No direct instruction patch was identified for this feature. Do not confuse the `MobJumpComponent+0x10` field with a direct offset inside `LocalPlayer`.

### Reproduction and tests

`/home/user/tools/re-venv/bin/python research/emulate_apollon_hooks.py` reproduces the synthetic hook table. `python3 research/verify_findings.py` checks input SHA-256, the recovered label, relevant Apollon/game instruction bytes and the target's vtable relocation against the supplied binaries. These are **offline evidence regression checks**, not ABI, integration, or functional gameplay tests. Output and intermediate Ghidra scripts are recorded under `/home/user/ghidra_projects`; only small scripts and this report are tracked in the checkout.

## Phase 1: observations, not a finished port

All `so+` addresses below are ELF RVAs. Ghidra loaded the Apollon ELF at image base `0x100000`, so add that base to get the addresses printed by the Ghidra scripts.

- `noJumpDelay` is in `.rodata` at file offset and RVA `0x3f931`; Capstone found string-address construction at `so+0x979c0`/`0x979c4` and `so+0x9e1e0`/`0x9e1e4`. Ghidra identifies the first function as configuration loading and the second as serialization; both refer to byte state at `so+0x357ee4` (Ghidra `0x457ee4`).
- The state lies in `.bss`, so its ELF initial storage is zero; **UNRESOLVED**: whether constructors/other dynamic paths change the effective default before configuration is loaded. Configuration read uses the existing byte as an argument and writes back the result. The effective default and linked options are not established.
- A UI-area reference passes the byte address in `x1` at `so+0xa4574`–`0xa457c`. The enormous containing function timed out during a 90-second Ghidra decompilation. Fallbacks: inspect its local Ghidra disassembly and the small label/control functions, independently corroborate those instructions with Capstone. The continuation section above records the recovered label and toggle operation; all possible related options remain **UNRESOLVED**.
- Ghidra found a read of the same byte in callback `so+0xabb30`, at `so+0xae2d8`. Relevant independently decoded bytes/instructions (addresses are RVAs):

```text
0ae2c8  481500d0  adrp x8, #0x358000
0ae2cc  08414339  ldrb w8, [x8, #0xd0]
0ae2d0  68020037  tbnz w8, #0, #0xae31c
0ae2d4  481500b0  adrp x8, #0x357000
0ae2d8  08917b39  ldrb w8, [x8, #0xee4]
0ae2dc  1f050071  cmp w8, #1
0ae2e0  81000054  b.ne #0xae2f0
0ae2e4  e0031caa  mov x0, x28
0ae2e8  e1031f2a  mov w1, wzr
0ae2ec  ab0c0094  bl #0xb1598
```

The callback's decompiler identifies `x28` at this site as `param_1 + 1` in its inferred `long **` type. The callback first checks `so+0x357f68` and `so+0x1d4a20`, performs several pointer checks, and requires a nonzero conversion of a float at `+0x7c` of a returned object before reaching this code. See the continuation section above for the precise conversion and independent hook-table evidence. **UNRESOLVED**: exact identities and meaning of all guards, and the full caller-verified ABI.
- Helper `so+0xb1598` follows a hash/bucket/component-like lookup. Its AArch64 code tests identifier `0xc52c8f78`; under several lookup validity checks it performs:

```text
0b1650  286968f8  ldr x8, [x9, x8]
0b1654  08fdffb4  cbz x8, #0xb15f4
0b1658  49194092  and x9, x10, #0x7f
0b165c  8a028052  mov w10, #0x14
0b1660  2821aa9b  umaddl x8, w9, w10, x8
0b1664  011100b9  str w1, [x8, #0x10]
0b1668  c0035fd6  ret
```

At the observed call, `w1` is zero. This shows a conditional four-byte zero write to offset `0x10` within a selected `0x14`-byte element. Subsequent game-side evidence identifies the element as `MobJumpComponent`; see the continuation section above. This is not a direct `Mob`/`Player` struct field offset. Do not replace it with a guessed direct write or instruction patch.

## Remaining verification

1. Complete the guard trace: verify safe runtime initialization and handling of the health attribute, all screen/global transitions, exact runtime default, and linked behavior. Trace all indirect state references, not merely the five static references Ghidra found. Identify relevant screen names or prove that handling the observed hashes is sufficient.
2. Complete caller-side signature/return-type analysis of the `LocalPlayer` virtual at `0x0af5e460` and the screen function at `0x09b82744`. Verify prologue relocation and hook framework behavior, and review safe module version checks. A type/hash/slot match is not complete ABI verification. No full auto-analysis of the game ELF.
3. Implement and test only after the above evidence exists. Avoid publishing a speculative mod or claiming a green build proves behavioral equivalence.
4. The corrected target is `NgawiNgab/NoJumpDelay`. No branch push, PR, CI run, `.levipack`, `.so` artifact, or `verification.json` exists from this attempt.

## Reproduction

The following local tools and analysis artifacts are under `/home/user`; files outside the checkout may not survive a later sandbox session. Preserve the supplied downloads as needed; do not commit game/mod binaries or Ghidra projects. The small Ghidra scripts are in this directory.

```bash
export JAVA_HOME=/home/user/tools/jdk-21.0.5+11
export PATH="$JAVA_HOME/bin:$PATH" MAXMEM=1400M TMPDIR=/home/user/ghidra_projects
# Launcher configured with SerialGC and user-owned cache/temp locations.
/home/user/tools/ghidra_11.2.1_PUBLIC/support/analyzeHeadless \
  /home/user/ghidra_projects Apollon2 \
  -import /home/user/research_resources/libReverseSigma.so \
  -scriptPath /home/user/NoJumpDelay/research -postScript FindNoJumpDelay.py \
  -max-cpu 2 -analysisTimeoutPerFile 600
# On the existing project, use -process libReverseSigma.so -noanalysis and
# post-scripts TraceNoJumpDelay.py and InspectNoJumpHelper.py respectively.
```

The captured output from the successful runs is `/home/user/ghidra_projects/find.out`, `trace.out`, `helper.out`, and `apollon2.log`. Ghidra's analysis said it succeeded; this is not evidence of complete semantic recovery. `TraceNoJumpDelay.py` intentionally does not repeat decompilation of the two large configuration functions.

### Phase 3 implementation notes (experimental)

- On user direction the hold was lifted; no Ghidra rerun and no resource redownload was done. A direct Capstone check of `0x10192184` confirms the `x0` context, hash, stride, write and error path described above.
- The health getter `0x0fdabbfc` instructions at `0x0fdabc08`–`0x0fdabc0c` construct hash `0xfd3b0613`; `0x0fdabcb4`–`0x0fdabcbc` use stride `0x58`. The first 16 bytes are `fd7bbfa9fd030091090840f96ac28052`. The implementation checks for that component before invoking the getter; the component can still change concurrently, so this is not a complete memory-safety proof.
- Host CMake initially absent; installed CMake into the existing Python environment and built/ran synthetic tests successfully. These tests are not game runtime or ABI tests. Screen hooks and their guards are deliberately omitted because their ABI and names are unresolved, and the >0 finite float health condition differs from `fcvtps`/nonzero. Consequently 1:1 behavior is not claimed. The worker polls for the library instead of hooking loader functions. A live hook prevents safe unload because the hook API exposes no thread-quiescence guarantee; disabling stops modifications but keeps the hook until process restart. Map checks are best-effort and not a defense against concurrent unmapping or structural races.
