# Transparent HUD — feature plan

*Drafted 2026-11-11 on owner ask ("Lets draft a plan for transparent_hud"). Owner calls
already made: scope = **HUD bar + all panels**, **fixed ~70% alpha**, **always-on when
toggled** (no fade/idle), key = **`[features] transparent_hud`**, recon depth = pins
before code. The remaining open calls are §9. Status: **PLANNED — no code yet.**

---

## 1. Goal and non-goals (v1)

**Goal.** With `[features] transparent_hud=1`, the in-game UI — the bottom HUD bar (orbs,
skill/belt buttons, XP bar) and every open panel (inventory, stash, cube, vendor, quest
log, skill tree, party, waypoint) — renders at a fixed ~70% opacity so the world shows
through while the panels are up. Pure display: every hover, tooltip, drag and click lands
exactly where it does in vanilla, because nothing but the blend changes.

**Non-goals for v1** (each is a clean follow-up, deliberately deferred):
- No 8-bit DDraw path (D2DDraw / D2Gdi) — v1 targets the two wide-class renderers the
  resolution work already covers (Glide + Direct3D), same scope discipline as the
  camera-zoom plan; 8-bit blend tables are a follow-on.
- No configurable opacity — one fixed literal (§9 call 1), not an ini value, not a slider.
- No fade-on-idle / combat-aware behavior — the toggle is a plain always-on gate.
- No automap changes — the automap overlay is drawn outside the UI bracket (verify, §3)
  and stays retail.
- No out-of-game (OOG) menu transparency — MainMenus/D2Launch screens stay vanilla.
- Nothing in any save file; no state to persist; display-only, no input changes.

## 2. The hard invariants (pillars)

1. **Default OFF = vanilla byte-for-byte.** With `transparent_hud=0` (or a missing
   ini/section/key) every touched site takes its vanilla branch; the frame path is
   byte-identical at every resolution, both renderers.
2. **Cursor truth.** The cursor is never translucent and never re-routed: the pixel under
   the cursor is the thing the click hits and the thing the tooltip describes. Alpha
   changes nothing hit-test-wise — this feature cannot lie about geometry because it never
   touches geometry.
3. **Legibility floor.** Text stays readable over a moving world at the fixed alpha. The
   v1 default draws UI-phase art (panels *and* their text) at one alpha; if field QA says
   text washes out, the §9 call 2 carve-out is the escape hatch — but the plan ships one
   rule first, not two.
4. **Renderer parity.** Glide and D3D ship the same alpha in the same round. The wide-stop
   arc (R109 field matrix fork) is the lesson: one renderer first is how a round dies.
5. **One choke point.** The gate is read per frame through the shared 2 s hot-read idiom;
   the draw-mode rewrite happens at one publish site (§4), not sprinkled per panel.

## 3. Where things live today (recon, this tree — the engine already does the hard part)

**The engine already renders translucent sprites.** The whole feature rides an existing
path — no new blending is invented:

- **Glide draw-mode dispatch**: `RENDERER_GLIDE_SetDrawMode` (`Source/D2Glide/Renderer/
  Glide.cpp:2285`) maps `nDrawMode` → blend: **5/7 → alpha-blend mode 1 (opaque
  chromakey — what all UI art uses)**; 3 → 2 (additive); 4 → 3 (modulate); **0/1/2 →
  alpha-blend mode 4 + `grConstantColorValue` 0x40/0x80/0xc0 (25%/50%/75% constant
  alpha)**. `RENDERER_GLIDE_SetAlphaBlendMode` (`Glide.cpp:1282`) is the blend-state
  setter it feeds.
