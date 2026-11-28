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
./scripts/audit_preloader_symbols.sh build/libApollonKillAura_host.so

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
    for cand in \
        "$ANDROID_NDK_HOME/bin/aarch64-linux-android28-clang++" \
        "$ANDROID_NDK_HOME/bin/clang++" \
        "$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++" \
        "$ANDROID_NDK_HOME/toolchain/llvm/bin/clang++"; do
        [[ -x "$cand" ]] && CLANGXX="$cand" && break
    done
fi
test -x "$CLANGXX" || { echo "NDK clang++ not found under $ANDROID_NDK_HOME"; exit 1; }

echo "::notice::stage: host tests + cross-check passed; starting arm64-v8a compile"
echo "== arm64-v8a build ($CLANGXX) =="
mkdir -p build/native
"$CLANGXX" -target aarch64-linux-android28 \
    -std=c++20 -O2 -fno-strict-aliasing \
    -shared -fPIC -fvisibility=hidden -fvisibility-inlines-hidden \
    -I src -I third_party -I third_party/preloader/include -I third_party/fmt/include \
    -Wl,-z,max-page-size=16384 \
    -llog \
    -o build/native/libApollonKillAura.so ${SOURCES} 2> build/native-compile.log || {
        grep -E "error|undefined|Error" build/native-compile.log | head -15 \
            | cut -c1-250 | sed 's/^/::error::/'
        tail -5 build/native-compile.log | cut -c1-250 | sed 's/^/::error::tail: /'
        exit 1
    }

# ---- dynamic symbol audit (pl::/ll:: must match libpreloader.so) -------------
echo "::notice::stage: arm64-v8a compile OK; running preloader symbol audit"
echo "== preloader dynamic symbol audit (nm -Du / readelf) =="
if ! ./scripts/audit_preloader_symbols.sh build/native/libApollonKillAura.so; then
    echo "::error::preloader symbol audit failed on the arm64-v8a library"
    exit 1
fi
if readelf --dyn-syms -W build/native/libApollonKillAura.so \
    | grep -q "_ZN2ll3mod9NativeMod7currentEv"; then
    echo "::notice::abi: ll::mod::NativeMod::current (libpreloader.so runtime ABI)"
else
    echo "::error::ll::mod::NativeMod::current missing from arm64-v8a dynsyms"
    exit 1
fi

# ---- 4. levipack packaging ----------------------------------------------------
echo "::notice::stage: symbol audit OK; packaging"
echo "== packaging ApollonKillAura-1.26.60.levipack =="
rm -rf build/package && mkdir -p build/package/ApollonKillAura
cp build/native/libApollonKillAura.so build/package/ApollonKillAura/
sed -e 's/@MOD_NAME@/Apollon KillAura/' \
    -e 's/@MOD_AUTHOR@/NgawiNgab/' \
    -e 's/@MOD_VERSION@/1.0.1/' \
    -e 's/@MOD_LIBRARY_NAME@/ApollonKillAura/' \
    -e 's/@MOD_ICON@//' \
    -e 's/@MOD_MINECRAFT_VERSIONS@/["1.26.60"]/' \
    manifest.json.in > build/package/ApollonKillAura/manifest.json
( cd build/package && zip -qr ../ApollonKillAura-1.26.60.levipack ApollonKillAura )

# ---- 5. sanity checks ----------------------------------------------------------
echo "== ELF checks =="
LOAD_LINES="$(readelf -lW build/native/libApollonKillAura.so | grep -E "LOAD")"
echo "$LOAD_LINES"
ARCH="$(readelf -hW build/native/libApollonKillAura.so | grep Machine | awk '{print $2}')"
PLSYM="$(readelf --dyn-syms -W build/native/libApollonKillAura.so | grep -c PLGetModRegistration || true)"
SHA_SO="$(sha256sum build/native/libApollonKillAura.so | cut -d' ' -f1)"
if echo "$LOAD_LINES" | grep -qE "0x0+4000 0x0+4000"; then
    echo "FAIL: 4KB-aligned PT_LOAD found"; exit 1
fi
python3 - <<'EOF'
import subprocess
out = subprocess.check_output(
    ["readelf", "-lW", "build/native/libApollonKillAura.so"], text=True)
bad = 0
for line in out.splitlines():
    if line.strip().startswith("LOAD"):
        f = line.split()
        off = int(f[1], 16); va = int(f[2], 16); align = int(f[-1], 16)
        # Android 16 KB rule: (p_vaddr - p_offset) % p_align == 0, p_align >= 16384
        if (va - off) % max(align, 16384) != 0:
            bad += 1
print("ALIGN-FAIL" if bad else "ALIGN-OK: all PT_LOAD satisfy (vaddr-offset)%16384==0")
EOF
[ "$ARCH" = "AArch64" ] || { echo "FAIL: not an AArch64 ELF ($ARCH)"; exit 1; }
[ "$PLSYM" -ge 1 ] || { echo "FAIL: PLGetModRegistration not exported"; exit 1; }
echo "16 KB max-page-size OK; arch=$ARCH; PLGetModRegistration exported"
echo "sha256(libApollonKillAura.so) = $SHA_SO"
file_size=$(stat -c %s build/native/libApollonKillAura.so)
echo "libApollonKillAura.so: ${file_size} bytes"

# CI annotations: verification evidence readable via the API.
echo "$LOAD_LINES" | cut -c1-250 | sed 's/^/::notice::elf: /'
echo "::notice::elf-arch: $ARCH"
echo "::notice::pl-export: PLGetModRegistration entries=$PLSYM"
echo "::notice::sha256-so: $SHA_SO"

python3 - <<'EOF'
import zipfile, json, hashlib
z = zipfile.ZipFile("build/ApollonKillAura-1.26.60.levipack")
names = z.namelist()
assert "ApollonKillAura/libApollonKillAura.so" in names, names
assert "ApollonKillAura/manifest.json" in names, names
manifest = json.loads(z.read("ApollonKillAura/manifest.json"))
assert manifest["minecraft_versions"] == ["1.26.60"], manifest
assert manifest["entry"] == "libApollonKillAura.so", manifest
print("levipack OK:", names, "manifest:", manifest)
sha = hashlib.sha256(open("build/ApollonKillAura-1.26.60.levipack","rb").read()).hexdigest()
print("sha256(levipack) =", sha)
print("::notice::levipack: " + ",".join(names)[:230])
print("::notice::manifest: " + json.dumps(manifest)[:230])
print("::notice::sha256-levipack: " + sha)
EOF
echo "::notice::stage: all checks done"
echo "== done =="
