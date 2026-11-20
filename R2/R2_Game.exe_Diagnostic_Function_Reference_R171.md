# R2 Game.exe Diagnostic Function Reference (R171)

**Date:** 2026-11-20
**Binary:** `Game.exe` 1.14d retail, md5 `2b3600c10ee0d6387675a2f6cf0ebed2`, base `0x400000`.
**Purpose:** the functions you will need when diagnosing gameplay issues — organized by
subsystem and by symptom, every address verified against the binary this session with the
capstone harness (`scripts/r2/r2_validate*.py`; phase-9 additions: `r2_validate9.py`).
Naming convention: fork-confirmed names come from a 1:1 structural match with the rebuild's
source; anything weaker is marked *candidate* / *unnamed*.

New pins this session: the per-frame loop order, the monster per-mode AI dispatch table
(`DropHandlerArray`, 21 entries), `MONSTER_OnDeathDispatch` (resolves the old
`0x5a6ff0` "MonDeathEvent" candidate), `MONSTER_EndOfFrameAiDispatch` with its frozen-skip
logic, the retail `GetDropHandler`, the death_delay set/clear census, the damage-tick
alive-gate, and the life-sync packet path.

---

## 1. Frame loop and per-frame order

| Retail VA | Identity | Evidence |
|---|---|---|
| `0x52d870` | **Game frame advance** (`GAME_AdvanceFrame`) | `add dword [game+0xa8], 1` (dwGameFrame); branches on `[game+0x1dc8]` (pause/state); case 3 pushes `0xe1c` and calls the FOG logging/assert helper `0x408090`. Called from `0x52fd19` and `0x564603` |
| `0x52d8dd` | **Per-frame game update sequence** | in order: `0x52d7b0`, `0x52d720`, `0x52d160`, **`GameRunUnitEvents 0x5414d0`**, `0x52d440`, `0x53b000`, `0x53a820`, then **every 20th frame** (`idiv 0x14; test edx; jne skip`) → `QUESTS_TimersRun 0x543e10` |
| `0x5414d0` | `GameRunUnitEvents` (unit/event system driver) | called once per frame from `0x52d8f2`; internally calls the monster timer dispatcher `0x541060` (sites `0x541535`/`0x541547`) |
| `0x541060` / `0x5410f0` | **Monster / player timer dispatchers** (fork `EVENT_DispatchMonsterTimers` / `EVENT_DispatchPlayerTimers`) | both call `MONSTEREVENT_DefaultCallback 0x5a7f80` (`0x5410c4`, `0x54112b`); the fork documents the same two-mode mechanism (immediate vs frame-matched buckets) with the player call sites at `0x540fa7`/`0x54100b` |

Diagnostic notes:
* **Quest timers run on a 20-frame cadence** (`[game+0xa8] % 0x14 == 0`) — quest timers that
  "miss a frame" may just be cadence, not a bug.
* The frame counter lives at **`[SGAMEDATA + 0xa8]`** — everything that schedules "next frame"
  (e.g. the STATREGEN requeue at `0x5a697a`) computes `[game+0xa8] + 1`.

## 2. Timer / event dispatch (monster)

| Retail VA | Identity | Evidence |
|---|---|---|
| `0x5a7f80` | `MONSTEREVENT_DefaultCallback` | `cmp type,0xe; ja ret`; route table `0x5a7fe8` for types 3–12; frozen-alive gate for types 0/1/2/6/7/9/11/13/14 (`GetUnitState(STATE_freeze)` + `CheckForModeZeroDeath`); `call [type*4 + 0x6e2490]` |
| `0x6e2490` | 15-slot monster `pTimerMethods` table | slots 0–14 mapped 1:1 to the fork (nulls 4/11/13/14); see Validation doc §3.2 |
| `0x5417d0` | `UnitQueueEvent` / `SetEvent` (queue a timer event) | 265 xrefs; used by the STATREGEN self-requeue |
| `0x5416b0` | Event-queue front-end (shared body with `0x5417d0`) | type bound `cmp 0xf`; **sets `STATE_uninterruptable` at `0x541712`** for specific queued events — a place where uninterruptable gets *applied*, useful when tracing stuck-cast reports |
| `0x540e60` | `EVENT_RemoveUnitTimersByType` | 141 xrefs |
| `0x5c0c30` | `UNITEVENT_DispatchEventsByType` | 12 callers; after-kill events 9 (kill credit) / 10 (death) are dispatched through it from the poison kill tail and the damage appliers |

