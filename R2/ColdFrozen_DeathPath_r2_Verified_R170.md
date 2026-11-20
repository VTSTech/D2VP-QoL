# Cold/Frozen Death Path — r2-Verified Retail Address Map & Residual Bug Analysis (R170 work)

**Binary:** Game.exe 1.14d Retail (`/upload/Game.exe-1.14d-Retail.zip`), PE32, PDB ref
`d:\diablo2\Diablo2\Exe\Release\Game.pdb`, base `0x400000`, compiled 2016-05-31.
**Tool:** r2-portable 6.1.5 (`tools/r2-portable/r2.sh`, from VTSTech-SRVEmu `tools/r2-portable.zip`).
**Method:** every address below was pinned by disassembling the retail binary and matching
branch structure/constants against the fork's decompiled source — not copied from prior
notes. Claims in the older docs that could not be reproduced are flagged.

Status: **R170 session** (extends the R162/R163/R164/R166 record in
[ColdFrozen_Bug_Complete_Analysis.md](ColdFrozen_Bug_Complete_Analysis.md) and
[D2VP_ColdFrozen_Validation_GameExe_Analysis.md](D2VP_ColdFrozen_Validation_GameExe_Analysis.md)).

---

## 1. Verified retail addresses

### 1.1 Death / state core (all verified this session)

| Retail VA | Identity | Evidence |
|---|---|---|
| `0x5544b0` | `SUNIT_SetUninterruptableState(pGame, pUnit, bSet)` (ecx=unit, edx=bSet) | Prologue pushes `0x36` (STATE_uninterruptable=54) then null-check halt with line `0xa29` @ `0x5544ca` — matches fork `SUnit.cpp` halt line. |
| `0x5541b0` | `SUNIT_CheckForModeZeroDeath` | Called as first step of both retail state-expiry callbacks (same call pattern the fork decompiles). |
| `0x553010` | `SUNIT_GetKillerUnit(pGame, pUnit)` | Called by `0x5544b0` between clearing `STATE_death_delay` and the kill pair. |
| `0x57ccb0` | `SERVER_KillMonster(pGame, pUnit, pAttacker, bRemoveFromOwner)` | Called by the `0x5544b0` no-killer branch as `push 1; push 0; edx=unit; ecx=pGame`. Entry guards: type 0/1, monster mode `0` and `0xc` reject, then **has-death-anim guard** `fcn.00457490(unit, 0xf)` — a monster without a death sequence silently returns, no kill. |
| `0x57cee0` | `SERVER_GetHitCalculation` (damage/kill application) | Called by `0x5544b0` with a memset-0x70 `D2DamageStrc` and `wResultFlags |= 2`. |
| `0x57cf7b` | **A retail `STATE_death_delay` set site — site 1 of 2** (inside `0x57cee0`) | Byte probe `6a 01 6a 5c` (`push 1; push 0x5c`) hits here. Context: `push 0x36; call GetUnitState` → `test flags,2` (KILL) → `death_delay=1` → `ret 8`. |

> **R170b correction (capstone re-verification, see
> [R2_Game.exe_Validation_Expansion_R170.md](R2_Game.exe_Validation_Expansion_R170.md) §2):**
> the original "THE ONLY set site" claim was a byte-probe artifact. Retail has a **second**
> `STATE_death_delay` set site at `0x5a6ac5..0x5a6cd` inside `RegenAndPsnDecreaser`
> (`0x5a6920`): the poison-tick kill of an uninterruptable victim pushes the value (`push 1`,
> `0x5a6ac1`) *before* the branch, then `push 0x5c` (`0x5a6ac5`) and calls `SetState 0x639db0`
> (`0x5a6ac8`) — the two pushes are never adjacent, so the `6a 01 6a 5c` probe missed it.
> The completion-side claim is unaffected: `SetUninterruptableState(unit, 0)` (`0x5544b0`)
> remains the single completion mechanism for deferrals from both sites.

**Correction vs. the older doc:** the deferral is *inside* `SERVER_GetHitCalculation`
(0x57cee0–0x57d3c6), not in a separate "post-hit dispatcher". Fork `SUnitDmg.cpp:2424`
mirrors this code; the fork's "post-hit dispatcher" label is a decompile artifact.

