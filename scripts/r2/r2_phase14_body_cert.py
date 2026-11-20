#!/usr/bin/env python3
"""Phase 14: body-level certification of the in-game D2GS layer.
For every tiny/stub/missing cpp body in D2Client and D2Net, disassemble the
corresponding Game.exe function (same address, disasm until ret) and compare
sizes. A tiny D2VP body is 'faithful-trivial' if Game.exe's own body is also
small; a large Game.exe body behind a tiny D2VP body = gutting suspect."""
import json, os, re, pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

BASE = "/home/z/my-project/D2VP-QoL/Source"
PE = "/tmp/gameexe/Game.exe"
MODULES = ["D2Client", "D2Net"]
TRIVIAL_MAX = 10          # Game.exe insns under which a tiny D2VP body is faithful

func_re = re.compile(r"^[a-zA-Z_][\w\s\*]*?\**\s*(\w+)\s*\([^;{]*\)\s*\{", re.M)

def parse_cpp(path):
    try: lines = open(path, encoding="utf-8").read().splitlines()
    except Exception: return {}
    out, i = {}, 0
    while i < len(lines):
        m = func_re.match(lines[i])
        if m:
            name = m.group(1); start = i
            depth = lines[i].count("{") - lines[i].count("}"); i += 1
            while i < len(lines) and depth > 0:
                depth += lines[i].count("{") - lines[i].count("}"); i += 1
            flags = []
            body = i - start
            seg = "\n".join(lines[start:i])
            if body < 5: flags.append("tiny")
            if "__debugbreak" in seg or "assert(0)" in seg: flags.append("debugbreak")
            if re.search(r"return\s+(0|false|nullptr)\s*;", seg) and body < 8:
                flags.append("constant-return")
            out[name] = (body, flags)
        else:
            i += 1
    return out

pe = pefile.PE(PE)
base_img = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
md = Cs(CS_ARCH_X86, CS_MODE_32)

def game_body_size(addr, max_insns=400):
    """disasm Game.exe function at addr until ret/jmp; return (insns, last_mnem)"""
    for s in pe.sections:
        lo = base_img + s.VirtualAddress
        if s.Name.decode().rstrip("\x00") == ".text" and lo <= addr < lo + s.Misc_VirtualSize:
            off = s.PointerToRawData + (addr - lo)
            buf = data[off:off + 1600]
            n = 0
            for i in md.disasm(buf, addr):
                n += 1
                if i.mnemonic in ("ret", "jmp", "jmp_far") and n > 1:
                    return n, i.mnemonic
                if i.mnemonic == "ret" or i.mnemonic.startswith("ret"):
                    return n, "ret"
                if n >= max_insns: return n, "max"
            return n, "end"
    return 0, "?"

results = []
for mod in MODULES:
    d = os.path.join(BASE, mod)
    for root, _dirs, files in os.walk(d):
        for fn in sorted(files):
            if not fn.endswith(".map"): continue
            try: dd = json.load(open(os.path.join(root, fn), encoding="utf-8"))
            except Exception: continue
            cpp = parse_cpp(os.path.join(root, fn[:-4]))
            for f in dd.get("functions", []):
                name = f["name"]; addr = int(f["address"], 16)
                if name in cpp:
                    body, flags = cpp[name]
                    if not flags: continue
                    kind = "tiny/stub"
                else:
                    body, flags = 0, ["MISSING"]
                    kind = "missing"
                g, last = game_body_size(addr)
                results.append((mod, root.replace(d + "/", ""), addr, name, kind, body, flags, g, last))

results.sort(key=lambda r: -r[7])
n_tot = len(results)
faithful = [r for r in results if r[7] <= TRIVIAL_MAX and "MISSING" not in r[6]]
suspects = [r for r in results if r[7] > TRIVIAL_MAX or ("MISSING" in r[6] and r[7] > TRIVIAL_MAX)]
out = os.path.join(BASE, "..", "R2", "phase14_body_cert_results.txt")
with open(out, "w") as fh:
    for r in results:
        fh.write(f"{r[0]:8s} {r[1]:24s} {r[2]:#x} {r[3][:44]:44s} {r[4]:9s} cpp={r[5]:4d} exe={r[7]:4d} {','.join(r[6])} [{r[8]}]\n")
print(f"checked {n_tot} tiny/stub/missing functions in {MODULES}")
print(f"faithful-trivial (exe body <= {TRIVIAL_MAX} insns): {len([r for r in results if r[7] <= TRIVIAL_MAX])}")
print(f"exe bodies > {TRIVIAL_MAX} insns: {len(suspects)}")
for r in suspects[:40]:
    print(f"  {r[0]}/{r[1]} {r[2]:#x} {r[3][:40]:40s} kind={r[4]} cpp={r[5]} exe={r[7]} {r[8]}")
print("full list:", out)
