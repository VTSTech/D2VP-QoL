#!/usr/bin/env python3
"""R2 phase 6: locate the retail monster timer-method table + dispatcher.

Patterns:
  a) indirect dispatch: ff 24 85 <imm32>  (call dword ptr [eax*4 + imm32])
     or ff 14 85 / ff 14 8d variants; extract table bases.
  b) 15-slot tables in .data where each slot is 0 or a code VA, >=10 code.
  c) for each candidate table, print slots; identify entry 3 (RegenAndPsnDecreaser)
     by its signature: calls SetEvent/type-3 requeue (UnitQueueEvent 0x5417d0),
     pushes 0x3e (62) / state 2 poison, GetStatsByState-ish, hp pct 0xab packet.
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
                 s.PointerToRawData, s.SizeOfRawData, s.Misc_VirtualSize, s.Characteristics))

def off2va(off):
    for lo, name, raw, sz, vs, ch in secs:
        if raw <= off < raw + sz:
            return lo + (off - raw), name
    return None, None

def va2sec(va):
    for lo, name, raw, sz, vs, ch in secs:
        if lo <= va < lo + max(sz, vs):
            return name
    return None

CODE_LO, CODE_HI = 0x401000, 0x66BA00

# (a) indirect call [reg*4 + imm32]
print("== indirect call table bases (ff 24 85 / ff 14 85 / ff 14 8d) ==")
bases = {}
for pat, reg in ((b"\xff\x24\x85", "eax"), (b"\xff\x14\x85", "eax"), (b"\xff\x14\x8d", "ebp")):
    start = 0
    while True:
        i = data.find(pat, start)
        if i < 0:
            break
        imm = struct.unpack_from("<I", data, i + 3)[0]
        hva, sec = off2va(i)
        if hva and va2sec(imm) in (".data", ".rdata"):
            bases.setdefault(imm, []).append((hva, reg))
        start = i + 1
for b, sites in sorted(bases.items()):
    # shape check: 15 slots each 0/code
    ok = False
    if va2sec(b) == ".data":
        off = None
        for lo, name, raw, sz, vs, ch in secs:
            if name == ".data":
                off = raw + (b - lo)
        slots = [struct.unpack_from("<I", data, off + 4 * k)[0] for k in range(15)]
        ncode = sum(1 for v in slots if CODE_LO <= v < CODE_HI)
        nnull = sum(1 for v in slots if v == 0)
        ok = ncode + nnull == 15 and ncode >= 8
        print(f"  base {b:#x} from {[hex(s[0]) for s in sites]} slots15: code={ncode} null={nnull} ok={ok}")
        if ok:
            for k, v in enumerate(slots):
                print(f"    [{k}] {v:#x}" if v else f"    [{k}] NULL")

# (b) generic 15-slot scan with nulls allowed
print("\n== 15-slot code/null tables in .data (>=10 code) ==")
dl = [s for s in secs if s[1] == ".data"][0]
raw, sz = dl[2], dl[3]
va0 = dl[0]
for off in range(0, sz - 60, 4):
    slots = [struct.unpack_from("<I", data, off + 4 * k)[0] for k in range(15)]
    ncode = sum(1 for v in slots if CODE_LO <= v < CODE_HI)
    nnull = sum(1 for v in slots if v == 0)
    if ncode >= 10 and ncode + nnull == 15:
        # require >=2 slots >=0x500000 (D2Game region) to avoid noise
        nhigh = sum(1 for v in slots if 0x500000 <= v < CODE_HI)
        if nhigh >= 4:
            print(f"  table @ {va0 + off:#x}:")
            for k, v in enumerate(slots):
                print(f"    [{k}] {v:#x}" if v else f"    [{k}] NULL")
