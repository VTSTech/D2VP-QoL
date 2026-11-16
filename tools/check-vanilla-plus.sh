#!/usr/bin/env bash
# [R161 TEST-01] The reduced gate suite. The original 525-check
# tools/check-vanilla-plus.sh was retired at rev 152 with the upstream merge
# line (git history keeps the script); this wrapper restores the surviving
# enforcement as one command:
#   1. docs_gate.py   - the ten round checks (round identity, row-table
#      invariants, FEATURES row-count parity, smoke-sync contract, overlay
#      verification when a d2vp-r<N>-overlay.zip is present)
#   2. whole-repo VP-marker census - BEGIN == END across every fork file
#      (docs_gate's pairing check covers git-changed files only, so the
#      ROB-09 class stayed invisible until a file was touched)
#   3. LF normalization (handoff.md §8) - no CRLF bytes in tracked
#      source/docs
# The compile gate of record stays the owner's Win11 build (handoff.md §8);
# the gate first-poll shape stays a review convention (ARCH-01).
set -u
cd "$(dirname "$0")/.."
nFail=0

echo "== 1. docs_gate (round checks) =="
python3 tools/docs_gate.py || nFail=$((nFail+1))

echo "== 2. VP marker census (whole repo) =="
nBegin=$(grep -r "VP-BEGIN" --include="*.cpp" --include="*.h" Source Redivivus 2>/dev/null | wc -l)
nEnd=$(grep -r "VP-END" --include="*.cpp" --include="*.h" Source Redivivus 2>/dev/null | wc -l)
echo "VP-BEGIN: $nBegin  VP-END: $nEnd"
if [ "$nBegin" -ne "$nEnd" ]; then
    echo "FAIL: marker census unbalanced"
    nFail=$((nFail+1))
fi

echo "== 3. LF normalization =="
crlfFiles=$(grep -rlP '\r' --include='*.cpp' --include='*.h' --include='*.md' Source Redivivus tools 2>/dev/null)
for f in CHANGELOG.md FEATURES.md README.md SMOKE-CHECKLIST.md ROUND-SUMMARY.md handoff.md; do
    [ -f "$f" ] && grep -qP '\r' "$f" && crlfFiles="$crlfFiles $f"
done
if [ -n "$crlfFiles" ]; then
    echo "FAIL: CRLF bytes found in:"; echo "$crlfFiles" | head -5
    nFail=$((nFail+1))
else
    echo "PASS: LF-normalized"
fi

if [ "$nFail" -eq 0 ]; then
    echo "check-vanilla-plus: ALL GREEN"
else
    echo "check-vanilla-plus: $nFail section(s) failed"
    exit 1
fi
