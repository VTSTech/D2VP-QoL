#!/usr/bin/env python3
"""R2 phase 3: find the monster timer-method table (RegenAndPsnDecreaser hunt).

The fork's D2Game::GAME::Event::Dispatch::Monster::pTimerMethods is a
15-entry function-pointer table; index 3 = RegenAndPsnDecreaser. Retail
must have the same table. Scan .data/.rdata for runs of >=8 code VAs and
match tables containing the pinned lifecycle stubs.
"""
import struct
import pefile

EXE = "/tmp/gameexe/Game.exe"
pe = pefile.PE(EXE)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__

CODE_LO, CODE_HI = 0x401000, 0x66BA00
KNOWN = {
    0x5a6ff0: "MonDeathEvent/lifecycle (0 xrefs, ptr-called)",
    0x5a4ef0: "stub calling XP 0x57e990",
    0x5a6df0: "stub calling GetHitCalculation",
    0x5a6ab7: "stub calling KillMonster",
    0x57e990: "XP_OnMonsterKill",
    0x543a30: "QUESTS_MonsterDeathEvent",
}

tables = []
for s in pe.sections:
    name = s.Name.decode().rstrip("\x00")
    if name not in (".data", ".rdata"):
        continue
    raw, sz = s.PointerToRawData, s.SizeOfRawData
    va0 = base + s.VirtualAddress
    blob = data[raw:raw + sz]
    run = []
    run_start = None
    for off in range(0, len(blob) - 4, 4):
        v = struct.unpack_from("<I", blob, off)[0]
        if CODE_LO <= v < CODE_HI:
            if not run:
                run_start = off
            run.append((va0 + off, v))
        else:
            if len(run) >= 8:
                tables.append(run)
            run = []
    if len(run) >= 8:
        tables.append(run)

print(f"pointer runs >=8: {len(tables)}")
for run in tables:
    vas = [v for _, v in run]
    hit = [hex(v) for v in vas if v in KNOWN]
    interesting = len(hit) > 0 or (all(0x5a0000 <= v < 0x5b0000 or 0x530000 <= v < 0x560000 for v in vas))
    if interesting:
        print(f"\n-- table at {run[0][0]:#x} ({len(run)} ptrs) members-hit: {hit}")
        for tva, v in run:
            tag = KNOWN.get(v, "")
            print(f"   [{tva:#x}] -> {v:#x} {tag}")
