#!/usr/bin/env bash
# Build the arm64 native mod. Device behavior has not been validated.
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
sdk=${PRELOADER_SDK:?Set PRELOADER_SDK to the pinned independent preloader checkout}
fmt=${FMT_INCLUDE:?Set FMT_INCLUDE to fmt 11.2.0 include}
json=${NLOHMANN_INCLUDE:?Set NLOHMANN_INCLUDE to nlohmann/json v3.11.3 single_include}
ndk=${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to an Android NDK}
out=${BUILD_DIR:-"$root/build/fresh-port-native"}
mkdir -p "$out"
log="$out/native-build.log"
trap 'status=$?; if (( status != 0 )) && [[ -f "$log" ]]; then text=$(tail -30 "$log" | tr "\n" " " | cut -c1-3000); echo "::error::Native build failed: $text"; fi' EXIT
clang="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++"
test -x "$clang" || { echo 'NDK clang++ not found' >&2; exit 1; }
cmake -S "$sdk" -B "$out/preloader" -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-include cstdlib" 2>&1 | tee "$log"
cmake --build "$out/preloader" --target preloader --parallel 2 2>&1 | tee "$log"
preloader=$(find "$out/preloader" -type f -name libpreloader.so -print -quit)
test -n "$preloader" || { echo 'preloader build did not produce libpreloader.so' >&2; exit 1; }
"$clang" -target aarch64-linux-android28 -std=c++20 -O2 -fno-strict-aliasing \
  -include cstdlib -fPIC -shared -fvisibility=hidden -fvisibility-inlines-hidden -DFMT_HEADER_ONLY=1 \
  -I"$root/fresh-port/src" -I"$root/fresh-port/native" -I"$sdk/include" -I"$fmt" -I"$json" \
  "$root/fresh-port/native/mod.cpp" "$root/fresh-port/native/verification.cpp" \
  "$root/fresh-port/native/hooks.cpp" "$root/fresh-port/native/combat.cpp" "$root/fresh-port/native/watcher.cpp" "$root/fresh-port/native/rotation.cpp" \
  "$root/fresh-port/src/config.cpp" "$root/fresh-port/src/engine.cpp" \
  -L"$(dirname "$preloader")" -static-libstdc++ -nostdlib++ -Wl,--exclude-libs,libc++_static.a \
  -Wl,--exclude-libs,libc++abi.a -Wl,--no-undefined -Wl,-z,max-page-size=16384 \
  -Wl,--start-group -l:libc++_static.a -l:libc++abi.a -Wl,--end-group \
  -lpreloader -llog -ldl -o "$out/libApollonKillAura.so" 2>&1 | tee "$log"
python3 "$root/fresh-port/scripts/check_elf.py" "$out/libApollonKillAura.so" 2>&1 | tee "$log"
python3 - "$root/fresh-port/manifest.json" "$out/libApollonKillAura.so" "$out/ApollonKillAura-1.26.60.4.levipack" <<'PY'
import json,sys,zipfile
from pathlib import Path
manifest, library, output = map(Path, sys.argv[1:])
metadata=json.loads(manifest.read_text())
assert metadata['minecraft_versions']==['1.26.60.4']
assert metadata['entry']==library.name
assert metadata['name']=='Apollon KillAura'
with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED) as z:
    z.write(manifest,'manifest.json')
    z.write(library,library.name)
with zipfile.ZipFile(output) as z:
    assert set(z.namelist())=={'manifest.json',library.name}
    assert json.loads(z.read('manifest.json')) == metadata
print('Packaged:',output)
PY
