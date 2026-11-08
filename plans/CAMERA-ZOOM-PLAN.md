# Camera Zoom (Z-key) — feature plan

*Drafted 2026-11-09 on owner ask ("Lets make a plan for z to zoom too"), for slate row 0a —
the Task C item (handoff §9 item 3) that has been scoped-but-unstarted since the slate was
ranked. D2R parity: the launch feature — **Z** (or mouse wheel) zooms the camera; PC D2R
toggles between zoom steps. Owner constraints not yet set — the open calls are §9.
Slate row 0a (`[features] camera_zoom`). Status: **PLANNED — no code yet.**

---

## 1. Goal and non-goals (v1)

**Goal.** With `[features] camera_zoom=1`, **Z** toggles the world view between zoom steps
(D2R parity; wheel steps are §9 call 1). The world scales; the UI does not; the cursor stays
truthful — every hover, tooltip and click lands where the pixels are.

**Non-goals for v1** (each is a clean follow-up, deliberately deferred):
- No 8-bit DDraw path — v1 targets the two wide-class renderers the resolution work already
  covers (Glide + Direct3D), same scope discipline as the wide stops (640x480/800x600
  byte-identical).
- No continuous/smooth zoom — discrete steps only (the engine draws whole subtiles; smooth
  scale is a follow-on if the steps feel wrong).
- No automap zoom (the automap is its own pass; it stays retail).
- No zoom persisted across sessions (per-session state, default level 0 each game join —
  §9 call 4 may change this).
- No FOV/perspective changes — zoom scales the existing projection; it does not invent a
  new camera.

## 2. The hard invariants (pillars)

1. **UI stays 1:1.** Tooltips, panels, buttons, the belt — never scaled. The d2dx rule: zoom
   is a world-view transform, not a screen blit. The UI-vs-world discrimination signal is
   the plan's biggest tree-verify item (§3, §7).
2. **Cursor truth.** At every zoom level, the pixel under the cursor is the thing the click
   hits and the thing the tooltip describes. The R112 hosted-stop lesson — the stretch
   ladder had to map host pixels back into the logical frame or clicks landed wrong — is the
   same bug class one level up: a zoom factor published to the game's screen→world mapping.
3. **Default OFF = vanilla byte-for-byte.** Gate predicate-false at every touched site; with
   `camera_zoom=0` the frame path is byte-identical at every resolution, both camera modes.
4. **Renderer parity.** Glide and D3D ship the same levels in the same round. The wide-stop
   arc taught this the hard way (the R109 field matrix forked Glide-green vs D3D-crash);
   one renderer first is how a round dies.

## 3. Where things live today (recon, this tree — the groundwork the fork already paid for)

The fork has already solved, for other reasons, most of what a zoom needs:

- **Seeing more world than retail's view rect** (the zoom-out problem): the R117–R122 wide
  arc — `WALL2_CollectRoomsBeyondAdjacency` (Wall2.cpp, BFS depth 4, room caps 256), the
  floor pass drawing hop-2/3 rooms, R119's reach in game coords
  (`W*5/32 + H*5/16 + 450` at 1080p), R122's `rcExtendedView` re-fit to the live backbuffer
  (`SetViewRectangleSize`), and the fill bands keyed on `VPRES_IsWideLayoutMode`. Zoom-out
  levels feed these same knobs a scale factor as an extra input — reuse, don't fork.
- **Scale-at-present + inverse cursor mapping**: the R112 hosted-stop stretch ladder —
  `VPRES3D_PresentHostedFrame` (`D2Direct3D/Renderer/Direct3D.cpp`) presents a stretched
  frame and maps host pixels back into the logical frame so clicks land where the stretched
  pixels are. That is the exact shape a zoom factor needs at the D3D present path.
- **Projection-level code ownership**: the camera-mode system the R120 vpdiag dumps treat as
  a first-class toggle — Glide's perspective vertex tables and image-size table
  (`gaGlidePerspectiveVertices`, `gaPerspectiveImageSizeTable`, `D2Glide/Renderer/Glide.cpp`)
  and the ortho path beside them. The fork already reads and re-fits the projection inputs
  in both renderers.
- **Renderer funnels**: Glide — `grDrawVertexArray` / `grDrawVertexArrayContiguous` /
  `grDrawTriangle` / `grDrawLine` call sites (Glide.cpp); D3D — the present/transform path
  R112 already owns. A vertex- or present-side scale intercepts here (the d2dx/d2gl class).
- **Input**: the row 0b cmd-key read idiom — `GetAsyncKeyState(vk) < 0` per frame with the
  CmdTbl virtual-key honored (remap-safe), `VanillaPlusCore.cpp` — a Z default slots in
  beside it; wheel would ride the mouse-wheel message path (verify site).
- **Rejected class**: engine-side world-to-screen rescale (the hackmap-era zoom) — it
  fights the game's own click mapping instead of owning the projection. Renderer-side scale
  with a published inverse map is the plan.

## 4. Design (recommended shape)

**State**: one VP zoom level per game session, `0` = off/retail. Levels are a small literal
table (e.g. `1.0 / 1.25 / 1.5` in-steps; out-steps below 1.0 are §9 call 4 — they are where
the reach knobs engage). Z is edge-triggered (level++ wrapping to 0, D2R toggle feel);
wheel steps up/down if shipped.

