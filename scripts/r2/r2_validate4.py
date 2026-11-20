#!/usr/bin/env python3
"""R2 phase 4: find all raw references to the lifecycle stubs + identify
the monster timer dispatch (RegenAndPsnDecreaser hunt, cont.).
"""
import struct
import pefile

EXE = "/tmp/gameexe/Game.exe"
pe = pefile.PE(EXE)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
secs = []
for s in pe.sections:
    secs.append((base + s.VirtualAddress, s.Name.decode().rstrip("\x00"),
                 s.PointerToRawData, s.SizeOfRawData))

def off2va(off):
    for lo, name, raw, sz in secs:
        if raw <= off < raw + sz:
            return lo + (off - raw), name
    return None, None

TARGETS = {
    0x5a6ff0: "MonDeathEvent/lifecycle stub",
    0x5a4ef0: "XP-wrapper stub (calls 0x57e990)",
    0x5a6df0: "GetHitCalculation-wrapper stub",
    0x5a6ab7: "KillMonster-wrapper stub",
    0x57e990: "XP_OnMonsterKill",
    0x543a30: "QUESTS_MonsterDeathEvent",
    0x5414d0: "GameRunUnitEvents",
    0x543e10: "QUESTS_TimersRun",
}

for va, label in TARGETS.items():
    pat = struct.pack("<I", va)
    hits = []
    start = 0
    while True:
        i = data.find(pat, start)
        if i < 0:
            break
        hva, sec = off2va(i)
        if hva:
            hits.append((hva, sec))
        start = i + 1
    print(f"\n{label} {va:#x}: {len(hits)} raw refs")
    for h, sec in hits[:20]:
        print(f"   at {h:#x} ({sec})")
