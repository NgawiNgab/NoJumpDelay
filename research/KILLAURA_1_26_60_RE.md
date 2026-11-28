# KillAura / killItems / antiBot — Reverse Engineering Notes for Minecraft Bedrock 1.26.60 (arm64-v8a)

Source mod: `libReverseSigma.so` (ApollonClient, sha256 `c48da8d725e3515ef80e686c58a6a6fa2178ec7cce6b21d1356f53468d0c5b6f`,
AArch64 PIE, 3,473,688 B, embeds shadowhook 2.0.1). Target game lib:
`libminecraftpe.so` 1.26.60 (342,231,392 B, PT_LOAD main segment file offset == vaddr).

## 1. Method

1. **Ghidra headless** decompilation of `libReverseSigma.so` (project `KillAuraSigma`):
   config parser, menu, engine, helpers, shadowhook internals — `re_workspace/decomp/*.c`.
2. **Byte-signature verification FAILED**: all 152 BedrockTools IDA patterns scanned against the real
   1.26.60 `libminecraftpe.so` → **151/152 MISS** (only `ContainerScreenControllerOnContainerSlotSelected`
   hits at RVA 0x9dd3d88). Sig-based resolution is dead for this build. BedrockTools *struct offsets*
   remain valid candidates (see §5).
3. **Unicorn ARM64 emulation of the obfuscated decoder** (`re_workspace/scripts/emu_offsets.py`):
   - maps sigma at `SIGMA_BASE=0x10000000` (segments at p_vaddr), applies all `.rela.dyn` ABS64/GLOB_DAT
     relocs (1841 internal, 10 imported→fake objects), stubs all 482 PLT imports, fake JavaVM/JNIEnv,
     stubs `shadowhook init` (sigma+0x15EAD8) and intercepts the hook wrapper (sigma+0x15ECA8).
   - runs every `.init_array` entry, then `JNI_OnLoad` (vaddr 0xB5568, dynsym).
   - JNI_OnLoad contains ~88 self-modifying "VM" blocks (obfuscated control flow over a 26-byte buffer,
     byte-wise self-inverse transforms keyed by 32-bit constants `0xAC894E70, 0x1A06BA62, 0x1E109950,
     0x43C2132A, …`). Each block ends with `target = base + *(u32*)buffer`.
   - **Counter model**: the two shadowhook counters (sigma+0x35FEE0/0x35FEE8) are *loop state* — the VM
     seeds its index register from them, so they must remain 0 during successful hook installation
     (shadowhook only increments them via its internal path; emulating them as error counters = model 5).
   - `/proc/self/maps` emulation: canned line `7f0000000000-… libminecraftpe.so` so the runtime base
     lookup (`FUN_001931b8`: fopen/fgets/strstr/strtoul) returns `MCPE_BASE=0x7F0000000000`.
   - Internal debug-hexdump logger (sigma+0x93380) skipped; ofstream ctor (sigma+0x935bc) faked with a
     benign vtable (object → fake vtable with vbase-offset 0 at `-0x18`).
   - **59 hook installations** captured (`re_workspace/emu_hooks.json`), **28 game-function pointers**
     decoded into the fn table sigma+0x457B70..0x457C48 (`re_workspace/emu_fns.json`).
4. **Validation**: every decoded target RVA was verified in the real 1.26.60 `libminecraftpe.so`:
   58/58 hook targets are 4-byte-aligned function starts with valid prologues (48 strict
   `stp x29,x30/paciasp/sub sp`, 10 leaf accessors/thunks — verified by full-contiguity disassembly);
   25/28 fn-table entries are valid function starts; the remaining 3 (`0x457C38/40/48`) are
   **data constants** (component-type IDs passed as `getComponent` args), not code.
   Semantic cross-check: `fn[0x457B78]` = `ldr x0, [x0, #0x478]` — exactly BedrockTools'
   **`Level::mActorManager = 0x478`** for 1.26.x.

## 2. Hook table (1:1 port targets)

