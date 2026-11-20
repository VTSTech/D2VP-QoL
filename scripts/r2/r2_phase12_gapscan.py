#!/usr/bin/env python3
"""Gap analysis: compare .map (Ghidra-identified Game.exe functions) vs .cpp
(implemented bodies) for the Battle.net-path modules of D2VP."""
import json, os, re, sys

BASE = "/home/z/my-project/D2VP-QoL/Source"
MODULES = ["Bnclient", "D2MCPClient", "D2Multi", "D2Launch", "D2Net", "D2Client"]

func_re = re.compile(
    r"^[a-zA-Z_][\w\s\*]*?\**\s*(\w+)\s*\([^;{]*\)\s*\{", re.M)

def parse_map(path):
    try:
        d = json.load(open(path, encoding="utf-8"))
    except Exception as e:
        return [], str(e)
    return [(f["name"], f.get("address", "?")) for f in d.get("functions", [])], None

def parse_cpp(path):
    """Return {name: (start_line, body_lines, suspicious_flags)}."""
    try:
        text = open(path, encoding="utf-8").read()
    except Exception:
        return {}
    lines = text.splitlines()
    out = {}
    i = 0
    while i < len(lines):
        m = func_re.match(lines[i])
        if m:
            name = m.group(1)
            depth = lines[i].count("{") - lines[i].count("}")
            start = i
            i += 1
            while i < len(lines) and depth > 0:
                depth += lines[i].count("{") - lines[i].count("}")
                i += 1
            body = i - start
            flags = []
            seg = "\n".join(lines[start:i])
            if body < 5:
                flags.append("tiny")
            if "__debugbreak" in seg or "assert(0)" in seg:
                flags.append("debugbreak")
            if "TODO" in seg or "FIXME" in seg:
                flags.append("todo")
            if seg.count("return") >= 1 and body < 8 and ";" in seg and "(" not in seg.split("return", 1)[1].split(";")[0]:
                flags.append("constant-return")
            out[name] = (start + 1, body, flags)
        else:
            i += 1
    return out

for mod in MODULES:
    d = os.path.join(BASE, mod)
    if not os.path.isdir(d):
        print(f"== {mod}: MISSING DIR"); continue
    total_map, missing, stubs, ok = 0, [], [], 0
    for root, _dirs, files in os.walk(d):
      for fn in sorted(files):
        if not fn.endswith(".map"):
            continue
        base = fn[:-4]
        funcs, err = parse_map(os.path.join(root, fn))
        if err:
            print(f"== {mod}/{fn}: parse error {err}"); continue
        cpp = parse_cpp(os.path.join(root, base))
        total_map += len(funcs)
        for name, addr in funcs:
            if name not in cpp:
                missing.append((addr, name))
            else:
                _, body, flags = cpp[name]
                if "tiny" in flags or "debugbreak" in flags or "todo" in flags:
                    stubs.append((addr, name, body, flags))
                else:
                    ok += 1
        # cpp functions not in map (extra/local helpers) not counted as gaps
    print(f"== {mod}: {total_map} map functions | implemented+real: {ok} | stub/tiny: {len(stubs)} | missing from cpp: {len(missing)}")
    if stubs:
        print("   stubs:")
        for a, n, b, fl in stubs[:25]:
            print(f"     {a} {n} ({b} lines {fl})")
    if missing:
        print("   missing:")
        for a, n in missing[:25]:
            print(f"     {a} {n}")
