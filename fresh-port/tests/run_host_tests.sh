#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
out=${BUILD_DIR:-"$root/build/fresh-port"}
json_include=${NLOHMANN_INCLUDE:?Set NLOHMANN_INCLUDE to nlohmann/json single_include}
mkdir -p "$out"
export KILLAURA_TEST_DIR="$out"
cxx=${CXX:-g++}
flags=(-std=c++20 -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -Wextra -Werror "-I$root/fresh-port/src")
"$cxx" "${flags[@]}" -I"$root/fresh-port/native" "$root/fresh-port/tests/test_rotation.cpp" "$root/fresh-port/native/rotation.cpp" -o "$out/test_rotation"
"$cxx" "${flags[@]}" -I"$root/fresh-port/native" "$root/fresh-port/tests/test_combat.cpp" "$root/fresh-port/native/combat.cpp" "$root/fresh-port/src/engine.cpp" -o "$out/test_combat"
"$cxx" "${flags[@]}" "$root/fresh-port/tests/test_engine.cpp" "$root/fresh-port/src/engine.cpp" -o "$out/test_engine"
"$cxx" "${flags[@]}" "-I$json_include" "$root/fresh-port/tests/test_config.cpp" "$root/fresh-port/src/config.cpp" -o "$out/test_config"
"$cxx" -shared -fPIC -x c++ -o "$out/libminecraftpe.so" - <<<'extern "C" void dummy() {}'
"$cxx" "${flags[@]}" "-I$root/fresh-port/native" "$root/fresh-port/tests/test_verification.cpp" "$root/fresh-port/native/verification.cpp" -ldl -o "$out/test_verification"
"$cxx" "${flags[@]}" -Ddl_iterate_phdr=aura_test_iterate_phdr "-I$root/fresh-port/native" "$root/fresh-port/tests/test_verification_matches.cpp" "$root/fresh-port/native/verification.cpp" -o "$out/test_verification_matches"
"$out/test_rotation"
"$out/test_combat"
"$out/test_engine"
"$out/test_config"
"$out/test_verification" "$out/libminecraftpe.so"
"$out/test_verification_matches"
python3 -m unittest discover -s "$root/fresh-port/tests" -p 'test_*.py' -v
