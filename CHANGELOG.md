# Changelog — D2LoD Vanilla+ (`d2vp.exe`)

**For humans — one short entry per shipped release; keep it that way.** The
engineering-level detail (root causes, gate counts, merge resolutions, verification
digests) lives in [`handoff.md`](handoff.md) (current round journal) and
[`HISTORY.md`](HISTORY.md) (verbatim archive) — the full pre-restructure entries are
archived in [HISTORY.md](HISTORY.md) **H41**. Working rounds between releases carry
build self-IDs `Vanilla+0.0.x-Ryy`; the first `d2boot.log` line pins any shipped exe to
its source round. Per-tag notes: the
[releases page](https://github.com/VTSTech/D2VP-QoL/releases).

## [0.0.8] — 2026-11-06 ([`R128`](https://github.com/VTSTech/D2VP-QoL/releases/tag/R128))

- **R147 — Automap clock: Session + System time** (owner ask: "Let's add an in game
  clock to the Automap. Session Time and System Time"). With `automap_clock=1` the
  automap overlay gains two lines under the build tag, right-aligned with the other
  overlay text: **Session** (real time since you entered the current game, `H:MM:SS`)
  and **System** (your PC's local clock, `HH:MM` 24-hour). Opt-in (default off), a
  **Automap Clock** row joins the Vanilla+ Options page (the page is two columns of
  seven now, box 0x151→0x171 — still whole at 640x480). The five round-tooling scripts (docs gate, overlay
  pack/verify, smoke sync, round bump, handoff offload) move into `tools/` and
  ship in the overlay. Self-ID `Vanilla+0.0.8-R147`.
- **R146 — Shared Stash: 100 pages** (owner ask: "Good job, works. Let's increase the
  Shared Stash pages to 100" — the "Then 100" leg of the original R63 roadmap). The
  R82 design was built for this: the `.sss` v2 header grows to 812 bytes (one
  size/checksum pair per page), only used pages cost file space, and narrower files
  (pre-R75, pre-R82, ten-page) still load with the missing pages empty and rewrite as
  a hundred on the next save. Labels `Shared 1`…`Shared 100`; a hundred-page file on
  an older build is rejected whole (file untouched; the old build's save keeps the
  full generation in `.bak`). Same `<<`/`>>` pager, one page per click. Self-ID
  `Vanilla+0.0.8-R146`.
- **R145 — Shared Stash: the pager arrows read `<<` / `>>`** (owner ask: "Can we make
  the navigation label '<<' and '>>' instead of just < >, i find just one is a little
  small to click on"). Same outlined tab, same right-anchored spot, same one-page
  stepping — the glyphs (and with them the click bands, which come from the same
  measured layout) double in width; the composite still grows leftward into the empty
  band above the gold display. Self-ID `Vanilla+0.0.8-R145`.
- **R144 — Vanilla+ Options page: two columns + the interpolation row lands on the
  panel** (owner ask: "Is unlock_fps_interp on the Vanilla+ Options Panel? … Can we
  make the panel two column instead of one?"). The page was one column of eleven rows —
  the box had grown to 497 px and clipped its empty bottom margin at 640x480. It is now
  **two columns of six rows**: labels left, On/Off right-aligned per column,
  per-column hover highlight and hit-testing, Previous Menu full-width below; the box
  is 337 px again and fits every video stop whole, 640x480 included. **`unlock_fps_interp`
  leaves ini-only** and takes the twelfth row, **FPS Interpolation**, directly under
  Unlock FPS — same write bridge as every row (flip → ini write → 2 s cache flush).
  The row reads a missing key as **1** through a new per-row read default
  (`D2VP_GetFeatureToggleDefault`) so the display always matches what the gate acts on:
  on a pre-R143 ini without the key (the template is only written for a missing file,
  never backfilled) it shows On — the truth — and the first click writes a real `0`
  instead of a silent no-op `1`. The R143.2 glide is field-accepted (owner:
  "Ok, I think it looks good now"). Self-ID `Vanilla+0.0.8-R144`.
- **R143 — Unlock FPS: sub-tick interpolation** (plan Phase 2, owner: "lets continue";
  revised **R143.1** after the owner's field report "the interpolation is jittery, looks
  better at 0").
  With `unlock_fps=1` the world now **glides between game ticks at the present rate**:
  the camera, the sprite world offset and everything world-anchored (floor, monsters,
  missiles, items) glide between the last two tick-snapped samples — applied at draw
  time only, nothing written back to the path; the sim stays byte-exact 25 Hz (the
  d2boot presents/ticks pair still proves it). The character keeps its retail screen
  position (the camera cancels the delta), shadows/auras stay glued. Ships under a
  second, ini-only key **`[features] unlock_fps_interp`** (default 1, no Options row;
  `0` = the R142 step-cadence look, hot-read). Self-ID `Vanilla+0.0.8-R143`. **R143.1**:
  the first cut extrapolated a velocity prediction (`pos + screenVelocity × α`); a
  prediction is never exact, so every tick boundary snapped by its own error — the
  reported jitter. The revision interpolates between two real position samples taken at
  each client world update, which is continuous at every tick boundary by construction;
  one frame-shared offset (camera and every sprite ride the same delta), teleports and
  level changes snap instead of gliding. The d2boot mark now reads
  `+ sub-tick interp (two-sample)` — that string identifies the revised build.
  **R143.2** (owner field: "still looks jittery at 1, looks better at 0"): the two-sample
  math was right, the delivery was not — three wiring defects fixed. (1) The snapshot
  ran AFTER the draw, so on tick+present frames the draw read the live post-tick
  position against the previous segment's pair and the picture sawtoothed a full tick
  at every boundary (a sandbox loop-order harness reproduces an 18 px backward kick on
  a 32 px walking step — the exact reported jitter); the snapshot call now runs before
  the draw block. (2) The sprite world offset's delta sat behind the shake early-return
  in `DRAW_UpdateMouseWorldOffsets`, dead in normal play — unit shadows, state overlays,
  on-screen culling and the mouse world-pick stayed tick-locked against the gliding
  camera; the delta now applies before that check. (3) The player-sprite cancel
  double-added in perspective mode; it now subtracts there (MouseOffsets already carried
  the delta into projected coords). Sandbox wiring harness 10/10 green: sawtooth
  reproduction, monotone bounded glide, kick-free stop and turn.

- **R142 — Unlock FPS: presentation-FPS unlock** (owner ask, no issue). With
  **`d2vp.ini [features] unlock_fps`** (default 0, like every feature; the Vanilla+
  Options page's new **"Unlock FPS"** row) the game **presents at ~60 FPS while the
  simulation keeps the exact retail 25 Hz cadence**. The recon found the 25 FPS
  presentation is only a draw gate in the client frame loop — `CLIENT_GameLoopFrame`
  presents one draw per 40 ms tick window — so R142 moves the draw onto a 60 Hz present
  pacer (skip-never-queue: a slow GPU re-anchors, never queues), paces the message-pump
  idle sleep to the same deadline, and drops `D2GFX_EndScene`'s 5 ms anti-spin budget to
  1 ms under the gate. Server ticks, client world updates, movement, animations and
  timers stay byte-exact; vsync untouched. The immediate wins: a 60 Hz cursor,
  hover/tooltip/target-highlight responsiveness and up to ~40 ms less input-to-photon
  latency. `d2boot.log` carries a presents/ticks-per-second pair once per 5 s while the
  gate is on — the field proof the game speed is unchanged. Retail's own
  `GAMETYPE_SINGLEPLAYER_UNCAPPED` QA path was considered and **rejected** — it speeds
  the *server*, a game-speed change. Sub-tick interpolation (world *motion* at 60) is
  the deliberate follow-on; plan at [plans/FPS-UNLOCK-PLAN.md](plans/FPS-UNLOCK-PLAN.md).
  Self-ID `Vanilla+0.0.8-R142`. Field ask: SMOKE **§S** (items 61–64).
- **R141 — Ctrl+Click: the vacated-slot fix** (field catch on R140,
  [#35](https://github.com/VTSTech/D2VP-QoL/issues/35)). The R140 build sent the `0x19`
  pick and `0x18` place back to back; the two echoes drained in one packet batch and the
  source container's client-side grid entry went stale — the emptied slot refused every
  later placement. R141 parks the place packet and releases it one full frame later (the
  timing of a fast manual drag). Field-verified 2026-11-09 — "R141 confirmed", issue
  closed. Self-ID `Vanilla+0.0.8-R141`.
- **R140 — Ctrl+Click stash transfer** (owner ask, no issue). With the stash open,
  **Ctrl+Click** now moves an item — inventory → stash or stash → inventory, one
  click, no dragging; the shared-stash pages work unchanged, and a full target
  plays the "won't fit" sound. Retail wired Ctrl+Click to sell-in-shop only; with
  no shop open the branch was a dead click (no packet, no cursor), so it is
  re-expressed as the real transfer — the same `0x19` pick + `0x18` place pair a
  manual drag sends, with the free-cell finder the cube path uses (the server's
  `0x18` handler rejects a place unless the item is on the cursor, which is why
  the first field build — `0x18` alone — silently did nothing: sound played,
  item stayed). Gated on **`d2vp.ini [features] ctrl_click_xfer`** (default 0,
  like every feature; the Vanilla+ Options page's "Ctrl Click Transfer" row) —
  off is the retail dead click.
- **R139 — On-screen build tag** (owner ask, no issue). The title screen and the
  in-game automap overlay used to show the retail version line ("v 1.14d"); both
  now show the fork build tag — **`D2VP-R139`** — so the exe identifies itself at
  a glance. One define (`Redivivus/d2_boot.h`) feeds the title screen, the automap
  line and the `d2boot.log` self-ID, so every future round bumps a single line.
- **R138 — Windowed 1080p crash fix** ([issue #31](https://github.com/VTSTech/D2VP-QoL/issues/31),
  owner-closed 2026-11-09). Selecting 1920x1080 while running windowed (`-w`) crashed the
  game on the first world frame — fullscreen was fine. The software tile blitters' row-offset
  lookup table was sized for retail's tallest resolution (700 rows) and read past its end at
  the fork's taller stops, handing the wall-tile blitter wild write destinations. The table is
  sized for the 1080p stop now (one constant). Field-verified 2026-11-09
  ("R138 confirmed closes #31").
- **R137 — Proc tooltip fix** ([issue #33](https://github.com/VTSTech/D2VP-QoL/issues/33),
  owner-closed 2026-11-08). Magic/rare items with a "Chance to Cast" affix showed
  "<x>% Chance to Cast Level 0" with no skill name and no event tail. Display-only —
  the items' hidden proc stats were always correct. The tooltip formatter now passes
  the real level and skill name, as vanilla did. Field-verified 2026-11-08
  ("R137 confirmed closes Issue #33").
- **R136 — Countess-chest crash fix** ([issue #29](https://github.com/VTSTech/D2VP-QoL/issues/29),
  owner-closed 2026-11-08). Killing The Countess crashed the game seconds after death,
  as the scripted smoke-to-chest sequence opened her reward chest — the chest's potion
  reward returned a memory address instead of the potion's item code, so every kill
  halted at the first drop. The potion tags are returned by value now; the issue repro
  is the standing regression check. Field-verified 2026-11-08
  ("R136 confirmed closes #29").
- **R130 — the tenth upstream sync** (merge-only; upstream `dd2cbd94..30384f1a`, Ghidra DB → 1970, 6 commits, 1875 files: the module-directory restructure — every Blizzard module under `Source/<module>/`, Storm under `3rdparty/Storm/`, the shared headers under `Source/D2Headers/`, support under `Redivivus/` — plus one static library per module from `project.json` with a module-gate report, and the accessor refile round). Resolved per docs/merge-upstream.md with git rename detection carrying the fork's files into the new layout; the per-module visibility gates' fallout fixed (the UI globals include, two fork accessors re-homed outside module-private gates, a fork-owned D2Sound bridge TU for the boot verdict); the merge tools re-pinned to the new tree. Compile gate of record: all 492 TUs `-fsyntax-only` clean with the exact cross flags; audits green (519/519, include-path/refile-aliasing/placement/branch/visibility clean). Full link + in-game smoke = owner-side.
- **R129 — the ninth upstream sync** (merge-only; upstream `b8e3087f..dd2cbd94`, Ghidra DB → 1969, 6 commits, 366 files: "globals in the modules that own them" — 558 globals moved into owning-module namespaces and file-local globals refiled into their using TUs as statics, plus msvc8 tooling rounds). All 20 conflicted files resolved per docs/merge-upstream.md; 225 fork lines re-spelled for the namespace move; Direct3D's gadwBitMasks re-regen NOT taken (the field-proven literal-mask forms stay, build-independent); the merge-site syntax sweep caught and fixed a party-switch splice; audits green (519/519, sweep 0, include-path/refile-aliasing/placement/branch/visibility clean).
- **R128 — Gheed gamble crash fix** ([issue #32](https://github.com/VTSTech/D2VP-QoL/issues/32),
  owner-closed 2026-11-06). Clicking **Gamble** crashed the game on every round since the
  source regeneration — the gamble screen's per-level item-count lookup read a table that
  no longer sits where the original binary kept it. Fixed with a bounds-checked accessor;
  new `gamble:` lines in `d2boot.log` pin the fix in the field. Field-verified 2026-11-06.
- **R127 — vpdiag retirement.** The R120 wide-view diagnostic probes leave the tree
  whole (the R103 diag-retirement class); zero behavior change — the wide-view fixes
  the probes shipped with stay. `d2boot.log` no longer carries `vpdiag` lines.
- **R126 — Shift-compare shield-slot fix; release line rolls 0.0.7 → 0.0.8.** With both
  arm slots occupied, a shield/quiver hover now compares the off-hand item and a weapon
  hover the primary — the same-kind slot pick retail's own auto-equip applies.
  Field-verified 2026-11-06 ("Great job, works").

## [R106–R125] — the 0.0.7 line's later rounds (shipped as per-round tags)

- **R125 — Shift Item Compare** (slate row 0b): hold **Shift** over an equippable item
  to draw the equipped item's tooltip beside it — inventory, stash, cube, trade screen
  and the vendor (shop stock included). Options-page row 9.
- **R124 — D3D fullscreen boot-crash fix** — the eighth-sync merge split the
  texture-cache node-base spelling and the first texture re-upload dereferenced a
  near-null base. Field-verified 2026-11-06 ("d3d fullscreen works again").
- **R123 — the eighth upstream sync** (Ghidra DB → 1948: 693 unreferenced functions
  defined, module-local symbols refiled into their using modules, Duriel's death no
  longer crashes — upstream-side).
- **R117–R122 — the wide-view rendering campaign** at the wide stops: edge-void band
  fill (R117), BFS room walk extended to hop 4 (R118/R121), reach-unit fix (R119),
  vpdiag instrumentation (R120 — retired R127), fill un-gate + snapshot triggers
  (R121), cull-rect re-fit (R122).
- **R116 — windowed 1366x768 shear fix** (the GDI DIB renderer rode the logical width
  as scanline pitch).
- **R115 — 1920x1080**, the video-options slider's sixth stop (wrapper pairing:
  Glide3xVK 0.4.8).
- **R114 — Glide3xVK 0.4.7** wrapper entry for 1366x768 (wrapper-side, no game change).
- **R113 — esc-menu hotfix** (R112's struct growth broke mouse navigation and label
  writes).
- **R112 — hosted stops stretch to fill the host** (edge-to-edge, cursor mapped back)
  **+ 1366x768 fifth stop**.
- **R110/R111 — D3D 1344x700 fullscreen presentation** (probe-first mode entry,
  host-sized frame).
- **R109 — 1344x700 fourth stop** (retail's dormant mode 3, first exposure).
- **R108 — Mana Burn Fix joins the Vanilla+ Options page** (eighth row).
- **R106/R107 — `mana_burn_fix`**: Mana Burn uniques drained 256x the intended mana
  with melee hits (512x in Hell); the opt-in fix drains the designed 66–100% of
  monster-level damage (33–50% for Nightmare/Hell minions), matching D2R post-2.4.
- Repo housekeeping: repository renamed **`D2VP-QoL`**; the glide wrapper completed its
  spin-out to [Glide3xVK](https://github.com/VTSTech/Glide3xVK).

## [R105] — 2026-11-04

- The sixth upstream sync (Ghidra DB → 1894; merge round).

## [R104] — 2026-11-04

- Per-TU restructure fallout sweep (SCmd roster mark + TU-private VP accessors
  re-homed). The build also carried the fourth/fifth syncs (R99/R102), the unit-sprite
  pan fix under `no_panel_shift` (R100), the Options row (R101) and the diag retirement
  (R103). **The full smoke belt passed on this build**
  ([#28](https://github.com/VTSTech/D2VP-QoL/issues/28)).

## [R103] — 2026-11-04

- Diag retirement: `d2light-diag` + `d2drop-diag` + hooks out; zero behavior change.

## [R102] — 2026-11-04

- The fifth upstream sync (Ghidra DB → 1892, 459 files; merge round).

## [R101] — 2026-11-04

- `no_panel_shift` leaves ini-only: the **No Panel World Shift** row joins the Vanilla+
  Options page (issue #26 round 9).

## [R100] — 2026-11-04

- `no_panel_shift` unit-sprite pan fix: all 14 unit-anchored consumers ride the border
  neutralizer — "no_panel_shift is working as intended now".

## [R98] — 2026-11-03

- The D3D perspective-toggle crash **root-fixed**: the flat-wall builder's `uint8_t`
  subtile iterator wrapped after 12 subtiles and read +4-shifted pseudo-records —
  latent since the original decompilation; widened to `int`.

## [R97] — 2026-11-03

- D3D texture-cache promote guards + re-armed transition probes (the R96 blind spot).

## [R96] — 2026-11-03

- D3D perspective-toggle triage probes (five one-shot `d2boot.log` marks; issue #26
  round 5).

## [R95] — 2026-11-03

- **No Panel World Shift** (`[features] no_panel_shift`): with one panel open at the
  wide layouts the world — terrain, units, effects — stays exactly where it was; the
  panel floats over an unmoved camera instead of the vanilla quarter-screen slide.

## [R94] — 2026-11-03

- Issue #26 round 3: the border-3 world-draw pass + bottom-band cuts ("both panels
  open, no more blackness").

## [R93] — 2026-11-03

- Issue #26 round 2: non-perspective half-cuts in every renderer + the border-3
  scene-list skip.

## [R92] — 2026-11-03

- Issue #26 round 1: window-fraction panel-cull family (single-panel margins at 720p).

## [R91] — 2026-11-03

- Docs + field-verify round: the third sync field-run green; the necro minion
  skeleton fix confirmed ("Skeletons are finally white again", [#8](https://github.com/VTSTech/D2VP-QoL/issues/8)).

## [R90] — 2026-11-03

- The third upstream sync (Ghidra DB → 1855) + R90b build fixes; FIELD-RUN GREEN.

## [R89] — 2026-11-02

- Monster component-model data fix (owner-applied; skeleton palette-ownership
  hardening).

## [R88] — 2026-11-02

- The second upstream sync (Ghidra DB → 1851): the supersession ledger opens —
  upstream wins on the sound-union fix and the R54/R85 gossip hand-tables (retail-exact
  tables recovered upstream); fork root-cause notes stay in code.

## [R87] — 2026-11-02

- The first upstream sync of the line (Ghidra DB 1806 → 1839); field-run green.

## [R86] — 2026-11-02

- The `-nointro` boot switch: no logo videos, straight to the title screen.
  Field-verified ("R86 verified, -nointro works").

## [R85] — 2026-11-02

- A1Q2 Bloodraven turn-in fix ([#22](https://github.com/VTSTech/D2VP-QoL/issues/22),
  owner-closed 2026-11-02).

## [R84] — 2026-11-02

- Item-tooltip zero fix ([#16](https://github.com/VTSTech/D2VP-QoL/issues/16)): "0
  poison damage over 0 seconds" / "+0 Cold Damage" lines now show the real rolled
  values — tooltip-only, the item stats were never wrong.

## [0.0.7] — 2026-11-01/02

The first 0.0.7 build is the R66 content state (tag `R67`); the R67→R77 block shipped
as tag [`R77`](https://github.com/VTSTech/D2VP-QoL/releases/tag/R77); `R78`–`R86` mark
fix rounds, `R87`/`R88` the sync rounds.

### Added

- **Shared Stash** (SP): an outlined "Shared" tab over retail-shaped stash pages every
  character on the PC shares — items and gold, one pool, own `_d2vp_shared.sss` file;
  `.d2s` files stay retail-compatible. Round 1 (one page) R63–R66; outlined label +
  second page + pager arrows R75; **ten pages** R82. Field-proven end to end.
- **Item Level Tooltips** (`item_ilvl`, R55–R59): ` (N)` appended to every item name
  line.
- **Vanilla+ Options page**: every feature also toggles in-game (Esc → Options).
- The smoke checklist ships as an interactive GitHub issue template (R83+).

### Fixed

- Waypoint panel row icons ~120 px above their buttons at 720p
  ([#21](https://github.com/VTSTech/D2VP-QoL/issues/21), R81).
- Video-options 1280x720 label alignment
  ([#23](https://github.com/VTSTech/D2VP-QoL/issues/23), R83).
- Alt-toggle stale selection: walk-clicking away from a latched label picked the item
  up ([#15](https://github.com/VTSTech/D2VP-QoL/issues/15), R58).
- Stash tab geometry: pager arrow over the gold-amount text (R76 nudge, R77 lift).

### Changed

- Upstream syncs to Ghidra 1806; every merge field-run green, and the full SMOKE belt
  passed on the R83 build ([#24](https://github.com/VTSTech/D2VP-QoL/issues/24)).
- `tools/syntax-check.sh` host syntax net added (R71); shipped client renamed
  `d2vp.exe` (R74/R96).

## [0.0.6] — 2026-10-30 ([`D2LoD-Vanilla-Plus-0.0.6`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.6) @ `0eb2655`)

The 720p release: native 1280x720 widescreen; loading-screen freeze fixed; sound
constants repaired; the Vanilla+ Options page visible; inventory, skill flyouts and
dialogs aligned at 720p. Post-release rounds shipped as tags `R47`–`R62`:

- Glide `-3dfx` first-frame crash (R46, [#11](https://github.com/VTSTech/D2VP-QoL/issues/11)).
- Auto Gold Pickup restored (R47, [#10](https://github.com/VTSTech/D2VP-QoL/issues/10)).
- 720p deposit/withdraw gold dialogs (R48, [#12](https://github.com/VTSTech/D2VP-QoL/issues/12)).
- Unique monster palettes (R49, [#9](https://github.com/VTSTech/D2VP-QoL/issues/9)); necro
  minion skeletons gold (R50/R51, [#8](https://github.com/VTSTech/D2VP-QoL/issues/8)).
- 720p belt hover popup (R52, [#14](https://github.com/VTSTech/D2VP-QoL/issues/14)).
- A1Q1 Den of Evil never completed (R54, [#13](https://github.com/VTSTech/D2VP-QoL/issues/13)).
- Alt-toggle stale selection (R58, [#15](https://github.com/VTSTech/D2VP-QoL/issues/15); fixed for 0.0.7).
- Save-and-re-enter crash (R60/R61, [#19](https://github.com/VTSTech/D2VP-QoL/issues/19),
  field-verified "R61 Confirmed fixes #19").
- Upstream to Ghidra 1783 (R62), field-run green + full smoke pass.

## [0.0.5] — 2026-10-29 ([`D2LoD-Vanilla-Plus-0.0.5`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.5))

Per-class player sounds restored (77 values byte-matched from the retail exe);
full-inventory pickup crash and walk-click mis-pickup fixed; **Persistent Globe Text**
(the 0.0.4 globe-hover readout reworked to retail's own always-on above-orb display).

## [0.0.4] — 2026-10-29 ([`D2LoD-Vanilla-Plus-0.0.4`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.4))

Auto Gold Pickup; globe hover values.

## [0.0.3] — 2026-10-28 ([`D2LoD-Vanilla-Plus-0.0.3`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.3))

Portable saves (`[features] portable_save` — saves read/write `<game folder>\save`,
registry untouched); alt-toggle click-through fix; diagnostics quieted (probe
retirement).

## [0.0.2] — 2026-10-28 ([`D2LoD-Vanilla-Plus-0.0.2`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.2))

First opt-in feature: the alt-key item-label toggle (`[features] alt_toggle`), with
self-provisioning `d2vp.ini`.

## [0.0.1] — 2026-10-27 ([`D2LoD-Vanilla-Plus-0.0.1`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.1))

First public release. Glide-renderer defect series closed (fixes 1–13), item drops
fixed, sound fixed.

## Wrapper tags

`0.3.1`, `0.4.1`–`0.4.8` (the [Glide3xVK](https://github.com/VTSTech/Glide3xVK) repo —
formerly the branch of the same name here) — `glide3x.dll` wrapper versions; the
current release 7z ships wrapper **0.4.8** (0.4.6 knows 1344x700, 0.4.7 adds 1366x768,
0.4.8 adds 1920x1080).
