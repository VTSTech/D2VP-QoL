# Presentation FPS unlock — feature plan

*Drafted 2026-11-09 on owner ask (chat, no issue). The ask, verbatim:* "How can we unlock
the FPS to 60 in D3D fullscreen?" *followed by the constraint that scopes it:* "I know that
the 25FPS is tied to the game tick rate and that that is a limiting factor. I want to know,
now that we have the source code, can we maintain the default tick rate while unlocking the
presentation FPS". *The same-session recon answered **yes** — the 25 FPS presentation is a
draw gate in the client frame loop, not a structural coupling; the simulation paces itself
independently and never has to change. Owner constraints not yet set — the open calls are
§9. Slate row 17 (`[features] render_fps`). Status: **PLANNED — no code yet.**

---

## 1. Goal and non-goals (v1)

**Goal.** With `[features] render_fps=1`, the game **presents at 60 Hz while the simulation
stays at retail 25 Hz** — server ticks, client world updates and every gameplay timer run
byte-exact retail cadence; only the draw+present cadence decouples. The immediate wins are
a 60 Hz software cursor, hover/tooltip/target-highlight responsiveness and up to ~40 ms
less input-to-photon latency; the follow-on interpolation work (§4 Phase 2, deliberately
deferred) is what makes world *motion* look 60.

**Non-goals for v1** (each is a clean follow-up, deliberately deferred):
- **No sub-tick interpolation in v1** — v1 is the decoupled present pacer only; world state
  still updates at 25 Hz, so movement cadence looks retail (with better latency). Camera →
  missile → unit interpolation is the follow-on arc (§4 Phase 2 sketch), its own round(s).
- **No tick-rate changes of any kind.** `SrvProcessAllGames`, `ClientSideGameLoop` and the
  frame-skip math keep the 40 ms cadence. The retail `GAMETYPE_SINGLEPLAYER_UNCAPPED`
  QA path is explicitly **rejected** as a mechanism — see §3 (it speeds the *server*).
- No options beyond the 60 Hz target in v1 (a numeric target value is §9 call 1).
- No rendering-path changes: the pacer lives upstream of the renderer, so Glide, D3D and
  DDraw all inherit it unchanged (field matrix still walks each — §8).

## 2. The hard invariants (pillars)

1. **Simulation stays byte-exact retail.** With the gate on, the number of server ticks and
   client world updates per wall-clock second is unchanged. Verified in the field by
   counting, not eyeballing (§8): animation frame advances and walk speed over a fixed
   route must be identical gate-on vs gate-off.
2. **Default OFF = vanilla byte-for-byte.** With `render_fps` missing/0 the draw gate, the
   Sleep(10) idle pacing and the EndScene 5 ms budget are untouched — predicate-false at
   every site (contract rule 1).
3. **One pacing owner.** Under the gate exactly one new pacer owns the present deadline;
   the old draw gate (frame-skip counter) stops gating draws and keeps its tick-side
   meaning. No second Sleep competing with EndScene's budget (rule 4's choke point).
4. **The user's input never waits on the pacer.** Input rides the message pump as today;
   the pacer only decides when to *draw*, never when to poll.

## 3. Where things live today (recon, this tree — all sites read, none yet verified under load)

**The cap itself — one draw gate:**
- `CLIENT_GameLoopFrame` — `Source/D2Client/GAME/Game/Game.cpp:2670`: the in-game
  MessageGameLoop callback. `uGlobalTickCountB = 0x28` (**40 ms**, line 2672) is the tick
  budget. The draw is gated to once per tick: `if (!gnFrameSkipCounter) {
  gClientGameData.CurrentDrawFunction(0); ... }` (lines 2832–2838) with
  `gnFrameSkipCounter++` after (2846). That gate is the whole 25 FPS *presentation* cap.
- The pump — `MessageGameLoop`, `Source/D2Client/Message.cpp:183`: calls the callback every
  free iteration, `Sleep(10)` when idle and no mouse-button event is pending (224–226).
- The SP server tick — `SrvProcessAllGames(1)` runs **every** pump iteration in SP
  (`Game.cpp:2760`) and paces itself internally: `Frames_TimeBetweenFrames = 1000/25`,
  `Frames_PerSecond = 0x19` (`D2Game/GAME/Server.cpp:3137–3165`, 184). Nonzero return = a
  real server tick happened this iteration → `ClientSideGameLoop()` (`Game.cpp:903`, the
  25 Hz client world update) → the draw window.
- Tick-side frame skip — `ComputeFrameSkipLevel` (`Game.cpp:1207`) +
  `UpdateFrameStats`/`fps_drawn` (`Game.cpp:1225`): anti-runaway for *ticks*, and its
  counter doubles as the draw gate today. Under the pacer the two roles must split (§4).

