# Round summary — R154 (rev 170, 2026-11-11): transparent_hud — the bottom bar's background goes fully transparent

## The ask

Owner, 2026-11-11 (three messages, one refinement arc): "Lets draft a plan for
transparent_hud" → scope corrections ("thats not exactly what i want ... the panels should
remain untouched. it is just the bottom bar ... not the globes, belt bar or skill buttons.
just everything else") → the XP/stamina keep ("the xp/stamina bar should remain opaque as
well") → the alpha call ("I want to make the transparent items fully transparent"). Slate
row 4; plan at `plans/TRANSPARENT-HUD-PLAN.md` — Round 1 (tree-verify) pinned the same day.

## Round 1 findings (the pins that shaped the build)

The parent dispatch is `DRAW_UI`; the bar block is
`DRAW_UI_BordersAndOrbsAndFillers` (`Panel.cpp:2230`), and every element draws from its
own named function — the keep-list is separable by construction. The frame art is
`PANEL_DrawCharacterInfo` (:2249): six `CtrlPnl7`/`800CtrlPnl7` `DRAW_SOLID` pieces
(spanning the console, the chat area background, the orb sockets and button wells).
`PANEL_DrawPanelLayout` is a misnomer (orb text, not layout); `PANEL_DrawBottomPanel` is
belt internals; the chat input is text-only. The hostile indicator sits outside the bar
block (`Draw.cpp:842`). Because the owner chose **fully** transparent, the design
collapsed: alpha 0 = skip the draw — no blend modes, no renderer code, no 2×5 site
table. The original draft's renderer-side constant-alpha machinery (draw modes 0/1/2 →
25/50/75% in both renderers) is recorded in the plan as the follow-up design for a
future "faded HUD" only.

## What changed

Four guarded call sites, one gate, no state:

1. `Panel.cpp` `DRAW_UI_BordersAndOrbsAndFillers` — `PANEL_DrawCharacterInfo`,
   `PANEL_DrawRunButton`, `PANEL_DrawMenuButton` wrapped in
   `if (!VANILLAPLUS_HudBarTransparent())` (`// VP-BEGIN/END [transparent-hud R154]`
   bracketing the block).
2. `Draw.cpp` `DRAW_UI` — `PANEL_DrawHostileIndicator` wrapped the same way (the message
   log text right after it is a separate call and keeps drawing).
3. `VanillaPlusCore.{h,cpp}` — `VANILLAPLUS_HudBarTransparent()`: the shared 2 s
   hot-read gate shape (`D2VP_GetFeatureToggle("transparent_hud")`), cache
   `dwHudBarTranslucentLastCheck` busted in `D2VP_ConfigReloaded`; one
   `hud-transparent:` d2boot.log mark on first activation (the unlock-fps one-way
   latch); the documented `transparent_hud=0` block appended to the auto-created ini
   template.
4. `VanillaPlusOptions.cpp` — the fourteenth row (`Transparent HUD`); the page stays two
   columns of seven, col 1 fills its 7th band, geometry unchanged (back row 0x144,
   box 0x171, 0x13 bottom margin — re-verified by the docs-gate arithmetic).

`Redivivus/d2_boot.h`: `D2VP_ROUND "153"→"154"`.

## Invariants held by construction

- Gate off / missing ini = every wrapped call executes exactly as retail (byte-for-byte).
- Keep-list untouched: globes (+ orb text + gold), belt (all states), skill buttons,
  XP bar, stamina bar — plus every panel, tooltip, automap, chat text, cursor.
- Hidden ≠ intangible: input paths untouched, so the run/menu buttons keep their hit
  areas and tooltips.
- Zero renderer code: no Glide/D3D axis, 8-bit DDraw needs no exception.

## Verification

Static — docs_gate green (options-page counts 14/14/7 + geometry + ini-template
coverage; FEATURES "fourteen features ship" == 14 rows; handoff §2 count; round identity
boot/handoff/CHANGELOG/ROUND-SUMMARY; VP-marker pairing + brace/paren balance on the
changed TUs) + smoke_sync 0 warnings (no smoke rows this round — they land with the
field round). Compile gate of record: the owner's Win11 MSYS2 build (overlay zip
`d2vp-r154-overlay.zip`).

## Field ask (the plan's Round 3, pending)

The plan's §8 exit loop on the owner's build: {640, 800, wide} × {bar alone, each
keep-list element hovered/used, chat open, belt expanded, panel open, cursor over the
empty bar region}; SMOKE-CHECKLIST rows land with that round.

Round identity: self-ID `Vanilla+0.0.8-R154`, automap tag `D2VP-R154`
(`D2VP_ROUND "153"→"154"`).
