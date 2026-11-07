#!/usr/bin/env python3
"""vanilla-plus: full-tree mingw i686 cross-sweep (compile_commands.json driven).

Replicates the owner's Win11 build compile-by-compile: every TU from
compile_commands.json is re-run with -fsyntax-only, same flags, i686 mingw
compiler, on the Linux host (no execution, no codegen). Exit 0 = all clean.

    python3 scripts/cross_sweep.py [build-dir]

Build-dir defaults to /tmp/build-sweep (the cross-configured CMake dir).
"""
import json
import shlex
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

BUILD = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith("--") else "/tmp/build-sweep"
DB = Path(BUILD) / "compile_commands.json"

# optional --start N --end M chunking (foreground runs must fit the tool timeout)
START, END = 0, None
if "--start" in sys.argv:
    START = int(sys.argv[sys.argv.index("--start") + 1])
if "--end" in sys.argv:
    END = int(sys.argv[sys.argv.index("--end") + 1])

entries = json.loads(DB.read_text())
entries = entries[START:END]
print(f"cross-sweep: chunk [{START}:{END}] -> {len(entries)} entries (db total on first line)")

DROP_FLAGS = {"-MD", "-MMD", "-MF", "-MT", "-MQ"}


def rewrite(cmd_args):
    out, i, drop_next = [], 0, False
    for a in cmd_args:
        if drop_next:
            drop_next = False
            continue
        if a in DROP_FLAGS:
            if a in ("-MF", "-MT", "-MQ"):
                drop_next = True
            continue
        if a == "-o":
            drop_next = True
            continue
        if a == "-c":
            continue
        out.append(a)
    out.append("-fsyntax-only")
    return out


def run(entry):
    args = rewrite(shlex.split(entry["command"])) if "command" in entry else rewrite(entry["arguments"])
    src = entry["file"]
    try:
        proc = subprocess.run(
            args, cwd=entry["directory"], capture_output=True, text=True, timeout=45
        )
    except subprocess.TimeoutExpired:
        return (src, "TIMEOUT after 45s", 1)
    if proc.returncode == 0:
        return None
    # first error line + count, enough to classify
    errs = [l for l in proc.stderr.splitlines() if ": error:" in l or "error:" in l]
    return (src, errs[0] if errs else proc.stderr.strip().splitlines()[:1], len(errs))


with ThreadPoolExecutor(max_workers=8) as pool:
    results = []
    for i, r in enumerate(pool.map(run, entries)):
        results.append(r)
        if (i + 1) % 50 == 0:
            print(f"  ... {i + 1}/{len(entries)} ({sum(1 for x in results if x)} failing)", flush=True)

fails = [r for r in results if r]
if not fails:
    print(f"cross-sweep: chunk clean, {len(entries)}/{len(entries)} TUs ok (i686 mingw -fsyntax-only)")
    sys.exit(0)

print(f"cross-sweep: {len(fails)} FAILING TU(s):")
for src, first, n in fails:
    print(f"  {src}\n      {first}  ({n} error lines)")
sys.exit(1)
