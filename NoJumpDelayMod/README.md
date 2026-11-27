# NoJumpDelay research prototype — NOT an operational mod

This project contains a C++ reconstruction of the component lookup and write
recovered from ReverseSigma, plus a Levi Preloader lifecycle skeleton. **It
installs no hook and enable returns false.** The original detour's complete
preconditions/server guard and the obfuscation variables' live values have not
been verified; a 1:1 port must not be claimed from the lookup alone. See
`../nojumpdelay_port_report.md` for evidence and limitations.

The lookup requires real, live game-owned pointers and the original outer
preconditions. The added null checks do not make malformed/stale pointers safe.

## Host synthetic test

```sh
mkdir -p /home/user/analysis/tmp
TMPDIR=/home/user/analysis/tmp g++ -std=c++20 -Wall -Wextra -Werror \
  -fsanitize=address,undefined tests/component_lookup_test.cpp \
  src/ComponentLookup.cpp -o /home/user/analysis/nojump_test
/home/user/analysis/nojump_test
```

## Android build (not verified in this environment)

Use an Android NDK, CMake, and the exact Preloader SDK and shared library from
the launcher build. The included `manifest.json` follows the example's
`preload-native` format. `libpreloader.so` is a runtime dependency and should
come from the launcher rather than bundling a second Preloader.

```sh
export TMPDIR=/home/user/analysis/tmp
cmake -S . -B /home/user/NoJumpDelayMod-build \
 -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
 -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
 -DPRELOADER_INCLUDE_DIR=/path/to/preloader/include \
 -DPRELOADER_LIBRARY=/path/to/arm64/libpreloader.so
cmake --build /home/user/NoJumpDelayMod-build
```

Neither the Android build nor launcher loading was tested; no `.so` is supplied.
