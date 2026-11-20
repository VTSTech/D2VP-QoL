# R2 Game.exe Validation & Expansion — R170 (capstone re-verification + monster timer dispatch)

**Date:** 2026-11-20
**Binary:** `Game.exe` 1.14d retail, md5 `2b3600c10ee0d6387675a2f6cf0ebed2`, base `0x400000`,
`.text` `0x401000..0x6cb5a1`, `.rdata` `0x6cc000..0x704a10`, `.data` `0x705000..0x994ca4`.
**Method:** full linear capstone disassembly of every executable section (1,068,342 instructions,
78,025 direct calls), byte-pattern probes, and per-claim instruction-level checks. Tooling is
scripted (no radare2 dependency): `scripts/r2_validate.py` … `r2_validate8.py` in the repo.
This session re-verified every address claim in the surviving R2 docs against the binary,
**corrected one central claim of the R170 verification doc** (a second retail
`STATE_death_delay` set site), and pinned the retail monster timer-dispatch subsystem that
hosts the retail equivalent of the fork's `RegenAndPsnDecreaser`.

---

## 1. Re-verification of the R170 verified set — ALL PASS

Every claim in [`ColdFrozen_DeathPath_r2_Verified_R170.md`](ColdFrozen_DeathPath_r2_Verified_R170.md)
§2 reproduced independently with capstone:

