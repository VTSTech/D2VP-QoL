#!/usr/bin/env python3
"""Phase 13: extend the phase-12 gap analysis to the in-game layer.
Per-module map-vs-cpp scan for D2Game/D2Net + address-coverage diff of Game.exe
call targets inside each module's address span (D2Client, D2Game, D2Net)."""
import json, os, re, pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

BASE = "/home/z/my-project/D2VP-QoL/Source"
PE = "/tmp/gameexe/Game.exe"

func_re = re.compile(r"^[a-zA-Z_][\w\s\*]*?\**\s*(\w+)\s*\([^;{]*\)\s*\{", re.M)

def parse_map(path):
    try:
        d = json.load(open(path, encoding="utf-8"))
    except Exception:
        return [], []
    return [(f["name"], int(f["address"], 16)) for f in d.get("functions", [])], None

def parse_cpp(path):
    try:
        lines = open(path, encoding="utf-8").read().splitlines()
    except Exception:
        return {}
    out, i = {}, 0
    while i < len(lines):
        m = func_re.match(lines[i])
        if m:
            name = m.group(1); start = i
            depth = lines[i].count("{") - lines[i].count("}")
            i += 1
            while i < len(lines) and depth > 0:
                depth += lines[i].count("{") - lines[i].count("}")
                i += 1
            body = i - start
            seg = "\n".join(lines[start:i])
            flags = []
            if body < 5: flags.append("tiny")
            if "__debugbreak" in seg or "assert(0)" in seg: flags.append("debugbreak")
            out[name] = (start + 1, body, flags)
        else:
            i += 1
    return out

def module_scan(mod):
    d = os.path.join(BASE, mod)
    total, missing, stubs, ok = 0, [], [], 0
    if not os.path.isdir(d): return None
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".map"): continue
        funcs, _ = parse_map(os.path.join(d, fn))
        cpp = parse_cpp(os.path.join(d, fn[:-4]))
        total += len(funcs)
        for name, addr in funcs:
            if name not in cpp: missing.append((hex(addr), name))
            else:
                _, body, flags = cpp[name]
                if flags: stubs.append((hex(addr), name, body, flags))
                else: ok += 1
    return total, ok, stubs, missing

# --- per-module scans for the in-game layer ---
for mod in ["D2Game", "D2Net", "D2Client", "D2Hell", "D2Multi"]:
    r = module_scan(mod)
    if not r: print(f"== {mod}: no dir"); continue
    total, ok, stubs, missing = r
    print(f"== {mod}: {total} map fns | real: {ok} | tiny/stub: {len(stubs)} | missing: {len(missing)}")
    for m in missing[:8]: print("   MISSING", m)

# --- module address spans from maps ---
spans = {}
for mod in ["D2Client", "D2Game", "D2Net", "D2Hell"]:
    addrs = []
    d = os.path.join(BASE, mod)
    for fn in os.listdir(d):
        if fn.endswith(".map"):
            for name, a in parse_map(os.path.join(d, fn))[0]:
                addrs.append(a)
    if addrs:
        spans[mod] = (min(addrs), max(addrs), len(addrs))
        print(f"span {mod}: {min(addrs):#x}–{max(addrs):#x} ({len(addrs)} fns)")

# --- Game.exe call-target coverage per span ---
pe = pefile.PE(PE)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
md = Cs(CS_ARCH_X86, CS_MODE_32)
for s in pe.sections:
    if s.Name.decode().rstrip("\x00") != ".text": continue
    lo = base + s.VirtualAddress; raw = s.PointerToRawData; sz = s.SizeOfRawData
    calls = []
    for i in md.disasm(data[raw:raw+sz], lo):
        if i.mnemonic == "call" and i.op_str.startswith("0x"):
            calls.append(int(i.op_str, 16))
    for mod, (a, b, n) in spans.items():
        tgts = sorted({c for c in calls if a <= c <= b})
        cov = set()
        d = os.path.join(BASE, mod)
        for fn in os.listdir(d):
            if fn.endswith(".map"):
                for name, addr in parse_map(os.path.join(d, fn))[0]:
                    cov.add(addr)
        miss = [c for c in tgts if not any(0 <= c - x < 0x10 for x in cov)]
        print(f"coverage {mod}: {len(tgts)} call targets in span, {len(miss)} uncovered")
        for m in miss[:15]: print(f"   {m:#x}")
