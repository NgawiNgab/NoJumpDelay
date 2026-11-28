# Apollon Kill Aura for Bedrock 1.26.60.4

This independent native implementation uses the supplied binary evidence and
public preloader/BedrockTools references; the earlier root implementation was
not used. The user's Scaffold repository was inaccessible to the configured
GitHub integration. The preloader SDK snapshot is
`cc08b40a31471d3e2dedb267f5f17b28da2b0604`.

## Runtime and limitations

At enable time the mod checks twelve 16-byte executable prologues in loaded
`libminecraftpe.so`. A mismatch fails enable without installing either hook;
a failure installing the second hook rolls the first back. Disable removes
both hooks through `pl::memory::unhook`. The native menu and SVG shortcuts
persist configuration in the preloader's config directory.

**Prologue matches are not ABI verification.** The two detour signatures,
component/actor fields, data addresses, `std::vector` allocation/ownership,
rotation writes and emulated-native-attack calls are inferred or supplied
assumptions. In particular, one native-attack routine appears to clear and
free a pointer. This mod has not been loaded or tested on a device, and the
exact installed preloader's ABI is unverified. Host tests simulate game objects
and cannot establish behavior, crash safety or 1:1 parity with Sigma. `Switch`
mode is not implemented in the selection model. Rotation component registry
updates and some Sigma filtering and timing paths are not reproduced. See
[research/KILLAURA_1_26_60_RE.md](../research/KILLAURA_1_26_60_RE.md).

The SDK unhook API does not provide a documented mechanism to drain callbacks
already executing on another thread. Disabling/unloading concurrently with a
game callback has not been validated; treat it as a lifecycle risk.

## Host tests

```sh
NLOHMANN_INCLUDE=/path/to/json/single_include ./fresh-port/tests/run_host_tests.sh
PRELOADER_SDK=/path/to/preloader-android FMT_INCLUDE=/path/to/fmt/include \\
NLOHMANN_INCLUDE=/path/to/json/single_include JAVA_HOME=/path/to/jdk \\
./fresh-port/tests/run_sdk_compile.sh
```

The C++ model, combat mock, hook-lifecycle mock, config and signature checks
use ASan/UBSan; Python tests cover the scanning logic. No device test is included.

## Android build

GitHub Actions builds with a runner-provided Android NDK, links against the
pinned preloader source, checks AArch64 and 16 KB PT_LOAD alignment, and
uploads `ApollonKillAura-1.26.60.4.levipack`. To reproduce on Linux:

```sh
ANDROID_NDK_HOME=/path/to/ndk \\
PRELOADER_SDK=/path/to/preloader-android \\
FMT_INCLUDE=/path/to/fmt/include \\
NLOHMANN_INCLUDE=/path/to/json/single_include \\
./fresh-port/scripts/build_native.sh
```

The script requests C++20, `-O2 -fno-strict-aliasing` and 16 KB max page size.
An xmake layout is supplied but has not been validated. A successful build is
not evidence of compatibility or parity.