`target` = RVA in 1.26.60 `libminecraftpe.so` to be hooked; `detour` = RVA of the replacement logic in
sigma; `orig` = trampoline slot in sigma's data (the original function pointer, filled by shadowhook).

| # | target RVA (1.26.60) | detour RVA (sigma) | orig slot (sigma) |
|---|---|---|---|
| 1 | 0x000012340 (dlsym nativeKeyHandler — runtime addr) | 0x0B4F30 | 0x35B0D8 |
| 2 | 0x009C0E520 | 0x0B37F0 | 0x35AFC0 |
| 3 | 0x00CC8934C | 0x0B4A98 | 0x35B0D0 |
| 4 | 0x009D0C2E0 | 0x0B4A68 | 0x35B0C8 |
| 5 | 0x011F094B0 | 0x0B4A24 | 0x35B0C0 |
| 6 | 0x00FA71380 | 0x0B4844 | 0x35B0B0 |
| 7 | 0x00FEF8DF4 | 0x0B4908 | 0x35B0B8 |
| 8 | 0x01009CF3C | 0x0B414C | 0x35B0A0 |
| 9 | 0x01009D860 | 0x0B44D0 | 0x35B0A8 |
| 10 | 0x01009E250 | 0x0B40D0 | 0x35B098 |
| 11 | 0x00B092590 | 0x0B40A4 | 0x35B090 |
| 12 | 0x00A589D98 | 0x0B3FF0 | 0x35B088 |
| 13 | 0x00AF5D244 | 0x0B3F7C | 0x35B078 |
| 14 | 0x00B0BCAB8 | 0x0B3FB8 | 0x35B080 |
| 15 | 0x00B7CF720 | 0x0B3F38 | 0x35B070 |
| 16 | 0x011EA52BC | 0x0B3CC8 | 0x35B050 |
| 17 | 0x009B8DB20 | 0x0B396C | 0x35AFD0 |
| 18 | 0x008A74EB4 | 0x0B3C98 | 0x35B048 |
| 19 | 0x00B092088 | 0x0B399C | 0x35AFD8 |
| 20 | 0x00B092330 | 0x0B39DC | 0x35AFE0 |
| 21 | 0x011E5BA08 | 0x0B3A14 | 0x35AFE8 |
| 22 | 0x011E5C9EC | 0x0B3A44 | 0x35AFF0 |
| 23 | 0x00C257EC4 | 0x0B3A74 | 0x35AFF8 |
| 24 | 0x00C124C68 | 0x0B3AB0 | 0x35B000 |
| 25 | 0x00C10BFF8 | 0x0B3AEC | 0x35B008 |
| 26 | 0x011EB5D50 | 0x0B3B28 | 0x35B010 |
| 27 | 0x00CD49814 | 0x0B3B58 | 0x35B018 |
| 28 | 0x0099B27C4 | 0x0B23BC | 0x35AF20 |
| 29 | 0x011C38BA0 | 0x0B23F8 | 0x35AF28 |
| 30 | 0x00FD9B858 | 0x0B3860 | 0x35AFC8 |
| 31 | 0x00AF5E460 | 0x0ABB30 | 0x35AEB0 |
| 32 | 0x00C1B345C | 0x0B2290 | 0x35AF18 |
| 33 | 0x010383480 | 0x0AB7D4 | 0x35AEA8 |
| 34 | 0x00B80021C | 0x0B3B88 | 0x35B020 |
| 35 | 0x00B7FF4AC | 0x0B3BC0 | 0x35B028 |
| 36 | 0x009A213F0 | 0x0B2258 | 0x35AF10 |
| 37 | 0x009B82744 | 0x0B16AC | 0x35AF00 |
| 38 | 0x009A23484 | 0x0B3C60 | 0x35B040 |
| 39 | 0x00ACAD41C | 0x0B241C | 0x35AF30 |
| 40 | 0x010F7FDE0 | 0x0B25A0 | 0x35AF38 |
| 41 | 0x010F2CC0C | 0x0B283C | 0x35AF40 |
| 42 | 0x010F43E7C | 0x0B2918 | 0x35AF48 |
| 43 | 0x010E6A098 | 0x0B29F4 | 0x35AF50 |
| 44 | 0x010F399DC | 0x0B2A54 | 0x35AF58 |
| 45 | 0x010F4E844 | 0x0B2AB4 | 0x35AF60 |
| 46 | 0x00AF60330 | 0x0B166C | 0x35AEF8 |
| 47 | 0x00B767FB0 | 0x0B3BF8 | 0x35B030 |
| 48 | 0x009F180DC | 0x0B3C28 | 0x35B038 |
| 49 | 0x010F56C1C | 0x0B2B14 | 0x35AF68 |
| 50 | 0x010F598B0 | 0x0B2B74 | 0x35AF70 |
| 51 | 0x010E68FB4 | 0x0B2C50 | 0x35AF78 |
| 52 | 0x010F54410 | 0x0B2D2C | 0x35AF80 |
| 53 | 0x010F73930 | 0x0B2D8C | 0x35AF88 |
| 54 | 0x010F3B7F0 | 0x0B2DEC | 0x35AF90 |
| 55 | 0x010F6C4C4 | 0x0B2E4C | 0x35AF98 |
| 56 | 0x009C54098 | 0x0B3568 | 0x35AFB0 |
| 57 | 0x009C40254 | 0x0B3604 | 0x35AFB8 |
| 58 | 0x010926D30 | 0x0B2EAC | 0x35AFA0 |
| 59 | 0x0100A6874 | 0x0B352C | 0x35AFA8 |
Key rows for the KillAura port:

