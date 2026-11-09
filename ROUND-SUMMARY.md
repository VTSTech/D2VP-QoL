# Round summary — R143.2 (rev 158, 2026-11-09): sub-tick interpolation delivery fix

Owner field: **"still looks jittery at 1, looks better at 0"** — the R143.1 two-sample
glide still kicked in the field. Root cause: **delivery wiring, not the two-sample
math**. Three defects, all fixed this round:

1. **The snapshot ran AFTER the draw** (`UNLOCKFPS_NoteClientTick` sat in the
   `didClientLoop` block behind the present gate). On a tick+present iteration the draw
   reads the live post-tick position while the prev/cur pair still described the
   previous segment — renderPos leapt a full tick forward (offset ≈ 0 on the new base)
   and was pulled back the next present: a sawtooth at the tick rate. The sandbox
   loop-order harness reproduces it exactly: an **18 px backward kick per boundary on a
   32 px walking step**. Fix (Game.cpp): the snapshot call moved **before** the draw
   block, so the pair always matches the base the draws are about to read.
2. **The sprite world offset's delta was dead code** — R143.1 added it after
   `DRAW_UpdateMouseWorldOffsets`' `if (!bHasShake) return;`, so in normal play (no
   shake) `MouseXOffset/MouseYOffset` never received it. Unit shadows, state overlays,
   `GFX_IsUnitOnScreen` culling and the mouse world-pick stayed tick-locked against the
   gliding camera — the detached-layer shimmer. Fix (Path.cpp): the delta applies
   before the shake check, unconditionally; shake then adds on top as retail.
3. **The perspective cancel double-added** — with (2) fixed, MouseOffsets already
   carries the delta into perspective-projected coords, so the Wall2 player-sprite
   cancel must SUBTRACT there (it adds top-down, where the view center is the carrier).
   Fix (Wall2.cpp): sign flip behind the existing `bPerspective` flag.

## R143.2 files (5 code + docs)

- `Source/D2Client/GAME/Game/Game.cpp` — snapshot call moved before the draw block;
  the post-draw block keeps only `DRAW_AfterDraw()`.
- `Source/D2Client/UI/Path.cpp` — the interp delta applied to `MouseXOffset/MouseYOffset`
  before the shake early-return.
- `Source/D2Client/GAME/Wall2.cpp` — the player-sprite cancel is perspective-aware
  (adds top-down, subtracts perspective).
- `Source/D2Client/UI/VanillaPlusCore.h` / `.cpp` — R143.2 rationale comments on
  `UNLOCKFPS_GetPlayerRenderOffset` + `UNLOCKFPS_NoteClientTick` (mechanism unchanged).

Docs: CHANGELOG (R143.2 note), README row, SMOKE item 65 + template (R143.2 watch),
FEATURES header/row 17/§17, plan status + §4 as-built + delivery bullet, handoff rev 158.

## R143/R143.1 record (the mechanism this round delivered)

Owner: **"lets continue"**, after publishing R142 (tag `R142`) and field-passing §S-61
(the d2boot presents/ticks pair) + the §S-64 menu flip. Code round: with
`[features] unlock_fps` on, the world now **glides between game ticks at the present
rate** while the simulation stays byte-exact retail 25 Hz. Sub-gate
**`[features] unlock_fps_interp`** (ini-only, default 1, no Options row; `0` = the R142
step-cadence look, hot-read). Self-ID stays **`Vanilla+0.0.8-R143`** (one `D2VP_ROUND`
line; R143.2 ships the same mark string — the round is the zip/self-ID line). Plan:
[plans/FPS-UNLOCK-PLAN.md](plans/FPS-UNLOCK-PLAN.md) §4 Phase 2 as-built (FEATURES.md
row 17).

## What changed across R143+R143.1 (7 code files)

- `Redivivus/d2_boot.h` — `D2VP_ROUND "142"` → `"143"`.
- `Source/D2Client/UI/VanillaPlusCore.h` — `UNLOCKFPS_GetPlayerRenderOffset` declared
  with the write-nowhere contract.
