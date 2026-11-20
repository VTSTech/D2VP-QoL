#!/usr/bin/env python3
"""R2 phase 5: dump .rdata pointer table around 0x6e2260 and identify
each entry's behavior (the fork's 15-entry monster timer table).
"""
import struct
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

EXE = "/tmp/gameexe/Game.exe"
pe = pefile.PE(EXE)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__

rd = [s for s in pe.sections if s.Name.decode().rstrip("\x00") == ".rdata"][0]
rd_va = base + rd.VirtualAddress
rd_raw, rd_sz = rd.PointerToRawData, rd.SizeOfRawData

def rdptr(va):
    off = rd_raw + (va - rd_va)
    return struct.unpack_from("<I", data, off)[0]

CODE_LO, CODE_HI = 0x401000, 0x66BA00
print("pointer slots 0x6e21c0 .. 0x6e2310:")
for va in range(0x6e21c0, 0x6e2310, 4):
    v = rdptr(va)
    mark = "  <== 0x5a6ff0 (MonDeathEvent)" if va == 0x6e2260 else ""
    tag = "CODE" if CODE_LO <= v < CODE_HI else ("data" if 0x705000 <= v < 0x995000 else ("rdata" if rd_va <= v < rd_va + rd.Misc_VirtualSize else "str?"))
    print(f"  {va:#x}: {v:#x} {tag}{mark}")

# disassemble entry candidates briefly
md = Cs(CS_ARCH_X86, CS_MODE_32)
def peek(f, n=18):
    off = None
    for s in pe.sections:
        lo = base + s.VirtualAddress
        if lo <= f < lo + s.Misc_VirtualSize and (s.Characteristics & 0x20000000):
            off = s.PointerToRawData + (f - lo)
            break
    if off is None:
        return [f"  {f:#x}: not in code"]
    out = []
    for i in md.disasm(data[off:off + 220], f):
        out.append(f"  {i.address:#x}: {i.mnemonic} {i.op_str}")
        if len(out) >= n:
            break
    return out

print("\n-- entry list first 15 ptrs starting 0x6e2254 (assuming [0] near MonDeathEvent slot):")
# find table start: walk back while slots are code ptrs
tstart = 0x6e2260
while True:
    v = rdptr(tstart - 4)
    if CODE_LO <= v < CODE_HI:
        tstart -= 4
    else:
        break
print(f"table start guess: {tstart:#x}")
idx = 0
va = tstart
while va <= 0x6e2310:
    v = rdptr(va)
    if not (CODE_LO <= v < CODE_HI):
        break
    print(f"[{idx}] {va:#x} -> {v:#x}")
    for line in peek(v, 10):
        print(line)
    idx += 1
    va += 4
