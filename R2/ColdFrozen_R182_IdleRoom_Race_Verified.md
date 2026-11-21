# Cold/Frozen Vanish — R182 Session: Full Death-Path Re-Verification & the Idle-Room Race

**Binary:** Game.exe 1.14d Retail (`/home/vtstech/workspace/D2-Game.exe-1.14d-Retail/Game.exe`),
PE32, binsz 3618792 (same binary as the R170 pass).
**Tool:** r2-portable 6.1.5 (`/home/vtstech/r2-portable/bin/radare2`, needs
`LD_LIBRARY_PATH=/home/vtstech/r2-portable/lib`). Persistent analysis project:
`P game114d` (saved via `aaa; Ps game114d` with `GIT_AUTHOR_*`/`GIT_COMMITTER_*` env set —
project save silently fails without a git identity). NOTE: `Ps <name>.r2` is rejected by
r2 6.1.5 ("Invalid project name") — no extension. `af @ <addr>; pdf` needed per function;
the project load does **not** restore function analysis.

**Method:** fresh per-function `pdf` disassembly of every retail function on the death
path, byte-diffed against the fork source. This extends
[ColdFrozen_DeathPath_r2_Verified_R170.md](ColdFrozen_DeathPath_r2_Verified_R170.md).

---

## 1. Re-verified retail-faithful (fork == retail, byte level)

| Retail VA | Function | Fork site | Verdict |
|---|---|---|---|
| `0x5a6920` | `RegenAndPsnDecreaser` (full body dumped this session) | `Monster/MonsterMode.cpp` `RegenAndPsnDecreaser` | ✅ HP/regen math, mode 0/0xC death gate, poison-owner lookup (`GetStatsByState` states 2 and 0x3E), `dwLastHitClass=0`, kill tail `KillMonster(pGame,pUnit,pAttacker,1)` → `DispatchEventsByType(10, pUnit, pAttacker, 0)` → `DispatchEventsByType(9, pAttacker, pUnit, 0)` (fcn `0x5c0c30`), and the deferral `GetUnitState(0x36)` → `SetState(0x5c,1)` at `0x5a6ab7..0x5a6acd` — all identical. **Retail has NO cold/frozen check anywhere in this function.** The fork's R166 `bIsColdOrFrozen` branch was byte-equivalent duplication → removed in R182. |
| `0x57ccb0` | `SERVER_KillMonster` | `UNIT/SUnitDmg.cpp:2234` | ✅ Entry guards: mode `[unit+0x10]` == 0 / 0xC reject; **`fcn.00457490(classId, 0xf)` == fork `TXT_MonStats_GetFlagsByBitOffset(nClassId, killable)` — `killable = 15` (d2_enums.h:17879), so the fork's "killable flag" IS retail's third guard**, not a divergence. Pet removal `0x58f0d0`/`0x5751a0`, XP/arena `0x5a4ef0`/`0x53f720`, `SetCurrentSkill` `0x5a7e60`, lifecycle `0x5a7c20`, quest event `0x543a30` (gated on flags bit 31), objCol `0x4638a0(classId, 0x12)` + 0x1b0/0x1b1 companion cleanup `0x57cc30` — all match. |
| `0x5a7c20` | `MONSTERAI_ExecuteLifecycleTransition` (54 xrefs) | `MonsterMode.cpp:1696` | ✅ `0x5a7c41..0x5a7c73`: `GetUnitState(0x36)` → halt(line `0x753`) → `exit(-1)` — terminal in retail too. The "dead" recovery block (`0x5a7c78`: re-test 0x36 → `SetUninterruptableState(0)` `0x5a7c88` → `CheckForModeZeroDeath` `0x5a7c8f` → return 1) is unreachable-after-`exit` in **retail as well**; the fork mirrors it exactly. Not a fork bug. |
| `0x5544b0` | `SetUninterruptableState` | `UNIT/SUnit.cpp:1361` | ✅ set/clear `0x36` first; null-halt `0xa29`; clear path: `GetUnitState(0x5c)` → clear → `GetKillerUnit` (`0x553010`) → `GetHitCalculation(0x570)` memset-0x70 + result-flag 2, else `KillMonster(g,u,null,1)`. **Tail `0x55454d..0x55455a`: monster → `EVENT_RemoveUnitTimersByType(pGame, pUnit, 2, 0)` (AITHINK removal) — retail removes AITHINK timers on BOTH set and clear; fork matches.** |
| `0x5416b0` | retail event **queue** core (wrapped by `UnitQueueEvent` `0x5417d0` / `0x541800`) | `GAME/Event.cpp` `SetEventInternal` | ✅ **Retail queue-guard found at `0x5416f5..0x541712`: `pUnit && [pUnit]==1 (UNIT_MONSTER) && type==2 (AITHINK)` → `GetUnitState(0x36)` set → `SetUninterruptableState(pUnit, 0)`.** The fork already has this verbatim at `Event.cpp:684-690` (`SetEventInternal`). This is a major retail completion mechanism the R170 doc had not pinned: **any AITHINK queued for an uninterruptable monster clears the state and completes a pending deferred kill at queue time.** |

