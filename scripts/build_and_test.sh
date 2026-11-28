#!/usr/bin/env bash
# Host unit tests, host cross-check compile, native arm64-v8a build + package.
#
#   scripts/build_and_test.sh               # tests + syntax/link cross-check
#   scripts/build_and_test.sh --native      # + arm64-v8a build + levipack
#                                           #   requires ANDROID_NDK_HOME
#
# Compile flags required by the project (applied to the native build):
#   -std=c++20 -O2 -fno-strict-aliasing -Wl,-z,max-page-size=16384
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build

SOURCES="
    src/main.cpp
    src/kaura/AuraCore.cpp src/kaura/AuraConfig.cpp src/kaura/KillAuraMod.cpp
    src/kaura/re/Prologues.cpp src/kaura/re/Resolver.cpp
    src/kaura/re/GameAccess.cpp src/kaura/re/Hooks.cpp
"

# ---- 1. host unit tests under ASan/UBSan ------------------------------------
echo "== host unit tests (ASan/UBSan) =="
g++ -std=c++20 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I src -I third_party \
    tests/test_aura_core.cpp src/kaura/AuraCore.cpp src/kaura/AuraConfig.cpp \
    -o build/test_aura_core
./build/test_aura_core

# ---- 2. full-source syntax + shared-object link cross-check on the host -----
# Validates every TU (including the Android-only binding layer) against the
# same SDK headers. pl:: SDK symbols stay unresolved (runtime-resolved by the
# launcher); __android_log_* use the weak stubs from kaura/re/Logger.h.
echo "== host cross-check (all sources, -shared) =="
g++ -std=c++20 -O2 -fno-strict-aliasing -fPIC -shared \
    -DFMT_HEADER_ONLY=1 \
    -I src -I third_party -I third_party/preloader/include \
    -I third_party/fmt/include \
    ${SOURCES} -o build/libApollonKillAura_host.so
echo "cross-check OK"

if [[ "${1:-}" != "--native" ]]; then
    echo "== done (tests + cross-check) =="
    exit 0
fi

# ---- 3. native arm64-v8a build via the Android NDK ---------------------------
if [[ -z "${ANDROID_NDK_HOME:-}" ]]; then
    echo "ANDROID_NDK_HOME is required for --native (e.g. <ndk>/toolchains/llvm)"
    exit 1
fi
CLANGXX="$(command -v aarch64-linux-android28-clang++ || true)"
if [[ -z "$CLANGXX" ]]; then
    CLANGXX="$ANDROID_NDK_HOME/bin/clang++"
    [[ -x "$CLANGXX" ]] || CLANGXX="$ANDROID_NDK_HOME/bin/aarch64-linux-android28-clang++"
fi
test -x "$CLANGXX" || { echo "NDK clang++ not found under $ANDROID_NDK_HOME"; exit 1; }

echo "== arm64-v8a build ($CLANGXX) =="
"$CLANGXX" -target aarch64-linux-android28 \
    -std=c++20 -O2 -fno-strict-aliasing \
    -shared -fPIC -fvisibility=hidden -fvisibility-inlines-hidden \
    -I src -I third_party -I third_party/preloader/include -I third_party/fmt/include \
    -Wl,-z,max-page-size=16384 \
    -llog \
    -o build/native/libApollonKillAura.so ${SOURCES}
mkdir -p build/native

# ---- 4. levipack packaging ----------------------------------------------------
echo "== packaging ApollonKillAura-1.26.60.levipack =="
rm -rf build/package && mkdir -p build/package/ApollonKillAura
cp build/native/libApollonKillAura.so build/package/ApollonKillAura/
sed -e 's/@MOD_NAME@/Apollon KillAura/' \
    -e 's/@MOD_AUTHOR@/NgawiNgab/' \
    -e 's/@MOD_VERSION@/1.0.0/' \
    -e 's/@MOD_LIBRARY_NAME@/ApollonKillAura/' \
    -e 's/@MOD_ICON@//' \
    -e 's/@MOD_MINECRAFT_VERSIONS@/["1.26.60"]/' \
    manifest.json.in > build/package/ApollonKillAura/manifest.json
( cd build/package && zip -qr ../ApollonKillAura-1.26.60.levipack ApollonKillAura )

# ---- 5. sanity checks ----------------------------------------------------------
echo "== ELF checks =="
readelf -lW build/native/libApollonKillAura.so | grep -E "LOAD" | head -4
if readelf -lW build/native/libApollonKillAura.so | grep -qE "0x0+4000 0x0+4000"; then
    echo "FAIL: 4KB-aligned PT_LOAD found"; exit 1
fi
echo "16 KB max-page-size OK"
file_size=$(stat -c %s build/native/libApollonKillAura.so)
echo "libApollonKillAura.so: ${file_size} bytes"
python3 - <<'EOF'
import zipfile, json
z = zipfile.ZipFile("build/ApollonKillAura-1.26.60.levipack")
names = z.namelist()
assert "ApollonKillAura/libApollonKillAura.so" in names, names
assert "ApollonKillAura/manifest.json" in names, names
manifest = json.loads(z.read("ApollonKillAura/manifest.json"))
assert manifest["minecraft_versions"] == ["1.26.60"], manifest
print("levipack OK:", names, "manifest:", manifest)
EOF
echo "== done =="
