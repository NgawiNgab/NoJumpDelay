#!/usr/bin/env bash
# Targeted target-ELF investigation; deliberately no whole-program analysis.
set -euo pipefail
workspace=/home/user/re_workspace
root=$(cd "$(dirname "$0")" && pwd)
export JAVA_HOME="$workspace/tools/jdk-21.0.5+11"
export PATH="$JAVA_HOME/bin:$PATH"
export TMPDIR="$workspace/temp"
export MAXMEM=1200M
export _JAVA_OPTIONS="-Djava.io.tmpdir=$TMPDIR -XX:ParallelGCThreads=2"
mkdir -p "$TMPDIR" "$workspace/ghidra_proj" "$workspace/decomp"
ghidra="$workspace/tools/ghidra_11.2.1_PUBLIC/support/analyzeHeadless"
target="$workspace/mc_libs/1.26.60.4/libminecraftpe.so"
if [[ -e "$workspace/ghidra_proj/GameFresh.gpr" ]]; then
  args=(-process libminecraftpe.so)
else
  args=(-import "$target")
fi
"$ghidra" "$workspace/ghidra_proj" GameFresh "${args[@]}" -noanalysis \
  -scriptPath "$root" -postScript GameEvidence.java -max-cpu 2 \
  -log "$workspace/ghidra_proj/game.log"
