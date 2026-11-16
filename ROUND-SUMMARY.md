# Round summary — R158 (rev 174, 2026-11-16): extended_stats — the character screen's second view

## The ask

Owner, 2026-11-16, after field-verifying the R157 cube legs ("Ctrl+Click working fine for Cube in R157"):
"Implement extended stats plan" — the same-day plan at plans/EXTENDED-STATS-PLAN.md (§9 open calls settled
to the plan's defaults: tab-flip inside the character screen, all twelve rows, zero rows shown as 0%).

## The design

- **The sites moved vs. the plan's guess**: the character screen is `DRAW_UI_StatWindow_I` in
  `ui.cpp:9422` (gated on `GFX_bShowUIFlag.CharStat`), not Panel.cpp — the recon re-anchored both
  the draw (the function's tail at ui.cpp:10044, after the hover-tooltip windows II/III) and the click
  (`INPUT_StatScreenMouseDown`'s head, ui.cpp:9195).
- **Display-only pillars kept**: `SERVER_GetUnitStat` reads only; zero packets; gate off = the draw
  hook returns before any pixel and the click hook before any consumption.
- **The safety edge the plan's §6 called out**: with the page up, the click head consumes the whole
  body rect BEFORE the retail hit-tests — the covered +stat buttons never arm, so an invisible button
  can never spend a stat point; the Done button band stays outside the consume rect.
- **The twelve rows** ride one static table (`gaVpExtStatsRows`) with the verified stat ids
  (80/79/85/87/67/93/105/99/102/89/110/118 — d2_enums.h), kind-tagged (%i%% / %i / Yes--).
- **The tab** reuses the shared-stash tab's outlined-text idiom (`VPSHAREDUI_DrawOutlinedText`,
  same TU) with ONE shared measure for draw + click band; the flip plays the retail button-click
  sound (the Done-button idiom); fully-qualified `D2Client::Sound::SOUND_PlaySound` — core sits
  outside `namespace D2Client`.
- **The view reset**: a 2 s draw-gap check (no close hook — the automap clock's per-frame shape).
- **The Options row** filled column 1's band 7 — the band R155's geometry already reserved — so
  the back row and box height are UNCHANGED: two full columns of eight, zero layout move.

## Verification

Static only: symbol audit (StatsEx.h + D2gfx D2GFX.h already in the TU's include set; the Sound
namespace qualification fix), brace/flow re-read of both VP blocks, the layout arithmetic re-derived
and printed in the layout comment. Compile gate of record: the owner's Win11 MSYS2 build.

## R158.1 — in-round compile fix (owner build log 2026-11-16)

The owner's first R158 build caught ONE error: `STRING_CopyCharToWCharWithSetMaxSize` takes `char*`
(`Source/D2Lang/Unicode/UNISYS.h:35`) and the row table ships `const char*` labels — fixed with a
`const_cast` at the copy site (the function never writes the source; the table stays const).
Post-fix audit against real declarations: all 12 `UNITSTAT_*` ids confirmed in `d2_enums.h`, and
`wsprintfA`/`GetTickCount` confirmed declared in the force-included `d2_platform.h` (lines 364/359).
Everything else in the section compiles against signatures already used by the TU.

## R158.2 — field fix (owner field report 2026-11-16, first R158 build)

Two bugs, both in the new section's read/layout assumptions:

1. **Every value read 0%, Faster Run/Walk read a spurious 100%.** Root cause:
   `SERVER_GetUnitStat` reads ONLY the root statlist's base array
   (`STATLIST_LookupBaseStatWithMinAccr`, StatsEx.cpp:591) — item mods live in
   child statlists it never walks, so all gear stats read 0. The 100 is the
   ItemStatCost **minaccr clamp**: velocitypercent IS in the player's base
   array (at 0) with the FMIN flag, and the clamp lifts any sub-minaccr value
   to `minaccr << valshift` (= 100). Fix: the row loop now calls
   **`UNITS_CalcCombinedStatValue`** (StatsEx.h:159, impl StatsEx.cpp) — the
   game's own effective-value computation: base + every child statlist via
   `STATLIST_CalcFullStatFromChildren`, NO display clamp. Retail corroborates
   the semantics: `STATLIST_GetStatBonusFromLists` (Inv.cpp:2858, the IAS
   tooltip) is exactly `clamp(full) − clamp(base)`, i.e. the same
   children-walk with both terms clamped — useless for our rows because both
   terms clamp at once and cancel real gear below minaccr.

2. **The tab word rode the panel's top edge** ("drawing below the art, I can
   see just a tiny portion of it"). Root cause: the draft baseline −0x1db put
   the glyph halo (baseline−13..baseline) 8 px ABOVE the panel top edge
   (−0x1e0) — the documented R69 float-word class — while the click band
   (nY−4..nY+14) still caught the word's bottom, which is why clicking worked.
   Fix: baseline **−0x1cc** = panel top + the R77-proven 0x14 stash inset
   (halo top −0x1d9, 7 px inside the edge; the char panel art spans
   −0x1e0..−0x30 in both modes, so one constant serves classic and
   expansion), and the click band now hugs the glyph span exactly like the
   stash tab (nY−16..nY+6; the old shape only caught the bottom sliver once
   the baseline moved).

Re-verified after the patch: `UNITS_CalcCombinedStatValue` is declared inside
`D2Common::Stats::StatsEx` (StatsEx.h:41..184), players' root statlists carry
the 0x80000000 full-list flag the wrapper requires (same precondition
`STATSEX_GetCurrentStat` relies on in CmdTbl), and no stale `SERVER_GetUnitStat`
call remains in the section. SMOKE §W 92/93 carry the regression asks (FRW gear
value, never 100 ungated; word fully inside the panel).

## R158.3 — compile fix of the field fix (owner build log 2026-11-16)

The R158.2 getter (`UNITS_CalcCombinedStatValue`) is declared in StatsEx.h :159
but inside the module-private guard
`#if !defined(D2_MODULE_LIBRARIES) || defined(D2_MODULE_D2Common)`
(StatsEx.h :121..181) — the D2Client TU defines `D2_MODULE_LIBRARIES`, so the
generated header compiled the declaration out: "error: 'UNITS_CalcCombinedStatValue'
is not a member of 'D2Common::Stats::StatsEx'". The sanctioned surface for this
is the R152 precedent, verbatim: hand re-exposure in `Redivivus/d2_hand.h`
(force-included into every TU, never generator-emitted, regen-safe) with the
exact generated signature; the call links the existing D2Common definition —
no wrapper, no new code, the generated header untouched (it is "DO NOT EDIT").
Two static checks before shipping: (1) `eD2UnitStat` is a plain global typedef
of `uint32_t` (d2_enums.h :14592), so spelling the typedef early in d2_hand.h
is the same type with the same mangled signature and a legal redeclaration
later; (2) a two-side mock TU pair compiles and links with BOTH sides mangling
identically (`...EP10D2UnitStrcj`) — typedef-transparency and duplicate-decl
legality proven, scripts/mock_link_check.cpp keeps the harness.

## R158.4 — the tab label raised 30 px (owner ask, 2026-11-16)

"Can we move the 'Extended' label up about 30px" - the tab word's baseline
moved 30 px (0x1e) up, from the R158.2 stash inset (-0x1cc, 20 px BELOW the
panel top edge) to -0x1ea: 10 px ABOVE the panel top edge, so the whole glyph
spans -0x1f7..-0x1ea and floats in the top world strip, clear of the panel
frame seam the R158 draft straddled. One constant in VPEXTSTATS_TabLayout
(draw and click band share it, the VPSHAREDUI_TabLayout idiom); the click band
widened nY-16..nY+6 -> nY-26..nY+6 so it keeps hugging the glyph span (the top
of the glyph now reaches baseline-23, outside the old band). If the word looks
off in the world strip on the field pass, the constant dials 1 px per 0x1.

## R158.5 — the tab draw moved after the panel border art (owner field report 2026-11-16)

The R158.4 raise landed in the right spot but the word was invisible: the
retail UI pass draws `Panel::DRAW_UI_BordersAndOrbsAndFillers()` AFTER the
whole stat window (Draw.cpp, right behind `DRAW_UI_StatWindow_I`), and the
border pieces cover the strip above the panel top edge - owner: "its in the
right spot, but the art is drawing on top of it". Fix: the tab word got its
own exported draw, `VPEXTSTATS_DrawTab()` (same TabLayout measure, same
outlined-text, its own font save/restore), called from Draw.cpp's UI pass
tail immediately after the border art, gated on `GFX_bShowUIFlag.CharStat`.
`VPEXTSTATS_DrawOverlay()` keeps the page body + the 2 s draw-gap reset at
the field-proven call site (the page sits inside the frame, borders never
touched it in the R158 field pass); DrawOverlay no longer draws the tab.
Nothing retail draws after the border art reaches the strip above the char
panel (skill grids, stat icons, panel layout, mini panel, chat input are all
inside their own panels or the bottom strip). Position (-0x1ea baseline),
click band (nY-26..nY+6) and the flip behavior unchanged.

## Field ask

SMOKE **§W 92–96**: tab + flip; live values (base+47 MF gear test, plus the
R158.2 FRW regression ask); the covered +stat buttons stay dead with the Done
button alive; the 2 s reset + the gate flip both directions; the sixteenth row
at 640x480.