### 1.2 Cold / freeze application (older doc addresses confirmed)

| Retail VA | Identity | Evidence |
|---|---|---|
| `0x57af80` | `COMBAT_ApplyCold` | Pushes `0xb` (STATE_cold=11) at `0x57af93`; sets expire fn `0x57ad80` via `push 0x57ad80; call 0x625ce0` (STATLIST_SetExpireFunction) at `0x57b08c`. |
| `0x57b230` | `COMBAT_ApplyFreeze` | Type switch, then `push 0x36; call 0x639df0` (GetUnitState, uninterruptable) at `0x57b25e`; **three fallback paths** `call 0x57af80` (→ ApplyCold) at `0x57b2af`, `0x57b3fc`, `0x57b40e`. Sets expire fn `0x57b170` at `0x57b36f`. Fires event `0xc` (expiry) and event `2` (AITHINK resume) via `0x5417d0`. |
| `0x57ad80` | `DAMAGE_OnColdExpire` (cold expire callback) | Dead+state-active early return via `0x5541b0`/`0x63a4a0`; clears state; clears `0x6b` (STATE_shatter) when alive; `push 0x152; push ".\UNIT\SUnitDmg.cpp"; call 0x623f50` (UpdateAnimSpeed). **No death completion in retail.** |
| `0x57b170` | Freeze expire callback (fork: `SUNITDMG_StatExpireCallback_ResumeAI`) | Same early-return guard; clears state; `call 0x553570` = `FreeCombatAndChangeAnimMode(pGame, unit, 1)`; AITHINK remove via `0x540e60`, resume event 2 via `0x5417d0` with `aidel` delay. **No death completion in retail.** |

Supporting helpers pinned: `0x639db0` = SetState(unit, state, val), `0x639df0` =
UNIT_GetUnitState(unit, state), `0x63a4a0` = STATES_IsStateActive, `0x553570` =
FreeCombatAndChangeAnimMode, `0x540e60` = EVENT_RemoveUnitTimersByType(2 args + unit),
`0x5417d0` = UnitQueueEvent — `0x5417d0` matches the older doc's `sub_005417D0_UnitQueueEvent`.

### 1.3 Corrections / still-unpinned

| Older doc claim | Status |
|---|---|
| `0x0058abc0` = `sub_0058ABC0_RegenAndPsnDecreaser` | **WRONG.** `0x58abc0` references `d:\diablo2\...\Quests\a5q3.cpp` (line `0x7d1`) and `"ice "` string compares — it is an A5Q3 quest timer function. The retail poison killer is elsewhere (see §2.3). |
| `0x0057CCB0_UnitKilled` | Address correct; it is `SERVER_KillMonster` (kill handler), not a generic "unit killed" — renamed above. |
| `0x0057e990` XP_OnMonsterKill, `0x00543a30` QUESTS_MonsterDeathEvent, `0x005414d0` GameRunUnitEvents, `0x00543e10` QUESTS_TimersRun | **VERIFIED (R170b capstone pass)** — `0x57e990`/`0x543a30` called via the wrapper behind `SERVER_KillMonster` (`0x57cd44` → `0x5a4ef0`/`0x57cdb7`); `0x5414d0`/`0x543e10` called back-to-back from the per-frame dispatcher at `0x52d8dd..0x52d980`. See `R2_Game.exe_Validation_Expansion_R170.md` §3.4. |
| Retail poison killer | **Not pinned.** `0x160` (UNITSTAT_last_sent_hp_pct) and packet `0xAB` byte-searches produced quest/logging false positives (documented in §3.3). Candidate region: the SUnit cluster `0x55xxxx`–`0x566xxx`; one caller of the 0x40-byte packet helper `fcn.00558c50` is `fcn.00565ab0`, whose tail region is trade code — mapping still ambiguous. |

### 1.4 Constants used (from fork `d2_enums.h`, matched in disasm)

