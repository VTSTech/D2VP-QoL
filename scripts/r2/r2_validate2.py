#!/usr/bin/env python3
"""R2 phase 2: instruction-level claim checks + poison-killer hunt.

Requires r2_validate.py's structures; re-derives them via import of the
same PE load logic (self-contained here).
"""
import sys
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

EXE = "/tmp/gameexe/Game.exe"
pe = pefile.PE(EXE)
base = pe.OPTIONAL_HEADER.ImageBase
secs = []
for s in pe.sections:
    va = base + s.VirtualAddress
    secs.append((va, va + s.Misc_VirtualSize, s.Name.decode().rstrip("\x00"),
                 s.PointerToRawData, s.SizeOfRawData, s.Characteristics))
data = pe.__data__

def va2off(va):
    for lo, hi, name, raw, sz, ch in secs:
        if lo <= va < hi and va - lo < sz:
            return raw + (va - lo)
    return None

def off2va(off):
    for lo, hi, name, raw, sz, ch in secs:
        if raw <= off < raw + sz:
            return lo + (off - raw)
    return None

md = Cs(CS_ARCH_X86, CS_MODE_32)
insns, calls, jumps, xrefs = {}, {}, {}, {}
for lo, hi, name, raw, sz, ch in secs:
    if not (ch & 0x20000000):
        continue
    code = data[raw:raw + sz]
    pos = 0
    while pos < len(code):
        last_end = 0
        for i in md.disasm(code[pos:pos + 65536], lo + pos):
            insns[i.address] = (i.mnemonic, i.op_str, i.size)
            if i.mnemonic in ("call", "jmp") and i.op_str.startswith("0x"):
                t = int(i.op_str, 16)
                (calls if i.mnemonic == "call" else jumps)[i.address] = t
                xrefs.setdefault(t, []).append(i.address)
            last_end = i.address + i.size - (lo + pos)
        pos += last_end if last_end else 1

# ---- function bounds via call-target entries ----
entries = sorted(set(t for t in xrefs if t in insns))
def func_bounds(f):
    import bisect
    i = bisect.bisect_right(entries, f)
    end = entries[i] if i < len(entries) else 0x66baa1
    return f, end

def containing_func(va):
    import bisect
    i = bisect.bisect_right(entries, va) - 1
    if i < 0:
        return None
    f = entries[i]
    f2, end = func_bounds(f)
    return (f, end) if f <= va < end else None

def win(f, end=0):
    """call targets within [f, end)"""
    if not end:
        f, end = func_bounds(f)
    return sorted(set(t for a, t in calls.items() if f <= a < end and t != a))

def at(va):
    return insns.get(va)

def near_calls(lo, hi, target):
    """call sites to target within [lo,hi)"""
    return sorted(a for a, t in calls.items() if lo <= a < hi and t == target)

def near_push(lo, hi, imm):
    return sorted(a for a, (m, o, s) in insns.items() if lo <= a < hi and m == "push" and o == imm)

def check(label, ok, detail=""):
    print(f"{'PASS' if ok else 'FAIL'}  {label}  {detail}")

print("== instruction-level claims from ColdFrozen_DeathPath_r2_Verified_R170.md ==")
# SetUninterruptableState pushes 0x36 early
push36 = near_push(0x5544b0, 0x554520, "0x36")
check("SetUninterruptableState pushes 0x36 (STATE 54) in first 0x70 bytes",
      len(push36) > 0, f"at {[hex(a) for a in push36][:3]}")
# GetKillerUnit called at 0x554510
check("GetKillerUnit call site 0x554510 (inside SetUninterruptableState)",
      calls.get(0x554510) == 0x553010, f"calls[0x554510]={calls.get(0x554510):#x}" if 0x554510 in calls else "no call at 0x554510")
# GetHitCalculation / KillMonster call sites inside SetUninterruptableState
f, e = func_bounds(0x5544b0)
tg = win(f, e)
check("SetUninterruptableState calls GetHitCalculation 0x57cee0", 0x57cee0 in tg)
check("SetUninterruptableState calls KillMonster 0x57ccb0", 0x57ccb0 in tg)
kill_sites = near_calls(f, e, 0x57ccb0)
ghc_sites = near_calls(f, e, 0x57cee0)
check("... KillMonster site listed as 0x554548 in doc", 0x554548 in kill_sites,
      f"actual {[hex(a) for a in kill_sites]}")
check("... GetHitCalculation site 0x554539 in doc", 0x554539 in ghc_sites,
      f"actual {[hex(a) for a in ghc_sites]}")

# GetHitCalculation deferral: uninterruptable check then death_delay set at 0x57cf7b
m, o, s = at(0x57cf7b)
check("0x57cf7b is push 1", m == "push" and o == "1", f"{m} {o}")
setstate_sites = near_calls(0x57cf7b, 0x57cf90, 0x639db0)
check("SetState 0x639db0 called right after 0x57cf7b (push1;push0x5c;call)",
      len(setstate_sites) == 1, f"sites {[hex(a) for a in setstate_sites]}")
getu_sites = near_calls(0x57cf40, 0x57cf7b, 0x639df0)
check("GetUnitState 0x639df0 called shortly before 0x57cf7b (uninterruptable check)",
      len(getu_sites) >= 1, f"sites {[hex(a) for a in getu_sites]}")
m, o, s = at(0x57d00f)
check("0x57d00f calls KillMonster 0x57ccb0", calls.get(0x57d00f) == 0x57ccb0, f"{m} {o}")

