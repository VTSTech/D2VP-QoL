#!/usr/bin/env python3
"""R2 phase 7: find MONSTEREVENT_DefaultCallback in retail.

Shape (fork-derived): __fastcall(pGame=ecx, pUnit=edx, type, a1, a2);
  cmp type,0xe; ja ret; GetUnitState(STATE_freeze=1); GetUnitState(cold=0xb);
  CheckForModeZeroDeath 0x5541b0; then indirect call through a 15-slot table.
Find functions calling both 0x639df0 and 0x5541b0 that contain an indirect
call with scale 4.
"""
import struct
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import bisect

EXE = "/tmp/gameexe/Game.exe"
pe = pefile.PE(EXE)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
secs = []
for s in pe.sections:
    secs.append((base + s.VirtualAddress, s.Name.decode().rstrip("\x00"),
                 s.PointerToRawData, s.SizeOfRawData, s.Characteristics))

md = Cs(CS_ARCH_X86, CS_MODE_32)
insns, calls, jumps, indirects, xrefs = {}, {}, {}, {}, {}
for lo, name, raw, sz, ch in secs:
    if not (ch & 0x20000000):
        continue
    code = data[raw:raw + sz]
    pos = 0
    while pos < len(code):
        last_end = 0
        for i in md.disasm(code[pos:pos + 65536], lo + pos):
            insns[i.address] = (i.mnemonic, i.op_str, i.size)
            if i.mnemonic in ("call", "jmp"):
                if i.op_str.startswith("0x"):
                    t = int(i.op_str, 16)
                    xrefs.setdefault(t, []).append(i.address)
                elif "*4" in i.op_str or "* 4" in i.op_str.replace(" ", ""):
                    indirects[i.address] = (i.mnemonic, i.op_str)
            last_end = i.address + i.size - (lo + pos)
        pos += last_end if last_end else 1

entries = sorted(set(xrefs))
def containing(va):
    i = bisect.bisect_right(entries, va) - 1
    if i < 0:
        return None
    f = entries[i]
    return f

def func_end(f):
    i = bisect.bisect_right(entries, f)
    return entries[i] if i < len(entries) else f + 0x3000

print(f"indirect scale-4 calls: {len(indirects)}")
# candidates: indirect call whose containing function also calls 0x639df0 and 0x5541b0
for a, (m, op) in sorted(indirects.items()):
    f = containing(a)
    if f is None:
        continue
    e = func_end(f)
    tgts = set(t for aa, t in xrefs.items() for aa2 in [] )  # unused
    has_getu = any(t == 0x639df0 for aa, t in ((k, v) for k, v in []))
sites = [a for a, t in [] ]
# simpler: build per-function call target set
from collections import defaultdict
ftg = defaultdict(set)
for aa, t in xrefs.items():
    pass
for iaddr, t in []:
    pass
# rebuild: calls recorded in xrefs have caller addresses; iterate xrefs
caller_targets = defaultdict(set)
for t, callers in xrefs.items():
    for c in callers:
        caller_targets[c] = t
# group by function
fcalls = defaultdict(set)
for c, t in caller_targets.items():
    f = containing(c)
    if f is not None:
        fcalls[f].add(t)

for a, (m, op) in sorted(indirects.items()):
    f = containing(a)
    if f is None:
        continue
    tgs = fcalls.get(f, set())
    if 0x639df0 in tgs and 0x5541b0 in tgs:
        print(f"\nINDIRECT {a:#x}: {m} {op}   func {f:#x}..{func_end(f):#x}")
        print(f"   also calls: {sorted(hex(t) for t in tgs)[:25]}")