**The present path (D3D fullscreen, the ask):**
- `D2GFX_EndScene` — `D2gfx/D2GFX.cpp:989–1022`: dispatches backend EndScene1/2, with a
  5 ms anti-spin budget (`gdFrameSleepRemainder`) ahead of it. Not a 25 FPS limiter — a
  spin guard that becomes counterproductive at 60 Hz (eats a third of a 16.6 ms frame).
- The actual fullscreen present — `EndScene2`, `D2Direct3D/Renderer/Direct3D.cpp:550–570`:
  DirectDraw7 `Flip(backbuffer, nullptr, 1)` vsync'd or `9` (WAIT|NOVSYNC) per
  `bVSync`; hosted/wide stops ride `VPRES3D_PresentHostedFrame` (2487) instead. The Flip
  happens whenever called — **the flip is not the limiter**. EndScene is shared by all
  renderers, so a client-side pacer is renderer-agnostic by construction.

**Per-tick unit state (the Phase 2 fuel, recorded here while fresh):**
- `CLIENT_UpdateUnitGfxState` (`D2Client/UNIT/CUnit.cpp:1785`) → `CLIENT_UpdateUnitFrame`
  (`D2Client/UNIT/Path.cpp:359`) advance path/gfx/animation state once per
  `ClientSideGameLoop`.
- `D2DynamicPathStrc` (`D2Common/D2DynamicPathStrc.h`) already carries fixed-point
  **per-tick deltas**: `Coords` (+0x00), `ScreenPos` (+0x08), `nVelocityX/Y` (+0x72/0x76).
  Sub-tick render position = `pos + velocity × α`, `α = ms_since_tick / 40` — no snapshot
  buffer needed for v2; a render-time shadow, never written back.

**The trap (wrong tool, in-tree warning):** `GAMETYPE_SINGLEPLAYER_UNCAPPED`
(`d2_enums.h:3035`) + `CONNECTIONTYPE_SINGLEPLAYERUNCAPPED` (`d2_enums.h:3026`) is retail's
own QA game type — it speeds the **server** (skips a 500 ms packet throttle,
`D2Net/SRC/D2Net.cpp:54`; dispatch bypass), i.e. a game-*speed* unlock that changes
gameplay. Not this feature; the plan touches none of it.

**Gate plumbing (all shipped precedent):** `D2VP_GetFeatureToggle`
(`D2Client/UI/VanillaPlusCore.cpp:948` — the 2 s hot-re-read choke point),
`ALTTOGGLE_WriteIniTemplate` (VanillaPlusCore.cpp:182, contract rule 3), options rows in
`D2Client/UI/VanillaPlusOptions.cpp`. `timeGetTime` is already imported
(`d2_platform.h:704`), so `timeBeginPeriod(1)` (same winmm import) is available for stable
sleep granularity — verify at implement time.

## 4. Design (recommended shape)

**Gate**: `[features] render_fps` — bool in v1, `1` = 60 Hz present target (numeric target
values 30–240 = §9 call 1). Ini template entry + a Vanilla+ Options row (row 11 — call 4
decides now or ini-only first). d2boot.log mark on pacer enable + present-rate stats in the
existing telemetry habit.

