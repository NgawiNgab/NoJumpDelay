#!/usr/bin/env bash
# Reproduce the fresh analysis without using the legacy project or its offsets.
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
target="$workspace/targets/libReverseSigma.so"
if [[ -e "$workspace/ghidra_proj/SigmaFresh.gpr" ]]; then
  echo 'SigmaFresh already exists; use a fresh workspace or remove only that project intentionally.' >&2
  exit 1
fi
"$ghidra" "$workspace/ghidra_proj" SigmaFresh -import "$target" \
  -scriptPath "$root" -postScript KillAuraXrefs.java -analysisTimeoutPerFile 1800 \
  -max-cpu 2 -log "$workspace/ghidra_proj/analysis.log"
"$ghidra" "$workspace/ghidra_proj" SigmaFresh -process libReverseSigma.so -noanalysis \
  -scriptPath "$root" -postScript KillAuraGlobals.java -max-cpu 2 \
  -log "$workspace/ghidra_proj/globals.log"
"$ghidra" "$workspace/ghidra_proj" SigmaFresh -process libReverseSigma.so -noanalysis \
  -scriptPath "$root" -postScript DecodeEvidence.java -postScript CombatHelpers.java -max-cpu 2 \
  -log "$workspace/ghidra_proj/combat-helpers.log"
