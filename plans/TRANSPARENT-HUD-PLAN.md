# Transparent HUD — feature plan

*Drafted 2026-11-11 on owner ask ("Lets draft a plan for transparent_hud"); scope revised
same day on owner corrections — **bottom bar only**, panels untouched, keep-list opaque
(globes, belt slots, skill buttons, XP bar, stamina bar); then finalized: the translucent
set goes **fully transparent (invisible)**, not ~70%. Owner calls made: scope, alpha =
0 (skip the draw), always-on when toggled, key = **`[features] transparent_hud`**.
**Round 1 (tree-verify) is complete** — the pins below are read off this tree, and the
design collapsed to call-site draw skips with **zero renderer code**. The remaining open
calls are §9. Status: **ROUND 1 DONE — ready to implement (Round 2).**

---

## 1. Goal and non-goals (v1)

**Goal.** With `[features] transparent_hud=1`, the bottom HUD bar's **frame/background
art** (the `CtrlPnl7`/`800CtrlPnl7` console pieces — which includes the chat/message area
background) and the run button, menu button and hostile indicator are **not drawn at
all**: the world shows through where they used to be. Every functional element keeps its
vanilla opaque draw and remains fully interactive: the Life and Mana globes (+ their
readout text and gold amount), the belt (slots, pop-belt art, item sprites, hotkey
labels), the skill buttons, the experience bar and the stamina bar. Open panels,
tooltips, the automap, chat text and the cursor are not part of this feature — they draw
byte-identically whether the gate is on or off. Display-only: nothing hit-test-related
changes — the hidden run/menu buttons stay exactly as clickable as before.

**Non-goals for v1** (each is a clean follow-up, deliberately deferred):
- **No panel transparency** — a future "transparent panels" feature is a separate plan
  (the blend-mode machinery research in the first draft of this file is its starting
  point; it is unnecessary for draw-skip).
- No partial alpha — the set is invisible, not faded (owner call). If field QA ever
  wants ~70% back, the renderer-side constant-alpha path documented in §3 is the
  follow-up design.
- No 8-bit DDraw concern — draw-skip is renderer-agnostic by construction.
- No configurable element list, no fade/idle behavior — one gate, one fixed set.
- No automap, tooltip or chat-text changes; no OOG changes.
- Nothing in any save file; no state to persist; display-only, no input changes.

## 2. The hard invariants (pillars)

1. **Default OFF = vanilla byte-for-byte.** With `transparent_hud=0` (or a missing
   ini/section/key) every touched call site draws exactly as retail.
2. **The keep-list is inviolable.** Globes, belt, skill buttons, XP bar, stamina bar —
   and all panels — are never wrapped: their call sites are untouched code. The skip set
   is an explicit allow-list of four call sites (§4), never a blanket phase rewrite.
3. **Invisible ≠ intangible.** The hidden buttons keep their hit areas and behaviors
   (input paths untouched); tooltips that name them (`PANEL_ShowRunButtonTooltip`,
   `PANEL_DrawLife`) still work. A hidden HUD must not become a broken HUD.
4. **Text survives.** The skip set contains no text draws: orb readouts, gold amount,
   belt hotkeys, chat input/message log all draw on their vanilla paths over the now
   empty bar region.
5. **Renderer parity for free.** No renderer code ships — the skip happens at the D2Client
   UI call sites, upstream of every renderer. There is no Glide/D3D parity surface, no
   8-bit exception, nothing to walk per renderer.

## 3. Where things live today (Round 1 pins — read off this tree, 2026-11-11)

**The parent dispatch.** The in-game UI pass is `DRAW_UI` (`Source/D2Client/UI/Draw.cpp`,
decl `Draw.h:53`; the pass that also hosts the `AUTO_GOLD_Tick` hook). The bar block:

