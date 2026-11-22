#!/bin/bash
# Run one QCADesigner-E coherence-vector-energy simulation in its own folder.
# usage: run_one.sh TAG DESIGN.qca T_ps N_PERIODS TIME_STEP VEC1 [VEC2]
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TAG=$1; QCA=$2; T=$3; NP=$4; STEP=$5; shift 5
DIR="$ROOT/runs/$TAG"
mkdir -p "$DIR"
cd "$DIR" || exit 1
if [ -f done.txt ]; then exit 0; fi
s=$(date +%s)
"$ROOT/src/run_cve" "$ROOT/designs/$QCA" "$T" "$NP" "$STEP" 1 "$@" > out.txt 2> err_raw.txt
rc=$?
e=$(date +%s)
# drop progress-bar noise, keep the engine log
tr '\r' '\n' < err_raw.txt | grep -a -v -E '^\s*(\x1b\[K)?[0-9.]+%\s*$' | sed 's/\x1b\[K//g' > engine_log.txt
rm -f err_raw.txt QCADesignerE_Diss.trace
gzip -f trace.csv
printf 'tag=%s\ndesign=%s\nT_ps=%s\nn_periods=%s\ntime_step=%s\nvectors=%s\nrc=%s\nwall_s=%s\n' \
  "$TAG" "$QCA" "$T" "$NP" "$STEP" "$*" "$rc" "$((e-s))" > params.txt
[ $rc -eq 0 ] && touch done.txt
echo "$TAG rc=$rc $((e-s))s"
