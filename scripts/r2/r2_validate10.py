#!/usr/bin/env python3
"""R2 phase 10: Battle.net connect/auth/charselect/game-create mapping.

- Cluster strings around the module assert files (McpConnect.cpp, CharSel.cpp,
  ChatDlg.cpp, BNetGW.cpp) and the auth/message clusters.
- Find every .text reference (push imm32) to each string, resolve containing
  functions, and group functions per module.
"""
import pefile, re, struct, bisect
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
            off = raw + (va - lo)
            if off < raw + sz:
                return off
    return None

def cstr(va, maxlen=90):
    off = sec_ptr(va)
    if off is None: return None
    end = data.find(b"\x00", off, off + maxlen)
    if end < 0: return None
    t = data[off:end].decode("latin1")
    return t if all(32 <= ord(c) < 127 for c in t) else None

md = Cs(CS_ARCH_X86, CS_MODE_32)
insns, calls, xrefs = {}, {}, {}
for lo, name, raw, sz, ch in secs:
    if name != ".text" or not (ch & 0x20000000):
        continue
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

def rawrefs(va):
    pat = struct.pack("<I", va)
    out, s = [], 0
    while True:
        i = data.find(pat, s)
        if i < 0: break
        for lo, name, raw, sz, ch in secs:
            if name == ".text" and raw <= i < raw + sz:
                out.append(lo + i - raw)
        s = i + 1
    return out

# ---- cluster strings around module anchors ----
def cluster(anchor, span=0x400, label=""):
    print(f"\n== string cluster @{anchor:#x} {label} ==")
    for va in range(anchor - span, anchor + span):
        t = cstr(va)
        if t and len(t) >= 5:
            print(f"  {va:#x}: {t}")

cluster(0x6d5af0, 0x380, "McpConnect.cpp region")
cluster(0x6d4dbc, 0x180, "CharSel.cpp region")
cluster(0x6dec6c, 0x200, "BNetGW.cpp region")

# ---- module membership: functions referencing each .cpp assert string ----
print("\n== functions referencing module assert strings ==")
for anchor, label in [(0x6d5af0, "McpConnect.cpp"), (0x6d4dbc, "CharSel.cpp"),
                      (0x6d5460, "ChatDlg.cpp"), (0x6dec6c, "BNetGW.cpp")]:
    funcs = []
    for site in rawrefs(anchor):
        f = containing(site)
        if f and (f, site) not in funcs:
            funcs.append((f, site))
    print(f"  {label}: {len(funcs)} ref sites")
    for f, site in sorted(set(funcs)):
        print(f"    func {f:#x}..{fend(f):#x} (site {site:#x})")

# ---- key message strings -> functions ----
print("\n== key message anchors ==")
KEYS = [
    (0x6d412c, "Can't connect to Battle.Net. Try again later."),
    (0x6d4d58, "Error from MCP trying to create '%s' (err=%d)"),
    (0x6d55bc, "Could not restore MCP connection."),
    (0x6df078, "Connected to Battle.net"),
    (0x6df1f4, "Connecting to Battle.net..."),
    (0x6df078, "dup"),
    (0x6df70c, "Failed ExtraWork archive authentication"),
    (0x6df964, "INVALID: AuthCheck"),
    (0x6dfb60, "INVALID: AuthAccountLogon"),
    (0x6dfc28, "INVALID: AuthAccountLogonProof"),
    (0x6df090, "Checking for the latest version"),
    (0x6decf0, "Data\\Global\\Realms.bin"),
    (0x6df05c, "bnserver-D2DV.ini"),
    (0x6df0d4, "Connected to server %s"),
]
seen = set()
for va, label in KEYS:
    if va in seen: continue
    seen.add(va)
    sites = rawrefs(va)
    fs = sorted(set(containing(s) for s in sites if containing(s)))
    print(f"  [{label[:40]!r}] {va:#x}: sites {[hex(s) for s in sites]} funcs {[hex(f) for f in fs]}")
