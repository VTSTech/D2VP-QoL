#!/usr/bin/env python3
"""R2 phase 11: full annotated disassembly of Game.exe BNet-path functions
for the D2VP recreation reference (R173). Dumps each function completely to
/tmp/r2fn/<addr>.asm with string and call-target annotations."""
import pefile, struct, bisect, os
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

EXE = "/tmp/gameexe/Game.exe"
OUT = "/tmp/r2fn"
os.makedirs(OUT, exist_ok=True)
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

def cstr(va, maxlen=100):
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

# function dump list: (start, label)
FUNCS = [
    # stage 1: gateway/server selection
    (0x51ca90, "sFastestConnect"),
    (0x51d0a0, "BattleConnectFastest"),
    (0x51cdbb, "BNET_ConnectLoop_statusmachine"),
    (0x5182b0, "RealmsBin_Reader"),
    (0x51c0e0, "BnserverIni_Parser"),
    (0x518bc0, "GW_fn_a"),
    (0x51c240, "GW_fn_b"),
    (0x51c290, "GW_fn_c"),
    (0x518d40, "StatusLog_Printer"),
    (0x518c70, "SocketHelper"),
    (0x51a480, "DebugLog_Printer"),
    # stage 2: auth
    (0x51c5c0, "BNCS_PacketQueueSend"),
    (0x51a540, "Auth_AdvanceStep?"),
    (0x520c20, "AuthStatusText_Get"),
    (0x520c30, "AuthStatusText_Set"),
    (0x520c40, "Auth_cb_1"),
    (0x520c50, "Auth_cb_2"),
    (0x520c60, "Auth_cb_3"),
    (0x520c80, "Auth_cb_4"),
    (0x520cb0, "Auth_cb_chain1_a"),
    (0x520d50, "Auth_cb_5"),
    (0x520d60, "Auth_cb_6"),
    (0x520d70, "Auth_cb_LogonAck"),
    (0x520da0, "Auth_cb_7"),
    (0x520dc0, "Auth_StatusDispatcher"),
    (0x520e50, "Auth_chain1_b"),
    (0x520f60, "Auth_chain1_c"),
    (0x520fc0, "Auth_chain2_a"),
    (0x520fe0, "Auth_chain3_a"),
    (0x5210a0, "Auth_chain1_head"),
    (0x5210c0, "Auth_x1"),
    (0x5210d0, "Auth_x2"),
    (0x5210e0, "Auth_x3"),
    (0x5210f0, "Auth_x4"),
    (0x521110, "Auth_x5"),
    (0x521150, "Auth_x6"),
    (0x5211d0, "Auth_x7"),
    (0x5211e0, "Auth_x8"),
    (0x5211f0, "Auth_x9"),
    (0x521200, "Auth_chain2_b"),
    (0x521210, "Auth_x10"),
    (0x521220, "Auth_chain4_head"),
    (0x5212d0, "Auth_chain4_b"),
    (0x5215b0, "Auth_AccountLogonResult"),
    (0x5216b0, "Auth_AccountLogonProofHead"),
    (0x521760, "Auth_chain4_d"),
    (0x521800, "Auth_chain4_e"),
    (0x5218a0, "Auth_chain4_f"),
    (0x523ac0, "SHA1_Init"),
    (0x523af0, "SHA1_Transform"),
    (0x5204a0, "BNCS_AbortWithStatus"),
    (0x51fcb2, "ExtraWork_VerCheck_site"),
    # stage 3: MCP/D2CS + charsel
    (0x449e70, "D2CS_RecvDispatch"),
    (0x44aae0, "D2CS_SendWrapper"),
    (0x44a170, "D2CS_PayloadAppend"),
    (0x449b30, "MCP_IsConnected"),
    (0x449bf0, "MCP_fn_before_send"),
    (0x449c00, "D2CS_LastStatus"),
    (0x449d20, "MCP_fn_d"),
    (0x449df0, "MCP_fn_e_charlist_parse"),
    (0x437ab0, "CharSel_CacheNodeAdd"),
    (0x438ad0, "CharSel_CharRecordCreate"),
    (0x438ea0, "CharSel_CacheTeardown"),
    (0x435e10, "CharSel_CreateChar_Handler"),
    (0x43a460, "CharSel_CharlistBlockParse"),
    (0x43aab0, "CharSel_CharlistWalker"),
    (0x441c95, "MCP_ConnectionRestore_site"),
    # stage 4: game server connect
    (0x44c000, "GameSrv_StateEnter"),
    (0x44c060, "GameSrv_StateLeave"),
    (0x462d00, "GameSrv_Transition"),
    (0x463dd0, "GameSrv_QueryFn"),
    (0x468740, "GameSrv_CapEncode1"),
    (0x468730, "GameSrv_CapEncode2"),
    (0x43aab0 + 0, "dup-skip"),
]

import io
manifest = []
for start, label in FUNCS:
    if label == "dup-skip":
        continue
    end = fend(start)
    if end <= start:
        end = start + 0x200
    off = sec_ptr(start)
    if off is None:
        continue
    buf = io.StringIO()
    buf.write(f"; ==== {label} start={start:#x} end~{end:#x} (len {end-start:#x}) ====\n")
    for i in md.disasm(data[off:off + (end - start)], start):
        ann = ""
        if i.mnemonic in ("call", "jmp") and i.op_str.startswith("0x"):
            t = int(i.op_str, 16)
            ann = f"    ; -> {t:#x}"
        if i.mnemonic == "push" and i.op_str.startswith("0x"):
            v = int(i.op_str, 16)
            s = cstr(v) if v > 0x401000 else None
            if s: ann = f'    ; "{s[:60]}"'
        buf.write(f"{i.address:#x}: {i.mnemonic} {i.op_str}{ann}\n")
    with open(f"{OUT}/{start:06x}_{label}.asm", "w") as fh:
        fh.write(buf.getvalue())
    manifest.append((start, label, end - start))

with open(f"{OUT}/MANIFEST.txt", "w") as fh:
    for a, l, n in manifest:
        fh.write(f"{a:#x} {l} ({n:#x} bytes)\n")
print(f"dumped {len(manifest)} functions to {OUT}")
for a, l, n in manifest:
    print(f"  {a:#x} {l} ({n:#x})")
