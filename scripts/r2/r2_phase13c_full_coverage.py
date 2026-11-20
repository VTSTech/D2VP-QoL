#!/usr/bin/env python3
"""Phase 13c: definitive global coverage with capstone resync.
Full sweep of .text (0x401000-0x6cc000), all call targets vs all D2VP map addrs.
Outputs: overall %, uncovered regions sorted by target count, and per-window
breakdown for the in-game layer (0x44c200-0x516000) and BNet layer."""
import json, os, pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

BASE = "/home/z/my-project/D2VP-QoL/Source"
PE = "/tmp/gameexe/Game.exe"

cov = set()
for root, dirs, files in os.walk(BASE):
    for fn in files:
        if fn.endswith(".map"):
            try: d = json.load(open(os.path.join(root, fn), encoding="utf-8"))
            except Exception: continue
            for f in d.get("functions", []):
                a = f.get("address", "")
                if a: cov.add(int(a, 16))
print(f"D2VP map functions: {len(cov)}")

pe = pefile.PE(PE)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
md = Cs(CS_ARCH_X86, CS_MODE_32)
calls = {}
for s in pe.sections:
    if s.Name.decode().rstrip("\x00") != ".text": continue
    lo = base + s.VirtualAddress; raw = s.PointerToRawData; sz = s.SizeOfRawData
    buf = data[raw:raw+sz]
    pos = 0
    while pos < len(buf):
        n = 0
        for i in md.disasm(buf[pos:], lo+pos):
            n = (i.address + i.size) - (lo + pos)
            if i.mnemonic == "call" and i.op_str.startswith("0x"):
                t = int(i.op_str, 16)
                calls[t] = calls.get(t, 0) + 1
        pos += n if n else 1

tgts = sorted(calls)
miss = sorted(c for c in tgts if not any(0 <= c - x < 0x10 for x in cov))
print(f"distinct call targets: {len(tgts)} | uncovered: {len(miss)} ({len(miss)/len(tgts)*100:.1f}%)")

def regions(lst, gap=0x800):
    out, cur = [], []
    for a, b in zip(lst, lst[1:]):
        cur.append(a)
        if b - a > gap:
            out.append(cur); cur = []
    if lst: cur.append(lst[-1]); out.append(cur)
    return out

regs = [r for r in regions(miss) if len(r) > 3]
regs.sort(key=len, reverse=True)
print(f"uncovered regions >3 targets: {len(regs)}")
for r in regs[:20]:
    print(f"  {r[0]:#x}–{r[-1]:#x}  ({len(r)} targets, span {r[-1]-r[0]:#x})")

for name, lo, hi in [("BNet BNCS 0x518000-0x52c000", 0x518000, 0x52c000),
                     ("BNet MCP/CharSel 0x435000-0x44c200", 0x435000, 0x44c200),
                     ("in-game client 0x44c200-0x516000", 0x44c200, 0x516000),
                     ("in-game server(D2Game) 0x5ec000-0x6a0000", 0x5ec000, 0x6a0000)]:
    t = [c for c in tgts if lo <= c < hi]
    m = [c for c in miss if lo <= c < hi]
    print(f"{name}: {len(t)} targets, {len(m)} uncovered ({len(m)/max(len(t),1)*100:.0f}%)")
