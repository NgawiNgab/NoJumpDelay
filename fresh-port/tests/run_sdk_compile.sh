#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
out=${BUILD_DIR:-"$root/build/fresh-port"}
sdk=${PRELOADER_SDK:?Set PRELOADER_SDK to independent preloader-android checkout}
fmt=${FMT_INCLUDE:?Set FMT_INCLUDE to fmt include directory}
json=${NLOHMANN_INCLUDE:?Set NLOHMANN_INCLUDE to nlohmann/json single_include}
jdk=${JAVA_HOME:?Set JAVA_HOME to a JDK}
mkdir -p "$out"
"${CXX:-g++}" -std=c++20 -O2 -fno-strict-aliasing -fPIC -shared -Wall -Wextra -Werror -DFMT_HEADER_ONLY=1 \
  -I"$root/fresh-port/src" -I"$root/fresh-port/native" -I"$root/fresh-port/host" \
  -I"$sdk/include" -I"$fmt" -I"$json" -I"$jdk/include" -I"$jdk/include/linux" \
  "$root/fresh-port/native/mod.cpp" "$root/fresh-port/native/verification.cpp" \
  "$root/fresh-port/src/config.cpp" "$root/fresh-port/src/engine.cpp" -ldl -o "$out/libApollonKillAura-host.so"
nm -D "$out/libApollonKillAura-host.so" | grep -Eq ' [TW] PLGetModRegistration$'
if nm -D "$out/libApollonKillAura-host.so" | grep -Eq 'JNI_OnLoad|NativeMod7current'; then
  echo 'Unexpected JNI_OnLoad or NativeMod::current symbol' >&2; exit 1
fi
echo 'Host SDK compile passed; this is not a device ABI or Android build test.'
