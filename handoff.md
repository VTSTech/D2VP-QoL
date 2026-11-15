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
| Tooltip background panel | **OPEN** — no static defect: publish → Draw_UnitNameUnderCursor → DrawSolidRectEx(0, DRAW_LESSTRANSPARENT=75% alpha) → renderer vtable (all four wired) → D3D palette color all verified correct. Needs field repro: (a) after the wide-res fix, (b) at 800x600 vs wide, (c) item vs monster-name tooltips. |
| Alt toggle (`alt_toggle=1`) | **FIELD DEAD AGAIN after the P12 fix — no static defect found (P13, this session)**: every link re-verified byte-for-byte (or semantically-equivalent) against `origin/main` — CmdTbl.cpp dispatch/registration/defaults/entry 72 + `D2KeyBindStrc` layout, D2ClientRoot CMD bodies (vs fork D2Client.cpp), VanillaPlusCore.cpp (byte-identical), ShowItems.cpp (byte-identical), CUnit.cpp label-read, Game.cpp click-converge block, ui.cpp Set/GetUIFlag, globals.h `AltShowItems` at index 13, Message.cpp registrations, DRAW_UI gate + `DRAW_AltItems` call. The break is therefore runtime-side: stale exe (overlay rule §4), or the ini read. **R157 field diag added** — one-way d2boot.log marks: first `alt_toggle` ini read names the resolved path + value; first Alt keydown/keyup handler fire names toggle_mode + latch; title/automap tag is now `D2VP-R157` (proves the exe carries the P12 fix). Decision tree in HISTORY P13. |

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
3. **Tooltips**: answer (a)/(b)/(c) above before any code change.
4. **Alt toggle (R157 diag)**: rebuild clean, confirm the title screen says
   `D2VP-R157` (anything older = the exe predates the P12 fix — rebuild before
   testing). Then, with `alt_toggle=1` in the d2vp.ini next to the running
   d2.exe, press Alt once in-game and read the `alt-toggle:` lines in
   d2boot.log — they answer the whole chain in one run:
   - no `Alt keydown handler fired` mark at all → the running exe is stale or
     key dispatch never registered (rebuild clean; check for stale .obj per §4);
   - `keydown … (toggle_mode=0)` → the ini read is the broken link: the mark's
     `read [features] alt_toggle=<n> from <path>` line names the file actually
     being read — set `alt_toggle=1` in THAT file (a d2vp.ini in another
     directory, or a launcher copy, does nothing);
   - `keydown … (toggle_mode=1)` but labels don't flip → only then is the
     SetUIFlag/draw gate the suspect (hold-Alt labels test decides: hold works
     + toggle dead in the same build would be a new contradiction to chase);
   - vanilla hold sanity: `alt_toggle` deleted (or =0) → hold-Alt labels must
     still appear (vanilla path), with the same keydown mark at `toggle_mode=0`.

## 6. Document map

- `port-plan.md` — plan, file inventory, per-round build fix details.
- `HISTORY.md` — the port's chronological session log (repurposed).
- `SMOKE-CHECKLIST.md`, `FEATURES.md`, `VANILLA-PLUS.md` — fork QA/feature docs,
  reused as the field protocol and feature ledger for the port.