- `Draw.cpp:720` — `Panel::DRAW_UI_BordersAndOrbsAndFillers()` (`Panel.cpp:2230`), which
  draws, in order:
  | # | Call (site) | What it is | Set |
  |---|---|---|---|
  | 1 | `DRAW_UI_BorderLeft/Right` (:2240/:2245) | Act side borders, only when a border type is active — not the bottom bar | untouched |
  | 2 | `PANEL_DrawCharacterInfo` (:2249) | **The bar frame art**: `Panel\CtrlPnl7` (640/narrow) or `Panel\800CtrlPnl7` (800/wide), frames 0–5, each `D2GFX_DrawImage(..., DRAW_SOLID, ...)` — the six console pieces spanning the bar, incl. the chat-area background and the orb sockets / button wells | **SKIP** |
  | 3 | `PANEL_DrawHealthOrb` (:2250) | Life globe | keep |
  | 4 | `PANEL_DrawManaOrb` (:2251) | Mana globe | keep |
  | 5 | `PANEL_DrawGoldAmount` (:2252) | Gold readout text over the left orb | keep |
  | 6 | `PANEL_DrawRunButton` (:2253) | Run/walk toggle button | **SKIP** |
  | 7 | `PANEL_DrawStaminaBar` (:2254) | Stamina bar | keep (owner call) |
  | 8 | `PANEL_DrawMenuButton` (:2255) | Menu (ESC) button | **SKIP** |
  | 9 | `PANEL_DrawBottomPanel` (:2256) | Belt internals: pop-belt rows (`ctrlpnl_popbelt`, :2137), belt item sprites (:2191), slot highlight rects, hotkey labels | keep |
  | 10 | `PANEL_DrawSkillIcons` (:2257) | Skill buttons | keep |
  | 11 | `PANEL_DrawExperienceBar` (:2258) | XP bar | keep (owner call) |
  | 12 | `PANEL_DrawLife` (:2268) | Life tooltip | keep |
- `Draw.cpp:746` — `PANEL_DrawPanelLayout()` — **misleading name**: it draws the orb
  hover/persistent HP/MP readout text (the `globe_persistent` site), not the bar layout.
  Keep.
- `Draw.cpp:747-749` — `MiniPanel::DRAW_UI_MiniPanel()` — the mini-panel strip; default
  untouched (§9 call 2).
- `Draw.cpp:750-752` — `Chat::DRAW_UI_ChatInput()` — text/IME only; the chat background
  is part of the CtrlPnl7 center console art, so skipping the frame already clears the
  chat area. Text stays.
- `Draw.cpp:842` — `Panel::PANEL_DrawHostileIndicator()` (near the pass's end, over the
  message log; the log text at :843 is a separate call) — **SKIP**.

**Everything the skip set draws goes through `D2GFX_DrawImage` with `DRAW_SOLID`** (the
opaque chromakey mode; both renderers resolve it in `RENDERER_GLIDE_SetDrawMode`,
`Glide.cpp:2285`, and `SetDrawModeAndGetAlpha`, `Direct3D.cpp:3986`) — but with
fully-transparent the skip upstream of `D2GFX_DrawImage` needs none of that machinery;
it is recorded here as the follow-up design for a future partial-alpha variant.

