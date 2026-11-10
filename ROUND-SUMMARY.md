# Round summary — R156 (rev 172, 2026-11-11): camera_zoom Glide port — the Z zoom renders on the Glide renderer

## The ask

Owner, 2026-11-11, after field-verifying the R155 D3D zoom ("Great job, z to zoom works as
intended in D3D", "R155 sent to Github"): "Now lets do Glide." The R155 plan had deferred
the Glide path ("D3D only this round — the Glide port rides the gaGlidePerspective
vertices work"); this round = the port @ R156.

## The design (what the port actually needed)

- **The funnels already existed.** Glide's `AdjustPerspectivePosition` /
  `PerspectiveScalePosition` (Glide.cpp, `namespace D2Glide::Renderer::Glide`) are the
  byte-analogs of the D3D pair — same constants, same anchors (W/2, H/2−0x20 and W/2,
  H/2−0x10). Every perspective layer (floor/roof light grids, wall/shadow fans, unit
  sprites, shadows, weather) projects through them, so the perspective half of the port
  is two post-scale blocks, identical in shape to R155's.
- **Top-down mode scales at the same emitters**: `DrawPalettedQuad` + `DrawShadowQuad`
  (the four local geometry quantities ride the factor, the anchor position scales about
  the window anchor; the perspective branch is skipped — retail's depth-size factor
  `gnGlidePerspectiveScale` stays untouched, the same leak argument as D3D's
  `g_aSmackerSurfaceArray`), and the two flat subtile builders (`DrawWallTileSimple` /
  `DrawShadowTileSimple` — position + the 32 px span, cull on the scaled coords; the wall
  color-table index stays on the raw local offset).
- **Ground tiles needed nothing**: `DrawGroundTile` reads the shared client light grid the
  R155 round already scales in `Draw.cpp` — renderer-agnostic, rides for free.
- **The helpers live at the top of the namespace.** The R155 D3D round shipped
  use-before-declare, then learned the namespace-mangling link lesson (global-scope decls
  + a namespace-wrapped body = two mangled names = undefined references). The port
  defines `RENDERER_GLIDE_VPZoomFactor` / `RENDERER_GLIDE_VPZoomScalePoint` right after
  `namespace D2Glide::Renderer::Glide {` opens — no forward decls to misplace.
- **Clamp lift**: `VANILLAPLUS_CameraZoomFrame` accepts renderer mode 4 (Glide) alongside
  6 (D3D); DDraw/Gdi still clamp with the updated mark; the ini template note says D3D and
  Glide.

## What changed

1. `Source/D2Glide/Renderer/Glide.cpp` — the two zoom helpers (namespace-top) + six scaled
   sites: the two perspective funnels, the two top-down quad emitters, the two flat
   subtile builders. Also landed the missing `// VP-END [issue #26 / R92]` pair on the
   `RENDERER_GLIDE_CalculateScreenBounds` wide-UI block (pre-existing since R92; the gate
   only audits changed files, so it surfaced now — the same class of find as R155's
   Direct3D.cpp fix).
2. `Source/D2Client/UI/VanillaPlusCore.cpp` — the clamp accepts mode 4; the clamp mark and
   the ini template note updated.
3. `Source/D2Direct3D/Renderer/Direct3D.cpp` — build fix on the owner's tree: the R155
   helper forward-decls moved from global scope INSIDE `namespace D2Direct3D::Renderer`
   (the file's body is namespace-wrapped; global decls made the calls mangle to
   unreferenced symbols).

`Redivivus/d2_boot.h`: `D2VP_ROUND "155"→"156"`.

## Invariants held by construction

- Gate off / missing ini = factor 1.0 re-published every frame; every scaled site no-ops
  at 0x100 (byte-for-byte retail) — Glide and D3D alike.
- UI 1:1: the world-phase bracket is the only gate the renderer reads — Glide UI-phase
  draws never scale, exactly as on D3D.
- Input truth: the cursor inverse is renderer-agnostic (it maps the raw pixel into the
  retail world-screen space; the retail projection that follows is untouched).
- DDraw/Gdi: still clamped to 1.0 with the one-way d2boot.log mark.

## Known v1 limits (plan §8, accepted with the owner)

- The Glide lightmap halftone lattice (`RENDERER_GLIDE_DrawLightMap`) is baked once at
  init — its dots keep their retail screen spots at 1.25/1.5 while the world scales (a
  soft additive-glow offset; SMOKE §V item 87 decides if a per-factor lattice rebuild
  rides a later round).
- Glide blob shadows keep retail size in perspective mode (`gnGlidePerspectiveScale`
  untouched — mirrors the D3D rule).
- Carried from R155 (unchanged): overhead name/HP plates keep their unzoomed anchor
  offset at 1.25/1.5; the H/2−0x20 vs H/2−0x10 anchor disagreement (≤ 8 px click bias at
  1.5).

## Verification

Static — docs_gate (round identity boot/handoff/CHANGELOG/ROUND-SUMMARY; VP-marker
pairing + brace/paren balance on the changed TUs; options-page counts) + smoke_sync 0
warnings (§V items 85–87 + item 0 synced to R156). Compile gate of record: the owner's
Win11 MSYS2 build.

## Field ask (the plan's Round 3, pending)

SMOKE-CHECKLIST §V: D3D items 81–84 (81 field-verified by the owner 2026-11-11) + Glide
items 85–87: zoom steps + UI 1:1 in both camera modes (85), click truth (86), the
lightmap note (87) — the Wall2 click-debug overlay is the instrument if 82/86 report
drift.

Round identity: self-ID `Vanilla+0.0.8-R156`, automap tag `D2VP-R156`
(`D2VP_ROUND "155"→"156"`).