- **D3D draw-mode dispatch**: `SetDrawModeAndGetAlpha` (`Source/D2Direct3D/Renderer/
  Direct3D.cpp:3986`) is the symmetric twin — 5/7 → `SetBlendMode(1)` + alpha 0xff;
  **0/1/2 → `SetBlendMode(4)` + per-draw alpha 0x40/0x80/0xc0**. `SetBlendMode`
  (`Direct3D.cpp:689`) mode 4 = SRCALPHA/INVSRCALPHA (render states 0x13=5, 0x14=6).
  The alpha byte rides into the quad draw — no texture-format change is needed (the
  RGB565 paletted-conversion path has no alpha channel; the constant comes from the
  draw-mode resolver, not the texture — Round 1 verifies the byte's final consumer).
- **The 2×5 site table.** Both renderers implement the same five entry points, each
  resolving `nDrawMode` locally: `DrawImage` (Glide.cpp:2414 / Direct3D.cpp:4122),
  `DrawShiftedImage` (:2453 / :4144), `DrawVerticalCropImage` (:2471 / :4182),
  `DrawClippedImage` (:2599 / :4279), `DrawSolidRectEx` (:3105 / :4667). Any rewrite at
  the D2GFX dispatch layer upstream (the `fpDrawShiftedImage` funnel, `Source/D2gfx/
  D2GFX.h:132` and the export-table slot at :305) covers both renderers at once — the
  zero-renderer-touch variant rewrites mode 5 → mode 2 (75%); the exact-70% variant adds
  one VP case per renderer's mode resolver (§4).
- **Where UI draws enter**: UI art flows through `D2GFX_DrawImage` with `eDrawMode`
  (`Source/D2Launch/MainMenus.h:60` — "the constructor stores DRAW_SOLID (5)"; same
  default in `D2WinImageEx.h:23`, `D2WinAnimImage.h:32`), plus the D2Win helpers
  (`IMAGE_DrawDC6ImageOnScreen`, `Source/D2Win/Src/D2WinFont.h:80`; solid-rect text
  backings, `D2WinFont.h:93/97/101/103`). World sprites use the same mode-5 funnel —
  **draw mode alone cannot discriminate UI vs world; the discriminator is the phase
  bracket** below.
- **The UI-phase bracket**: the in-game UI draw pass already exists as a VP hook point —
  `AUTO_GOLD_Tick` is "called from DRAW_UI" (`VanillaPlusCore.h`), and the
  `globe_persistent` gate draws from `Panel.cpp` inside the same pass
  (`Source/D2Client/UI/Panel.cpp`). A `gbVPHudUiPhase` flag set on entry / cleared on
  exit of that pass is the v1 discriminator candidate; Round 1 pins the exact
  entry/exit pair and verifies what else draws inside it (tooltips, alt-labels, cursor —
  cursor must be excluded).
- **Gate + surface plumbing** (the shared idiom this feature copies): `D2VP_GetFeature
  Toggle("transparent_hud")` (`VanillaPlusCore.cpp:1440`) behind a 2 s hot-read cache
  (the `VANILLAPLUS_GlobeTextPersistent` shape, `VanillaPlusCore.cpp:533-540`), the
  auto-created documented ini template (`VanillaPlusCore.cpp:146-151`), `D2VP_Config
  Reloaded()` cache-bust (`:1476-1489`), and a Vanilla+ Options row through the
  `gaVanillaPlusRows` bridge (`VanillaPlusCore.cpp:1429-1438`, rows in
  `VanillaPlusOptions.cpp` — docs gate check 5 then requires the ini template to cover
  the new key).

## 4. Design (recommended shape)

**State**: none, beyond the gate cache. The feature is a pure per-draw mode rewrite —
"always-on when toggled" (owner call) means no timers, no hover tracking, nothing to
unwind.

**Gate**: `[features] transparent_hud`, read through the shared toggle idiom (2 s
hot-read, template auto-create, `D2VP_ConfigReloaded()` bust), plus a Vanilla+ Options
page row and a documented key in the ini template.

**Per draw** (the one choke point): a single resolver — draft name
`VPHUD_TranslateDrawMode(nDrawMode)` — consulted where the UI bracket's image draws
resolve their mode:

- Bracket closed (or gate off): `nDrawMode` passes through untouched — byte-identical.
- Bracket open + gate on + `nDrawMode == 5` (DRAW_SOLID): render translucent at the fixed
  alpha instead of opaque chromakey. Two implementations, Round 1 picks:
  - **(a) zero-renderer-touch**: rewrite 5 → 2 at the D2GFX dispatch — both renderers
    then take their existing 75% constant-alpha path (0xc0). Cost: the alpha is 75%, not
    ~70%, and text also rides it.
  - **(b) exact alpha** (recommended): keep mode 5 flowing but add one VP case per
    renderer's mode resolver (Glide `SetDrawMode`, D3D `SetDrawModeAndGetAlpha`) that
    takes the existing alpha-blend path with `0xb3` (~70%). Both files are fork-owned;
    the diff is a handful of lines each, and the alpha stays a named literal.

**Exclusions**: the cursor (its draws sit outside the UI art rewrite — verify the exact
draw call in Round 1; it must stay opaque and last-drawn); the automap overlay (expected
outside the bracket — verify); OOG menus (never inside the in-game bracket).

**Interaction to verify**: retail dims the world behind an open panel. v1 keeps that dim
as-is (the dim is world-side, the panels go translucent on top); whether the dim should
soften when panels are see-through is field QA feel, not a code call yet.

## 5. Failure and recovery matrix (degrade to mark, never halt)

| Event | Behavior |
|---|---|
| Renderer has no VP draw-mode case (old binary / future renderer) | Mode 5 passes through opaque — vanilla rendering, no crash; log mark |
| Gate flipped OFF mid-session (ini edit or Options row) | Next 2 s poll re-reads; the very next panel draw is opaque again — no state to unwind |
| A world draw lands inside the UI bracket (misclassification) | That sprite renders translucent — visible immediately in QA; Round 1's bracket pin + the field matrix bound it; kill-switch is the ini key |
| Gate on at 640x480 / 800x600 | Same rewrite applies (resolution-independent); both narrow stops walked in the field matrix |
| Death / TP / area transition / lag | Nothing to recover — per-draw rewrite, no state |
| Save → load, character swap | Unaffected — render-side only, nothing in any save file |

## 6. Roadmap — three code rounds, each field-verifiable

- **Round 1 — tree-verify (pins)**: the UI-phase bracket entry/exit pinned to exact call
  sites (what draws inside: panels, tooltips, alt-labels, cursor — cursor's draw call
  pinned for exclusion); the D2GFX dispatch site for variant (a) vs the two renderer
  resolvers for variant (b) decided; the alpha byte's final consumer verified in the D3D
  quad path (RGB565 no-alpha caveat above); what else rides mode 5 inside the bracket
  (text glyphs, text backing rects) enumerated; TU list frozen; the (a)-vs-(b) call made
  with the effort table updated.
- **Round 2 — implement**: gate + cache + template key + Options row
  (`gaVanillaPlusRows` — docs gate check 5 follows); the mode rewrite at the chosen choke
  point, both renderers in the same commit family; d2boot.log mark on gate flips; FEATURES
  row lands when it ships (docs gate check 6 tracks the count).
- **Round 3 — field QA**: the exit loop (§8) on the owner's Win11 build; SMOKE-CHECKLIST
  rows land with this round; the matrix walked once — {Glide, D3D} × {640, 800, wide} ×
  {each major panel open, HUD with no panel, cursor over panel, tooltips} — then
  spot-checked.

## 7. Risks — mapped to the class lessons already on the books

- **Text washes out** (legibility pillar 3): text glyphs and their backing rects ride the
  same alpha in v1. Bounded by the field matrix reading a tooltip over a busy background;
  escape hatch is §9 call 2 (carve text out of the rewrite), priced in Round 1's
  enumeration.
- **Bracket misclassification** (the UI-vs-world discrimination, same class as the
  camera-zoom exempt problem): a world element drawn inside the UI pass goes ghost.
  Bounded by the Round 1 pin and the §5 kill-switch.
- **Renderer drift** (the R109 lesson): both renderers in one round, parity rows in the
  field matrix; a renderer missing the VP case degrades to opaque, never to a crash.
- **D3D alpha consumer** (the one unknown): the mode resolver returns the alpha byte, but
  where it lands in the 16-bit quad path (vertex color vs texture factor) is verified in
  Round 1 before any code — the additive/modulate modes already consume the same
  machinery, so the risk is containment, not invention.
- **Options-page geometry** (docs gate check 5): one more row must fit the 480p band
  arithmetic and the ini template — mechanical, but it gates the build.

## 8. Exit (all must hold on the field build)

- With `transparent_hud=1`: HUD bar and every major panel (inventory, stash, cube,
  vendor, quest log, skill tree, party, waypoint) render translucent at the fixed alpha;
  the world is visible through them; Glide and D3D look the same.
- Cursor opaque, truthful, and drawn last — hover/click/tooltip land exactly as vanilla.
- Tooltips readable over a moving world (else §9 call 2 resolves before ship).
- Automap, OOG menus, chat unchanged; 640x480/800x600 walked.
- `transparent_hud=0` from a fresh start = vanilla byte-for-byte; hot-reload honored
  (ini edit and Options row both flip it live, ≤ 2 s); save → load unaffected.

## 9. The remaining owner calls

1. **The alpha literal** — `0xb3` (~70%, draft default, one VP case per renderer) or
   reuse mode 2's `0xc0` (75%) for the zero-renderer-touch variant (a)? The delta is one
   hex value vs one dispatch-layer rewrite.
2. **Text carve-out** — if QA fails legibility: keep panel art translucent and text
   opaque (costs an image-vs-text discrimination inside the bracket), or accept
   translucent text?
3. **World-dim interplay** — keep retail's world dim behind open panels (draft) or soften
   it while the feature is on (a second, world-side change — bigger tree)?
4. **8-bit DDraw follow-up** — explicitly deferred in v1; confirm it stays a non-goal
   until the wide-class pair proves out in the field. 