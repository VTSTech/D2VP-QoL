# Transparent HUD — feature plan

*Drafted 2026-11-11 on owner ask ("Lets draft a plan for transparent_hud"); scope revised
same day on owner corrections — **the panels stay untouched**, this is **the bottom bar
only**, and within the bar the functional elements stay opaque: **globes, belt slots,
skill buttons, XP bar, stamina bar**. Only "everything else" in the bar goes translucent.
Owner calls also made: **fixed ~70% alpha**, **always-on when toggled** (no fade/idle),
key = **`[features] transparent_hud`**. The remaining open calls are §9.
Status: **PLANNED — no code yet.**

---

## 1. Goal and non-goals (v1)

**Goal.** With `[features] transparent_hud=1`, the bottom HUD bar's **frame/background
art** (the big panel graphic, including the chat/message area's frame) and its small
auxiliary buttons render at a fixed ~70% opacity so the world shows through the empty
parts of the bar. Every functional element of the bar keeps its vanilla opaque draw:
the Life and Mana globes, the belt potion slots, the skill buttons, the experience bar
and the stamina bar. Open panels (inventory, stash, cube, vendor, quest log, skill tree,
party, waypoint) are **not part of this feature at all** — they draw byte-identically
whether the gate is on or off. Pure display: every hover, tooltip, drag and click lands
exactly where it does in vanilla, because nothing but the blend changes.

**Non-goals for v1** (each is a clean follow-up, deliberately deferred):
- **No panel transparency** — the panel set is untouched code; a future "transparent
  panels" feature would be a separate plan riding the same mechanism.
- No 8-bit DDraw path (D2DDraw / D2Gdi) — v1 targets the two wide-class renderers the
  resolution work already covers (Glide + Direct3D), same scope discipline as the
  camera-zoom plan; 8-bit blend tables are a follow-on.
- No configurable opacity — one fixed literal (§9 call 1), not an ini value, not a slider.
- No fade-on-idle / combat-aware behavior — the toggle is a plain always-on gate.
- No automap, tooltip or chat-text changes — text and overlays draw exactly as vanilla.
- No out-of-game (OOG) menu transparency.
- Nothing in any save file; no state to persist; display-only, no input changes.

## 2. The hard invariants (pillars)

1. **Default OFF = vanilla byte-for-byte.** With `transparent_hud=0` (or a missing
   ini/section/key) every touched site takes its vanilla branch; the frame path is
   byte-identical at every resolution, both renderers.
2. **The keep-list is inviolable.** Globes, belt slots, skill buttons, XP bar, stamina
   bar — and all open panels — draw through their vanilla paths even with the gate on.
   The translucent set is an explicit allow-list of specific draw sites (§3), never a
   blanket phase rewrite: if a draw site is not on the allow-list, it is untouched.
3. **Cursor truth.** The cursor is never translucent and never re-routed. Alpha changes
   nothing hit-test-wise — this feature cannot lie about geometry because it never
   touches geometry, only how the bar's background pixels blend.
4. **Text stays opaque by construction.** Because the rewrite is per-site (frame art and
   buttons, not the whole UI pass), font/text draws are not wrapped — chat text and
   tooltips keep full contrast over the now-translucent bar automatically. No carve-out
   logic needed.
5. **Renderer parity.** Glide and D3D ship the same alpha in the same round (the R109
   field-matrix-fork lesson: one renderer first is how a round dies).
6. **One choke point.** The gate is read per frame through the shared 2 s hot-read idiom;
   the per-draw rewrite happens at one published flag + one mode-rewrite site (§4), not
   per renderer per panel.

## 3. Where things live today (recon, this tree — the engine already does the hard part)

**The engine already renders translucent sprites.** The whole feature rides an existing
path — no new blending is invented:

- **Glide draw-mode dispatch**: `RENDERER_GLIDE_SetDrawMode` (`Source/D2Glide/Renderer/
  Glide.cpp:2285`) maps `nDrawMode` → blend: **5/7 → alpha-blend mode 1 (opaque
  chromakey — what UI art uses)**; 3 → 2 (additive); 4 → 3 (modulate); **0/1/2 →
  alpha-blend mode 4 + `grConstantColorValue` 0x40/0x80/0xc0 (25%/50%/75% constant
  alpha)**. `RENDERER_GLIDE_SetAlphaBlendMode` (`Glide.cpp:1282`) is the blend-state
  setter it feeds.
- **D3D draw-mode dispatch**: `SetDrawModeAndGetAlpha` (`Source/D2Direct3D/Renderer/
  Direct3D.cpp:3986`) is the symmetric twin — 5/7 → `SetBlendMode(1)` + alpha 0xff;
  **0/1/2 → `SetBlendMode(4)` + per-draw alpha 0x40/0x80/0xc0**. `SetBlendMode`
  (`Direct3D.cpp:689`) mode 4 = SRCALPHA/INVSRCALPHA. The alpha byte rides into the quad
  draw — no texture-format change is needed (the RGB565 paletted-conversion path has no
  alpha channel; the constant comes from the draw-mode resolver, not the texture — Round
  1 verifies the byte's final consumer).
- **The 2×5 site table.** Both renderers implement the same five entry points, each
  resolving `nDrawMode` locally: `DrawImage` (Glide.cpp:2414 / Direct3D.cpp:4122),
  `DrawShiftedImage` (:2453 / :4144), `DrawVerticalCropImage` (:2471 / :4182),
  `DrawClippedImage` (:2599 / :4279), `DrawSolidRectEx` (:3105 / :4667). A rewrite at the
  D2GFX dispatch layer upstream (the `fpDrawShiftedImage` funnel, `Source/D2gfx/D2GFX.h:
  132`, export-table slot at :305) covers both renderers at once — the zero-renderer-touch
  variant rewrites mode 5 → mode 2 (75%); the exact-70% variant adds one VP case per
  renderer's mode resolver (§4).
- **The bar is already decomposed into named draw functions** — the recon gift that makes
  the keep-list enforceable (`Source/D2Client/UI/Panel.h`): `PANEL_DrawPanelLayout()` (the
  bar frame/layout art), `PANEL_DrawHealthOrb()` / `PANEL_DrawManaOrb()` (globes — keep
  opaque), `PANEL_DrawBeltSlots(nFrameNumber)` (belt — keep opaque), `PANEL_DrawSkill
  Icons()` / `SPELLSEL_DrawSkillIcon` (skill buttons — keep opaque), `PANEL_Draw
  ExperienceBar()` (XP — keep opaque, owner call), `PANEL_DrawStaminaBar()` (stamina —
  keep opaque, owner call), `PANEL_DrawRunButton()` / `PANEL_DrawMenuButton()` (auxiliary
  buttons — translucent set), `PANEL_DrawHostileIndicator()` (translucent set),
  `LoadPanelImages()` / `FreePanelImages()` (the bar's DC6 handles). The per-frame call
  sequence that invokes these — the parent UI draw dispatch (the pass that also hosts the
  `AUTO_GOLD_Tick` hook, `VanillaPlusCore.h`) — is the Round 1 pin that fixes the
  allow-list in code order.
- **Gate + surface plumbing** (the shared idiom this feature copies): `D2VP_GetFeature
  Toggle("transparent_hud")` (`VanillaPlusCore.cpp:1440`) behind a 2 s hot-read cache
  (the `VANILLAPLUS_GlobeTextPersistent` shape, `VanillaPlusCore.cpp:533-540`), the
  auto-created documented ini template (`VanillaPlusCore.cpp:146-151`), `D2VP_Config
  Reloaded()` cache-bust (`:1476-1489`), and a Vanilla+ Options row through the
  `gaVanillaPlusRows` bridge (`VanillaPlusCore.cpp:1429-1438`, rows in
  `VanillaPlusOptions.cpp` — docs gate check 5 then requires the ini template to cover
  the new key).

## 4. Design (recommended shape)

**State**: none, beyond the gate cache. "Always-on when toggled" (owner call) means no
timers, no hover tracking, nothing to unwind.

**Gate**: `[features] transparent_hud`, read through the shared toggle idiom (2 s
hot-read, template auto-create, `D2VP_ConfigReloaded()` bust), plus a Vanilla+ Options
page row and a documented key in the ini template.

**Per frame, in the parent UI draw dispatch** (Round 1 pin): a tiny flag
`gbVPHudBarTranslucent` is set immediately before the **translucent-set** draws and
cleared immediately after them — draft membership, §9 call 2:

- `PANEL_DrawPanelLayout()` — the bar frame/background, including the chat/message area
  frame (the orb sockets and button wells are part of the frame art; the orb/belt/button
  art drawn *over* it stays opaque — the visual result is checked in field QA).
- `PANEL_DrawRunButton()` / `PANEL_DrawMenuButton()`.
- `PANEL_DrawHostileIndicator()`.

Everything else — globes, belt slots, skill icons, XP bar, stamina bar, chat text, all
panels, tooltips, automap, cursor — draws on its vanilla path with the flag never set.

**Per draw** (the one choke point): the same single resolver as the wide-scope draft —
`VPHUD_TranslateDrawMode(nDrawMode)` — consulted where image draws resolve their mode:

- Flag clear (or gate off): `nDrawMode` passes through untouched — byte-identical.
- Flag set + `nDrawMode == 5` (DRAW_SOLID): render translucent at the fixed alpha instead
  of opaque chromakey. Two implementations, Round 1 picks:
  - **(a) zero-renderer-touch**: rewrite 5 → 2 at the D2GFX dispatch — both renderers
    then take their existing 75% constant-alpha path (0xc0). Cost: the alpha is 75%, not
    ~70%.
  - **(b) exact alpha** (recommended): keep mode 5 flowing but add one VP case per
    renderer's mode resolver (Glide `SetDrawMode`, D3D `SetDrawModeAndGetAlpha`) that
    takes the existing alpha-blend path with `0xb3` (~70%). Both files are fork-owned;
    the diff is a handful of lines each, and the alpha stays a named literal.

**Interaction to verify**: retail dims the world when a panel opens; the bar is drawn
over that dim regardless. No interplay expected — the dim is world-side and the bar
rewrite is bar-side (verify in the field matrix with inventory open + gate on).

## 5. Failure and recovery matrix (degrade to mark, never halt)

| Event | Behavior |
|---|---|
| Renderer has no VP draw-mode case (old binary / future renderer) | Mode 5 passes through opaque — vanilla rendering, no crash; log mark |
| Gate flipped OFF mid-session (ini edit or Options row) | Next 2 s poll re-reads; the very next bar draw is opaque again — no state to unwind |
| A non-allow-listed draw lands inside the flag window (misordering) | That sprite renders translucent — visible immediately in QA; the flag wraps whole named functions, not a wide bracket, so the blast radius is the bar itself; kill-switch is the ini key |
| Gate on at 640x480 / 800x600 | Same rewrite applies (resolution-independent); both narrow stops walked in the field matrix |
| Death / TP / area transition / lag | Nothing to recover — per-draw rewrite, no state |
| Save → load, character swap | Unaffected — render-side only, nothing in any save file |

## 6. Roadmap — three code rounds, each field-verifiable

- **Round 1 — tree-verify (pins)**: the parent UI draw dispatch and its call order for
  the `PANEL_*` functions pinned; the frame-art draw sites inside `PANEL_DrawPanelLayout`
  enumerated (mode-5 image draws + any solid-rect backings); the chat/message frame site
  pinned; the keep-list functions confirmed separable (each is its own call site —
  verify nothing shares a draw call with the frame art); the alpha byte's final consumer
  verified in the D3D quad path; variant (a) vs (b) decided; TU list frozen.
- **Round 2 — implement**: gate + cache + template key + Options row
  (`gaVanillaPlusRows` — docs gate check 5 follows); the flag around the translucent-set
  call sites + the mode rewrite at the chosen choke point, both renderers in the same
  commit family; d2boot.log mark on gate flips; FEATURES row lands when it ships (docs
  gate check 6 tracks the count).
- **Round 3 — field QA**: the exit loop (§8) on the owner's Win11 build; SMOKE-CHECKLIST
  rows land with this round; the matrix walked once — {Glide, D3D} × {640, 800, wide} ×
  {bar alone, each keep-list element hovered/used, chat open, panel open + bar, cursor
  over bar} — then spot-checked.

## 7. Risks — mapped to the class lessons already on the books

- **Frame-vs-function coupling** (the one real recon risk): if an excluded element's art
  is drawn *inside* `PANEL_DrawPanelLayout` (e.g. the orb socket rims or button wells are
  part of the frame DC6 rather than separate draws), the keep-list can't separate them at
  call granularity — the translucent set then either includes those art pixels (accepted,
  QA-visible) or Round 1 splits the draw inside the function (bigger diff). Bounded by
  the Round 1 enumeration before any code.
- **Legibility of the translucent frame's contents**: text is not wrapped (pillar 4), but
  the chat entry line and the button glyphs sit on the frame — field QA reads them over a
  moving world; the §9 call 1 alpha value is the tuning knob if they wash out.
- **Renderer drift** (the R109 lesson): both renderers in one round, parity rows in the
  field matrix; a renderer missing the VP case degrades to opaque, never to a crash.
- **D3D alpha consumer** (the one unknown): the mode resolver returns the alpha byte, but
  where it lands in the 16-bit quad path is verified in Round 1 before any code — the
  additive/modulate modes already consume the same machinery, so the risk is containment,
  not invention.
- **Options-page geometry** (docs gate check 5): one more row must fit the 480p band
  arithmetic and the ini template — mechanical, but it gates the build.

## 8. Exit (all must hold on the field build)

- With `transparent_hud=1`: the bar frame/background (and chat area frame) and the
  run/menu buttons + hostile indicator render translucent at the fixed alpha; the world
  is visible through the bar's empty regions; Glide and D3D look the same.
- Globes, belt slots, skill buttons, XP bar, stamina bar pixel-identical to vanilla with
  the gate on.
- Every open panel, tooltip, the automap, chat text and the cursor pixel-identical to
  vanilla with the gate on; cursor truthful and drawn last.
- Bar buttons and belt/skill interactions behave exactly as vanilla (click truth).
- `transparent_hud=0` from a fresh start = vanilla byte-for-byte; hot-reload honored
  (ini edit and Options row both flip it live, ≤ 2 s); save → load unaffected.

## 9. The remaining owner calls

1. **The alpha literal** — `0xb3` (~70%, draft default, one VP case per renderer) or
   reuse mode 2's `0xc0` (75%) for the zero-renderer-touch variant (a)? The delta is one
   hex value vs one dispatch-layer rewrite.
2. **The translucent-set membership** — draft: bar frame/background incl. chat frame,
   run button, menu button, hostile indicator. Confirm, or name anything else in the bar
   ("everything else") that belongs in or out.
3. **8-bit DDraw follow-up** — explicitly deferred in v1; confirm it stays a non-goal
   until the wide-class pair proves out in the field. 