`STATE_freeze=1`, `STATE_poison=2`, `STATE_cold=11`, `STATE_preventheal=52 (0x34)`,
`STATE_uninterruptable=54 (0x36)`, `STATE_death_delay=92 (0x5c)`, `STATE_shatter=107 (0x6b)`,
`UNITSTAT_hpregen=74 (0x4a)`, `UNITSTAT_last_sent_hp_pct=352 (0x160)`,
`UNITEVENTCALLBACK_MODECHANGE=0`, `ENDANIM=1`, `AITHINK=2`, `STATREGEN=3`, `REMOVESTATE=12`,
`REMOVESKILLCOOLDOWN=14`.

---

## 2. Retail vs. fork — what actually differs

### 2.1 Retail's deferred-death invariant (verified end-to-end)

```
lethal hit (KILL flag) → SERVER_GetHitCalculation (0x57cee0)
    └─ victim uninterruptable? (0x57cf68) → STATE_death_delay=1 (0x57cf7b) → return   ← deferral site 1 of 2 (site 2: RegenAndPsnDecreaser 0x5a6ac5, see §2 correction)
    └─ else → full kill (calls SERVER_KillMonster at 0x57d00f)

completion paths for a deferred death (retail):
    └─ SetUninterruptableState(unit, 0) (0x5544b0)
         └─ death_delay set? → clear it → GetKillerUnit (0x553010)
              ├─ killer → SERVER_GetHitCalculation (0x57cee0) with KILL flag
              └─ none  → SERVER_KillMonster (0x57ccb0)
    (there is no other retail completion — the freeze/cold expiry callbacks
     at 0x57b170 / 0x57ad80 do NOT complete deferred deaths)
```

### 2.2 Fork-only deviations still active in R169

1. **R162** — `MONSTEREVENT_DefaultCallback` dispatches `MODECHANGE/ENDANIM/AITHINK`
   unconditionally; retail routes them through the frozen-alive skip
   (`skip_if_frozen` for every type outside STATREGEN/TRAP/ACTIVESTATE/PERIODICSKILLS/REMOVESTATE).
   This is what lets a frozen monster's death animation complete.
2. **R166** — `SUNITDMG_CompleteDeathDelayIfPending` added to *both* expiry callbacks.
   Retail has no equivalent (see 2.1): it is a genuine fork addition, not a retail port.
3. **R163+R166** — poison killer (fork `MonsterMode.cpp:654-700`) has a dedicated
   cold/frozen branch. After R166 restored the deferral, the branch is behaviorally
   identical to the non-cold branch; retail has no such branch (its poison kill runs
   through `GetHitCalculation`, whose internal deferral covers it). Net: fork == retail
   on this path, with one fork-only deferral site (`MonsterMode.cpp:669/686`).

### 2.3 Residual disappearing-monster holes (candidate causes for the field report)

**H1 — completion silently skipped while still uninterruptable (most likely). ✅ FIXED in R170 (applied to `Source/D2Game/UNIT/SUnitDmg.cpp`, build self-ID `Vanilla+0.0.8-R170`).**
`SUNITDMG_CompleteDeathDelayIfPending` early-returns when `STATE_uninterruptable` is set.
For a monster killed mid-cast, at freeze expiry the cast is usually still "in flight"
(frozen cast modes are skipped by `MONSTER_EndOfFrameAiDispatch` — fork
`Dispatch/Monster.cpp:342-348`, retail-faithful), so:
- freeze/cold expiry → completion no-ops (uninterruptable still set),
- cast-end clear (`SetUninterruptableState(0)`) is unreachable until unfreeze,
- a re-applied freeze/chill (sustained cold aura, repeated freeze projectiles)
  keeps pushing the statlist timeout out.
The unit sits at 0 HP with `death_delay` set — exactly the issue-#39 strand — until a
room update drops it: **disappears without loot**.

Fix applied (fork `SUnitDmg.cpp`, `SUNITDMG_CompleteDeathDelayIfPending`): the early
return was replaced with an explicit completion through the retail path, which clears the
state *and* completes the pending kill atomically:

