# Round summary — R155 (rev 171, 2026-11-11): camera_zoom — Z steps the world zoom 1.0/1.25/1.5, UI 1:1, D3D only

## The ask

Owner, 2026-11-09: "Lets make a plan for z to zoom too" (D2R parity — the launch feature:
Z toggles between zoom steps). Plan at `plans/CAMERA-ZOOM-PLAN.md`; Round 1 (tree-verify)
pinned the eight anchors (input idiom, transform funnels, the ViewDraw2 world-phase
bracket, the D3D image/tile sites, the cursor-truth trio, the rejected present-path
design); the four owner calls settled 2026-11-11: **in-steps only** {1.0, 1.25, 1.5},
**Z only**, **D3D renderer only first**, **level 0 each join**. This round = Round 2
(implement @ R155).

## The design (what Round 1 pinned and Round 2 built)

- **Publish point in D2gfx** — `D2GFX_SetCameraZoomFactor` (8.8 fixed, 0x100/0x140/0x180)
  + `D2GFX_SetCameraZoomWorldPass` (the bracket flag). D2gfx sits below both D2Client and
  D2Direct3D, so no module imports UI state and the module gates stay shut. Retail's own
  vestigial `nfpUpdateScaleFactor` channel was rejected: retail writes
  `SetDefaultScaleFactor` every frame (View.cpp:1142) and the D3D stub is a no-op.
- **The world-phase bracket** opens around the `ViewDraw2` call (`Game.cpp` InGameDraw,
  after `DRAW_UpdateMouseWorldOffsets`, alongside the per-frame
  `VANILLAPLUS_CameraZoomFrame()` poll) and closes right after: the renderer scales only
  world-phase draws, so the bottom bar, panels, tooltips, automap and cursor are exempt by
  construction — the UI 1:1 pillar holds with no per-call classification. Input and
  selection run outside the bracket and see retail geometry.
- **Perspective mode scales in one funnel**: `AdjustPerspectivePosition` +
  `PerspectiveScalePosition` post-scale their outputs about their own anchors (W/2,
  H/2−0x20 and W/2, H/2−0x10) — every perspective layer (floor/roof light grids, wall/
  shadow/ground fans, unit sprite quads via `DrawPerspectiveImage`, shadows, weather)
  projects through them, so positions AND fan/quad geometry scale in two functions.
  `g_aSmackerSurfaceArray` (the depth sprite-size factor) is deliberately NOT zoomed: it
  is read outside the bracket too (stale between frames), so folding the factor in would
  leak the zoom onto UI quads.
