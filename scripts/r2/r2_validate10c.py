#!/usr/bin/env python3
"""R2 phase 10c: MCP/D2CS handler disassembly + dispatch-table discovery."""
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

dump(0x435e10, 70, "MCP charcreate error handler")
dump(0x449e70, 40, "McpConnect.cpp fn")
dump(0x441c95, 10, "restore MCP connection (mid)")
dump(0x437ab0, 45, "CharSel fn A")
dump(0x438ad0, 30, "CharSel fn B")
dump(0x438ea0, 45, "CharSel fn C")

# xrefs
for va, lbl in [(0x6df7d8, "logon acknowledged"), (0x6df168, "sFastestConnect()")]:
    sites = []
    pat = struct.pack("<I", va)
    s = 0
    while True:
        i = data.find(pat, s)
        if i < 0: break
        for lo, name, raw, sz, ch in secs:
            if name == ".text" and raw <= i < raw + sz:
                sites.append(lo + i - raw)
        s = i + 1
    fs = sorted(set(containing(x) for x in sites if containing(x)))
    print(f"\nrefs to {lbl} {va:#x}: {[hex(x) for x in sites]} funcs {[hex(f) for f in fs]}")

# dispatch tables: consecutive code pointers into handler region in .data/.rdata
print("\n== pointer tables containing auth/handler-region pointers ==")
HANDLERS = set()
for f in [0x520c40, 0x520c50, 0x520c60, 0x520c80, 0x520ca0, 0x521664, 0x5204a0]:
    HANDLERS.add(f)
lo_txt, hi_txt = 0x51a000, 0x524000
for lo, name, raw, sz, ch in secs:
    if name not in (".rdata", ".data"): continue
    blob = data[raw:raw+sz]
    for off in range(0, sz - 3, 4):
        v = struct.unpack_from("<I", blob, off)[0]
        if lo_txt <= v < hi_txt:
            # print context: 4 dwords around
            ctx = [struct.unpack_from("<I", blob, off + k*4)[0] if off+k*4+4 <= sz else 0 for k in range(-2, 3)]
            inrange = sum(1 for x in ctx if lo_txt <= x < hi_txt)
            if inrange >= 3:
                va = lo + off
                print(f"  {va:#x} ({name}): {[hex(x) for x in ctx]}")