```cpp
if (D2Common::DATATBLS::ItemStats::UNIT_GetUnitState(pUnit, STATE_uninterruptable)) {
  // [R170] Cast-end clear can be unreachable for frozen-alive units (cast modes are
  // skipped by MONSTER_EndOfFrameAiDispatch) ... r2-verified: retail 0x5544b0 completes
  // the deferred kill here. Retail's own completion primitive clears STATE_uninterruptable
  // AND finishes the pending kill in one step (its STATE_death_delay branch runs the same
  // killer-aware pair below); the later cast-end call then finds no death_delay and no-ops.
  SUnit::SetUninterruptableState(pUnit, 0);
  return 1;
}
```

Rationale: `SetUninterruptableState(pUnit, 0)` is retail's completion primitive
(verified at `0x5544b0`), so the kill runs through the same killer-aware pair
(`GetHitCalculation`/`KillMonster`) retail uses; the later cast-end call becomes a no-op
because `death_delay` is already cleared. Shipped ungated, folded into the R166 behavior
(no new feature gate — it strictly removes a stall retail never exhibits).

**H2 — dead+state-active early return in both expiry callbacks. ✅ FIXED in R170.** Retail's guard
(`CheckForModeZeroDeath` true AND `STATES_IsStateActive` true → return before any
completion) also precedes the R166 completion call in the fork. A unit that entered
Death mode *with* a stale `death_delay` never gets the flag dropped or the kill finished
by the expiry path. Low probability, but cheap to harden: both `DAMAGE_OnColdExpire` and
`SUNITDMG_StatExpireCallback_ResumeAI` now drop the stale `STATE_death_delay` before the
early return (the retail early return itself stands).

**H3 — poison-killer deferral completes with no killer.** The fork's poison-killer
deferral (`MonsterMode.cpp:669`) sets `death_delay` without a damage event, so the R166
completion resolves the killer via `GetKillerUnit`; when unresolved it kills with
`nullptr` attacker → `SERVER_AwardKillExperience` skipped → **kill credit/XP loss** on
exactly the deferred-cold deaths (loot still drops). If kill credit matters on that path,
record the poison statlist owner GUID at deferral time and pass it at completion.

**H4 — not the bug, but rule out:** `palette_37` (R168) is gated off by default; if the
field build enabled it, out-of-window tint rows can render garbage palettes — visually
distinct from the strand (monsters vanish *alive*). Verify the field build's Options
state before chasing render paths.

### 2.4 Verified-faithful (no fork divergence found)

`SERVER_KillMonster` body, `COMBAT_ApplyCold/ApplyFreeze` (incl. the uninterruptable
fallback), both expire-callback bodies, `MONSTER_EndOfFrameAiDispatch` frozen skips,
`Client::Monster.cpp` frozen-inactive update skip — all match retail semantics.

---

## 3. Reproducing the analysis (r2-portable usage examples)

### 3.1 Install (see also R2_Project_Setup_Guide.md §r2-portable)

```bash
# https://github.com/VTSTech/VTSTech-SRVEmu/raw/main/tools/r2-portable.zip
unzip r2-portable.zip -d ~/tools/r2-portable && chmod +x ~/tools/r2-portable/r2.sh
~/tools/r2-portable/r2.sh -v    # radare2 6.1.5 +1 abi:87 @ linux-x86_64
```

Caveats found this session (zip bugs worth fixing upstream):
- `r2.sh` exports `RADARE2_SHARE_DIR=.../share/radare2/6.1.5`, but the zip's layout is
  `share/6.1.5` (no `radare2/` level) — the variable as shipped points nowhere.
- sdb lookups (`format/dll/*.sdb`) still fall back to `~/.local/share/radare2/6.1.5/` and
  print `ERROR: Cannot find ...` / `ERROR: Cannot find (null)` noise on stderr. Harmless;
  filter with `2>/dev/null` or copy the `share/6.1.5/format` tree into that user dir.
- No env override redirects the user dir (tried `R2_HOME`, `RADARE2_HOME`,
  `R2_USERDIR`, `XDG_DATA_HOME`, `R2_DATDIR`).

### 3.2 Core session (used for every address in §1)

