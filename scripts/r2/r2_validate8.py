#!/usr/bin/env python3
"""R2 phase 8: dump candidate jump tables + disassemble the small dispatcher
0x5a7f80 to confirm it is MONSTEREVENT_DefaultCallback; follow table targets
to identify the pTimerMethods equivalents (entry 3 = RegenAndPsnDecreaser).
"""
import struct
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

EXE = "/tmp/gameexe/Game.exe"
pe = pefile.PE(EXE)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
secs = []
for s in pe.sections:
    secs.append((base + s.VirtualAddress, s.Name.decode().rstrip("\x00"),
                 s.PointerToRawData, s.SizeOfRawData, s.Characteristics))

def sec_of(va):
    for lo, name, raw, sz, ch in secs:
        if lo <= va < lo + sz:
            return name, raw + (va - lo)
    return None, None

def readva(va):
    name, off = sec_of(va)
    if name is None:
        return None
    return struct.unpack_from("<I", data, off)[0]

md = Cs(CS_ARCH_X86, CS_MODE_32)
def dis(f, n=40, maxb=400):
    name, off = sec_of(f)
    out = []
    for i in md.disasm(data[off:off + maxb], f):
        out.append(f"  {i.address:#x}: {i.mnemonic} {i.op_str}")
        if len(out) >= n:
            break
    return out

for tbl, label in ((0x57230c, "cand1"), (0x5a3ce0, "cand2"), (0x5a7fe0, "cand3")):
    print(f"\n== jump table {label} at {tbl:#x} ==")
    for k in range(16):
        v = readva(tbl + 4 * k)
        print(f"  [{k}] {v:#x}" if v else f"  [{k}] ??")

print("\n== dispatcher cand3 0x5a7f80 ==")
for line in dis(0x5a7f80, 45):
    print(line)

print("\n== dispatcher cand3 first table targets ==")
seen = set()
for k in range(15):
    v = readva(0x5a7fe0 + 4 * k)
    if v and v not in seen:
        seen.add(v)
        print(f"-- target {v:#x} (slot {k}):")
        for line in dis(v, 14):
            print(line)