- **Top-down mode scales at the emitters**: `DrawPalettedQuad` + `DrawShadowQuad` (the
  four local geometry quantities ride the factor, the anchor position scales about the
  window anchor — the same shape as retail's perspective smacker-scale block), the two
  flat subtile builders (`RENDERER_D3D_BuildWallTileVerticesNoPerspective`,
  `RENDERER_D3D_BuildShadowTileVertices` — position + the 32 px span, cull on the scaled
  coords), and the client ortho floor/roof light grids (`Draw.cpp`
  `DRAW_CalcRoofLightGridOrtho` / `DRAW_CalcRoofLightOrtho` — the grid cells ARE the
  ground-tile vertices; `DrawGroundTile` writes them straight into the floor VB).
- **Cursor truth** (the R112 hosted-stop lesson: message pixels ride the space the game's
  mapping speaks): the raw cursor pixel is inverse-zoomed about the player's screen anchor
  (ScreenSizeX/2, ScreenSizeY/2−0x10) BEFORE the retail projection — at
  `SELECT_HandleMouseInput` (hover/selection, both border branches) and `ClickMap`
  (walk/cast — the walk target, spell ground target and attack all resolve through it).
  Downstream stays byte-retail: clamps, skew, MouseOffsets, hit rects, the SCmd packets.

## What changed

1. `Source/D2gfx/D2GFX.{h,cpp}` — the publish point: factor (clamped [0x100, 0x180]) +
   world-pass bracket flag, four accessors.
2. `Source/D2Direct3D/Renderer/Direct3D.cpp` — the zoom read-point
   (`RENDERER_D3D_VPZoomFactor` / `RENDERER_D3D_VPZoomScalePoint` helpers) + five scaled
   sites (the two perspective funnels, the two top-down quad emitters, both flat subtile
   builders).
3. `Source/D2Client/GAME/Draw.cpp` — the two ortho light-grid fills scale their cell
   positions (world-phase gated).
4. `Source/D2Client/GAME/Game/Game.cpp` — the per-frame poll + the world bracket around
   `ViewDraw2`.
5. `Source/D2Client/GAME/Select.cpp` + `Source/D2Client/UNIT/Player.cpp` — the cursor
   inverse at the two projection entries.
6. `VanillaPlusCore.{h,cpp}` — `VANILLAPLUS_CameraZoomActive` (the shared 2 s hot-read
   gate, cache `dwCameraZoomLastCheck` busted in `D2VP_ConfigReloaded`),
   `VANILLAPLUS_CameraZoomFrame` (Z edge poll vk 0x5A, level wrap 0→1→2→0, non-D3D clamp
   with a one-way mark, factor re-published every frame),
   `VANILLAPLUS_CameraZoomUnproject`, `VPCAMERAZOOM_ResetForJoin` (wired into
   `VPCLOCK_ResetSession` — the automap-clock game-entry slot); d2boot marks = activation
   + one per Z step (level/factor/perspective/renderer); the documented `camera_zoom=0`
   block appended to the ini template.
7. `VanillaPlusOptions.cpp` — the fifteenth row (`Camera Zoom`); the page re-balances to
   two columns of EIGHT (`VPP_ROWS_PER_COL 7→8`): back row 0x144→0x164, box
   0x171→0x191 (401 px — the exact R108 geometry; fits 640x480 whole, top margin 39 px).

`Redivivus/d2_boot.h`: `D2VP_ROUND "154"→"155"`.

## Invariants held by construction

- Gate off / missing ini = factor 1.0 re-published every frame; every scaled site no-ops
  at 0x100 (byte-for-byte retail).
- UI 1:1: the bracket is the only gate the renderer reads — UI-phase draws never scale.
- Input truth: hit rects, walk targets and packets are computed outside the bracket from
  retail math; only the cursor pixel is remapped.
- Non-D3D renderers: the D3D sites are the only scaled paths; the client clamps the
  factor to 1.0 with a one-way d2boot.log mark (Glide port = a later round).

## Known v1 limits (plan §8, accepted with the owner)

- Overhead name/HP plates keep their unzoomed anchor offset at 1.25/1.5 (fix rides the
  Glide port / a later round).
- Perspective blob shadows keep retail size (positions scale through the funnel; the
  depth-size factor is untouched).
- The `AdjustPerspectivePosition` anchor (H/2−0x20) disagrees with the position funnel
  (H/2−0x10) by 0x10 px — ≤ 8 px residual click bias at 1.5, verifiable against the Wall2
  click-debug overlay in the field round.

## Verification

Static — docs_gate (options-page counts 15/15/8 + geometry + ini-template coverage;
FEATURES "fifteen features ship" == 15 rows; handoff §2 count; round identity
boot/handoff/CHANGELOG/ROUND-SUMMARY; VP-marker pairing + brace/paren balance on the
changed TUs) + smoke_sync 0 warnings (§V items 81–84 + item 0/2 synced). Compile gate of
record: the owner's Win11 MSYS2 build (overlay zip `d2vp-r155-overlay.zip`).

## Field ask (the plan's Round 3, pending)

SMOKE-CHECKLIST §V: Z steps + UI 1:1 (81), click truth in perspective (82), zoom + panel
open + `no_panel_shift` (83), top-down + negative checks + the Glide clamp (84) — with
the Wall2 click-debug overlay as the instrument if item 82 reports drift.

Round identity: self-ID `Vanilla+0.0.8-R155`, automap tag `D2VP-R155`
(`D2VP_ROUND "154"→"155"`).
