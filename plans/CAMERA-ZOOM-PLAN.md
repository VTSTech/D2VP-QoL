# Camera Zoom (Z-key) — feature plan

*Drafted 2026-11-09 on owner ask ("Lets make a plan for z to zoom too"), for slate row 0a —
the Task C item (handoff §9 item 3) that has been scoped-but-unstarted since the slate was
ranked. D2R parity: the launch feature — **Z** toggles the camera between zoom steps.
**Round 1 (tree-verify)** pinned the eight anchors below (2026-11-11). **Round 2 (implement
@ R155) is complete** — shipped this round, docs-gate green, overlay zip
`d2vp-r155-overlay.zip` verify OK; **field-verified by the owner 2026-11-11** ("z to zoom
works as intended in D3D"). Owner calls settled 2026-11-11 (§9): in-steps only,
Z only, D3D renderer only first, level 0 each join. **The Glide port shipped @ R156**
(owner order 2026-11-11) — the R155 D3D site set mirrored into Glide.cpp: helpers, the two
perspective funnels, the top-down quads, the two flat subtile builders; the clamp lifted
for renderer mode 4; the lightmap halftone lattice is a documented v1 limit (§8). **Round 3
(field QA) pending** — SMOKE-CHECKLIST §V items 81–84 (D3D) + 85–87 (Glide) are the ask.*

---

## 1. Goal and non-goals (v1)

**Goal.** With `[features] camera_zoom=1`, **Z** steps the world zoom **1.0 → 1.25 →
1.5 → 1.0** (owner call: in-steps only, no free-form zoom, no wheel). The world scales
about the character; the UI does not (pixel-1:1); the cursor stays truthful — every hover,
tooltip, walk and cast lands where the zoomed pixels are.

**Non-goals for v1** (each is a clean follow-up, deliberately deferred):
- ~~No Glide/DDraw/Gdi path~~ — **Glide shipped @ R156** (the site-set mirror described in
  the status line; the clamp now covers DDraw/Gdi only). The gaGlidePerspective vertices
  needed no work: every perspective layer already funnels through Glide's
  `AdjustPerspectivePosition`/`PerspectiveScalePosition`, the byte-analogs of the D3D pair.
- No smooth/stepped zoom animation — instant step, retail D2R shape.
- No reach/cull-rect re-fit for zoom-OUT (v1 only zooms IN: 1.25/1.5 — the tile-walk sees
  FEWER tiles, so the retail view rect never shows a hole; zoom-out would need the §7
  reach knobs fed).
- No overhead nameplate/HP-bar repositioning (they keep their unzoomed anchor offset at
  1.25/1.5 — accepted v1 limit, §8).

## 2. Round 1 anchors (the pins the build rides)

1. **Input idiom**: `SHIFTCOMPARE_ClickModifierDown` (VanillaPlusCore.cpp) is the tree's
   per-frame key shape (CMD_IsCommandBound/CMD_GetKeyForCommand + `GetAsyncKeyState < 0`).
   **Retail ships no zoom CmdTbl command id**, so v1 polls vk 0x5A ('Z') directly, once
   per frame from `VANILLAPLUS_CameraZoomFrame()` (Game.cpp InGameDraw, before the world
   bracket).
2. **The world phase is one bracket**: `View::ViewDraw2` (View.cpp:1025) called from
   `InGameDraw` (Game.cpp:1103 pre-R155) is the single world pass —
   `BeginScene → world (ViewDraw2) → UI (DRAW_UI, cursor) → EndScene → Present`. One
   world-phase flag set/cleared around that one call separates world from UI with no
   per-call classification.
3. **Perspective projection funnel**: `AdjustPerspectivePosition` +
   `PerspectiveScalePosition` (Direct3D.cpp) — every perspective layer (floor/roof light
   grids via `D2GFX_AdjustPerspectivePosition` Draw.cpp:614/:663/:686, wall/shadow/ground
   fans, unit sprite quads `DrawPerspectiveImage`:4019, shadows Gfx.cpp:782/:3374,
   weather Weather.cpp:1284) projects through them. Scaling their outputs about their own
   anchors (W/2, H/2−0x20 / W/2, H/2−0x10) scales positions AND fan/quad geometry in two
   functions.
4. **Top-down emitters**: sprites funnel into `RENDERER_DIRECT3D_DrawPalettedQuad`
   (:3903, via DrawImage/DrawShiftedImage/DrawClippedImage/DrawImageFast) and
   `RENDERER_D3D_DrawShadowQuad` (:3829) — the four local geometry quantities + anchor
   position scale exactly like retail's own `g_aSmackerSurfaceArray` perspective block
   does. Flat subtiles: `RENDERER_D3D_BuildWallTileVerticesNoPerspective` (:5352) +
   `RENDERER_D3D_BuildShadowTileVertices` (:5167) — position + the 0x20 span, cull on
   the scaled coords. Top-down floor: the 6x6 ortho light grids (`DRAW_CalcRoofLightGrid
   Ortho` / `DRAW_CalcRoofLightOrtho`, Draw.cpp) — their cell positions ARE the
   `DrawGroundTile` VB vertex positions; scale the cells, the floor follows.
5. **Cursor truth = three links** (the R112 hosted-stop lesson: message-space pixels map
   into the space the game's mapping speaks): (a) raw mouse enters per-frame via
   `SELECT_HandleMouseInput` (Select.cpp:380 — reads Cursor::GetMouseX/Y, clamps to the
   HUD exclusion rect in RAW pixel space, then projects); (b) the click/cast/walk path
   enters via `ClickMap` (Player.cpp:2181) → `ConvertCoordToPerspectiveView`
   (Path.cpp:269); (c) the projection itself is `SELECT_ScreenToWorldCoords`
   (Select.cpp:358 — perspective skew + `DRAW_GetWorldOffsetX/Y`, which is
   MouseXOffset−MouseYRelatedB / MouseYOffset−8; the offsets themselves come from
   `DRAW_UpdateMouseWorldOffsets` Path.cpp:426, the player anchored at
   ScreenSizeX/2, ScreenSizeY/2−0x10). **One inverse published before (a)/(b) covers
   hover, selection, walk, cast** — hit rects and targets stay byte-retail.
6. **Rejected: the present path.** `VPRES3D_PresentHostedFrame` (Direct3D.cpp:2487) blits
   the WHOLE composed frame — UI already composited — so scaling there breaks the UI 1:1
   pillar by construction. Not a candidate.
7. **Rejected: retail's scale-factor channel.** `nfpUpdateScaleFactor` is a no-op stub in
   every renderer and `D2GFX_SetDefaultScaleFactor` is rewritten every frame
   (View.cpp:1142) — retail owns it, repurposing it fights the retail flow.
8. **Reach knobs (recorded, not enabled)**: `WALL2_CollectRoomsBeyondAdjacency` (BFS
   depth 4), the R119 reach (`ScreenSizeX*5/32 + ScreenSizeY*5/16 + 450`),
   `rcExtendedView` re-fit — only feed these if a future zoom-OUT round needs more world
   on screen.

## 3. Design (as built)

- **State**: `gnCameraZoomLevel 0/1/2` → factor `0x100/0x140/0x180` (8.8 fixed, the
  units D2GFX's own scale plumbing speaks). Z is an edge poll (high bit now, clear last
  frame); the level wraps 0→1→2→0.
- **Publish point in D2gfx**: `D2GFX_SetCameraZoomFactor` / `D2GFX_GetCameraZoomFactor` +
  `D2GFX_SetCameraZoomWorldPass` / `D2GFX_GetCameraZoomWorldPass` (D2GFX.cpp) — below
  both D2Client and D2Direct3D, so the module gates stay shut; the D3D sites read it
  through `RENDERER_D3D_VPZoomFactor()` (world-flag open ? factor : 0x100).
- **The bracket** (`Game.cpp`): `SetCameraZoomWorldPass(1); ViewDraw2(...);
  SetCameraZoomWorldPass(0);` — UI phase exempt by construction.
- **Cursor inverse**: `VANILLAPLUS_CameraZoomUnproject(&x, &y)` —
  `p = anchor + (p − anchor)·0x100/f`, anchor (ScreenSizeX/2, ScreenSizeY/2−0x10),
  called from `SELECT_HandleMouseInput` (both border branches, after the clamp, before
  `SELECT_ScreenToWorldCoords`) and `ClickMap` (before
  `ConvertCoordToPerspectiveView`).
- **Failure matrix**: non-D3D renderer → factor clamped to 1.0 + one-way
  `camera-zoom: clamped to 1.0 on renderer mode N` mark; gate off → level 0 + factor 1.0
  the same tick; join → level 0 via the `VPCLOCK_ResetSession` slot; cursor misaligned →
  the whole feature is one ini key away from off.

## 4. Round 2 verification (this round)

docs_gate ALL PASS @ R155 (options counts 15/15/8 + geometry + template coverage;
FEATURES "fifteen features ship"; handoff §2; round identity; VP-marker pairing +
brace/paren balance on the changed TUs — with the gate's comment stripper upgraded to a
state machine, see below); smoke_sync 0 warnings (§V synced). Overlay zip
`d2vp-r155-overlay.zip`: 19 entries byte-identical, 6 content markers + the absent
`D2VP_ROUND "154"` check. Compile gate of record: the owner's Win11 MSYS2 build.

## 5. Known v1 limits (accepted; riders for a later round)

1. Overhead name/HP plates keep their unzoomed anchor offset at 1.25/1.5 (they project
   through the funnels but their draw sites anchor differently) — fix rides the Glide
   port / round 3 feedback.
2. Perspective blob shadows keep retail SIZE (positions scale through the funnel;
   `g_aSmackerSurfaceArray` is deliberately not zoomed — it is read outside the bracket
   too, stale between frames, so folding the factor in would leak the zoom onto UI
   quads).
3. The `AdjustPerspectivePosition` anchor (H/2−0x20) disagrees with the position funnel
   (H/2−0x10) by 0x10 px — ≤ 8 px residual click bias at 1.5. Instrument: the Wall2
   click-debug overlay; kill-switch: the ini key.
4. Glide (R156): the lightmap halftone lattice (`RENDERER_GLIDE_DrawLightMap`) is baked
   once at init (`bGlideLightMapPointsBuilt`) — its dots keep their retail screen spots at
   1.25/1.5 while the world scales (a soft additive-glow offset). Field QA (SMOKE §V item
   87) decides if a per-factor lattice rebuild rides a later round. Glide blob shadows:
   same retail-size rule as D3D (`gnGlidePerspectiveScale` untouched).
5. Tool repair shipped with the round: `tools/docs_gate.py`'s comment stripper is a
   state machine now — the old ordered-regex chain truncated `/* {@url https://...} */`
   at the `//` and corrupted the brace count (D2GFX.cpp pre-existing, surfaced when the
   file entered the changed set), and a `/*` inside a `//` doc (Select.cpp "/*pX/*pY")
   would over-eat the other way. Also landed the missing
   `// VP-END [issue #26 / R92]` pair in Direct3D.cpp's `CalculateViewportRect` (the
   BEGIN shipped at R112 unpaired; the gate only audits changed files, so it surfaced
   now).

## 6. Roadmap

- **Round 3 — field QA @ R155/R156**: SMOKE §V items 81–84 (D3D: Z steps + UI 1:1; click
  truth in perspective; zoom + panel open + no_panel_shift; top-down + negatives) + 85–87
  (Glide: steps + UI 1:1 in both camera modes, click truth, the lightmap note); Wall2
  click-debug overlay if 82/86 report drift. D3D side field-verified 2026-11-11.
- **Round 4+ (field-green only)**: overhead-plate reposition; blob-shadow size rider;
  the Glide lightmap lattice rebuild (if 87 reads as a defect); zoom-OUT with the §2.8
  reach knobs.

## 7. Exit

v1 exits when §6 Round 3 is green on the owner's build: Z steps land, UI stays 1:1 at
every step, clicks/walks/casts land on the zoomed world in both camera modes, the
negatives (gate off, Glide clamp, join reset) hold, and no SMOKE regression appears.