**Per frame**: read Z (the §3 input idiom) → level transition? → publish the level to the
three consumers below (a single `VPZOOM_Current()` read — rule 4's one choke point).

- **World path** (per renderer): scale the world projection about the **player anchor** —
  Glide: scale vertices at the `grDraw*` funnel; D3D: scale in the present path R112 owns.
  Zoom-in sees less world (trivial); zoom-out sees more (the reach knobs in §3 take the
  level as a second input, beside screen size).
- **UI path**: exempt — identified at the funnel and passed through unscaled. The
  discrimination signal is the key verify item: the wide arc already classifies world draws
  inside the floor/wall walks, and UI art flows through the D2Win palette path, so the
  bracket exists — Round 1 pins it exactly.
- **Cursor path**: the zoom level feeds the game's screen→world mapping (hover, tooltips,
  click-to-move/attack, alt-label hit) — the same "clicks land where the pixels are" slot
  R112 already solves for the stretch ladder. One published factor, many readers.

**Gate + surface**: `[features] camera_zoom` (ini template per rule 3) + a Vanilla+ Options
page row (the pitch pattern — row 0b/0c style) + d2boot.log marks on level changes (the
vpdiag habit: field reports carry the level, mode and renderer).

## 5. Failure and recovery matrix (R61 pattern — degrade to mark, never halt)

| Event | Behavior |
|---|---|
| Renderer has no zoom support compiled (future builds) | `VPZOOM_Current()` clamps to 0; log mark |
| Zoom level change mid-panel-open | World re-scales live; UI untouched; no state to unwind |
| Death / TP / area transition | Level persists in-session (world re-anchors on the new player pos) |
| Cutscene / cutscene-class overlay | Zoom clamps to 0 while active (verify the bracket; retail paths first) |
| Cursor mapping mismatch reported (hover lands off) | Field kill-switch: level clamps to 0 next launch via ini override — never ship a lying cursor |
| Gate flipped OFF mid-session | Level forced to 0 on the next tick; frame path returns to byte-identical |

## 6. Roadmap — three code rounds, each field-verifiable

- **Round 1 — tree-verify** (the round the row has waited for): the §3 anchors → exact
  pins; the UI-vs-world discrimination signal pinned per renderer; the screen→world mapping
  site for hover/click pinned (the cursor-truth slot); the reach-knob call sites that take
  the zoom-out input identified; the wheel message path pinned if wheel ships; TU list
  frozen. Effort firms here: the table's M reads **M–L** once two-renderer parity and the
  cursor map are priced in.
- **Round 2 — implement**: gate + level state + input; Glide world scale + UI exempt +
  cursor publish; D3D the same three; zoom-out reach feed; options row + ini template;
  d2boot.log marks; self-ID roll per the R139 one-line define.
- **Round 3 — field QA**: the exit loop (§8) on the owner's Win11 build; new SMOKE section
  rows land with this round; the mode matrix (persp × ortho × 640/800/wide stops × both
  renderers) walked once, then spot-checked.

## 7. Risks — mapped to the class lessons already on the books

- **Cursor lie** (the R112 class, again): the single worst bug — hover/click off by the zoom
  delta. Bounded by the §5 kill-switch and by testing hover truth at every level in the
  field matrix.
- **UI bleed** (a missed exempt draws a panel scaled): bounded by the Round 1 signal pin +
  the panel-open case in the field matrix.
- **Perf at zoom-out × 1080p** (the R119-class cost: more rooms, more tiles, the BFS caps):
  the reach knobs already carry caps that degrade by culling, not by halting; FPS watch is
  an exit item, and the R120 vpdiag counters extend to report zoomed coverage if needed.
- **Mode interaction** (persp × ortho × zoom; the R120 two-mode matrix): zoom must hold in
  both camera modes — the field matrix walks both, the R120/R121 diagnostics are the
  debugging instruments if a mode misbehaves.
- **Wide-stop interplay** (R122's rcExtendedView re-fit vs a zoomed backbuffer-space): the
  re-fit is keyed on the live backbuffer, which zoom does not resize — verify only.

## 8. Exit (all must hold on the field build)

- Hover truth: at every level, the tooltip names the item under the cursor and
  click-to-move/attack lands on the pixel clicked — both renderers, both camera modes.
- UI pixel-perfect 1:1 at every level (tooltips, panels, belt, buttons, orbs).
- Same levels in Glide and D3D; automap unchanged; 640x480/800x600 byte-identical with the
  gate on but level 0.
- FPS sane at 1080p zoomed out in a dense area (owner's call on what "sane" is — the R118
  field round set the precedent of watching it live).
- `camera_zoom=0` from a fresh start = vanilla byte-for-byte; save → load unaffected
  (render-side only, nothing in any save file).

## 9. The four owner calls

1. **Steps & direction** — D2R-style Z toggle between 2 levels (draft default: retail →
   1.25 → 1.5 → back)? Or wheel steps? And how far out, if out-steps ship in v1 at all
   (out-steps engage the reach knobs — in-steps alone are a cheaper first ship).
2. **The key** — Z default (D2R parity), CmdTbl-remap honored, in the ini template?
3. **Renderer scope** — Glide + D3D in v1 (draft), 8-bit DDraw explicitly deferred?
4. **Session persistence** — default level 0 every game join (draft), or sticky per
   session/last-used?
