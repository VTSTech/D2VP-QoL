# Round summary — R142 (rev 154, 2026-11-09): Unlock FPS — the presentation-FPS unlock

Owner: **"start R142"**, after scoping v1 the same day: *"bool for v1, we can do numeric
target fps later, for v1 lets target 60 - decoupled first - vsync untouched - for v1 call
the option Unlock FPS"*. Code round: with `[features] unlock_fps` on (default OFF), the
game **presents at ~60 FPS while the simulation keeps the exact retail 25 Hz cadence**.
Self-ID bumps to **`Vanilla+0.0.8-R142`** (line 1 of `d2boot.log`). Plan:
[plans/FPS-UNLOCK-PLAN.md](plans/FPS-UNLOCK-PLAN.md) (FEATURES.md row 17).

## What changed (6 code files)

- `Redivivus/d2_boot.h` — `D2VP_ROUND "141"` → `"142"`.
- `Source/D2Client/UI/VanillaPlusCore.h` — the `UNLOCKFPS_*` API block (gate, pacer,
  telemetry, idle pacing) with the mechanism summary.
- `Source/D2Client/UI/VanillaPlusCore.cpp` — the **[vanilla-plus unlock-fps]** section:
  `UNLOCKFPS_Active` (2 s hot-read, row-28 pattern), `UNLOCKFPS_ShouldPresent`
  (60 Hz deadline pacer — skip-never-queue; ≥ 4 periods stale re-anchors, small
  lateness stays on the absolute grid), `UNLOCKFPS_NotePresent`/`UNLOCKFPS_NoteClientTick`
  (tick/present counters), `UNLOCKFPS_StatsIfDue` (one d2boot mark per 5 s window:
  the presents/ticks pair that proves the sim is unchanged), `UNLOCKFPS_IdleSleepMs`
  (sleep-until-deadline, 100 ms recency-out for the loops that stop polling).
  `timeBeginPeriod(1)` raised once on first activation (one-way latch; winmm already
  linked). Plus: the `D2VP_ConfigReloaded` cache bust, the `<mmsystem.h>` include, and
  the ini template block (`unlock_fps=0`, documented).
- `Source/D2Client/GAME/Game/Game.cpp` — `CLIENT_GameLoopFrame`'s draw block moves
  behind the pacer when the gate is on (`bVpPacedPresent || (!active && !gnFrameSkipCounter)`);
  gate off reduces to the retail block byte-for-byte. `UNLOCKFPS_NoteClientTick`
  piggybacks the `didClientLoop` branch. The tick paths above the block
  (`SrvProcessAllGames` 1000/25, `ClientSideGameLoop`, the frame-skip math) are untouched.
- `Source/D2Client/Message.cpp` — `MessageGameLoop`'s idle `Sleep(10)` paces to the
  pacer deadline under the gate (`UNLOCKFPS_IdleSleepMs()`; ≤ 0 → the retail Sleep(10)).
  Input still rides the pump — the pacer only decides when to draw.
- `Source/D2Client/UI/VanillaPlusOptions.cpp` — the eleventh feature row **Unlock FPS**
  (`unlock_fps`); back row 0x1a4→0x1c4, box 0x1d1→0x1f1. Layout check re-run: **first
  miss at 640x480** — the box is one pitch taller than 480p, so only its empty bottom
  margin clips (box top −8; first row at 72, back row 444..470 — every row on-screen and
  clickable). 800x600 and every wide stop fit whole.
- `Source/D2gfx/D2GFX.cpp` — `D2GFX_EndScene`'s 5 ms anti-spin budget drops to 1 ms
  under the gate (its job — don't spin at the 25 Hz draw gate — is gone once the pacer
  owns the cadence). D2gfx keeps its own 2 s-poll ini reader (the `MANABURN_FixActive`
  server-gate shape): no D2Client imports across the renderer boundary. Gate off = the
  5 ms math, byte-exact.

Deliberately untouched: the tick paths, vsync (`bVSYNC` semantics), the SP pause path
(it draws every pump iteration in retail and never polls the draw site — the recency-out
keeps it retail), cutscenes (Bink paces itself) and loading screens (own loops).

## Docs (5 files + template)

1. `CHANGELOG.md` — the R142 entry (recon summary, mechanism, the UNCAPPED rejection,
   the interpolation follow-on).
2. `README.md` — the **Unlock FPS** table row; status → R142; the since-R128 list.
3. `SMOKE-CHECKLIST.md` + `.github/ISSUE_TEMPLATE/smoke-test.md` — build line + item 0
   → R142, item 2 row count (eleven) + the 480p note, and the new **§S** (items 61–64).
4. `FEATURES.md` — v9 header + row 17 + §17 → **Implemented R142, field pass pending**;
   the shipped-as-of tally (eleven features).
5. `plans/FPS-UNLOCK-PLAN.md` — status → implemented (rounds 1–2), the implementation
   record and the round-1 pins that held.
6. `handoff.md` — this rev's header record (rev 154); R139's paragraph compressed into
   the PREVIOUS list with R140/R141 one-liners.

## Verification

- Sandbox (no cross toolchain — the compile gate of record is your Win11 build):
  the pacer section extracted **verbatim** from `VanillaPlusCore.cpp` into a stubbed
  harness (`scripts/build_unlockfps_test.sh`) — **15/15 green, `-Wall -Wextra`, zero
  warnings**: steady 60 Hz grid (~344 presents/5.5 s), 18 ms wake jitter (no drift
  explosion), 1 s absence → re-anchor, no backlog burst, GetTickCount wrap, gate
  flips take effect the same poll, idle sleep bounded 1..16, gate off = zero marks +
  the retail sentinel, stats window math (~312 presents vs ~125 ticks per 5 s).
- Consumer sweep: the only `fps_drawn`/`fps_skip` readers outside the frame loop are
  the lightmap quality ladder (top quality at both 25 and 60) and the `/fps` debug
  overlay (cosmetic) — the stats split is purely additive.
- Field ask: **SMOKE §S items 61–64** — the d2boot presents/ticks pair (sim identity
  by counting), the stopwatch walk/attack check, the 60 Hz cursor/tooltip feel on a
  60 Hz panel (vsync on and off), and the boundaries (pause/loading/cutscenes/menus +
  the mid-session flip both ways).
- d2boot.log line 1 must say `Vanilla+0.0.8-R142`.

## Still owed (unchanged from rev 152)

- §M2 `gamble:` log-chain discriminators, §M3 one purchase, §M4 the same-session
  re-open (the §M1 click is green on the salvage build).
- The explicit 16-bit gossip log check (the `msgType0=` mark + the 0x31 echo).
- SMOKE ride-alongs on any future build; §P item 54's 1366x768 `-w` select doubles as
  the R116 eyeball.
- §7 triages: #25 (track R, shrine XP buff), #18 (track Q, attribute-point crash).
- The unlock-fps follow-on when the field pass lands: sub-tick interpolation
  (camera → missiles → units, plan §4 Phase 2) and, if wanted, the numeric target
  (30–240) — both scoped in the plan.