| detour | role | target | orig slot |
|---|---|---|---|
| 0x0ABB30 | **KillAura engine (NormalTick detour)** — killItems/antiBot/rotations/attack logic | 0x0AF5E460 | 0x035AEB0 |
| 0x0AB7D4 | **Rotations injector** — writes yaw/pitch into `actor+0x238`, tails to orig | 0x10383480 | 0x035AEA8 |
| 0x0B16AC | engine-adjacent hook | 0x09B82744 | 0x035AF00 |
| 0x0B4F30 | nativeKeyHandler detour (shortcut handling) | *(dlsym at runtime)* | 0x035B0D8 |

Port note: we do **not** port `nativeKeyHandler` (JNI) — the LeviLauncher floating SVG button
(`pl::modmenu::ButtonBuilder`) replaces it. The remaining 55 detours implement sibling features
(mobAura, autoClicker, ladderFly, noJumpDelay, antiFireball, hitBox, deathPosition, targetLock, …)
that are out of scope for this port.

## 3. Decoded game-function table (sigma fn table → 1.26.60 RVAs)

sigma slot | 1.26.60 RVA | semantics (from engine decompile + prologue check)
---|---
| 0x457b70 | 0x010B11D50 | Level::fetchNearbyActors?(lvl,+0x410 read) |
| 0x457b78 | 0x010B16414 | Level::getActorManager (ldr [x0,#0x478]) |
| 0x457b80 | 0x010B13684 | Actor::getEntityTypeId wrapper |
| 0x457b88 | 0x01009CBA4 | GameMode::attack(this,actor,bool,Vec3 const*) |
| 0x457b90 | 0x01009E2A0 | GameMode-related fn (attack2/baseUse) |
| 0x457b98 | 0x01009FDCC | GameMode-related fn |
| 0x457ba0 | 0x00FDABBFC | Actor::getComponent(actor, compId) (alive/health check) |
| 0x457ba8 | 0x00FD8E120 | interaction? (ldr [x0,#0x10]) |
| 0x457bb0 | 0x00FD881D4 | Actor::getNameTag component fetch (std::string@+0x48) |
| 0x457bb8 | 0x00A58A710 | ActorRegistry/lookup helper |
| 0x457bc0 | 0x010770724 | block/registry helper |
| 0x457bc8 | 0x010084910 | flag-test fn (tbnz w1,#0x1f) |
| 0x457bd0 | 0x0101E5C18 | block set/notify fn |
| 0x457bd8 | 0x010371248 | generic fetch: actorFromId / getBlock (vec2 key) |
| 0x457be0 | 0x010372410 | block-source helper |
| 0x457be8 | 0x011F09624 | Actor::setPos/move wrapper (ladderFly 0x12,1,0) |
| 0x457bf0 | 0x0101C2D7C | Actor classifier (-1/1/5/6 by type) |
| 0x457bf8 | 0x00FA73088 | member-accessor thunk (add x0,x0,#0x140) |
| 0x457c00 | 0x00AF60D78 | LocalPlayer::swing(lp,4,0) |
| 0x457c08 | 0x009B8F504 | helper (sub sp 0x40) |
| 0x457c10 | 0x009B6BBD4 | emulateNativeAttack A (itemInUse) |
| 0x457c18 | 0x009B956E0 | accessor (ldr [x0,#0x1a0]) |
| 0x457c20 | 0x009B942F8 | accessor |
| 0x457c28 | 0x009B8D5BC | accessor |
| 0x457c30 | 0x009B94C98 | emulateNativeAttack B |
| 0x457c38 | 0x0141D8F48 | comp-id A (health/alive check arg) |
| 0x457c40 | 0x0141D8D68 | comp-id B |
| 0x457c48 | 0x0141D9E30 | comp-id C |
Component-ID constants (decoded values, passed as `getComponent` args):
`0x457C38 → 0x141D8F48`, `0x457C40 → 0x141D8D68`, `0x457C48 → 0x141D9E30` (32-bit data constants,
not code — alive/health check, AABB, etc.).

## 4. Engine semantics (FUN_001ABB30 — 1:1 port spec)

Entrypoint: detour of the LocalPlayer tick function (target 0x0AF5E460). Signature `void(LocalPlayer* lp)`.
Calls `orig(lp)` (trampoline) at the very end (sigma 0x1569 in decomp order) — i.e. runs *before* the
original tick body.

### Guards
- `panic (0x457F68)` or `0045D4A20` flag → bail (no logic).
- Config getters: bool 001DCFB4/001DD67C, int 001DE77C, float 001E0EAC, list 001E13F4.

### Field offsets (LocalPlayer lp / Actor a) — as used by sigma, cross-checked vs BedrockTools
| field | offset |
|---|---|
| `lp->stateVector` (Vec3 pos) | `lp+0x228` |
| `lp->level` | `lp+0x1F0` |
| `lp->gameMode` | `lp+0xA08` |
| `lp->blockSource` | `lp+0xD0` |
| `lp->interaction` comp | `lp+0xCA8` |
| `actor->position` (Vec3) | `a+0x228` |
| `actor->hurtTime` (int) | `a+0x1B4` |
| `actor->rotation` (Vec2 yaw,pitch) | `a+0x238` |
| `actor->categories` | `a+0x220` |
| AABB component | `getComponent(a, compId[0x457C40])` → AABB at comp+0x30 (half-extents at +0x18) |
| name tag | `getNameComponent(a)` → std::string at +0x48 |

### Entity scan (1:1 behavior)
1. Actor list via `fn[0x457B78](level)` (→ Level+0x478 actor manager fetch).
2. Per actor:
   - **antiBot**: skip if name length < 2, or velocity < 0.05; skip if name matches `botList` (memcmp against loaded JSON array, mutex sigma+0x4584F0).
   - **friendList** skip (mutex sigma+0x4584C8).
   - **hurtCheck**: attack only if `*(int*)(a+0x1B4) < 2`.
   - alive: `getComponent(a, compId_0x457C38)` → int at comp+0x7C; 0 → excluded.
   - Name via `fn[0x457BB0](a)` → std::string@+0x48.
   - FNV-1a(name, basis 0x811C9DC5): `0xE1BED951` = fireball → dist ≤ 10 + antiFireball → attack; `0xF228491F` = item entity → dist ≤ 6 + **killItems** → attack.
3. Candidates appended via sigma 001AF948, committed sigma 0021058C (mutex 00458450).
4. Radii: collect radius = `rotations||targetLock ? rotationsRange(002D47A8) : killRange(002D47A4)`;
   **attack radius is always killRange**.
5. Selection: `auraType` 0 = Single (nearest in collect radius; attack iff dist ≤ attack radius);
   1 = Multi (attack every candidate ≤ attack radius). mobAura uses radius 002D47AC.
6. Rotations (if `rotations || targetLock`): `FUN_001AEF80(eye, target, AABB+0x18)` → writes
   yaw/pitch to `DAT_00458630/34`, sets `DAT_004586E0 = 1`. Math: clamp eye into AABB±half,
   `hdist = sqrt(dx²+dz²)`; pitch = `atan2f(dy, hdist) * -57.295776` (±90° if hdist ≤ 0.001).
7. Attack: write 1 at ret+0x18; hit-pos via `FUN_001AFA60` (MT19937, per-axis uniform jitter
   `pos + rand01*2*range - range`) → ret+0x2C; range → ret+0x30; then
   `fn[0x457B88](gameMode, actor, 1, &pos)` = **GameMode::attack**.
8. Swing: if `swing(002D47CC)` → `fn[0x457C00](lp, 4, 0)` = **LocalPlayer::swing**.
9. **emulateNativeAttack**: set `DAT_004586C4 = 1` instead of calling attack (autoClicker uses 002D4B24);
   tail: if flag → `fn[0x457C10](lp[0xCA8]); fn[0x457C30](lp[0xCA8]);` clear.

### CPS scheduler (FUN_001AE86C, steady_clock ns)
- A killAura (value 0045D348, tick 002D4B1C, min/max 002D47B0/B4), B mobAura, C autoClicker
  (gate 00457EE8). Elapsed ms ≥ 1000/CPS → tick=1, re-roll `rand() % ((max-min)+1) + min`, clamp [1,20];
  else tick=0. `emulateNativeAttack` forces `rotations = 1`.

### Other engine tails (context)
`noJumpDelay` → fn[0x457BE8](lp, 0x12, 1, 0); `ladderFly` → block-climb glitch via
getBlock fn[0x457BD8] + "tile.air" FNV cmp (001AFDAC); cache 0045AEC0–AED4.

## 5. Validation against BedrockTools (independent source)

| offset | BedrockTools | sigma decompile | status |
|---|---|---|---|
| Actor hurtTime | 0x1B4 | `a+0x1B4` | ✅ match |
| stateVector/position | 0x228 | `lp/a+0x228` | ✅ match |
| actorRotation | 0x238 | `a+0x238` (rotations injector) | ✅ match |
| Level::mActorManager | 0x478 | `ldr x0,[x0,#0x478]` in decoded fn[0x457B78] | ✅ match |
| GameMode::attack(void*,void*,bool,Vec3 const*) | signature | fn[0x457B88] used as `(gameMode, actor, 1, &pos)` | ✅ match |

## 6. Artifact paths

- `re_workspace/scripts/emu_offsets.py` — Unicorn emulator (final working version; `python3 emu_offsets.py targets/libReverseSigma.so 5`).
- `re_workspace/emu_hooks.json` — 59 hook triples `{target_off, detour_off, orig_off}`.
- `re_workspace/emu_fns.json` — 28 fn-table entries (sigma slot → 1.26.60 RVA).
- `re_workspace/decomp/` — Ghidra decompilations (engine_001abb30.c = main engine, engine_00ab7d4.c = rotations injector, sigtable_001b5568.c = JNI_OnLoad 12,954 lines).
- `re_workspace/sig_verify_results.json` — proof that BedrockTools sigs don't match 1.26.60.
