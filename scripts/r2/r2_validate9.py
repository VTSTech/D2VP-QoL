#!/usr/bin/env python3
"""R2 phase 9: diagnostic-reference verification.

1. Dump the retail per-mode AI dispatch table (fork DropHandlerArray,
   D2AiDispatchCallbacksStrc {pFn1,pFn2,pFn3,bBool}, 16B stride) at 0x6e2260
   (21 entries) and match against fork names.
2. Find the table consumer (raw refs to 0x6e2260) and verify stride/class-redirects.
3. Census: STATE_death_delay (0x5c) and STATE_uninterruptable (0x36) sites via
   GetUnitState 0x639df0 / STATES_IsStateActive 0x63a4a0, grouped per function.
4. Caller chain: per-frame dispatcher containing 0x52d8f2 -> up; timer dispatchers
   0x541060/0x5410f0 -> up; identify the game-loop function.
5. Life-sync: callers of 0x571a10 (packet 0xAB queue) and 0x623f50 (UpdateAnimSpeed).
"""
import struct, bisect
import pefile
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

def rd(va, n=4):
    off = sec_ptr(va)
    return struct.unpack_from("<I", data, off)[0] if off is not None else None

md = Cs(CS_ARCH_X86, CS_MODE_32)
insns, calls, xrefs = {}, {}, {}
for lo, name, raw, sz, ch in secs:
    if not (ch & 0x20000000):
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
    return entries[i] if i < len(entries) else f + 0x3000

def dis(f, n=30, maxb=260):
    off = sec_ptr(f)
    out = []
    for i in md.disasm(data[off:off + maxb], f):
        out.append(f"  {i.address:#x}: {i.mnemonic} {i.op_str}")
        if len(out) >= n:
            break
    return out

# ---- 1. per-mode AI dispatch table ----
FORK = {
    0: ("MONSTER_OnDeathDispatch", "MONMODE_ProcessBlockAnimation", "MONMODE_HandleSpecialEndDeath", 1),
    1: ("PlanExecutionOfMonster", "thunk_ExecuteTimerTrigger", None, 0),
    2: ("MONMODE_StartWalking", "MONMODE_ProcessWalkWithTrails", None, 1),
    3: ("MONMODE_StartOtherAttack", "thunk_ExecuteTimerTrigger", "MONSTER_EndOfFrameAiDispatch", 1),
    4: ("DropHandlerGeneric", "MONMODE_ProcessSkillCast", "MONSTER_EndOfFrameAiDispatch", 1),
    5: ("DropHandlerGeneric", "MONMODE_ProcessSkillCast", "MONSTER_EndOfFrameAiDispatch", 1),
    6: ("thunk_FreeCombatAndChangeAnimMode_B", "thunk_ExecuteTimerTrigger", "MONSTER_EndOfFrameAiDispatch", 1),
    7: ("DropHandlerGeneric", "MONMODE_ProcessSkillCast", "MONSTER_EndOfFrameAiDispatch", 1),
    8: ("DropHandlerGeneric", "MONMODE_ProcessSkillCast", "MONSTER_EndOfFrameAiDispatch", 1),
    9: ("DropHandlerGeneric", "MONMODE_ProcessSkillCast", "MONSTER_EndOfFrameAiDispatch", 1),
}
print("== retail per-mode AI dispatch table @ 0x6e2260 (21 x 16B) ==")
retail_names = {}
for k in range(21):
    va = 0x6e2260 + 16 * k
    p1, p2, p3, b = rd(va), rd(va + 4), rd(va + 8), rd(va + 12)
    fk = FORK.get(k)
    code = all(0x401000 <= p < 0x6cb5a1 for p in (p1, p2) if p) if p1 else False
    print(f"[{k:2}] {va:#x}: pFn1={p1:#x} pFn2={p2 or 0:#x} pFn3={(p3 or 0):#x} bBool={b:#x}"
          + (f"   fork: {fk}" if fk else "   fork: (boss override)")
          if code or p1 else f"[{k:2}] {va:#x}: pFn1={p1 or 0:#x} (end/other data)")
    if code:
        retail_names.setdefault(p1, f"pFn1[{k}]")
        retail_names.setdefault(p2, f"pFn2[{k}]")
        if p3:
            retail_names.setdefault(p3, f"pFn3[{k}]")

