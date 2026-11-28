# Vendored preloader SDK — ABI patch note

These headers are the official `LiteLDev/preloader-android` SDK headers
(pinned to the set the official `QYCottage/levilauncher-android-mod-template`
and working mods use), with **one documented ABI patch**:

`include/pl/Mod.hpp` defines the mod-lifecycle classes (`NativeMod`,
`ModContext`, `ModInfo`, `ModRegistration`) in **`namespace ll::mod`**, with
`namespace pl::mod` kept as source-level `using` aliases. Class layouts are
byte-identical to upstream; only the definition namespace moved.

## Why

The LeviLauncher runtime (`libpreloader.so`) shipped on device exports
`_ZN2ll3mod9NativeMod7currentEv` (`ll::mod::NativeMod::current()`) and does
**not** export the upstream `pl::mod`-mangled variant. Linking against the
upstream namespace produced:

```
dlopen failed: cannot locate symbol "_ZN2pl3mod9NativeMod7currentEv"
```

(the official template's own `MyMod.cpp` uses `ll::mod::NativeMod::current()`
for the same reason). `pl::modmenu::*` and `pl::memory::*` remain in `pl::`
unchanged, matching the runtime.

## Guardrail

`scripts/audit_preloader_symbols.sh` compiles
`scripts/probe_preloader_refs.cpp` (which references every `PL_EXPORT` runtime
API) and verifies the built library's undefined `pl::/ll::` dynamic symbols
are a subset of that allowlist — and specifically that
`_ZN2pl3mod9NativeMod7currentEv` is gone while
`_ZN2ll3mod9NativeMod7currentEv` is present. It runs in the build script and
in CI on both the host cross-check `.so` and the packaged arm64-v8a `.so`.
