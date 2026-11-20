#!/usr/bin/env python3
"""R2 validation harness for Game.exe 1.14d retail claims.

Loads the PE, linear-disassembles .text with capstone, and checks the
address claims made in the D2VP-QoL R2 docs:
  * is the claimed VA a real instruction start?
  * is it a plausible function entry (padding before, prologue)?
  * how many call-site xrefs point at it?
  * what does it call (first N direct call targets)?
  * targeted byte-pattern probes (state pushes, PDB path, source strings).

Usage: python3 r2_validate.py [/tmp/gameexe/Game.exe]
"""
import sys, re, struct
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

EXE = sys.argv[1] if len(sys.argv) > 1 else "/tmp/gameexe/Game.exe"
BASE_EXPECT = 0x400000

pe = pefile.PE(EXE)
base = pe.OPTIONAL_HEADER.ImageBase
print(f"image base: {base:#x}  size: {pe.OPTIONAL_HEADER.SizeOfImage:#x}")

# VA <-> offset helpers
secs = []
for s in pe.sections:
    va = base + s.VirtualAddress
    secs.append((va, va + s.Misc_VirtualSize, s.Name.decode().rstrip("\x00"),
                 s.PointerToRawData, s.SizeOfRawData))
    print(f"  sec {s.Name.decode().rstrip(chr(0)):8} va {va:#x} vsize {s.Misc_VirtualSize:#x} raw {s.PointerToRawData:#x}")

def va2off(va):
    for lo, hi, name, raw, sz in secs:
        if lo <= va < hi:
            d = va - lo
            if d < sz:
                return raw + d, name
    return None, None

def off2va(off):
    for lo, hi, name, raw, sz in secs:
        if raw <= off < raw + sz:
            return lo + (off - raw), name
    return None

data = pe.__data__

# ---- linear disassembly of all executable sections ----
md = Cs(CS_ARCH_X86, CS_MODE_32)
md.detail = False
insns = {}          # va -> (mnemonic, opstr, size)
calls = {}          # va -> target
xrefs = {}          # target -> [caller va,...]
jumps = {}
for lo, hi, name, raw, sz in secs:
    ch = pe.sections[[x[2] for x in secs].index(name)].Characteristics
    if not (ch & 0x20000000):  # IMAGE_SCN_MEM_EXECUTE
        continue
    code = data[raw:raw + sz]
    pos = 0
    CHUNK = 65536
    while pos < len(code):
        chunk = code[pos:pos + CHUNK]
        last_end = 0
        for i in md.disasm(chunk, lo + pos):
            insns[i.address] = (i.mnemonic, i.op_str, i.size)
            if i.mnemonic in ("call", "jmp") and i.op_str.startswith("0x"):
                t = int(i.op_str, 16)
                (calls if i.mnemonic == "call" else jumps)[i.address] = t
                xrefs.setdefault(t, []).append(i.address)
            last_end = i.address + i.size - (lo + pos)
        pos += last_end if last_end else 1

print(f"instructions: {len(insns)}  direct calls: {len(calls)}")

def is_func_entry(va):
    """Plausible entry: instruction start, preceded by padding/ret/jmp, prologue ok."""
    if va not in insns:
        return False, "not an instruction start"
    mn, op, sz = insns[va]
    pro = "ok" if mn in ("push", "mov", "sub", "jmp") else "?"
    # look back up to 16 bytes for padding or terminator
    prev_ok = False
    for back in range(1, 16):
        p = va - back
        if p in insns:
            pm = insns[p][0]
            prev_ok = pm in ("ret", "retn", "jmp", "int3", "retf", "leave", "call")
            if not prev_ok and back <= 3:
                prev_ok = False
            break
    else:
        prev_ok = True  # start of section
    return (pro == "ok" and prev_ok), f"prologue={mn} {op} | prev-terminator={prev_ok}"

def report(name, va, expect_calls=(), max_show=8):
    ok, why = is_func_entry(va)
    xr = xrefs.get(va, [])
    tgts = []
    for c, t in sorted(calls.items()):
        if t == va:
            pass
    out_calls = [t for c, t in calls.items() if va <= c < va + 0x3000 and t != va]
    # limit: collect calls within this function window only crudely
    print(f"\n== {name} @ {va:#x}")
    print(f"   entry-plausible: {ok} ({why})")
    print(f"   call xrefs in: {len(xr)}  e.g. {[hex(x) for x in xr[:max_show]]}")
    uniq = sorted(set(out_calls))
    print(f"   direct call targets (first 40 in window): {[hex(t) for t in uniq[:40]]}")
    for e in expect_calls:
        print(f"   expect-call {e:#x}: {'YES' if e in uniq else 'NO'}")
    return uniq

# ---- targeted byte probes ----
def byte_probe(label, pattern):
    hits = []
    pat = bytes.fromhex(pattern)
    start = 0
    while True:
        i = data.find(pat, start)
        if i < 0:
            break
        va = off2va(i)
        if va:
            hits.append(va[0] if isinstance(va, tuple) else va)
        start = i + 1
    print(f"\nbyte probe [{label}] {pattern}: {len(hits)} hits: {[hex(h) for h in hits[:10]]}")
    return hits

# ---- key probes ----
byte_probe("death_delay set (push1;push0x5c)", "6a016a5c")
byte_probe("uninterruptable push 0x36", "6a36")

# ---- claim checks: verified set (regression) ----
report("SetUninterruptableState", 0x5544b0,
       expect_calls=(0x553010, 0x57cee0, 0x57ccb0))
report("SERVER_KillMonster", 0x57ccb0)
report("SERVER_GetHitCalculation", 0x57cee0, expect_calls=(0x57ccb0,))
report("CheckForModeZeroDeath", 0x5541b0)
report("GetKillerUnit", 0x553010)
report("COMBAT_ApplyCold", 0x57af80, expect_calls=(0x625ce0,))
report("COMBAT_ApplyFreeze", 0x57b230, expect_calls=(0x57af80, 0x625ce0, 0x5417d0))
report("DAMAGE_OnColdExpire", 0x57ad80, expect_calls=(0x5541b0, 0x639db0))
report("Freeze expire cb", 0x57b170, expect_calls=(0x5541b0, 0x553570, 0x540e60, 0x5417d0))
report("SetState", 0x639db0)
report("GetUnitState", 0x639df0)
report("STATES_IsStateActive", 0x63a4a0)
report("STATLIST_SetExpireFunction", 0x625ce0)
report("UnitQueueEvent", 0x5417d0)
report("EVENT_RemoveUnitTimersByType", 0x540e60)
report("FreeCombatAndChangeAnimMode", 0x553570)

# ---- unverified candidates from the older docs ----
report("CANDIDATE XP_OnMonsterKill", 0x57e990)
report("CANDIDATE QUESTS_MonsterDeathEvent", 0x543a30)
report("CANDIDATE MonDeathEvent/lifecycle", 0x5a6ff0)
report("CANDIDATE GameRunUnitEvents", 0x5414d0)
report("CANDIDATE QUESTS_TimersRun", 0x543e10)
report("WRONG? RegenAndPsnDecreaser", 0x58abc0)
