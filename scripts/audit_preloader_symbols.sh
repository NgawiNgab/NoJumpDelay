#!/usr/bin/env bash
# Dynamic symbol audit for a built mod library (nm -Du / readelf equivalent).
#
#   scripts/audit_preloader_symbols.sh <path-to-libApollonKillAura.so> #       [probe-compiler command...]
#
# The probe is compiled with the SAME toolchain as the audited library so the
# C++-standard-library mangling (libstdc++ "St17..." vs libc++ "NSt3__1...")
# matches; pass the NDK clang++ command when auditing an arm64-v8a build.
#
# Fails the build when the .so references any undefined pl::/ll:: symbol that
# the vendored official preloader SDK headers do not expose, and specifically
# verifies that _ZN2pl3mod9NativeMod7currentEv is completely gone while
# _ZN2ll3mod9NativeMod7currentEv (the libpreloader.so device-runtime ABI) is
# present.
set -euo pipefail
REPO="$(cd "$(dirname "$0")/.." && pwd)"
SO="${1:?usage: audit_preloader_symbols.sh <path-to-.so> [probe-cxx...]}"
if [ $# -ge 2 ]; then
    PROBE_CXX=("${@:2}")
else
    PROBE_CXX=(g++ -std=c++20)
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# 1. Ground-truth allowlist: compile the probe against the same vendored
#    headers the mod uses and collect its undefined pl::/ll:: symbols.
"${PROBE_CXX[@]}" -shared -fPIC \
    -I "$REPO/third_party" -I "$REPO/third_party/preloader/include" -I "$REPO/third_party/fmt/include" \
    "$REPO/scripts/probe_preloader_refs.cpp" -o "$TMP/probe.so"
readelf --dyn-syms -W "$TMP/probe.so" | awk '$7=="UND"{print $8}' \
    | grep -E "^_ZN2(pl|ll)" | sort -u > "$TMP/expected" || true

# 2. The mod's own undefined pl::/ll:: dynamic symbols.
readelf --dyn-syms -W "$SO" | awk '$7=="UND"{print $8}' \
    | grep -E "^_ZN2(pl|ll)" | sort -u > "$TMP/mod_undefs" || true

# Normalize libc++ inline-ABI-namespace variance (__ndk1 vs __1) so the
# comparison is toolchain-independent; genuinely different symbols still fail.
sed -E 's/NSt6__ndk1/NSt3__1/g' "$TMP/mod_undefs" | sort -u > "$TMP/mod_norm"
sed -E 's/NSt6__ndk1/NSt3__1/g' "$TMP/expected" | sort -u > "$TMP/exp_norm"

echo "undefined pl::/ll:: symbols in $(basename "$SO"):"
sed 's/^/  /' "$TMP/mod_undefs"
sed 's/^/::notice::undef: /' "$TMP/mod_undefs"

# 3. Every mod import must be a preloader SDK export.
BAD="$(comm -23 "$TMP/mod_norm" "$TMP/exp_norm" || true)"
if [ -n "$BAD" ]; then
    echo "FAIL: undefined symbols NOT exported by LiteLDev/preloader-android:"
    echo "$BAD" | sed 's/^/  /; s/^  /::error::unexpected undef: /2'
    echo "$BAD" | while read -r sym; do echo "::error::unexpected undef: $sym"; done
    exit 1
fi

# 4. The NativeMod ABI regression guard (dlopen bug fixed here).
if grep -q "_ZN2pl3mod9NativeMod7currentEv" "$TMP/mod_undefs"; then
    echo "FAIL: _ZN2pl3mod9NativeMod7currentEv must be completely gone"
    exit 1
fi
if ! grep -q "_ZN2ll3mod9NativeMod7currentEv" "$TMP/mod_undefs"; then
    echo "FAIL: _ZN2ll3mod9NativeMod7currentEv (ll::mod::NativeMod::current) missing"
    exit 1
fi

echo "PRELOADER SYMBOL AUDIT OK"
