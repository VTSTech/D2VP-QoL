#!/usr/bin/env python3
"""R2 phase 10f: pinpoint MCP/BNCS dispatcher function starts + BNCS packet send."""
import pefile, struct, bisect
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

EXE = "/tmp/gameexe/Game.exe"
pe = pefile.PE(EXE)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
secs = [(base + s.VirtualAddress, s.Name.decode().rstrip("\x00"),
         s.PointerToRawData, s.SizeOfRawData, s.Characteristics) for s in pe.sections]

def sec_ptr(va):
    for lo, name, raw, sz, ch in secs:
        if lo <= va < lo + max(sz, 1) and raw:
            return raw + (va - lo)
    return None

def cstr(va, maxlen=80):
    off = sec_ptr(va)
    if off is None: return None
    end = data.find(b"\x00", off, off + maxlen)
    if end < 0: return None
    t = data[off:end].decode("latin1")
    return t if all(32 <= ord(c) < 127 for c in t) else None

md = Cs(CS_ARCH_X86, CS_MODE_32)
insns, calls, xrefs = {}, {}, {}
for lo, name, raw, sz, ch in secs:
    if name != ".text" or not (ch & 0x20000000): continue
    pos = 0
    while pos < sz:
        le = 0
        for i in md.disasm(data[raw + pos:raw + pos + 65536], lo + pos):
            insns[i.address] = (i.mnemonic, i.op_str, i.size)
            if i.mnemonic in ("call", "jmp") and i.op_str.startswith("0x"):
                t = int(i.op_str, 16)
                calls[i.address] = t
                xrefs.setdefault(t, []).append(i.address)
            le = i.address + i.size - (lo + pos)
        pos += le if le else 1
entries = sorted(set(xrefs))
def containing(va):
    i = bisect.bisect_right(entries, va) - 1
    return entries[i] if i >= 0 else None
def fend(f):
    i = bisect.bisect_right(entries, f)
    return entries[i] if i < len(entries) else f + 0x4000

def dump(f, n, label, maxlen=0x280):
    print(f"\n===== {label} {f:#x} =====")
    off = sec_ptr(f)
    c = 0
    for i in md.disasm(data[off:off + maxlen], f):
        ann = ""
        if i.mnemonic in ("call", "jmp") and i.op_str.startswith("0x"):
            ann = f" ; -> {int(i.op_str,16):#x}"
        if i.mnemonic == "push" and i.op_str.startswith("0x"):
            v = int(i.op_str, 16)
            s = cstr(v) if v > 0x401000 else None
            if s: ann = f" ; \"{s}\""
        print(f"  {i.address:#x}: {i.mnemonic} {i.op_str}{ann}")
        c += 1
        if c >= n: break

# 1. find prologue before 0x44bfe0: linear disasm from 0x44bfc0, look for int3 padding end
def find_start(before, window=0x80):
    off = sec_ptr(before - window)
    prev_int3 = None
    start = None
    for i in md.disasm(data[off:off + window], before - window):
        if i.mnemonic == "int3":
            prev_int3 = i.address
        elif prev_int3 is not None and start is None:
            start = i.address
    return start, prev_int3

for target, lbl in [(0x44bfe0, "MCP reply dispatcher"), (0x523af0, "BNCS region fn"),
                    (0x44efe0, "MCP region tail")]:
    s, p = find_start(target)
    print(f"{lbl}: first non-int3 after padding: {s:#x}" if s else f"{lbl}: none (padding at {p:#x})")

# 2. dump the MCP dispatcher from its start
for probe in [0x44bfe0]:
    s, _ = find_start(probe)
    if s: dump(s, 80, "MCP reply dispatcher (real start)")

# 3. BNCS 0x523af0 region: dump around
s, _ = find_start(0x523af0)
if s: dump(s, 70, "BNCS region fn (real start)")

# 4. BNCS packet send: mov byte [reg], 0xff in BNCS region
print("\n== functions writing 0xff byte (packet signature) ==")
hits = {}
for a, (mn, ops, sz) in insns.items():
    if mn == "mov" and ("byte ptr [" in ops) and ops.endswith(", 0xff"):
        f = containing(a)
        if f and 0x518000 <= f < 0x524000:
            hits.setdefault(f, []).append(a)
for f, sites in sorted(hits.items()):
    print(f"  func {f:#x}..{fend(f):#x}: {[hex(s) for s in sites[:6]]}")

# 5. the 0x72faa4 table: check for paired name strings
print("\n== 0x72faa4 table context (strings around) ==")
toff = sec_ptr(0x72fa90)
for k in range(6):
    v = struct.unpack_from("<I", data, toff + k*4)[0]
    s = cstr(v)
    print(f"  {0x72fa90 + k*4:#x}: {v:#x}" + (f'  "{s}"' if s else ""))

# 6. MCP recv loop + UI caller heads
dump(0x44aae0, 45, "MCP receive loop fn")
dump(0x43aab0, 45, "CharSel region caller of D2CS dispatcher")
