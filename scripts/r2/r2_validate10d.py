#!/usr/bin/env python3
"""R2 phase 10d: find BNCS/D2CS dispatch loops and jump tables."""
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
insns, calls, xrefs, jumptables = {}, {}, {}, {}
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
            # indirect jump tables: jmp/call dword [reg*4 + imm32]
            if i.mnemonic in ("jmp", "call") and ("*4 + 0x" in i.op_str or "*8 + 0x" in i.op_str):
                try:
                    tbl = int(i.op_str.rsplit("+ ", 1)[1], 16)
                    jumptables.setdefault(tbl, []).append(i.address)
                except Exception:
                    pass
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

# 1. who calls the D2CS dispatcher 0x449e70
for va, lbl in [(0x449e70, "D2CS recv dispatcher"), (0x449c00, "D2CS status getter"),
                (0x449b30, "MCP connected check"), (0x449df0, "charlist parse fn"),
                (0x449d20, "MCP send?"), (0x51ca90, "sFastestConnect")]:
    xs = sorted(set(xrefs.get(va, [])))
    print(f"== {lbl} {va:#x}: callers {[hex(x) for x in xs[:10]]}")

# 2. find function containing 0x520d82 ("logon acknowledged") -> BNCS auth reply dispatcher
f = containing(0x520d82)
print(f"\ncontaining(0x520d82) = {f:#x}")
# scan back for prologue
off = sec_ptr(f)
back = data[off:off + 0x400]
# 3. jump tables with auth-region pointers
print("\n== jump tables whose target arrays contain auth/D2CS-region pointers ==")
for tbl, sites in sorted(jumptables.items()):
    toff = sec_ptr(tbl)
    if toff is None: continue
    vals = [struct.unpack_from("<I", data, toff + k*4)[0] for k in range(64)
            if toff + k*4 + 4 <= len(data)]
    auth_hits = [v for v in vals if 0x51a000 <= v < 0x524000]
    if len(auth_hits) >= 4:
        end = next((k for k, v in enumerate(vals) if not (0x400000 <= v < 0x6f0000 or v == 0)), 64)
        print(f"  tbl {tbl:#x} (jt sites {[hex(s) for s in sites]}): {end} entries, "
              f"auth-region hits {len(auth_hits)}: {[hex(v) for v in auth_hits[:8]]}")

# 4. dump the BNCS dispatcher around 0x520d82 (search real prologue backwards)
start = None
for probe in range(0x520cc0, 0x520d82, 4):
    pass
# find nearest entry >= 0x520cc0
idx = bisect.bisect_right(entries, 0x520d82) - 1
while idx > 0 and entries[idx] < 0x520cc0:
    idx -= 1
f = entries[idx]
dump(f, 60, "BNCS auth-reply dispatcher (contains 'logon acknowledged')")

# 5. BNCS/D2CS send helpers: look at functions called by BattleConnect flow that reference 0x1f40/0xff packet sig
dump(0x5204a0, 30, "BNCS disconnect/status-set (called with 0x12)")
