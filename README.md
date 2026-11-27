# No Jump Delay

Experimental native Preloader mod for the **specific** Minecraft Bedrock 1.26.60 arm64 library studied in [the research log](research/NO_JUMP_DELAY_PORT.md). It is not verified on a device and is **not claimed to be a 1:1 Apollon port**. Use at your own risk. Backup your worlds and do not use on servers where it is not permitted.

## Use

Install the `.levipack` with a compatible LeviLauncher Preloader. In ModMenu, toggle **No Jump Delay** (default off). Alternatively edit `<mod root>/config/config.json` before launch: `{"noJumpDelay": true}`. Menu changes persist there. The mod appends `<mod root>/data/nojumpdelay.log`, flushing every message; if the file cannot be opened messages fall back to Android logcat (`NoJumpDelay`). A restart is needed after editing the file directly.

The library waits for `libminecraftpe.so`, verifies GNU build ID `eb291b883dfdfaff70c4999c2eabd61885ae3e15` and prologue bytes, and refuses unknown builds. With the toggle on, the tick detour checks pointers, health and component presence, resets the observed component field and calls the original. Pointer and map checks are best effort, not protection from concurrent mutation/unmapping or incompatible game state. Disable stops new modifications, but unload after hooking is refused: the hook API has no documented quiescence guarantee, so restart to unload. Health, screens and other conditions differ from Apollon; its screen guards are not implemented. No device compatibility or safety claim is made.

## Build and tests

Use Android NDK r28c, CMake 3.22+, and network access for pinned Preloader and its build dependencies:

```sh
cmake -S . -B build-android -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 -DANDROID_STL=c++_shared -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-android --target NoJumpDelay -j2
python3 scripts/package.py build-android/libNoJumpDelay.so artifacts
```

Preloader and its C++ shared runtime must be supplied by the launcher; the package does not ship a second preloader. The tested Preloader API is pinned in CMake and may not exist in older installations. Host tests: `cmake -S . -B build-host -DNOJUMP_TEST_ONLY=ON && cmake --build build-host && ctest --test-dir build-host --output-on-failure`. They exercise synthetic sparse-set layouts only. CI validates ELF architecture, 16 KB load alignment, exports and package contents; none are real-game or device tests.