Event types (from the fork enum, verified against the route table): 0 MODECHANGE, 1 ENDANIM,
2 AITHINK, 3 STATREGEN, 4 TRAP, 5 ACTIVESTATE/RESET, 6 FREEHOVER, 7 MONUMOD/QUESTFN,
8 PERIODICSKILLS, 9 PERIODICSTATS, 10 AIRESET, 11 DELAYEDPORTAL, 12 REMOVESTATE,
13 REFRESHVENDOR, 14 REMOVESKILLCOOLDOWN.

## 3. Monster per-mode AI dispatch (`DropHandlerArray`)

Retail table at **`.rdata 0x6e2260`**, 21 records × 16 bytes = `{pFn1, pFn2, pFn3, bBool}` —
matches the fork's `D2AiDispatchCallbacksStrc DropHandlerArray[21]`
(`Source/D2Game/Monster/MonsterMode.cpp:1473`) **record-for-record, including the boss
override slots**:

| idx | pFn1 | pFn2 | pFn3 | bBool | fork name (pFn1 / pFn2 / pFn3) |
|---|---|---|---|---|---|
| 0 death | `0x5a6ff0` | `0x5a7350` | `0x5a72b0` | 1 | `MONSTER_OnDeathDispatch` / `MONMODE_ProcessBlockAnimation` / `MONMODE_HandleSpecialEndDeath` |
| 1 | `0x5a73e0` | `0x5a6de0` | — | 0 | `PlanExecutionOfMonster` / `thunk_ExecuteTimerTrigger` / — |
| 2 walk | `0x5a7520` | `0x5a8490` | — | 1 | `MONMODE_StartWalking` / `MONMODE_ProcessWalkWithTrails` / — |
| 3 attack | `0x5a7580` | `0x5a6de0` | `0x5a8030` | 1 | `MONMODE_StartOtherAttack` / `thunk_ExecuteTimerTrigger` / `MONSTER_EndOfFrameAiDispatch` |
| 4–5,7–9 cast | `0x5a75c0` | `0x5a7670` | `0x5a8030` | 1 | `DropHandlerGeneric` / `MONMODE_ProcessSkillCast` / `MONSTER_EndOfFrameAiDispatch` |
| 6 | `0x5a77c0` | `0x5a6de0` | `0x5a8030` | 1 | `thunk_FreeCombatAndChangeAnimMode_B` / `thunk_ExecuteTimerTrigger` / `MONSTER_EndOfFrameAiDispatch` |
| 10 | `0x5a7490` | `0x5a74a0` | `0x5a74d0` | 0 | (special/boss override) |
| 11–15 | `0x5a74e0`/`0x5a7390`/`0x5a77d0`/`0x5a7870`/`0x5a7550` | … | `0x5a8520`/`0x5a8030`/— | | (boss overrides) |
| 16–20 | `0x5a75c0` | `0x5a7670` | `0x5a8030`/`0x5a8330`/`0x5a83e0`/`0x5a8030` | 1 | boss redirect targets (+0x10/+0x11/+0x12/+0x13/+0x14) |

| Retail VA | Identity | Evidence |
|---|---|---|
| `0x5a78a0` | **`GetDropHandler`** (retail) | type==`UNIT_MONSTER(1)` guard, MonStats bounds (`[0x744304]+0xa80`, stride `0x1a8`), `cmp mode,0xf; ja`, boss-class byte table `0x5a79c4` + jump table `0x5a79b4` redirecting class ids to slots `0x6e2380`(+0x12) etc. Called from `MONMODE_DispatchDeathDrop 0x5a7ba0` (`0x5a7bb6`), slot-1 `0x5a7be0` (`0x5a7bf6`), and the death/drop dispatch helper `0x5a7c20` (`0x5a7d4c`, `0x5a7df0`) |
| `0x5a6ff0` | **`MONSTER_OnDeathDispatch`** — **RESOLVED** (was "MonDeathEvent candidate") | table `[0] pFn1`; single raw ref `.rdata 0x6e2260`; body calls `SetUninterruptableState/CheckForModeZeroDeath/KillMonster` |
| `0x5a8030` | **`MONSTER_EndOfFrameAiDispatch`** | table pFn3 of cast modes; called from the walk processors `0x5a84d2`/`0x5a8504` and boss override `0x5a868d`. Tail (at `0x5a80aa..0x5a831c`): for class `0x6e` + mode 8 (cast) it does `GetUnitState(STATE_freeze)` → `CheckForModeZeroDeath` → **skips the AI dispatch (`jmp 0x5a831c`) while frozen-alive**, else `call [0x6e2498]` = pTimerMethods slot 2 = `MONSTERAI_ExecuteAiFunction` |
| `0x5b1740` | `MONSTERAI_ExecuteAiFunction` (pTimerMethods slot 2) | zero direct call xrefs — pointer-invoked from the slot-2 table and from the EOF dispatch tail |
| `0x5a7c20` | Death/drop dispatch helper — **candidate for fork `MONSTERAI_ExecuteLifecycleTransition`** | calls `GetDropHandler` twice, reads `STATE_uninterruptable` at `0x5a7c47`/`0x5a7c78`, calls `SetUninterruptableState 0x5544b0` at `0x5a7c88` |

