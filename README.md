# ApollonKillAura

Native LeviLauncher mod for **Minecraft Bedrock 1.26.60 (arm64-v8a)**: a 1:1
port of the KillAura engine (sub-modules `killItems`, `antiBot`) recovered from
`libReverseSigma.so`.

## How it was built

Every offset and function address in `src/kaura/re/Offsets.h` was extracted by
emulating `libReverseSigma.so`'s obfuscated `JNI_OnLoad` decoder (self-modifying
VM blocks) under Unicorn ARM64 and validating each decoded value against the
real `libminecraftpe.so 1.26.60`. The full method, the 59-hook table, the
28-entry function table and the engine spec are in
[`research/KILLAURA_1_26_60_RE.md`](research/KILLAURA_1_26_60_RE.md).

## Safety / fail-safes

- Before any hook is installed, the resolver verifies the expected 16-byte
  in-memory prologue of each target (captured from the real 1.26.60 library).
  On mismatch (game update, wrong build) nothing is hooked and the mod logs an
  error.
- No Dear ImGui, no JNI menu layer, and `JNI_OnLoad` is **not** exported — all
  user surface is the LeviLauncher `pl::modmenu` API plus a floating SVG
  shortcut button (`ButtonBuilder`).

## Features (1:1 with the original)

| Control | Key | Notes |
| --- | --- | --- |
| Kill Aura | `killAura` | master toggle |
| Aura Type | `auraType` | Single(0) / Multi(1); Switch(2) is a no-op exactly as in the original |
| Kill Range | `killRange` | slider 1.0–8.0 |
| CPS | `minKillAuraCPS` / `maxKillAuraCPS` | randomized delay, re-roll clamped to [1, 20] |
| Rotations | `rotations` / `rotationsRange` | yaw/pitch written to actor+0x238 |
| Swing | `swing` | `swing(lp, 4, 0)` after each attack |
| Hurt Check | `hurtCheck` | skips actors with hurtTime ≥ 2 |
| Emulate Native Attack | `emulateNativeAttack` | pulse via the recovered fn pair instead of `GameMode::attack` |
| Kill Near Items | `killItems` | item entities ≤ 6 blocks, raw position, no desc writes |
| Anti Bot | `antiBot` | skips short tags, degenerate AABBs, `botList` matches |
| Friend/Bot lists | `friendList` / `botList` | comma-separated in the menu, JSON arrays on disk |

Config persists to `<mod config dir>/config.json` with the same key names the
original's parser used.

## Building

```bash
# host unit tests (ASan/UBSan) + full-source cross-check compile
./scripts/build_and_test.sh

# native arm64-v8a .so + ApollonKillAura-1.26.60.levipack (needs an NDK)
ANDROID_NDK_HOME=<ndk> ./scripts/build_and_test.sh --native
```

The native build uses `-std=c++20 -O2 -fno-strict-aliasing` and links with
`-Wl,-z,max-page-size=16384` (16 KB page alignment for modern Android devices);
the script verifies the resulting PT_LOAD alignment and package layout.

CMake is also supported for the native build
(`cmake -B build -DCMAKE_TOOLCHAIN_FILE=<ndk>/build/cmake/android.toolchain.cmake
-DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 && cmake --build build --target levi_package`).

CI (`.github/workflows/build.yml`) runs the host tests, the NDK arm64-v8a build
and uploads the `.levipack` artifact on every push/PR.

## Layout

```
research/            RE deliverable (hook tables, engine spec, validation)
src/kaura/           engine port (AuraCore), config (AuraConfig), mod (KillAuraMod)
src/kaura/re/        game bindings: offsets, prologues, resolver, access, hooks
tests/               host unit tests (decision logic, math helpers, CPS)
third_party/         vendored deps: fmt 11.2.0, nlohmann/json 3.11.3, pl SDK headers
```
