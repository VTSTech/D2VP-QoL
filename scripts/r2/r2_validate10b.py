#!/usr/bin/env python3
"""R2 phase 10b: disassemble BNCS auth / MCP / CharSel key functions and
extract packet opcode constants (push imm8/imm32 near send/recv calls)."""
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

TARGETS = [
    (0x51cdbb, "BNCS connect status machine (Connected/Checking version)"),
    (0x51d07b, "caller of Connecting to Battle.net (BattleConnectFastest?)"),
    (0x520c20, "AuthCheck response handler"),
    (0x521664, "AuthAccountLogon + Proof handler"),
    (0x51fcb2, "ExtraWork archive auth (version check)"),
    (0x435e10, "MCP charcreate error handler"),
    (0x449e70, "McpConnect.cpp assert fn"),
    (0x441c95, "restore MCP connection"),
    (0x437ab0, "CharSel.cpp fn"),
]
for f, label in TARGETS:
    print(f"\n===== {label} {f:#x}..{fend(f):#x} =====")
    off = sec_ptr(f)
    n = 0
    for i in md.disasm(data[off:off + 0x400], f):
        ann = ""
        # annotate call targets and string pushes
        if i.mnemonic == "call" and i.op_str.startswith("0x"):
            t = int(i.op_str, 16)
            ann = f" ; -> {t:#x}"
        if i.mnemonic == "push" and i.op_str.startswith("0x"):
            v = int(i.op_str, 16)
            s = cstr(v) if v > 0x401000 else None
            if s: ann = f" ; \"{s}\""
        print(f"  {i.address:#x}: {i.mnemonic} {i.op_str}{ann}")
        n += 1
        if n >= 55: break