- `Source/D2Client/UI/VanillaPlusCore.cpp` — the **[unlock-fps-interp R143]** pieces,
  revised **R143.1** after the owner's jitter field report: `UNLOCKFPS_GetPlayerRenderOffset`
  now interpolates between two real position samples (`offset = −(cur − prev) × (1 − α)`,
  α = ms since the last client world update / 40 clamped to one tick; the samples are
  taken in `UNLOCKFPS_NoteClientTick` at each client world update) — continuous
  at every tick boundary by construction, where the first cut's velocity extrapolation
  (`pos + screenVelocity × α` via retail's `PATH_GetScreenVelocity`) snapped by its own
  prediction error every tick. One frame-shared offset per present (dedicated epoch
  counter), a >128 px/tick teleport snap, the `unlock_fps_interp` read inside
  `UNLOCKFPS_Active`'s 2 s hot-read (missing key = 1), the activation mark's
  `+ sub-tick interp (two-sample)` suffix, and the ini template block.
- `Source/D2Client/GAME/Game/Game.cpp` — `InGameDraw`: the view center rides the delta
  (the world scroll goes present-rate smooth; the weather per-draw camera delta
  inherits the smooth motion).
- `Source/D2Client/UI/Path.cpp` — `DRAW_UpdateMouseWorldOffsets`: the sprite world
  offset rides the same delta (floor + all world-anchored sprites scroll together; the
  mouse world-pick stays consistent with the rendered frame).
- `Source/D2Client/GAME/Wall2.cpp` — `DRAW_WORLD_Unit`'s player branch: the player's
  sprite rides the delta, which cancels the camera's — the character keeps its exact
  retail screen-fixed position while the WORLD glides (D2's camera follows 1:1).
- `Source/D2Client/Engine/Gfx.cpp` — `DrawUnitShadows` + `DRAW_UNIT_States`: the
  player's shadow and state overlays (auras/buffs) stay glued to the sprite.

Write-nowhere by contract: every site adds the delta to its draw position only;
`D2DynamicPathStrc` is never touched. Monsters/missiles/items ride the world offset
(they glide with the floor); their own per-unit sub-tick smoothing stays open as a
possible R144 refinement. Animations stay tick-locked. Perspective mode: the cancel
sign flips (R143.2), the mode itself stays dead by default
(`bPerspectiveEnabled = 0`). Known minor (bounded ≤ one tick of
travel): the automap arrow and the player's overhead label keep tick positions.

## Docs (6 files + template)

1. `CHANGELOG.md` — the R143 entry + the R143.1 and R143.2 revision notes.
2. `README.md` — the Unlock FPS row gains the interpolation sentence.
3. `SMOKE-CHECKLIST.md` + `.github/ISSUE_TEMPLATE/smoke-test.md` — §S header 61–65,
   item 63's v1 note updated, new **item 65** (the glide + the `unlock_fps_interp=0`
   hot-flip back to the R142 look), R143.2 watch.
4. `FEATURES.md` — row 17 + §17: the Phase 2 as-built bullet (five sites, the
   cancel-on-player shape, the sub-gate, the R143.2 wiring).
5. `plans/FPS-UNLOCK-PLAN.md` — status → R142 published + R143 Phase 2 shipped
   (R143.1 two-sample, R143.2 delivery); §4 Phase 2 as-built + delivery bullet.
6. `handoff.md` — rev 158 (R143.2 headline: the delivery diagnosis, the harness proof,
   the re-asked §S-65).

## Verification

- Sandbox (host gcc — no cross toolchain this session; the compile gate of record
  is your Win11 build): a **loop-order wiring harness**
  (`scripts/interp-r1432-harness.c`, `-Wall -Wextra`, **10/10 green**) simulating the
  60 Hz pacer + the 25 Hz tick accumulator faithfully: the **R143.1 order reproduces
  the field sawtooth** (18 px backward kick per tick+present boundary on a 32 px step —
  amplitude over half the tick displacement, i.e. plainly visible), and the **R143.2
  order is clean** — monotone forward glide, never outruns the sim (no per-present step
  over one tick), no kick at the stop boundary, per-present displacement bounded
  through a direction flip (turn), plus the carried checks: the offset ramp
  (α 0/10/20/30/39 of a (40,20) px step), the late-tick clamp (47 ms → parks on cur),
  the teleport snap (>128 px), standing-still zeros, and the FALSE contract (no
  snapshot → gated, zeroed).
- Design invariants: the delta is zero whenever the gate or sub-gate is off, when not
  moving, before the first snapshot, and clamps at one tick — byte-exact retail at every
  boundary; the sim is untouched (the d2boot presents/ticks pair still proves it).
- Known residual (documented in the plan §4): a bounded velocity wobble (±20% of one
  frame's travel) from the tick gate's 33.3/50 ms firing pattern under the 60 Hz loop —
  position-continuous, far under the old 25 Hz stepping; a sim-phase α clock is the
  R144 refinement if wanted.
- Field ask: **SMOKE §S-65** — walk with the gate on: the world scroll glides, the
  character stays screen-fixed, shadows/auras glued, other units don't shimmer, ticks
  still ~25/s; flip `unlock_fps_interp=0` → the R142 look within ~2 s, and back.
- d2boot.log line 1 must say `Vanilla+0.0.8-R143`.

## Still owed (unchanged)

- §S 62–64 remainder: the stopwatch sim identity, the 60 Hz feel + NOVSYNC spot-check,
  the Esc-pause/loading/cutscene boundary walk (+ the 480p Options-page row check).
- §M2 `gamble:` log-chain discriminators, §M3 one purchase, §M4 the same-session
  re-open; the explicit 16-bit gossip log check; §7 triages (#25/#18); the R116 `-w`
  1366x768 eyeball.
- Possible R144: per-unit/missile sub-tick smoothing at their own draw sites (the
  world-scroll glide already carries most of the win), the sim-phase α clock, and the
  numeric target FPS (30–240) if wanted.
- handoff.md is at 17.8 KB — past the old 16 KB budget line; the next trim-N offload
  should move the R142/R143 round detail into a HISTORY H-record.
