#!/usr/bin/env python3
"""R2 phase 10e: locate BNCS/D2CS opcode dispatch functions by opcode-constant density."""
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

def dump(f, n, label, maxlen=0x300):
    print(f"\n===== {label} {f:#x}..{fend(f):#x} =====")
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

# 1. linear disasm around the 'logon acknowledged' site
print("== linear scan 0x520cc0..0x520e00 ==")
off = sec_ptr(0x520cc0)
for i in md.disasm(data[off:off + 0x140], 0x520cc0):
    ann = ""
    if i.mnemonic == "push" and i.op_str.startswith("0x"):
        v = int(i.op_str, 16)
        s = cstr(v) if v > 0x401000 else None
        if s: ann = f" ; \"{s}\""
    if i.mnemonic in ("call", "jmp") and i.op_str.startswith("0x"):
        ann = f" ; -> {int(i.op_str,16):#x}"
    print(f"  {i.address:#x}: {i.mnemonic} {i.op_str}{ann}")

# 2. opcode-density scan
SID_SET = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0a, 0x0e, 0x0f, 0x12, 0x14, 0x17,
           0x19, 0x25, 0x3a, 0x3c, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x59, 0x5a, 0x5c}
def density(lo_f, hi_f, label):
    print(f"\n== opcode-constant density {label} ==")
    scored = []
    fns = [e for e in entries if lo_f <= e < hi_f]
    for k, f in enumerate(fns):
        end = fns[k + 1] if k + 1 < len(fns) else f + 0x3000
        hits = set()
        for a in range(f, min(end, f + 0x4000)):
            m = insns.get(a)
            if m and m[0] == "cmp":
                try: v = int(m[1].split(",")[1].strip(), 16)
                except Exception: continue
                if v in SID_SET: hits.add(v)
        if len(hits) >= 6:
            scored.append((len(hits), f, end, sorted(hex(h) for h in hits)))
    for n, f, e, h in sorted(scored, reverse=True)[:6]:
        print(f"  {f:#x}..{e:#x}: {n} distinct {h}")

density(0x518000, 0x524000, "BNCS region 0x518000-0x524000")
density(0x440000, 0x44c000, "MCP region 0x440000-0x44c000")
density(0x432000, 0x43b000, "CharSel region 0x432000-0x43b000")

# 3. callers of D2CS dispatcher
for x in [0x43ac84, 0x44b172]:
    f = containing(x)
    print(f"\ncall site {x:#x} in func {f:#x}..{fend(f):#x}")