| Claim | Verdict | Evidence |
|---|---|---|
| `0x5544b0` = `SetUninterruptableState(pGame, pUnit, bSet)` | **PASS** | prologue `push ebp`; pushes `0x36` (STATE 54) at `0x5544be`; 13 call xrefs |
| calls `GetKillerUnit 0x553010` at `0x554510` | **PASS** | `calls[0x554510] = 0x553010` |
| calls `GetHitCalculation 0x57cee0` at `0x554539`, `KillMonster 0x57ccb0` at `0x554548` | **PASS** | exact call sites confirmed |
| `0x57cf7b` = `push 1; push 0x5c` (death_delay set inside GetHitCalculation) | **PASS** | byte probe + `call SetState 0x639db0` at `0x57cf80` |
| uninterruptable check `GetUnitState(0x36)` before it | **PASS** | call site `0x57cf68` (doc said `0x57cf65` — 3 bytes off, claim stands) |
| `0x57d00f` = GetHitCalculation's kill path → KillMonster | **PASS** | exact call site |
| `0x57ccb0` = `SERVER_KillMonster` | **PASS** | 16 xrefs incl. `0x554548`, `0x57d00f`, `0x5a6adb`, `0x5a6fd8`; has-death-anim guard helper `0x457490` in window |
| `0x57af80` = `COMBAT_ApplyCold` (push `0xb` @ `0x57af93`; expire fn `0x57ad80` pushed @ `0x57b08c`, `call 0x625ce0` @ `0x57b092`) | **PASS** | exact sites |
| `0x57b230` = `COMBAT_ApplyFreeze` (uninterruptable check `push 0x36` @ `0x57b25e`; three ApplyCold fallbacks `0x57b2af`/`0x57b3fc`/`0x57b40e`; expire fn `0x57b170` pushed @ `0x57b36f`) | **PASS** | exact sites (the doc's "call at 0x57b25e/0x57b36f" names the `push`, the `call` lands 2–5 bytes later) |
| `0x57ad80` = `DAMAGE_OnColdExpire` (dead+active early return; UpdateAnimSpeed `0x623f50` with `push 0x152` @ `0x57adc2`; **no death completion**) | **PASS** | 0 call xrefs — pointer-registered expire callback, as documented |
| `0x57b170` = freeze expire callback (`0x5541b0` + `0x553570` + `0x540e60` + `0x5417d0`; **no death completion**) | **PASS** | all four call edges present |
| helpers: `SetState 0x639db0` (188 xrefs), `GetUnitState 0x639df0` (253), `IsStateActive 0x63a4a0` (7, incl. both expire callbacks `0x57ad91`/`0x57b181`), `STATLIST_SetExpireFunction 0x625ce0` (39), `UnitQueueEvent 0x5417d0` (265), `EVENT_RemoveUnitTimersByType 0x540e60` (141), `FreeCombatAndChangeAnimMode 0x553570` (30, incl. `0x57b1a4` inside the freeze callback) | **PASS** | xref counts and expected callers |
| `0x58abc0` = A5Q3 quest timer, **not** `RegenAndPsnDecreaser` | **PASS** | 0 call xrefs; pointer-referenced only; behavior = quest timers |

## 2. CORRECTION — retail has a SECOND `STATE_death_delay` set site

The R170 doc states `0x57cf7b` is "THE ONLY retail `STATE_death_delay` set site", based on a
binary-wide byte search for `6a 01 6a 5c` (`push 1; push 0x5c`). That search has a blind spot:
**argument push order.** The poison-tick killer pushes the *value* first and the *state* second,
with a branch between them, so the bytes never appear adjacent.

Retail `RegenAndPsnDecreaser` (pinned this session, §3) ends:

```asm
0x5a6920 ..0x5a6ab4    ; hp-regen / poison-tick body (see §3)
0x5a6ab7:  push 0x36              ; STATE_uninterruptable
0x5a6ab9:  push esi               ; unit
0x5a6aba:  call 0x639df0          ; UNIT_GetUnitState
0x5a6abf:  test eax, eax
0x5a6ac1:  push 1                 ; <-- value, pushed BEFORE the branch
0x5a6ac3:  je  0x5a6ad6           ; not uninterruptable -> kill below
0x5a6ac5:  push 0x5c              ; STATE_death_delay
0x5a6ac7:  push esi
0x5a6ac8:  call 0x639db0          ; SetState(unit, death_delay, 1)   <-- SET SITE #2
0x5a6acd:  ret 8                  ; defer
0x5a6ad6:  push edi               ; attacker
0x5a6ad9:  mov edx, esi
0x5a6adb:  call 0x57ccb0          ; SERVER_KillMonster(pGame, unit, attacker, ...)
0x5a6ae4:  mov edx, 0xa ; call 0x5c0c30   ; UNITEVENT_DispatchEventsByType(10, unit, attacker)
0x5a6af8:  mov edx, 0x9 ; call 0x5c0c30   ; UNITEVENT_DispatchEventsByType(9, attacker, unit)
```

**Corrected statement:** retail sets `STATE_death_delay` at exactly two sites —

| # | Site | Context |
|---|---|---|
| 1 | `0x57cf80` (inside `GetHitCalculation 0x57cee0`) | lethal *hit* lands on an uninterruptable victim (`GetHitCalculation` `push 1; push 0x5c`) |
| 2 | `0x5a6ac8` (inside `RegenAndPsnDecreaser 0x5a6920`) | poison *tick* kills an uninterruptable victim (`push 1` @ `0x5a6ac1`, `push 0x5c` @ `0x5a6ac5`) |

This **strengthens** the R170 fix rather than weakening it: the fork's R166 poison-path deferral
(`SetState(death_delay, 1)` + return) mirrors site 2 byte-for-byte in spirit, and the R170
completion path (`SetUninterruptableState(pUnit, 0)`) is the single completion mechanism for
deferrals from *both* sites. The completion-side claim ("retail completes a deferred kill in
exactly one place, `SetUninterruptableState(unit, 0)`") remains valid — no other function
clears `death_delay` and performs the killer-aware kill.

## 3. NEW PIN — retail monster timer-dispatch subsystem (fork `GAME::Event::Dispatch::Monster`)

### 3.1 `MONSTEREVENT_DefaultCallback` = `0x5a7f80`

`__fastcall(pGame=ecx, pUnit=edx, eTimerType=[ebp+8], nArg1=[ebp+0xc], nArg2=[ebp+0x10])`,
`ret 0xc`. Structure:

```asm
0x5a7f89:  cmp edi, 0xe           ; eTimerType > UNITEVENTCALLBACK_REMOVESKILLCOOLDOWN(14)?
0x5a7f90:  ja  ret                ;   -> silently ignore
0x5a7f92:  lea eax, [edi - 3]     ; types 3..12 use the 10-byte route table
0x5a7f98:  ja  0x5a7fa8           ; types 0,1,2,13,14 -> freeze-check path
0x5a7fa1:  jmp [eax*4 + 0x5a7fe0] ; route: slot0 = dispatch now, slot1 = freeze check
0x5a7fa8:  push 1 / push esi / call 0x639df0   ; GetUnitState(unit, STATE_freeze=1)
0x5a7fb6:  call 0x5541b0                       ; CheckForModeZeroDeath
;   frozen-alive unit -> return WITHOUT dispatch (events swallowed while frozen)
0x5a7fbf:  mov eax, [edi*4 + 0x6e2490]         ; pTimerMethods.fpFunctionExByType[type]
0x5a7fd6:  call eax                            ; __fastcall(pGame, pUnit, type, a1, a2)
```

Route table bytes (`0x5a7fe8`, 10 entries for types 3..12; 0 = dispatch unconditionally):

| type | name | route |
|---|---|---|
| 3 `STATREGEN` | hp-regen / poison tick | **unconditional** |
| 4 `TRAP` | trap update | **unconditional** |
| 5 `ACTIVESTATE`/`RESET` | state timer | **unconditional** |
| 6 `FREEHOVER` | hover text | freeze-check |
| 7 `MONUMOD`/`QUESTFN` | uMOD on hit | freeze-check |
| 8 `PERIODICSKILLS` | aura skills | **unconditional** |
| 9 `PERIODICSTATS` | aura stats | freeze-check |
| 10 `AIRESET` | AI reset | **unconditional** |
| 11 `DELAYEDPORTAL` | portal | freeze-check |
| 12 `REMOVESTATE` | state removal | **unconditional** |

Types 0 `MODECHANGE`, 1 `ENDANIM`, 2 `AITHINK`, 13 `REFRESHVENDOR`, 14 `REMOVESKILLCOOLDOWN`
always go through the frozen/mode-zero gate.

**Cross-check vs the fork** (`Source/D2Game/GAME/Event/Dispatch/Monster.cpp`): the fork's
`pTimerMethods` null pattern (`4, 11, 13, 14` = nullptr) matches retail exactly, and the fork's
doc-comment list of freeze-sensitive types matches the route table. Two nuances worth noting:
retail routes `AIRESET`(10) unconditionally and gates `MODECHANGE`/`ENDANIM`/`AITHINK` behind
the frozen check — i.e. retail **does** swallow queued MODECHANGE/ENDANIM/AITHINK events for a
frozen-alive monster, which is consistent with the R170 analysis (cast-end completion can be
unreachable while frozen-alive) and with the fork's R162 decision to dispatch those types
unconditionally in the rebuild.

Callers: `0x541060` and `0x5410f0` (the timer-run walkers beside the fork-documented player
dispatch sites `0x540fa7`/`0x54100b`).

### 3.2 The 15-slot monster timer table = `.rdata:0x6e2490`

Every slot maps 1:1 onto the fork's `D2Game::GAME::Event::Dispatch::Monster::pTimerMethods`:

| idx | retail | fork counterpart | identification evidence |
|---|---|---|---|
| 0 | `0x5a7ba0` | `MONMODE_DispatchDeathDrop` | death-drop dispatch shape |
| 1 | `0x5a7be0` | `ExecuteDropHandlerFn3` | same shape as [0] |
| 2 | `0x5b1740` | `MONSTERAI_ExecuteAiFunction` | AI dispatch body |
| 3 | **`0x5a6920`** | **`RegenAndPsnDecreaser`** | full behavioral match, §3.3 |
| 4 | NULL | nullptr | |
| 5 | `0x56d790` | `SKILLS_DispatchStateActiveFunc` | reads skill-state table `[0x744304]+0xba0` |
| 6 | `0x5a7f00` | `MONMODE_UpdateHoverTextExpiry` | unit+`0xa4` hover text, frees it |
| 7 | `0x5a4370` | `MONSTER_DispatchUModOnHit` | thunk `push 2` → `0x5a4270` |
| 8 | `0x56fcb0` | `SKILLS_PeriodicSkillsCallback` | periodic skill tick |
| 9 | `0x56fe40` | `SKILLS_PeriodicStatsCallback` | periodic stat tick |
| 10 | `0x5a7f70` | `AI_ClearSwitchAiTargetParamsCallback` | thunk → `0x573120` |
| 11 | NULL | nullptr | |
| 12 | `0x5a7ef0` | `RemoveStatListFromUnit` | thunk → `0x627460` (statlist remove) |
| 13 | NULL | nullptr | |
| 14 | NULL | nullptr | |

### 3.3 Retail `RegenAndPsnDecreaser` = `0x5a6920` (extends `0x5a6920..0x5a6b0a`)

Instruction-level match against the fork's `Source/D2Game/Monster/MonsterMode.cpp:570`
(`MonsterMode::RegenAndPsnDecreaser`), line for line:

| fork operation | retail site |
|---|---|
| `STATSEX_GetCurrentStat(hpregen)` | `push 0; push 0x4a; push unit; call 0x625480` |
| `STATES_HasProgressiveState` + subtract | `call 0x63a750`; `push 0; push 0x4a; call 0x6253b0` |
| `UNIT_GetUnitState(STATE_preventheal=52)` early-out | `push 0x34; call 0x639df0` @ `0x5a6955` |
| `SetEvent(type 3, frame+1)` self-requeue | `push 3; call 0x5417d0` @ `0x5a697a` |
| `EVENT_RemoveUnitTimersByType(STATREGEN)` when regen=0 | `push 3; call 0x540e60` @ `0x5a698b` |
| hitpoints(6) + max-hp read, town check | `0x625480`(6), `0x625d10`, `0x620bb0`/`0x61ab00` |
| clamp, `STATLIST_SetUnitStat(hp / last_sent_hp_pct=0x160)` | `call 0x627260` @ `0x5a69f9`/`0x5a6a39` |
| `MONSTER_GetHpPercent128` + packet 0xAB when \|Δ\|>4 | `call 0x5a5650`, `call 0x571a10` @ `0x5a6a2b` |
| mode 0/0xc (Death/Kick) reject | `cmp [unit+0x10], 0xc` @ `0x5a6a59` |
| poison statlist search over `{STATE_poison=2, 0x3e=62}` | `call 0x6256b0` (GetStatsByState) @ `0x5a6a7c` |
| owner GUID/type → attacker | `0x6252b0`/`0x625250` → `FindUnitFromTable 0x552f60` |
| uninterruptable → defer via `death_delay` | **`0x5a6ab7..0x5a6acd` — set site #2 (§2)** |
| kill + events 10/9 | `call 0x57ccb0` @ `0x5a6adb`; `UNITEVENT_DispatchEventsByType 0x5c0c30` (edx=10/9) |

Zero direct call xrefs — invoked only through table slot `0x6e249c`, exactly like the fork
(index 3 of `fpFunctionExByType`). **The R2 correction stands:** `0x58abc0` (A5Q3) was wrong;
`0x5a6920` is the retail poison killer.

### 3.4 Supporting pins

* `UNITEVENT_DispatchEventsByType` = **`0x5c0c30`** (12 callers: the damage appliers
  `0x57c1e0`/`0x57c6c0`, `0x57d600`, `0x57e510`, the poison kill tail `0x5a6aeb`, …). Matches
  the fork's `UNIT::SUnitEvent::UNITEVENT_DispatchEventsByType(events 9/10)` after-kill calls.
* `KillMonster 0x57ccb0` death sequence: the wrapper right behind it (`0x57cd44`) calls the
  XP wrapper `0x5a4ef0` → `0x57e990` (XP award) and the quest wrapper `0x57cdb7` →
  `0x543a30` (quest monster-death event). **This upgrades the previously "candidate" status of
  `0x57e990` and `0x543a30` to verified.**
* `0x5a6df0..0x5a6ea7` = the per-frame state-damage walker: walks the unit's statlist chain
  (`[room+0x74]`), memsets a 0x70 `D2DamageStrc`, sets result flags `|4` when the victim is
  *not* uninterruptable, then applies damage via `0x57c1e0` / `0x57c6c0` /
  `GetHitCalculation 0x57cee0` (`0x5a6e90`). Another GetHitCalculation consumer to keep in
  mind when tracing kill paths.
* `0x5a6eb0..0x5a6fe3` = state expire/damage callback that ends
  `KillMonster(pGame, unit, NULL, 1)` at `0x5a6fd8` — a **no-attacker kill**. Relevant to the
  open H3 item (kill credit/XP when a deferred kill resolves with no killer): retail itself
  performs attacker-less kills on this path, so the fork's H3 behavior has retail precedent.
* `0x5a6ff0` (fork `MONSTERAI_ExecuteLifecycleTransition` / "MonDeathEvent"): zero call xrefs,
  single raw reference at `.rdata 0x6e2260` (AI function table region), and its body calls
  `SetUninterruptableState 0x5544b0`, `CheckForModeZeroDeath 0x5541b0`, `KillMonster
  0x57ccb0` (`0x5a6fd8` is nearby but inside `0x5a6eb0`; `0x5a6ff0`'s own window includes the
  kill pair). Behavior-consistent with the fork's lifecycle function; still not formally named —
  kept as a strong candidate.

## 4. Document audit — what was removed and why

Every address in every R2 doc was classified against the Game.exe section map and, for code
addresses, checked for instruction-start + call-xref evidence. Results:

**Removed this session (9 files) — incorrect or unverifiable documentation:**

| File | Reason |
|---|---|
| `README.md` | byte-identical duplicate of `R2_CallFlow_Documentation_Index.md` |
| `Boot_TitleScreen_CallFlow.md` | fabricated addresses (`0x401000`, `0x401050`, `0x403000`, …) presented as real function pins; none are referenced code |
| `Entering_Game_CallFlow.md` | same (`0x406000`/`0x407000`/`0x408000` round-number placeholders) |
| `Activating_Shrine_CallFlow.md` | same (`0x487xxx` placeholders, invented names) |
| `Opening_Chest_CallFlow.md` | same (`0x486xxx` placeholders) |
| `Monster_Dropping_Loot_CallFlow.md` | same (`0x5a9xxx` placeholders) |
| `TCP_IP_Game_CallFlow.md` | **wrong module**: every address (`0x6e0790`, `0x6fc574`, …) falls inside Game.exe's `.rdata` — these are 1.10–1.13-era DLL addresses (D2Game.dll base `0x6FC00000`), not the statically-linked 1.14d rebuild |
| `TCP_IP_Game_Hosting_and_Joining_Analysis.md` | same wrong-module address set |
| `Battle.net_CallFlow.md` | same wrong-module address set (23 addresses, all `.rdata`) |

The narrative shapes of the removed docs (title screen, chest, shrine, loot, TCP/IP, BN) were
plausible, but their address tables were placeholders or foreign-module values — keeping them
would keep re-seeding wrong addresses into future rounds. The legitimate BN packet-level work
survives in `Battle.net_Authentication_Flow_Analysis.md` (constants only, informed R169).

**Corrected in place:**

* `Killing_Monster_CallFlow.md` — its three invented addresses (`0x57ae00`
  `COMBAT_ApplyDamage`, `0x57ad00`, `0x57acc0`) replaced with the verified pins
  (`0x57cee0`, `0x5541b0`, `0x5a7f80`); its four real addresses
  (`0x57ccb0`/`0x5a6ff0`/`0x57e990`/`0x543a30`) now carry verification evidence.
* `ColdFrozen_DeathPath_r2_Verified_R170.md` — §2.3 "only set site" corrected (§2 above).
* `D2VP_GameExe_Function_Mapping.md` / `D2VP_ColdFrozen_Validation_GameExe_Analysis.md` —
  candidate statuses upgraded to verified where §3.4 provides evidence.

**Unchanged (validated):** `ColdFrozen_Bug_Complete_Analysis.md`,
`D2VP_ColdFrozen_Validation_GameExe_Analysis.md`, `D2VP_GameExe_Function_Mapping.md`,
`R2_Game.exe_Analysis_Guide.md` (methodology; r2 commands remain valid, capstone harness now
supplements it), `R2_Project_Setup_Guide.md`, `Battle.net_Authentication_Flow_Analysis.md`,
`CD_Key_Analysis_Report.md`, `CD_Key_Functions_Game.exe_Analysis.md`,
`CheckRevision_Analysis_and_Fix_Strategies.md`.

## 5. Open items for the next R2 pass

1. ~~Name `0x5a6ff0` formally~~ DONE (R171): it is `MONSTER_OnDeathDispatch` — per-mode AI
   dispatch table `.rdata 0x6e2260` record [0] pFn1, matching the fork's `DropHandlerArray`
   entry 0. The lifecycle-transition candidate moved to `0x5a7c20`. See Diagnostic Reference §3.
2. Name `0x5a6df0` (state-damage walker) and `0x5a6eb0` (state expire/kill callback) — the
   latter matters for the H3 kill-credit decision.
3. Verify the remaining cold/frozen docs' minor site addresses (`0x57cf65`→`0x57cf68`,
   `0x57b25e` push-vs-call, expire-fn call `0x57b092`/`0x57b374`) were folded into §1 —
   no further action needed unless the docs are quoted byte-for-byte elsewhere.
4. Rebuild the removed call-flow docs (title screen, chest, shrine, loot, TCP/IP) only from
   re-derived pins; do not restore the old tables.

---
*Verification harness: `scripts/r2_validate*.py` (capstone; runs offline against
`/tmp/gameexe/Game.exe`). Byte evidence in this doc is reproducible from those scripts.*
