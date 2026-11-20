#!/usr/bin/env python3
"""R2 phase 10g: MCP reply dispatcher, BNCS packet builders, D2CS request senders."""
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

dump(0x44bfe0, 90, "MCP/D2CS reply dispatcher")
dump(0x51ad25, 20, "BNCS packet builder A")
dump(0x51adca, 20, "BNCS packet builder B")
dump(0x44a170, 35, "D2CS payload builder (from send wrapper)")
dump(0x43a460, 40, "charlist block parser")

# D2CS send wrapper callers (request senders)
for va, lbl in [(0x44aae0, "D2CS send wrapper"), (0x51adca, "BNCS builder B"),
                (0x51ad25, "BNCS builder A"), (0x51c5c0, "BNCS sender C")]:
    xs = sorted(set(xrefs.get(va, [])))
    fs = sorted(set(containing(x) for x in xs if containing(x)))
    print(f"\n== {lbl} {va:#x}: {len(xs)} call sites, funcs {[hex(f) for f in fs[:12]]}")

# BNCS callback registration table
print("\n== .data/.rdata pointers into auth-callback region 0x520c00-0x521800 ==")
lo, hi = 0x520c00, 0x521800
for s2 in secs:
    if s2[1] not in (".rdata", ".data"): continue
    lova, name, raw, sz, ch = s2
    blob = data[raw:raw + sz]
    for off in range(0, sz - 3, 4):
        v = struct.unpack_from("<I", blob, off)[0]
        if lo <= v < hi:
            ctx = [struct.unpack_from("<I", blob, off + k*4)[0] if off + k*4 + 4 <= sz else 0
                   for k in range(-2, 3)]
            print(f"  {lova + off:#x} ({name}): -> {v:#x}   ctx {[hex(x) for x in ctx]}")
