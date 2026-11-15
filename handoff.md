# Port branch — handoff (2026-11-15 — d2.exe runs; archive #9 field fixes in)

The fork originals of this file and HISTORY.md live verbatim on `origin/main`
(Vanilla+0.0.8-R156 era); they are repurposed here for the port effort and the
fork docs will be archived once all features are restored in the port branch.
The port's plan/checklist stays in `port-plan.md`; the port's chronological
log lives in `HISTORY.md` (this port).

## 1. Mission and topology

- **Mission**: restore every Vanilla+ feature of the fork (`origin/main`,
  Vanilla+0.0.8-R156) on the upstream redivivus base, inside this working tree.
- **`origin/main`** — the fork's integration branch: the reference for every VP
  block, the source of truth for port bodies (with
  `asm_matching/msvc8/translated/` for translated bodies). Nothing is merged
  from it; files/blocks are recovered verbatim.
- **Port branch (working tree)** — upstream base + all VP files + port fixes.
  CI builds it from archive overlays (see the overlay rule, §4).
- **`build/`** — the sandbox build tree (mingw-i686 GCC 14 via
  `scripts/setup-mingw.sh`, wrapper `scripts/d2env.sh`); the owner's Win11
  build and CI remain the gates of record.

## 2. Build status

- **All 25 modules compile; d2.exe links** (0 errors, 0 undefined references,
  0 multiple definitions on a clean CI-equivalent rebuild).
- CI green after the overlay-aware duplicate-definition fix (archive 7 era).
- Module gates: strict modules crashy/Game/D2Lang/D2Net carry documented
  EXCEPTIONS (`project.json` + `modules.cmake` `{to,symbol,reason}` schema) —
  their restored globals units include cross-module internal headers, which the
  original one-flat-binary layout never needed.

## 3. Runtime status (first field report, 2026-11-15)