```bash
R2=~/tools/r2-portable/r2.sh
EXE=Game.exe
# one-time full analysis, saved as a reusable project (~48 s for this binary)
$R2 -q -N -e scr.color=0 -c "aaa; Ps /tmp/game114d.r2" $EXE 2>/dev/null

# later sessions load the project instead of re-analyzing
$R2 -q -N -e scr.color=0 -e asm.bytes=false -e asm.lines=false \
    -c "P /tmp/game114d.r2; pdf @ 0x5544b0" $EXE 2>/dev/null
```

### 3.3 Anchor recipes that produced the §1 map

```bash
# (a) constant anchors: Fog error line numbers preserved by the decompile
#     push imm32 = 68 <imm LE>; scan for the halt sites
$R2 -q -N -c "?e push 0x753; /x 6853070000" $EXE 2>/dev/null   # lifecycle-transition halt
$R2 -q -N -c "/x 68290a0000" $EXE 2>/dev/null                  # SetUninterruptableState halt
#     → disasm a window below each hit: pd 40 @ <hit-0x10>

# (b) state-constant probes: SetState sites are 'push <val>; push <unit>; call 0x639db0'
$R2 -q -N -c "/x 6a016a5c" $EXE 2>/dev/null    # push 1; push 0x5c → death_delay=1 (1 hit: 0x57cf7b)
$R2 -q -N -c "/x 6a5c"     $EXE 2>/dev/null    # every push 0x5c (set+clear+test)

# (c) expire-callback discovery: STATLIST_SetExpireFunction = 'push <fn>; call 0x625ce0'
#     grep the ApplyCold/ApplyFreeze bodies for the pushed function address
$R2 -q -N -c "pd 220 @ 0x57af80" $EXE 2>/dev/null | grep -E "push 0x5[0-9a-f]+|call 0x625ce0"

# (d) xrefs after project analysis (find callers / kill sites)
$R2 -q -N -c "P /tmp/game114d.r2; axt 0x57ccb0" $EXE 2>/dev/null   # 16 callers

# (e) unique-constant hunts (false-positive prone — always disasm context)
$R2 -q -N -c "/x 6860010000" $EXE 2>/dev/null   # stat 352 (last_sent_hp_pct)
$R2 -q -N -c "/x 6aab"       $EXE 2>/dev/null   # packet 0xAB (BnetLog hit = false positive)
```

### 3.4 Function-boundary helpers

```bash
$R2 -q -N -c "P /tmp/game114d.r2; s 0x57cf7b; afi~addr,min,max" $EXE 2>/dev/null
# addr: 0x0057cee0  minaddr: 0x0057cee0  maxaddr: 0x0057d3c6   → bounds of GetHitCalculation
```

---

## 4. Open items for the next r2 round

1. Pin the retail poison killer (`RegenAndPsnDecreaser`). Approach: from the fork's
   `RegenAndPsnDecreaser` body, the distinctive retail-visible combo is
   `SetEvent(pGame, pUnit, 3, frame+1)` (STATREGEN self-reschedule) + the
   `hp<=0 → poison-owner lookup → kill` tail. Search every caller of
   `UnitQueueEvent`-type-3 helper and filter for `GetStatsByState(poison=2)`.
2. ~~Verify `0x57e990` (XP), `0x543a30` (quest death event), `0x5a6ff0` (death event)~~ DONE R170b/R171: `0x57e990`/`0x543a30` verified via KillMonster wrapper; `0x5a6ff0` = `MONSTER_OnDeathDispatch`;
   `0x5414d0` (GameRunUnitEvents) — the older doc lists them without evidence.
3. Diff the fork's `MONSTERAI_ExecuteLifecycleTransition` halt (line `0x753` sites at
   `0x4c39cd`, `0x5535a0`, `0x5a7c59`, `0x5d0ffa`) to pin which is the fork's lifecycle
   function (candidates all carry the same error line).

---

*Session: R170, 2026-11-20. Binary: Game.exe-1.14d-Retail (SHA-friendly identifier:
binsz 3618792, hdr.csum 0x0037ced2, guid 4A206F288F5D4842B2A073B37F649ED81).*
