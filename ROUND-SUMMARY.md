# Round summary — R166 (rev 179, 2026-11-18): issue #39 residual — deferred-death completion for chilled/frozen kills

## The ask

Issue #39 (owner): chilled/frozen monsters disappear without dying or
dropping loot. R162 (dispatcher bypass: MODECHANGE/ENDANIM/AITHINK reach the
timer handlers for frozen units) and R163 (poison-tick kill for cold/frozen
monsters bypasses the uninterruptable check) removed most of it — owner:
"still disappear rarely with cold/frozen status in R163, but not nearly as
much as R161", arrows that turned out to carry poison.

## Root cause (the residual rare case)

Every kill/drop path was diffed against upstream's binary-faithful
`asm_matching/msvc8/translated` tree: `SERVER_KillMonster`, the lifecycle
transition, `MONSTER_OnDeathDispatch` (drops ride the transition via
`MONSTERMODE_TryDropTreasure`) and the per-mode kill handlers are
retail-identical — the fork's only deviations were the R162/R163 patches
themselves. Two defects fall out of that comparison:

1. **The R163 bypass violates retail's uninterruptable invariant.** Retail
   NEVER routes a unit with `STATE_uninterruptable` into
   `MONSTERAI_ExecuteLifecycleTransition` — that function's first check
   **halts** on an uninterruptable victim (upstream asm_matching, line
   0x753), which is exactly why retail defers lethal kills on such units
   via `STATE_death_delay` (SUnitDmg post-hit dispatcher; retail's
   RegenAndPsnDecreaser poison-tick killer). `DAMAGE_ApplyFreeze` refuses
   to freeze an uninterruptable target, so **chilled** monsters (cold
   applied, still acting) can be mid-cast and uninterruptable — R163 fed
   those straight into the halt path. (Frozen monsters are never
   uninterruptable, which is why R163 still removed most of the symptom.)
2. **A deferred death on a chilled/frozen monster can never complete.**
   The only retail consumer of `STATE_death_delay` is
   `SetUninterruptableState(0)` — reachable from
   `MONSTER_EndOfFrameAiDispatch`'s non-flagged branch. For
   frozen-alive units the flagged-mode branch skips the whole dispatch, so
   a chilled monster killed mid-cast sat at 0 HP with death_delay set —
   untargetable, not dying, dropped by the next room update: the owner's
   "disappears without loot".

## Fix (R165 → R166)

1. `RegenAndPsnDecreaser` (MonsterMode.cpp): inside the R163 cold/frozen
   branch, restore retail's deferral first — if `STATE_uninterruptable` is
   active, set `STATE_death_delay` and return. Non-uninterruptable
   cold/frozen poison deaths stay immediate (R163's field-verified
   improvement unchanged).
2. `SUNITDMG_CompleteDeathDelayIfPending` (SUnitDmg.cpp, new): completes a
   stranded deferred death — monster, alive, death_delay active,
   uninterruptable cleared — through the same killer-aware pair retail's
   `SetUninterruptableState(0)` uses (`SERVER_GetHitCalculation` with the
   KILL result flag when a killer resolves, `SERVER_KillMonster`
   otherwise). Wired into **both** state-expiry callbacks: freeze
   (`SUNITDMG_StatExpireCallback_ResumeAI`) and cold (`DAMAGE_OnColdExpire`),
   so the death completes at the latest when the chill expires even when
   the cast-end path never ran.

## Docs + gates

- Version self-ID `0.0.8-R165` → `0.0.8-R166` (`D2VP_ROUND`; boot mark,
  title and automap tags follow). CHANGELOG +R166; handoff rev 179 header +
  self-ID; README badge/version; this file.
- `tools/docs_gate.py`: ALL PASS (0 fail).

## Field-verify asks (owner)

1. Rebuild (R166); first `d2boot.log` mark must read `Vanilla+0.0.8-R166`.
2. Repeat the old repro: poison+cold arrows into casting/chilled monsters,
   Blizzard/frozen mobs at close range — kills should always leave a corpse
   or loot, never a vanishing unit.
3. Regression: frozen monsters still die immediately (R163 behavior for the
   common case), chilled non-casting monsters die immediately, and the
   R162 dispatcher bypass stays untouched (no "unresponsive when merely
   cold" regression).
4. If any vanish survives R166, capture the `d2boot.log` line and the
   monster's state — the remaining suspect is the corpse-expiry event type
   still being frozen-gated in the dispatcher (types 6/7/9/10/11/13/14).

## Verification boundary

Sandbox-side: docs gate ALL PASS, VP-marker census balanced, brace/paren
balance green on touched files. Compile gate of record stays the owner's
Win11 MSYS2 build (handoff §8); the kill-completion behavior needs the
in-game repro (asks 2–4) as the deciding field pass.