**Keep-list separability — verified.** Every keep element is its own function called at
its own site in `DRAW_UI_BordersAndOrbsAndFillers` (table above); nothing shares a draw
call with the frame art. The one coupling is *visual*, not structural: the orb sockets
and button wells are baked into the CtrlPnl7 frame pieces, so they vanish with the frame
— opaque orbs/belt/skills/XP/stamina float on the world. That is the intended
fully-transparent look (confirmed by the owner's "fully transparent"); field QA checks
it reads well.

**Gate + surface plumbing** (the shared idiom this feature copies): `D2VP_GetFeature
Toggle("transparent_hud")` (`VanillaPlusCore.cpp:1440`) behind a 2 s hot-read cache
(the `VANILLAPLUS_GlobeTextPersistent` shape, `VanillaPlusCore.cpp:533-540`), the
auto-created documented ini template (`VanillaPlusCore.cpp:146-151`), `D2VP_Config
Reloaded()` cache-bust (`:1476-1489`), and a Vanilla+ Options row through the
`gaVanillaPlusRows` bridge (`VanillaPlusCore.cpp:1429-1438`, rows in
`VanillaPlusOptions.cpp` — docs gate check 5 then requires the ini template to cover
the new key).

## 4. Design (final shape)

**State**: none, beyond the gate cache. No timers, no hover tracking, nothing to unwind.

**Gate**: `[features] transparent_hud`, read through the shared toggle idiom (2 s
hot-read, template auto-create, `D2VP_ConfigReloaded()` bust), plus a Vanilla+ Options
page row and a documented key in the ini template.

**The implementation is four guarded call sites** — draft predicate
`VANILLAPLUS_HudBarTransparent()` (the same per-frame accessor shape as
`VANILLAPLUS_GlobeTextPersistent`):

- `Panel.cpp:2249` — `PANEL_DrawCharacterInfo` call wrapped: gate on → skipped (the
  whole console frame disappears).
- `Panel.cpp:2253` — `PANEL_DrawRunButton` call wrapped.
- `Panel.cpp:2255` — `PANEL_DrawMenuButton` call wrapped.
- `Draw.cpp:842` — `PANEL_DrawHostileIndicator` call wrapped.

Gate off (or ini missing) → each call executes exactly as retail. There is no renderer
code, no draw-mode rewrite, no blend state, no per-frame flag around a region — the
draw simply does not happen. The wrapper is one `if` per site, so the diff is a handful
of lines in two TUs plus the gate accessor, ini template key, and Options row.

**Interaction to verify in QA only**: retail dims the world when a panel opens; the bar
block runs the same with the frame skipped — no interplay expected (the dim is world-side).

## 5. Failure and recovery matrix (degrade to mark, never halt)

| Event | Behavior |
|---|---|
| Any renderer (old binary, future renderer, 8-bit DDraw) | Unaffected — the skip is upstream of the renderer; there is no renderer surface |
| Gate flipped OFF mid-session (ini edit or Options row) | Next 2 s poll re-reads; the very next frame draws the full bar again — no state to unwind |
| A call site outside the allow-list gets wrapped by mistake | The element vanishes — visible immediately in QA; the allow-list is four named sites, reviewed in the diff; kill-switch is the ini key |
| Hidden buttons confuse a field user | Buttons still work (hit areas untouched); the plan ships them hidden by design — §9 call 3 can revisit per-element membership |
| Gate on at 640x480 / 800x600 / wide | Same skip applies (resolution-independent); all stops walked in the field matrix |
| Death / TP / area transition / lag | Nothing to recover — per-draw skip, no state |
| Save → load, character swap | Unaffected — render-side only, nothing in any save file |

## 6. Roadmap

- **Round 1 — tree-verify: DONE** (this file, §3). The parent dispatch pinned
  (`DRAW_UI`); the bar block enumerated call-by-call with the allow-list and keep-list
  assigned; keep-list separability verified (per-function call sites, no shared draws);
  the frame-art contents confirmed (`CtrlPnl7`/`800CtrlPnl7` frames 0–5, sockets/wells
  included); the chat area confirmed frame-only (text draws elsewhere); hostile
  indicator site pinned (`Draw.cpp:842`). Design collapsed to draw-skip: the 2×5
  renderer table and blend machinery are no longer in scope.
- **Round 2 — implement**: gate accessor + cache bust + template key + Options row
  (`gaVanillaPlusRows` — docs gate check 5 follows); the four guarded call sites;
  d2boot.log mark on gate flips; FEATURES row lands when it ships (docs gate check 6
  tracks the count).
- **Round 3 — field QA**: the exit loop (§8) on the owner's Win11 build; SMOKE-CHECKLIST
  rows land with this round; the matrix walked once — {640, 800, wide} × {bar alone,
  each keep-list element hovered/used, chat open, belt expanded, panel open, cursor over
  the empty bar region} — then spot-checked. No renderer axis.

## 7. Risks

- **Floating-element look** (the resolved coupling): sockets/wells vanish with the frame;
  the orbs and buttons render over bare world. Intended per the owner's call; field QA
  judges readability of the keep-list elements without their wells (belt slots have their
  own highlight rects and item sprites, so they carry their own contrast).
- **Hidden-but-clickable confusion**: the run/menu buttons respond to clicks on empty
  world pixels. Bounded by pillar 3 (this is the retail behavior set, minus the pixels)
  and §9 call 3 (per-element membership can change in one line).
- **Options-page geometry** (docs gate check 5): one more row must fit the 480p band
  arithmetic and the ini template — mechanical, but it gates the build.
- **Diff discipline**: the skip wrappers touch `Panel.cpp` and `Draw.cpp`, both already
  VP-annotated TUs; keep the `VP:` comment markers per house rule (docs gate check 8).

## 8. Exit (all must hold on the field build)

- With `transparent_hud=1`: the console frame (incl. chat background), run button, menu
  button and hostile indicator are invisible; the world shows through; every renderer.
- Globes (+ readouts, gold), belt (all states, expanded included), skill buttons, XP bar,
  stamina bar pixel-identical to vanilla with the gate on — and fully interactive.
- Hidden buttons remain clickable and their tooltips appear; belt/skill/orb hit tests
  unchanged.
- Every open panel, tooltip, the automap, chat text and the cursor pixel-identical to
  vanilla with the gate on; cursor truthful and drawn last.
- `transparent_hud=0` from a fresh start = vanilla byte-for-byte; hot-reload honored
  (ini edit and Options row both flip it live, ≤ 2 s); save → load unaffected.

## 9. The remaining owner calls

1. **Mini-panel membership** — `MiniPanel::DRAW_UI_MiniPanel` (Draw.cpp:747-749) is
   default untouched (it is a separate strip, not the bar). In or out?
2. **Hostile indicator** — currently in the skip set ("everything else"); it is the tiny
   red PvP marker, easy to lose. Confirm, or move it to the keep-list (one line).
3. **Future partial alpha** — the renderer-side constant-alpha path (§3) is recorded for
   a possible "faded HUD" sibling feature; no action needed now.