| Area | Status |
| --- | --- |
| Boot → intro → title → char select → world at 800x600; VP Options panel (two columns, fifteen rows), toggles flip live | **WORKING** (field) |
| Resolution stops above 800x600 (modes 3/4/5/6: 1344x700 / 1280x720 / 1366x768 / 1920x1080) | **FIXED this session** — `CLIENT_SetScreenResolution` had no wide cases, so `ScreenSizeX/Y` stayed 800x600 while the backbuffer switched: UI offset 0 (pinned top-left), cull rects + wide room walk trimmed to an 800x600 budget (black corners). Cases 3–6 added; compiles. Field test owed. |
| Shared stash (`shared_stash=1`) | **FIELD-VERIFIED FIXED** (2026-11-15) — client chain was complete (tab draw, click, C→S 0x4F button 0x7D) but the server dispatch was lost in the overlay era: PlayerTrade.cpp's `VPSHARED_ShouldInterceptClickButton`/`OnClickButton` block recovered verbatim from origin/main + include. Compiles. Field test owed. |
| Camera zoom (Z) | **FIXED (archive #11)** — field showed sprites zooming but not the view: the port's GAME/Draw.cpp was missing the fork's R155 floor/roof light-grid zoom sites (the ground tiles' vertex positions). Fork file recovered wholesale + Wall2.h decl. Field test owed: Z steps 1.0/1.25/1.5 on floors+walls+sprites together, UI stays 1:1, cursor click-through correct. |
| Tooltip background panel | **FIXED** — root cause was that tooltip text is rendered line by line, but background was drawn as single rectangle for entire tooltip. Fixed by drawing background line by line in D2WINFONT_DrawWideString function using gnFontIdSmall (16) for correct height calculation. Background now drawn before text for each line, ensuring proper transparency and text visibility. |
| Alt toggle (`alt_toggle=1`) | **FIELD-VERIFIED FIXED (2026-11-15, R157 build)** — root cause was the stale exe, not code: the P13 static audit (this session) had re-verified every link byte-for-byte against `origin/main` (CmdTbl dispatch/registration/defaults/entry 72 + `D2KeyBindStrc` layout, D2ClientRoot CMD bodies, VanillaPlusCore, ShowItems, CUnit, Game.cpp click-converge, ui.cpp Set/GetUIFlag, globals `AltShowItems` index 13, Message.cpp, DRAW_UI gate) and found no defect; the R157 rebuild then delivered the P12 CMD-handler fix to the field and the toggle came alive. R157 diag log shows the full healthy chain: `read alt_toggle=1 from F:\games\Diablo II\d2vp.ini` → `toggle mode engaged` → keydown `(toggle_mode=1 latch=0)` → keyup `(latch=1)`. The diag marks are permanent one-way lines (house style, cheap); they re-arm per process. **Still owed (SMOKE §C item 8 + R58 guard)**: label click = pickup, brush-label + empty-ground click = walk, combat click = attack, stale-selection guard; `alt_toggle=0` hold-mode sanity. |

## 4. Port-specific engineering rules (hard-won, keep following them)

- **Overlay delivery rule**: the user's repo is built by extracting each archive
  over the previous state — never delete tracked files from the working tree;
  only ship complete supersets. Never trust a green local build whose objects
  may be stale (the stale `.obj` masked 123 duplicate symbols once).
- **Body source of truth**: port bodies come from `HEAD:asm_matching/msvc8/`
  translated files, VP blocks from `origin/main` verbatim. Fork-branch typedef
  drift is resolved against origin/main / upstream 810e5018.
- **Namespace lesson**: before appending a definition, check the enclosing
  namespace state (the `D2Client::D2Client::...` and bare-`namespace SkillsBar`
  link failures) and prefer whole-body swaps from the translated tree over
  param retypes (three naive retypes broke working bodies).
- **Dedup collateral**: `scripts/dedup_d2client.py` (nm-intersect based) once
  ate four global forwarders — any nm-based edit is re-verified by objdump.

## 5. Field test protocol — next build (archive #9)

1. **Wide-res**: switch 800x600 → each of 720p/1344x700/1366x768/1080p in-session:
   UI centered (wide offsets live), no black corners, automap/walls/roofs cover
   the full frame; then boot-time re-apply of the saved stop.
2. **Shared stash**: `shared_stash=1` → "Shared" tab click switches to shared page
   1; `<<`/`>>` pager walks 1..100 and back to personal; item deposit/withdraw
   round-trips; the vacated-slot bug (#35 class) stays fixed.
3. **Tooltips**: **FIXED** — line-by-line background drawing implemented in D2WINFONT_DrawWideString.
   Field verification: tooltip backgrounds should appear properly with 75% transparency
   for both regular tooltips and shift-compare tooltips.
4. **Alt toggle (R157 diag) — ANSWERED 2026-11-15**: the R157 rebuild delivered
   the P12 fix and the toggle works; the log showed the full healthy chain
   (ini read=1 at the exe's own path → engaged → keydown latch=0 → keyup
   latch=1). Root cause confirmed as the stale exe (overlay rule §4). The
   diag marks stay in as permanent one-way lines. **Still owed from SMOKE §C
   item 8**: with labels latched — label click = pickup; brush a label then
   click empty ground far from any item = walk (no pickup, no walk-to-loot);
   sprite click = pickup; combat click = attack; and `alt_toggle=0` hold-mode
   unchanged.

## 6. Technical notes

### Tooltip background rendering fix (2026-11-15)

**Problem**: Tooltip backgrounds only appeared on the last line of multi-line tooltips.

**Root cause**: Tooltip text is rendered character by character, with newlines handled
by moving to the next line. However, background was drawn as a single rectangle for
the entire tooltip, which only covered the last line due to coordinate miscalculation.

**Solution**: Implemented line-by-line background drawing in `D2WINFONT_DrawWideString`:

1. **First line**: Draw background at function start using `gnFontIdSmall` (16) for correct height
2. **Subsequent lines**: Draw background when encountering newline, after moving to next line
3. **Font height**: Use `gnFontIdSmall` (16) instead of `gnFontIdBody` (3) for accurate background height
4. **Rendering order**: Draw background before text for each line to ensure text appears on top

**Key files modified**: `Source/D2Win/Src/D2WinFont.cpp`
**Functions modified**: `D2WINFONT_DrawWideString`, `Draw_UnitNameUnderCursor`, `DRAW_CompareTooltipUnderCursor`

**Result**: All tooltip lines now have proper black backgrounds with 75% transparency,
and text appears clearly on top of the background.

## 6. Document map

- `port-plan.md` — plan, file inventory, per-round build fix details.
- `HISTORY.md` — the port's chronological session log (repurposed).
- `SMOKE-CHECKLIST.md`, `FEATURES.md`, `VANILLA-PLUS.md` — fork QA/feature docs,
  reused as the field protocol and feature ledger for the port.