**Phase 1 — the decoupled present (v1, this plan's rounds):**

- In `CLIENT_GameLoopFrame`: the tick gate stays exactly as-is for the SP server tick,
  `ClientSideGameLoop()` and the elapsed/catch-up math. The draw block
  (`CurrentDrawFunction(0)` + stats) moves out from behind `gnFrameSkipCounter` to behind a
  **present pacer**: a static deadline (`nextPresentTick`); when `now >= deadline`, draw
  and add the target period (16 ms at 60). If we arrive >1 period late (slow GPU), draw
  once and re-anchor — skip, never queue (the frame-skip lesson, moved to the present side).
- `MessageGameLoop` idle branch: under the gate, replace the fixed `Sleep(10)` with
  sleep-until-pacer-deadline (clamped ≥1 ms). With `timeBeginPeriod(1)` the granularity
  supports 16 ms; without it, Sleep rounding lands 10–15 ms — verify, then decide (§7).
- `D2GFX_EndScene`: under the gate the 5 ms budget drops to 1 ms — its job (don't spin at
  25 Hz) is gone once the pacer owns the sleep.
- Stats split: `fps_drawn`/`framesDrawn` keep meaning "frames actually presented"
  (ComputeFrameSkipLevel's `>0x1c → skip 0` then reads 60 and stays harmless); the new
  present-rate and tick-rate counters go to the d2boot telemetry so field reports can prove
  pillar 1.
- Untouched brackets, verified not changed: the esc-menu pause path (`uGlobalElapsed`
  reset, `Game.cpp:2690` — the world freezes because state freezes; the cursor keeps its
  60 Hz redraw, which is correct), the cutscene early-out
  (`EndCutSceneReturnSceneState`, Bink paces itself), and the loading-screen loop
  (`DRAW_LowLevelDrawing` / `GameLoopFuncInitGame` keep their own pacing).
- MP branch: `didTick` keeps gating `ClientSideGameLoop()`; the draw decouples identically.

**Phase 2 — sub-tick interpolation (follow-on arc, sketched):**

- Render-time only: `renderPos = pos + velocity × α` with `α` from the pacer's tick
  boundary; write to a draw-pass shadow, never into `D2DynamicPathStrc`.
- Order of value: **camera first** (one vector — the player-follow; the world scroll going
  60 Hz kills most perceived judder), then **missiles** (fast movers show the worst
  tick-step), then remaining units. Animation frames stay tick-locked — the D2R-era look.
- The camera site is the one anchor this recon has not pinned yet — Round 1 item.

## 5. Failure and recovery matrix (R61 pattern — degrade to mark, never halt)

| Event | Behavior |
|---|---|
| Gate flipped OFF mid-session | Next tick returns to the retail draw gate + Sleep(10) pacing; byte-identical from that frame |
| Sleep granularity too coarse (no timeBeginPeriod) | Pacer drifts late, never busy-waits; presents land on the next wakeable tick; mark in d2boot.log once |
| GPU can't hold 60 (frame cost > period) | Pacer skips presents (re-anchor), tick path unaffected; present-rate counter shows the shortfall |
| Loading screen / cutscene active | Those loops keep their own pacing — the pacer only lives in the in-game callback |
| Renderer present path busy (hosted-stretch route) | Unchanged code path; the pacer sits upstream of EndScene for every backend |
| future row 0a (camera zoom) interaction | Both touch the present path — land zoom's scale on the pacer's frame boundary; noted in both plans |

## 6. Roadmap — three code rounds, each field-verifiable

- **Round 1 — tree-verify**: pin the draw-block boundaries in `CLIENT_GameLoopFrame` and
  confirm the caller-owned stats move cleanly; confirm the draw pass has no other
  once-per-tick assumptions (grep `framesDrawn`/`gnFramesDrawnCounter` consumers); pin the
  camera anchor for Phase 2 while in the neighborhood; Sleep-granularity micro-bench with
  and without `timeBeginPeriod(1)`; TU list frozen.
- **Round 2 — implement**: gate + pacer + decoupled draw + EndScene budget knob + stats
  split + ini template + options row + d2boot marks; self-ID roll per the R139 define.
- **Round 3 — field QA**: §8 exit loop on the owner's Win11 build; new SMOKE-CHECKLIST
  section rows land with this round; the matrix (D3D vsync on/off × 60 Hz panel ×
  windowed/fullscreen; Glide + DDraw spot-check) walked once, then spot-checked.

## 7. Risks

- **Perf at 60 Hz** — draw cost ×~2.4 vs 25 Hz; today the renderer idles 37.5 ms of every
  40. Bounded by the skip-never-queue pacer and the present-rate counter; the -O0 debug
  build is the worst case and the owner's machine is the judge (the R118 FPS-watch
  precedent).
- **Pacing wobble** — Sleep(10) granularity vs a 16.6 ms period; `timeBeginPeriod(1)`
  from the already-imported winmm is the fix, verify before shipping the pacer.
- **Semantic drift in the frame-skip stats** — ComputeFrameSkipLevel's inputs must stay
  tick-side; the stats split is load-bearing, not cosmetic (a wrong split hides real
  skips in field reports).
- **"Feels faster" reports** — presentation FPS changes perception of game speed without
  changing it; the §8 counting test exists to rebut it, and the README row will say
  "sim stays 25 Hz" in as many words.

## 8. Exit (all must hold on the field build)

- Tick-rate identity: with a fixed route + stopwatch, animation advances, walk time and
  monster behavior identical gate-on/off (count ticks from the new telemetry — no eyeballs).
- `render_fps=0`/missing from a fresh start = vanilla byte-for-byte; save → load unaffected
  (nothing render-side persists; no save-format surface at all).
- 60 presents/s on a 60 Hz panel with vsync on (Flip throttles — expected and clean);
  NOVSYNC behavior sane on the owner's high-refresh panel, no tearing complaints.
- Pause, cutscene, loading screens behave retail; Alt-label/tooltip/cursor responsiveness
  visibly better (the actual point).
- SMOKE rows for the touched surface pass on the R142+ build; handoff ledger row written.

## 9. The four owner calls

1. **Bool vs numeric** — v1 draft is `render_fps=1` → fixed 60 Hz target (contract-fit,
   one answer to document). A numeric target (30–240, clamped) is the flexible shape —
   wanted now, or later?
2. **Interpolation scope** — ship the decoupled present first and judge it in the field
   (draft), or pull the camera interpolation into the first ship for the full D2R-feel
   target?
3. **Vsync default under the gate** — leave `bVSYNC` semantics untouched (draft: on a 60 Hz
   panel vsync gives exactly 60; NOVSYNC lets the pacer rule) or expose a per-feature
   override?
4. **Options page row** — add the **Render FPS** row to Vanilla+ Options in the same round
   (the row-10 pitch pattern; eleventh row moves Previous Menu one pitch again), or
   ini-only for a round first?