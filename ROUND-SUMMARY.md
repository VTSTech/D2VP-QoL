# Round summary — R151 (rev 167, 2026-11-10): the #38 residual pass — the origin delta shared per present + the selection-shape and WW-sprite riders

Owner field on the R150 build: **"R150 is improved in perspective. still a slight
intermittent stutter. but not constant jitter. was unable to reproduce ctrl+click crash
either"** — [#37](https://github.com/VTSTech/D2VP-QoL/issues/37) (crash NOT reproducible —
hold open until a clean session set, then close) +
[#38](https://github.com/VTSTech/D2VP-QoL/issues/38) (constant jitter gone, an intermittent
stutter left). R150 named one residual and left another class unexamined: per-call clock
sampling, and the two raw-projection sites outside the body/shadow/state trio.

## What changed

1. **The origin delta is computed once per present** (`VanillaPlusCore.cpp`): R150 grew
   `UNLOCKFPS_GetPlayerRenderOffsetOrigin` from R149's single ViewDraw2 call site to FOUR
   (View warp, Wall2 body, Gfx shadow + state overlays) but left it recomputing per call
   off a fresh `GetTickCount` — two sites straddling a 1 ms boundary disagreed by
   delta/40 within the same frame (camera vs body vs shadow), an intermittent sub-pixel
   tear in the glue. Now cached per present epoch (`dwUnlockFpsOriginFrame` +
   `gnUnlockFpsOriginOffX/Y` — the screen-unit accessor's R143.1 pattern).
2. **The hover selection shape rides** (`Gfx.cpp` `GFX_DrawUnitSelectionShape`): the
   perspective branch projected the unit's RAW precision position — for the player that
   is the shape stepping at the tick rate AROUND a screen-fixed body whenever the cursor
   rests on the character: the hover-only residual R150 named, reading as the owner's
   intermittent stutter. Player-gated; other units keep raw (their shape and sprite step
   together, retail-consistent).
3. **The WW sprite rides** (`SkillsBar.cpp` `SKILL_DrawUnitAtRotatedAngle`, Whirlwind's
   rotated-angle draw): raw precision input — the spinning character stepped against the
   gliding world for the whole spin. Player-gated; the `VanillaPlusCore.h` include added.

## Verification

Static only — the R151 suite + `docs_gate` green (compile gate of record: the owner's
Win11 MSYS2 build). Field asks: SMOKE §U +77 (hover shape glued) +78 (WW sprite glides);
73–76 stay from R150.

Round identity: self-ID `Vanilla+0.0.8-R151`, automap tag `D2VP-R151`
(`D2VP_ROUND "150"→"151"`).