Also confirmed from the R170 session and not contradicted: expiry callbacks
(`0x57ad80` cold / `0x57b170` freeze) have **no** death-completion in retail — the fork's
R166/R170 `SUNITDMG_CompleteDeathDelayIfPending` hooks are fork-only additions using
retail's own completion primitive (`0x5544b0`).

## 2. The residual vanish was a RACE, not another completion bug

The fork's completion chain is a superset of retail's (queue-guard ✓, expiry hooks ✓,
cast-end clears ✓, R162 unconditional dispatcher ✓). What remains is timing:

1. Lethal hit on a mid-cast monster → `STATE_death_delay`, HP 0 (deferral, faithful).
2. Frozen ⇒ cast-end clear unreachable; completion waits for freeze/cold expiry
   (2–4 s of freeze duration; re-application pushes it further).
3. The killer walks away ⇒ the room idles out fast: `DRLGROOM_GetIdleTicks`
   (`Dungeon.cpp:1146`) increments **every frame** with no clients, and
   `CleanupIdleRooms` (`Server.cpp`, every 12 frames) untiles at `idleTicks > 10`
   with no adjacent clients ⇒ **the room is destroyed inside the freeze window**.
4. `SUNITINACTIVE_SaveOrFreeUnit` → `UNITS_ProcessDeadOrInactiveMonster`
   (`SUnitInactive.cpp:1459`) has **no death_delay/HP awareness**: `bIsDeadBool` is
   false (alive mode), most monsters are `byRestore == 0` ⇒ **`FreeServerUnit`
   with no 0x0A removal packet — silent vanish, no loot** — or, for saved monsters,
   a **0 HP Neutral-mode zombie** on room re-init.
   Loot/XP/quest credit/`SaveKilledUnitGUID`/spawn-count decrement: none run.

This is exactly the field report: a frozen monster visibly at 0 HP that is gone
(no corpse, no loot) once the player returns.

## 3. Fix shipped in R182

`Source/D2Game/GAME/Server.cpp` `CleanupIdleRooms`: before the save/free unit walk of a
room being untiled, a **pre-pass** completes pending deferred deaths — for each
`UNIT_MONSTER` in the room, `UNIT::SUnitDmg::SUNITDMG_CompleteDeathDelayIfPending(pUnit)`
(the R166/R170 retail-derived completion primitive, now declared in
`Source/D2Game/UNIT/SUnitDmg.h`). The kill runs through the normal death flow (loot/XP/
quest events), the unit is in Death mode when the walk reaches it, and the dropped items
are added to the room list **before** the walk saves them with the room.

**Why a pre-pass instead of hooking `UNITS_ProcessDeadOrInactiveMonster` itself:**
the save/free walk is the room's LAST pass — `FreeRoom` (`Dungeon.cpp:1194`)
force-unlinks (flag `0x800000`) every unit still in the list when the room is freed,
and does **not** save items. A kill executed *inside* the walk drops items at the
list head **after** the walk has cached `pUnitNext` ⇒ the fresh loot would be
force-unlinked unsaved. The pre-pass runs before the walk, so dropped items are
present in (and saved by) the walk. Both walks cache the next pointer before each
call, so head-insertions are safe.

Also in R182: the redundant R166 `bIsColdOrFrozen` branch was removed from
`RegenAndPsnDecreaser` (`MonsterMode.cpp`) — r2-verified byte-identical to the general
path; retail has no such branch.

## 4. Open items (unchanged / next)

- **H3 (poison deferral killer):** the deferral at `0x5a6ab7` completes via
  `GetKillerUnit`; when unresolved, `KillMonster(pGame, pUnit, nullptr, 1)` — loot drops,
  XP/kill-credit skipped. If credit matters on that path, record the poison statlist
  owner GUID at deferral time (fork `MonsterMode.cpp` deferral) and pass it at completion.
- **Retail idle-cleanup counterpart:** retail's `UNITS_ProcessDeadOrInactiveMonster`
  twin was not pinned this session (`.UNIT\SUnitInactive.cpp` string at `0x6e0bd5` has no
  code xrefs; PlrSave2-region candidates rejected). It cannot matter behaviorally while
  the fork completes strands before cleanup — but pinning it would close the record.
- r2 project recipe for next session:
  `LD_LIBRARY_PATH=/home/vtstech/r2-portable/lib GIT_AUTHOR_NAME=… GIT_COMMITTER_NAME=… radare2 -q -N -e scr.color=0 -c "P game114d; af @ 0x…; pdf @ 0x…" Game.exe`

*Session: R182, 2026-11-20. Fork build self-ID: `Vanilla+0.0.8-R182`.*
