#!/usr/bin/env python3
"""Phase 14b: classify the 168 'tiny cpp but large Game.exe body' cases.
For tiny/stub D2VP bodies: extract the cpp body's call targets and verify each
target exists in the D2VP corpus with a real (non-tiny) body. All targets real
=> inlined forwarder, faithful structure. Any missing/tiny target => suspect.
Missing-cpp cases are classified by name as container/UI boilerplate."""
import json, os, re
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import pefile

BASE = "/home/z/my-project/D2VP-QoL/Source"
func_re = re.compile(r"^[a-zA-Z_][\w\s\*]*?\**\s*(\w+)\s*\([^;{]*\)\s*\{", re.M)

# global corpus: name -> (file, body_lines)
corpus = {}
for root, _dirs, files in os.walk(BASE):
    for fn in files:
        if fn.endswith(".cpp"):
            p = os.path.join(root, fn)
            lines = open(p, encoding="utf-8", errors="ignore").read().splitlines()
            i = 0
            while i < len(lines):
                m = func_re.match(lines[i])
                if m:
                    name = m.group(1); start = i
                    depth = lines[i].count("{") - lines[i].count("}"); i += 1
                    while i < len(lines) and depth > 0:
                        depth += lines[i].count("{") - lines[i].count("}"); i += 1
                    corpus.setdefault(name, (p.replace(BASE + "/", ""), i - start))
                else:
                    i += 1

call_re = re.compile(r"\b([A-Za-z_]\w*[A-Za-z_]\w*)\s*\(")
whitelist = {"if", "for", "while", "switch", "return", "sizeof", "memcpy", "memset", "memcmp",
             "strcpy", "strlen", "sprintf", "exit", "free", "malloc"}

pe = pefile.PE("/tmp/gameexe/Game.exe")
base_img = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
md = Cs(CS_ARCH_X86, CS_MODE_32)

res = open(os.path.join(BASE, "..", "R2", "phase14_body_cert_results.txt")).read().splitlines()
tiny_rows, missing_rows = [], []
row_re = re.compile(r"^\S+\s+\S+\s+(0x[0-9a-f]+)\s+(\S+)\s+(tiny/stub|missing)\s+cpp=\s*(\d+)\s+exe=\s*(\d+)")
for line in res:
    m = row_re.match(line)
    if not m: continue
    addr = int(m.group(1), 16); name = m.group(2)
    cpp = int(m.group(4)); exe = int(m.group(5))
    kind = m.group(3)
    if exe <= 10: continue
    (missing_rows if kind == "missing" else tiny_rows).append((addr, name, cpp, exe, line.split(" [")[0]))

print(f"large-exe rows: tiny={len(tiny_rows)} missing={len(missing_rows)}")

# classify missing rows by name pattern
BOILER = re.compile(r"(Constructor|Destructor|CopyConstructor|ScalarDeletingDestr|operator=|UnwindThunk|"
                    r"NewNode|InternalNew|InternalDelete|InternalClear|InternalDestroy|Reserve|ReallocData|"
                    r"SetCount|Clear|Destroy|Link|Unlink|CalcChunkSize|RoundToMultiple|Assign|NextLink|"
                    r"OperatorIndex|Unreferenced|NullSub|New$)")
boiler, other_missing = [], []
for addr, name, cpp, exe, raw in missing_rows:
    (boiler if BOILER.search(name) else other_missing).append((addr, name, exe))
print(f"missing rows: boilerplate-pattern {len(boiler)}, other {len(other_missing)}")
for a, n, e in other_missing:
    print(f"  OTHER-MISSING {a:#x} {n} exe={e}")

# verify forwarders
suspects, certified = [], 0
for addr, name, cpp, exe, raw in tiny_rows:
    # find the cpp body for this function
    loc = None
    for root, _d, files in os.walk(BASE):
        for fn in files:
            if fn.endswith(".cpp"):
                p = os.path.join(root, fn)
                t = open(p, encoding="utf-8", errors="ignore")
                lines = t.read().splitlines()
                for i, l in enumerate(lines):
                    if name in l and func_re.match(l):
                        depth = l.count("{") - l.count("}"); j = i + 1
                        while j < len(lines) and depth > 0:
                            depth += lines[j].count("{") - lines[j].count("}"); j += 1
                        loc = "\n".join(lines[i:j]); break
                if loc: break
        if loc: break
    if not loc:
        suspects.append((addr, name, "body-not-found")); continue
    targets = [w for w in call_re.findall(loc) if w not in whitelist and w != name]
    if not targets:
        suspects.append((addr, name, f"no-call-in-body ({len(loc.splitlines())} lines)")); continue
    bad = []
    for t in set(targets):
        if t in corpus:
            f, bl = corpus[t]
            if bl < 5:
                bad.append(f"{t}@tiny({bl})")
        else:
            bad.append(f"{t}@missing")
    if bad:
        suspects.append((addr, name, "targets:" + ",".join(bad[:3])))
    else:
        certified += 1
print(f"tiny forwarders certified: {certified}/{len(tiny_rows)}")
for s in suspects:
    print(f"  SUSPECT {s[0]:#x} {s[1][:44]:44s} {s[2]}")