# ApplyCold: push 0xb at 0x57af93; expire fn set at 0x57b08c
m, o, s = at(0x57af93)
check("0x57af93 push 0xb (STATE_cold=11)", m == "push" and o == "0xb", f"{m} {o}")
sef_sites = near_calls(0x57b060, 0x57b0a0, 0x625ce0)
check("ApplyCold sets expire fn via 0x625ce0 near 0x57b08c", len(sef_sites) >= 1,
      f"sites {[hex(a) for a in sef_sites]}")
if sef_sites:
    a0 = sef_sites[0]
    args = near_push(a0 - 16, a0, "0x57ad80")
    check("expire fn arg 0x57ad80 pushed before call", len(args) >= 1,
          f"at {[hex(x) for x in args]}")

# ApplyFreeze: GetUnitState after push 0x36; fallbacks; expire 0x57b170
m, o, s = at(0x57b25e)
check("0x57b25e calls GetUnitState 0x639df0", calls.get(0x57b25e) == 0x639df0, f"{m} {o}")
for a in (0x57b2af, 0x57b3fc, 0x57b40e):
    ok = calls.get(a) == 0x57af80
    check(f"{a:#x} calls ApplyCold 0x57af80", ok,
          f"{insns[a][0]} {insns[a][1]}" if a in insns else "missing")
m, o, s = at(0x57b36f)
check("0x57b36f calls SetExpireFunction", calls.get(0x57b36f) == 0x625ce0, f"{m} {o}")
args = near_push(0x57b35f, 0x57b36f, "0x57b170")
check("freeze expire fn 0x57b170 pushed before 0x57b36f", len(args) >= 1,
      f"at {[hex(x) for x in args]}")

# OnColdExpire: UpdateAnimSpeed push 0x152 + source string
found = None
for a in near_calls(0x57ad80, 0x57af80, 0x623f50):
    if near_push(a - 16, a, "0x152"):
        found = a
check("OnColdExpire calls UpdateAnimSpeed 0x623f50 with push 0x152", found is not None,
      f"site {found:#x}" if found else "")

print("\n== poison killer hunt: all callers of GetHitCalculation 0x57cee0 ==")
for a in sorted(set(xrefs.get(0x57cee0, []))):
    cf = containing_func(a)
    print(f"  call site {a:#x} in func {cf[0]:#x}..{cf[1]:#x}" if cf else f"  call site {a:#x} (no func)")

print("\n== all callers of KillMonster 0x57ccb0 ==")
for a in sorted(set(xrefs.get(0x57ccb0, []))):
    cf = containing_func(a)
    print(f"  call site {a:#x} in func {cf[0]:#x}..{cf[1]:#x}" if cf else f"  call site {a:#x} (no func)")

print("\n== candidate 0x5a6xxx decreaser region ==")
f, e = func_bounds(0x5a6e90) if 0x5a6e90 in insns else func_bounds(0x5a6adb)
print(f"  func bounds guess around 0x5a6e90: {f:#x}..{e:#x}")
tg = win(f, e)
for name, t in (("GetHitCalculation", 0x57cee0), ("KillMonster", 0x57ccb0),
                ("SetUninterruptableState", 0x5544b0), ("CheckForModeZeroDeath", 0x5541b0),
                ("SetState", 0x639db0), ("GetUnitState", 0x639df0),
                ("STATES_IsStateActive", 0x63a4a0), ("UnitQueueEvent", 0x5417d0),
                ("FreeCombatAndChangeAnimMode", 0x553570)):
    print(f"  calls {name} {t:#x}: {'YES' if t in tg else 'no'}")
sites_ghc = [a for a, t in calls.items() if f <= a < e and t == 0x57cee0]
sites_km = [a for a, t in calls.items() if f <= a < e and t == 0x57ccb0]
print(f"  GHC sites {[hex(x) for x in sites_ghc]}  KM sites {[hex(x) for x in sites_km]}")
who = set(xrefs.get(f, []))
print(f"  entry {f:#x} called from: {[hex(x) for x in sorted(who)]}")

print("\n== XP / quest-death / event-runner candidates ==")
for label, va in (("0x57e990 XP", 0x57e990), ("0x543a30 quest death", 0x543a30),
                  ("0x5a6ff0 MonDeathEvent", 0x5a6ff0), ("0x5414d0 GameRunUnitEvents", 0x5414d0),
                  ("0x543e10 QUESTS_TimersRun", 0x543e10)):
    cf = containing_func(xrefs.get(va, [va])[0]) if xrefs.get(va) else None
    print(f"  {label}: xrefs {[hex(x) for x in xrefs.get(va, [])]}  (caller func: {cf[0]:#x}..{cf[1]:#x})" if cf else f"  {label}: xrefs {[hex(x) for x in xrefs.get(va, [])]}")
# does KillMonster call quest death 0x543a30 and XP 0x57e990?
kf, ke = func_bounds(0x57ccb0)
kt = win(kf, ke)
print(f"  KillMonster bounds {kf:#x}..{ke:#x}")
print(f"  KillMonster calls quest-death 0x543a30: {0x543a30 in kt}")
print(f"  KillMonster calls XP 0x57e990: {0x57e990 in kt}")
print(f"  KillMonster calls 0x543e10: {0x543e10 in kt}; 0x5414d0: {0x5414d0 in kt}")
qf, qe = func_bounds(0x543a30)
print(f"  quest-death fn bounds {qf:#x}..{qe:#x}  calls QuestTimerRun 0x543e10: {0x543e10 in win(qf, qe)}")
