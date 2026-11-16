# Extended Stats page — feature plan

*Drafted 2026-11-16 on owner ask (chat session, no issue). The ask, verbatim:* "draft a
plan for a 'Extended Stats' page showing stats we cannot see like Magic Find, Gold Find".
Status: **IMPLEMENTED — R158 (rev 174, 2026-11-16); R158.1 compile fix; R158.2 field fix; R158.3 compile fix (d2_hand.h re-exposure of the getter, the R152 pattern — StatsEx.h declares it module-private) (2026-11-16)** — first field pass caught two bugs, both fixed same day: every value read 0% with Faster Run/Walk a spurious 100% (the draft's SERVER_GetUnitStat reads only the root base array — item mods live in child statlists — and the ItemStatCost minaccr clamp fired on velocitypercent; now UNITS_CalcCombinedStatValue, base+children, no clamp), and the tab word rode the panel's top edge (baseline −0x1db → the R77-proven −0x1cc stash inset). Field QA continues on SMOKE §W 92–96. Two deviations from the draft: the draw/click sites live in **ui.cpp** (`DRAW_UI_StatWindow_I` tail + `INPUT_StatScreenMouseDown` head — the plan's Panel.cpp guess was wrong; the character screen is the stat window, not the control bar), and the §9 open calls settled to the plan's defaults (tab-flip, all twelve rows, zeros shown). Slate candidate (would be row 3 of the Ctrl+Click
family's neighborhood — a new family: read-only stat display). Nothing here is
field-verified; every symbol below was grepped and read in this tree at rev 173.

---

## 1. Goal and non-goals (v1)

**Goal.** With `[features] extended_stats=1`, the **character screen** (the "C" panel)
gains a second view — a small read-only "Extended Stats" block, toggled in place — that
shows the stats retail LoD hides: Magic Find, Gold Find, and the rest of the §5 list,
computed live from the player's actual statlist (base + items + quest/shrine buffs),
the same numbers the drop and gold code already use.

**Non-goals for v1** (each a clean follow-up, deliberately deferred):
- No HUD/always-on overlay — the character screen only (an automap-corner mini readout
  is a natural follow-on once the page exists).
- No hireling/merc stats page (the merc's own statlist is reachable — §3 — but the
  display layout doubles; defer).
- No stat-history or DPS calculator — display only, no derived model, no balance
  surface. Nothing writes stats; the page is byte-read-only.
- No client-only fake numbers — every displayed value must come from the live statlist
  via the same getter the game code uses, so what the page says IS what the server
  rolls.
- No tooltip redesign / no changes to the existing character-screen rows.

## 2. The hard invariants (pillars)

1. **Zero packets, zero server code.** The page only READS the player's statlist
   client-side. No C→S message, no D2Game edit, nothing realm-facing — this is
   the display-only cousin of the Ctrl+Click family.
2. **Byte-read-only.** No stat write anywhere on the path. `SERVER_GetUnitStat` (§3)
   is a pure read; the page never touches `STATS_SetUnitStat` or the item mod lists.
3. **Default OFF = retail byte-for-byte.** With the gate off the character screen
   draws exactly retail pixels; no extra draw call, no text, no hit-rect. The gate is
   the standard charter shape (2 s hot-read + the Options write bridge).
4. **No layout collateral.** The extended block draws only while it is toggled in
   place of nothing — the retail character screen stays the "page 1"; the extended
   view REPLACES the stat-allocation block's pixels only while active, and the
   allocation buttons keep their hit areas (they are simply not drawn over; see §6
   for the v1 draw shape).

## 3. Where things live today (recon, this tree — all verified at rev 173)

- **The stat read primitive**: `D2Common::Stats::StatsEx::SERVER_GetUnitStat(pUnit,
  eStat, 0)` — the idiom every panel already uses (`Panel.cpp:2007` for level,
  `Inv.cpp:6272` for maxhp, the hireling stat rows at `Inv.cpp:6242`). It returns the
  COMBINED statlist value (base + equipped items + temporary buffs) — exactly what the
  drop rolls use for MF/GF. The player unit: `UNIT::Player::GetPlayerUnit_UI()` (the
  panel idiom, `Inv.cpp:9117`).
- **The stat IDs** (all verified in `D2Headers/d2_enums.h`):
  `UNITSTAT_item_magicbonus = 80` (Magic Find %), `UNITSTAT_item_goldbonus = 79`
  (Gold Find %), `UNITSTAT_item_addexperience = 85` (+% XP gained),
  `UNITSTAT_item_reducedprices = 87` (vendor price reduction %),
  `UNITSTAT_item_lightradius = 89` (light radius), `UNITSTAT_velocitypercent = 67`
  (FRW), `UNITSTAT_item_fasterattackrate = 93` (IAS), `UNITSTAT_item_fastercastrate
  = 105` (FCR), `UNITSTAT_item_fastergethitrate = 99` (FHR),
  `UNITSTAT_item_fasterblockrate = 102` (FBR), `UNITSTAT_item_poisonlengthresist
  = 110` (poison length reduction %), `UNITSTAT_item_halffreezeduration = 118`
  (half freeze duration). Presence of the enum is NOT presence of a rolled value —
  §5 explains the 0-value handling.
- **The draw site**: `PANEL_DrawCharacterInfo` (`Panel.cpp:1481`, called at
  `Panel.cpp:2259`) — the character screen's body. A VP block insert-after its tail
  (the R154 transparent_hud round already touched sibling sites in this file — the
  overlay workflow and markers are established here).
- **The UI precedents in-tree**:
  - `VPSHAREDUI_DrawTab` (VanillaPlusCore.cpp) — outlined text, saved/restored font,
    rect hit-test, page-aware layout math: the model for the page-flip affordance.
  - The Options page (`VanillaPlusOptions.cpp`) — the two-column row page, the box
    height arithmetic, the Previous Menu row: the model if the extended view becomes
    a full page of rows instead of one text block.
  - The automap clock (`AUTOMAP_*`, R148) — the model for per-frame recomputed
    single-line text with a hot-read gate.
- **The gate plumbing**: the `D2VP_GetFeatureToggle` + 2 s cache + `D2VP_ConfigReloaded`
  write-bridge pattern (e.g. `VPCTRLXFER_Active`, VanillaPlusCore.cpp:700) — copy the
  shape as `VPEXTSTATS_Active()`. Ini template + one Options row (row 16 — see §7 for
  the page math).

## 4. Design (v1 shape)

**The affordance.** While the character screen is open and the gate is on, the
retail "stat allocation" area keeps drawing exactly as retail; a small outlined
"Extended" tab (the shared-stash tab idiom) sits in the character screen's header
band. Click it → the panel's middle column (the stat list + buttons area) is
overdrawn by the extended stats block: 8–12 left-aligned text lines (§5), one stat
per line, label + value. Click the tab again (it stays visible, now reading
"Stats") → back to retail. State is a single static `gbExtStatsView` reset to
retail view when the panel closes (`CloseCube`-style reset slot / the panel-open
flag check at draw time — the automap clock's per-frame shape, no close hook
needed).

**The values.** One `SERVER_GetUnitStat` per displayed stat per drawn frame while
the view is active (≤ 12 reads/frame, reads are table walks the panels already do
several of per frame — no caching needed for v1; a 1 s value cache is the
documented follow-up if the frame cost ever shows). Formatting is the plain-ASCII
digit idiom the tooltips use (the ilvl suffix precedent), locale strings where a
retail one exists (e.g. the MF wording rides strtable if a suitable row exists —
else plain English labels like the Options page rows, which are English-only by
convention).

**Zero-value handling.** Many of these stats read 0 when no item grants them
(no +MF gear → stat 80 reads 0). v1 shows every line always, including `0%` —
predictable layout beats conditional lines; §9 call 3 offers the hide-zero
alternative.

**MF/GF display convention.** Vanilla shows nothing; D2R's stats page shows
"% Better Chance of Getting Magic Item". v1 labels: `Magic Find: 175%`,
`Gold Find: 230%` — the community-standard names, matching the ask verbatim.

## 5. The v1 stat list (what we cannot see today)

Order on the page (grouped, most-asked first):

| # | Line label | Stat (enum) | ID | Why retail hides it |
| --- | --- | --- | --- | --- |
| 1 | Magic Find | `UNITSTAT_item_magicbonus` | 80 | rolled by the drop code only |
| 2 | Gold Find | `UNITSTAT_item_goldbonus` | 79 | rolled by the gold-drop code only |
| 3 | +% Experience Gained | `UNITSTAT_item_addexperience` | 85 | XP roll only |
| 4 | Vendor Price Reduction | `UNITSTAT_item_reducedprices` | 87 | shop price calc only |
| 5 | Light Radius | `UNITSTAT_item_lightradius` | 89 | renderer-side |
| 6 | Faster Run/Walk | `UNITSTAT_velocitypercent` | 67 | movement calc only |
| 7 | Increased Attack Speed | `UNITSTAT_item_fasterattackrate` | 93 | anim-rate calc only |
| 8 | Faster Cast Rate | `UNITSTAT_item_fastercastrate` | 105 | cast-delay calc only |
| 9 | Faster Hit Recovery | `UNITSTAT_item_fastergethitrate` | 99 | stun-recovery calc only |
| 10 | Faster Block Rate | `UNITSTAT_item_fasterblockrate` | 102 | block-anim calc only |
| 11 | Poison Length Reduction | `UNITSTAT_item_poisonlengthresist` | 110 | status-length calc only |
| 12 | Half Freeze Duration | `UNITSTAT_item_halffreezeduration` | 118 | flag-like; shown as "Yes"/absent |

Notes: FCR/FHR/FBR/IAS breakpoints are NOT computed in v1 (raw values only — the
breakpoint tables are class/state-dependent and are §8 follow-up territory).
`item_halffreezeduration` is a boolean-ish stat — display "Yes" when > 0. The
 melee-/skill-DCR-family stats are deliberately out of scope (v1 keeps the list to
what the community reads off D2R's stats page).

## 6. Draw mechanics (v1)

- Insert-after VP block at the tail of `PANEL_DrawCharacterInfo` (Panel.cpp:1481's
  function body), markers `// VP-BEGIN/END [extended-stats]`. Gate off → the block
  returns before any draw (byte-retail pixels).
- The tab affordance + the block itself draw with the panel's own font stack
  (`D2Win::Src::D2WinFont::SetAndReturnLastFont` save/restore, the shared-tab
  idiom) so no font state leaks into the retail rows drawn before/after.
- Hit-testing rides the existing panel mouse pass — the tab is a small rect
  checked in the same VP block's click half (the shared-stash tab's click shape:
  rect hit-test in the panel's click path, no new input hook). v1 keeps the
  click handling inside the character screen's existing click dispatch (verify
  the exact site when implementing — `Panel.cpp` owns the char-screen clicks).
- No renderer work — plain `DRAW_text` calls, panel-space coordinates like every
  sibling; works identically on DDraw/D3D/Glide by construction (the R154 lesson:
  stay upstream of the renderer).

## 7. Gate, ini, Options row

- `[features] extended_stats`, default 0, the standard 2 s hot-read +
  write-bridge cache-bust (`VPEXTSTATS_Active()` in VanillaPlusCore.cpp; the
  cache DWORD busts in `D2VP_ConfigReloaded`).
- Ini template: the documented-all-defaults block gains its paragraph (the
  ctrl_click_xfer template shape).
- Options row 16 ("Extended Stats") — the page arithmetic must be re-run: R155
  left the page two columns of EIGHT with FIFTEEN rows; row 16 makes it two
  columns of eight exactly full (16 = 2×8, no second-of-eight gap) — verify the
  box height + Previous-Menu offset math the R140/R155 rounds each re-derived,
  and the 640x480 whole-page check (SMOKE item 2).
- d2boot marks: none (display-only; no latches, no one-way state — the clock
  precedent).

## 8. Follow-ups (explicitly deferred)

- Breakpoint readouts (cast/hit-recovery breakpoints computed per class).
- A merc side (the hireling statlist is already walked at `Inv.cpp:6242`).
- An automap-corner mini readout (MF/GF only) for loot sessions.
- Value caching if the per-frame reads ever cost (they won't at 12 reads).
- Resistances-overview block (retail shows resists, but not the cap math —
  a "resist + max-resist" line pair is a candidate if the owner asks).

## 9. Open calls (owner decisions before implementation)

1. **View shape**: tab-flip inside the character screen (this plan's v1) vs. a
   separate Options-style page on its own panel. Tab-flip is smaller and keeps
   the context; the separate page scales to more rows later.
2. **Row set**: all 12 of §5 vs. the loot pair only (MF/GF/XP/prices — 4 lines).
   The 12-line set matches D2R's advanced page expectations.
3. **Zero lines**: show `0%` always (v1 plan) vs. hide zero rows (shorter list,
   jittery layout).
4. **Effort call**: assessed **S–M** — one file (Panel.cpp) + gate + template +
   row, no server half, no packets; the risk sits entirely in the char-screen
   click dispatch (§6) and the Options-page arithmetic (§7).

## 10. Verification sketch (for the implementation round)

- Static: VP marker pair + brace balance; gate-off byte-retail (no draw call
  reachable); the 12 getters' enum IDs re-grepped against `d2_enums.h`; font
  save/restore pair; Options-page math re-derived and printed.
- Field ask (SMOKE, one new section): tab flips without artifacts; values match
  a known gear set (e.g. +47 MF armor visible as +47); gate off = retail pixels;
  640x480 whole page; gate flips land ≤ 2 s; no interaction with the
  stat-allocation buttons while the extended view is up (their hotkeys keep
  working — the block never consumes input).