**This is the retail mechanism behind the cold/frozen cast bug (#39):** a monster frozen while
casting is skipped by `MONSTER_EndOfFrameAiDispatch` (`0x5a80aa..0x5a80da`), and its queued
MODECHANGE/ENDANIM/AITHINK timers are swallowed by `MONSTEREVENT_DefaultCallback`
(`0x5a7fa8..0x5a7fbd`) — so nothing clears the cast until the freeze/cold state expires and
`SetUninterruptableState(0)` completes the deferred death. The fork's R162/R166/R170 fix
mirrors this exactly.

## 4. Death and damage pipeline (verified core + new census)

Core set (all re-verified R170/R171): `SetUninterruptableState 0x5544b0`,
`CheckForModeZeroDeath 0x5541b0`, `GetKillerUnit 0x553010`, `SERVER_KillMonster 0x57ccb0`,
`SERVER_GetHitCalculation 0x57cee0` (kill at `0x57d00f`), `COMBAT_ApplyCold 0x57af80`,
`COMBAT_ApplyFreeze 0x57b230`, `DAMAGE_OnColdExpire 0x57ad80`, freeze expire callback
`0x57b170`. Full evidence tables: `ColdFrozen_DeathPath_r2_Verified_R170.md` §1–2.

**`STATE_death_delay` site census (binary-wide, byte-verified):**

| Site | Effect | Context |
|---|---|---|
| `0x57cf80` (`push 1` @ `0x57cf7b`) | **set** | lethal *hit* on uninterruptable victim, inside `GetHitCalculation` |
| `0x5a6ac8` (`push 1` @ `0x5a6ac1`) | **set** | poison-tick kill on uninterruptable victim, inside `RegenAndPsnDecreaser 0x5a6920` |
| `0x554507` (`push ebx` @ `0x554503` — passes the function's own `bSet`) | **clear** | inside `SetUninterruptableState`; runs only when `bSet==0` and `GetUnitState(death_delay)` was active (`0x5544f7`), then `GetKillerUnit 0x553010` and the kill pair |

**Exactly one clear site exists** — the "single completion mechanism" claim is now
byte-proven, not just inferred. If a unit is stuck at 0 HP with `death_delay` set, the
completion did not run: check that `SetUninterruptableState(unit, 0)` (or, since R170, the
freeze/cold expiry callbacks `0x57b170`/`0x57ad80` path) was reached.

| Retail VA | Identity | Evidence |
|---|---|---|
| `0x57a980` | **Damage-tick alive gate** (`SUNITDMG_...CanDamageTick`-style) | returns 0 if arg≤0, if `CheckForModeZeroDeath`, or if `GetUnitState(death_delay)`; called by both state-damage appliers `0x57c1e0` (`0x57c551`) and `0x57c6c0` (`0x57c84a`) — i.e. burn/aura ticks stop while a death is pending |
| `0x5a6df0..0x5a6ea7` | Per-frame state-damage walker | walks unit statlist chain, memset 0x70 `D2DamageStrc`, result flags `|4` when not uninterruptable, applies via `0x57c1e0`/`0x57c6c0`/`GetHitCalculation 0x57cee0` |
| `0x5a6eb0..0x5a6fe3` | State expire/kill callback with **no-attacker kill** | ends `KillMonster(pGame, unit, NULL, 1)` at `0x5a6fd8` — retail precedent for attacker-less kills (relevant to H3 kill credit) |
| `0x5a6920..0x5a6b0a` | `RegenAndPsnDecreaser` | full line-for-line match with fork `MonsterMode.cpp:570`; uninterruptable → defer (`0x5a6ac5`), else kill + events 10/9 (`0x5a6adb`→`0x57ccb0`, `0x5c0c30`) |

## 5. State / statlist core

| Retail VA | Identity | Evidence |
|---|---|---|
| `0x639db0` | `SetState(unit, state, val)` | 188 xrefs; all three `death_delay` writes clear/set through it |
| `0x639df0` | `UNIT_GetUnitState(unit, state)` | 253 xrefs |
| `0x63a4a0` | `STATES_IsStateActive` | 7 xrefs, incl. both expire callbacks (`0x57ad91`, `0x57b181`) |
| `0x6256b0` | `GetStatsByState(unit, state)` | poison search `{2, 0x3e}` @ `0x5a6a7c` |
| `0x625ce0` | `STATLIST_SetExpireFunction` | 39 xrefs — every registered expire callback can be enumerated from its 39 call sites |
| `0x625480` / `0x6253b0` | `STATSEX_GetCurrentStat` / subtract variant | hpregen `0x4a` reads @ `0x5a6935`/`0x5a694b` |
| `0x625d10` | `GetMaxLife`-style max-stat reader | used by the damage-tick gate and the regen clamp |
| `0x627260` | `STATLIST_SetUnitStat`-style setter | hp (6) and `last_sent_hp_pct` (0x160) writes @ `0x5a69f9`/`0x5a6a39` |
| `0x552f60` | `FindUnitFromTable(pGame, type, guid)` | 200+ xrefs; attacker resolution from statlist owner GUID/type |

State IDs in play (fork enum, verified where used): `STATE_freeze=1`, `STATE_poison=2`,
`STATE_cold=11`, `STATE_preventheal=52` (`0x34`), `STATE_uninterruptable=54` (`0x36`),
`STATE_shatter=0x6b`, `STATE_death_delay=92` (`0x5c`). Stat IDs: hitpoints=6, hpregen=74
(`0x4a`), `last_sent_hp_pct=352` (`0x160`), poison-search partner state `0x3e`=62.

## 6. Life sync and animation

| Retail VA | Identity | Evidence |
|---|---|---|
| `0x571a10` | **Packet 0xAB life-update queue** (fork `UNIT::SUnitMsg::UnitQueuePacket0xAB`) | 5 callers: `0x5806b4`, `RegenAndPsnDecreaser` (site `0x5a6a2b`), and three functions at `0x5be6d4`/`0x5bea3d`/`0x5bed42` (player/merc life sync path) |
| `0x5a5650` | `MONSTER_GetHpPercent128` | 7 callers incl. `RegenAndPsnDecreaser` (`0x5a6a10`) and `GetHitCalculation` (`0x57d153`) |
| `0x623f50` | `UpdateAnimSpeed` | 31 callers; cold-expire (`0x57adc2`, `push 0x152` source-line) and ApplyCold (`0x57b0cf`) both feed it — first stop when a frozen unit animates at the wrong speed |
| `0x553570` | `FreeCombatAndChangeAnimMode` | 30 xrefs; freeze expire callback uses it (`0x57b1a4`) |
| `0x457490` | Anim-mode lookup helper (KillMonster's has-death-anim guard) | 14 callers incl. `SERVER_KillMonster` (`0x57ccf9`) |
| `0x5a6b10..0x5a6b6b` | `MONSTER_HasAnimModeSequence` | reads the `gnMonsterAnimModeSequenceFlags` table (`[mode*12 + 0x6e23d0]`), bitmask `[0x6ce268]`, MonStats2 `byMvFlags` — matches the fork's doc comment verbatim |
| `0x5734c0` / `0x5734e0` | Unit anim-mode query helpers (unnamed) | `0x5734c0` called from `GetHitCalculation` region (`0x57d083`/`0x57d119`) and the `0x5a68e0` thunk |

## 7. Symptom playbook

**"Monster frozen mid-cast is stuck / dies late" (issue #39 family)**
Path: `GetHitCalculation 0x57cee0` → uninterruptable (`0x57cf68`) → `death_delay` set
(`0x57cf80`) **or** poison tick → `0x5a6ac8`. Completion requires
`SetUninterruptableState(0)` (`0x554507`). Blockers while frozen-alive:
`MONSTEREVENT_DefaultCallback` swallows types 0/1/2 (`0x5a7fa8..0x5a7fbd`) and
`MONSTER_EndOfFrameAiDispatch` skips the cast-mode AI dispatch (`0x5a80bf..0x5a80da`).
Log: `[unit+0x10]` (mode), uninterruptable/death_delay states, which expiry callback ran.

**"Unit at 0 HP but still alive / disappearing monster"**
Check in order: (a) `death_delay` set and clear site `0x554507` never reached; (b)
`SERVER_KillMonster 0x57ccb0` early guards — type 0/1, mode 0/0xc reject, and the
has-death-anim guard (`0x457490` @ `0x57ccf9`) which **silently returns** for monsters
without a death sequence (classic invisible-vanish source); (c) for poison deaths, the
deferral at `0x5a6ac5` and the state-damage tick gate `0x57a980`.

**"No XP / no kill credit after a kill"**
Kill-credit events go through `UNITEVENT_DispatchEventsByType 0x5c0c30` (event 10 on the
victim, event 9 on the attacker) right after `SERVER_KillMonster`; XP is awarded via the
wrapper behind KillMonster (`0x57cd44` → `0x5a4ef0` → `0x57e990`). If the kill came from the
no-attacker path (`0x5a6fd8`, attacker=NULL), event 9 is skipped by design (`0x5a6af0: je
0x5a6b04`) — retail behaves the same.

**"HP bar doesn't update / wrong life display"**
Life sync = `RegenAndPsnDecreaser` diff check (`0x5a6a15..0x5a6a3e`, threshold 4/128) →
packet 0xAB via `0x571a10`; `last_sent_hp_pct` lives at stat `0x160`. Stale bars usually mean
the requeue (`0x5a697a`, STATREGEN every frame) or the packet queue never ran — check whether
the unit is frozen-alive (STATREGEN is dispatched unconditionally, so a stalled bar points at
the queue side `0x571a10` or the 0x160 stat, not the freeze gate).

**"Quest timers seem skipped"**
`QUESTS_TimersRun 0x543e10` runs only every 20th frame (`0x52d912..0x52d920`) — cadence, not
a bug.

## 8. `STATE_uninterruptable` read-site census (34 functions)

For tracing "stuck while uninterruptable": reads via `GetUnitState(0x36)` at —
`0x4aa900`, `0x4ac200`, `0x4c2240`, `0x4c8350`, `0x4c8cd3`, `0x4c90ed`, `0x4c9c26`
(animation/render-side helpers), `0x5416b0` (event-queue front-end, sets it @ `0x541712`),
`0x54cb10`, `0x56b9c0`, `0x56da0b`, `0x570420`, `0x5704df`, `0x57b230` (ApplyFreeze),
`0x57cee0` (GetHitCalculation deferral), `0x57ec10`, `0x57eec0`, `0x58bec0`, `0x5a3942`
(monster event runner), `0x5a6ab7` (poison kill), `0x5a7c20` ×2 (death/drop dispatch), 
`0x5a80aa` (EOF cast-mode skip), `0x5ad730`, `0x5b0e00`, `0x5cf782`, `0x5da61b`, `0x5dd480`,
`0x5ddd00`, `0x5dfb8b`, `0x5eeb70`, `0x5fdca0`, `0x5fdd60`. Writes (`SetState(0x36)`):
`0x4c2180`, `0x4ca000`, `0x54c045` (×2), `0x5544b0` (SetUninterruptableState itself).

Attribution caveat: functions that are pointer-referenced only (expire callbacks, table
slots, `0x57ad80`, `0x5a6920`) fall outside the call-xref entry list; census entries adjacent
to them (e.g. `0x5a6ab7` vs `0x5a6920`) belong to the pointer-called function's body.

## 9. Open items

1. Name the per-frame helpers in `0x52d8dd` (`0x52d7b0`, `0x52d720`, `0x52d160`, `0x52d440`,
   `0x53b000`, `0x53a820`) and the frame-advance callers `0x52fd19`/`0x564603`.
2. Confirm `0x5a7c20` as fork `MONSTERAI_ExecuteLifecycleTransition` (behavior strongly
   matches; needs the fork-side line-shape comparison).
3. Identify the three `0x5bexxx` packet-0xAB senders (player/merc life sync) and the client
   side of the 0xAB handler.
4. Enumerate the 39 `STATLIST_SetExpireFunction` registrations to build a complete
   state→expire-callback map (useful for any future "state never expires" report).

---
*All addresses verified against the retail binary with the scripted capstone harness in
`scripts/r2/` (phases 1–9). Byte evidence reproducible offline; see also
`R2_Game.exe_Validation_Expansion_R170.md` for the prior validation round.*