# ---- 2. table consumer ----
def rawrefs(va):
    pat = struct.pack("<I", va)
    out, s = [], 0
    while True:
        i = data.find(pat, s)
        if i < 0:
            break
        for lo, name, raw, sz, ch in secs:
            if raw <= i < raw + sz:
                out.append((lo + i - raw, name))
        s = i + 1
    return out

print("\n== raw refs to table base 0x6e2260 ==")
for hva, sec in rawrefs(0x6e2260):
    cf = containing(hva) if sec in (".text",) else None
    print(f"  {hva:#x} ({sec})" + (f" inside func {cf:#x}..{fend(cf):#x}" if cf else ""))
    if cf:
        for line in dis(cf, 40):
            print(line)

# ---- 3. census of death_delay / uninterruptable state sites ----
print("\n== census: push 0x5c (death_delay) / push 0x36 (uninterruptable) before GetUnitState(0x639df0) or IsStateActive(0x63a4a0) ==")
sites = {}
for a, t in sorted(calls.items()):
    if t not in (0x639df0, 0x63a4a0):
        continue
    f = containing(a)
    if f is None:
        continue
    # scan back 12 bytes for push imm
    for back in range(1, 13):
        m = insns.get(a - back)
        if m and m[0] == "push" and m[1] in ("0x5c", "0x36"):
            sites.setdefault((f, m[1], t), []).append((a - back, a))
for (f, state, t), lst in sorted(sites.items()):
    tag = "death_delay WRITE->SetState?" if state == "0x5c" and t == 0x639db0 else (
          "death_delay READ" if state == "0x5c" else "uninterruptable READ")
    print(f"  func {f:#x}: {state} via {t:#x} ({tag}) sites {[hex(p) for p, c in lst]}")

print("\n== census: push 0x5c / 0x36 before SetState 0x639db0 ==")
for a, t in sorted(calls.items()):
    if t != 0x639db0:
        continue
    f = containing(a)
    for back in range(1, 13):
        m = insns.get(a - back)
        if m and m[0] == "push" and m[1] in ("0x5c", "0x36"):
            print(f"  func {f:#x}: SetState arg {m[1]} call {a:#x} (push @{a-back:#x})")
            break

# ---- 4. caller chain to the game loop ----
print("\n== timer dispatchers & per-frame chain ==")
def show_callers(f, label):
    xs = sorted(set(xrefs.get(f, [])))
    cfs = sorted(set(containing(x) for x in xs if containing(x)))
    print(f"  {label} {f:#x}: call xrefs {[hex(x) for x in xs]}  caller funcs {[hex(c) for c in cfs]}")
    return cfs

c1 = show_callers(0x541060, "timer-dispatch A")
c2 = show_callers(0x5410f0, "timer-dispatch B")
show_callers(0x5414d0, "GameRunUnitEvents")
show_callers(0x543e10, "QUESTS_TimersRun")
show_callers(0x5a7f80, "MONSTEREVENT_DefaultCallback")

f52 = containing(0x52d8f2)
print(f"\n  func containing 0x52d8f2 (calls GameRunUnitEvents+QUESTS_TimersRun): {f52:#x}..{fend(f52):#x}")
print("  first insns:")
for line in dis(f52, 20):
    print(line)
up = sorted(set(xrefs.get(f52, [])))
print(f"  callers of {f52:#x}: {[hex(x) for x in up]}")
for x in up[:6]:
    cf2 = containing(x)
    print(f"   call site {x:#x} in func {cf2:#x}..{fend(cf2):#x}")
    for line in dis(cf2, 14):
        print(line)

# ---- 5. life sync ----
print("\n== life-sync ==")
show_callers(0x571a10, "packet-0xAB queue (UnitQueuePacket0xAB?)")
show_callers(0x623f50, "UpdateAnimSpeed")
show_callers(0x457490, "KillMonster death-anim guard helper")
show_callers(0x5a5650, "MONSTER_GetHpPercent128?")
