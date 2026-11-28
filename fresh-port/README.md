# Independent Kill Aura investigation and inactive prototype

This directory is a fresh implementation based on the supplied binaries,
Ghidra/Capstone evidence, and independent upstream LeviLauncher/BedrockTools
references. It does not use the failed root KillAura implementation.

**Not a functional or 1:1 mod. Do not use for combat or device testing.** The
LeviLauncher menu and settings persistence are a prototype; in-memory prologues
are checked, but *no hooks are installed*, including when they match. The
host-testable target-selection model is an approximation, not validated game
behavior. In particular, rotation component traversal, entity lifetimes,
item identification, CPS initialization, antiBot edge cases and several
calling conventions remain unverified. The Switch model is deliberately inert;
this is not proof of complete behavior.

Findings: [research/KILLAURA_1_26_60_RE.md](../research/KILLAURA_1_26_60_RE.md).
The Capstone verifier prints game code and Sigma displacements separately to
avoid presenting an instruction's displacement as a verified game class field.
The upstream preloader snapshot used here is `cc08b40a31471d3e2dedb267f5f17b28da2b0604`;
it uses `pl::mod::ModContext`, `PL_REGISTER_MOD`, and `pl::modmenu`. This mod
does not call `NativeMod::current()` or export `JNI_OnLoad`; whether its other
imports match a particular device's installed preloader is not yet validated.
The user's Scaffold repository was inaccessible to the configured GitHub
integration, so the build layout was studied from BedrockTools instead.

## Host tests

```sh
# Use an independent checkout of nlohmann/json v3.11.3:
NLOHMANN_INCLUDE=/path/to/json/single_include ./fresh-port/tests/run_host_tests.sh
```

The C++ model and config tests use ASan/UBSan; Python tests cover the scanning
logic. Tests of the actual game hooks and Android loading are not implemented.

## Android build (inactive prototype only)

GitHub Actions builds the arm64 prototype with a runner-provided Android NDK,
links against the pinned preloader source, and checks ELF architecture, exported
registration, absence of `JNI_OnLoad`, and 16 KB PT_LOAD alignment. It uploads
an explicitly named `ApollonKillAura-INACTIVE-PROTOTYPE-1.26.60.4.levipack`.
The build is not a device, installed-preloader ABI or gameplay validation.
The local sandbox cannot obtain an NDK, so Android compilation ran in Actions.

To reproduce using Linux, CMake and an Android NDK:

```sh
ANDROID_NDK_HOME=/path/to/ndk \
PRELOADER_SDK=/path/to/preloader-android \
FMT_INCLUDE=/path/to/fmt/include \
NLOHMANN_INCLUDE=/path/to/json/single_include \
./fresh-port/scripts/build_native.sh
```

The script requests C++20, `-O2 -fno-strict-aliasing` and 16 KB max page size.
An additional xmake layout is supplied but has not been validated. The old root
build is not part of this port. Neither build produces a working combat mod.
