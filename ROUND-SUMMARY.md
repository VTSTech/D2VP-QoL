# Round summary — R144 (rev 159, 2026-11-09): the Options page goes two-column + the interpolation row lands on the panel

Owner field: **"Ok, I think it looks good now"** — the R143.2 glide is field-accepted
(§S-65's look; the d2boot tick-count and boundary sub-checks stay open) — followed by
**"Is unlock_fps_interp on the Vanilla+ Options Panel ? if it is the panel is too long
and I cant see it. Can we make the panel two column instead of one ?"**. It was NOT on
the panel (ini-only by design through R143), and the page had indeed grown too long:
one column of eleven rows, box 0x1f1 = 497 px, clipping its empty bottom margin at
640x480. Both halves answered this round.

## What changed

1. **Two-column reflow** (`VanillaPlusOptions.cpp`): row i draws at column `i / 6`,
   band `i % 6` (`VPP_ROWS_PER_COL = 6`). Each column's strip is its half of the box
   (`VPPAGE_COL_WIDTH = 0x136`) inset 0x0e per side; the label sits 0x10 into the band
   (column 0 keeps the 0x1e x the page has always used) and the On/Off value
   **right-aligns** inside the band (measured per draw via `DRAW_text0` — a fixed left
   x sized for the full box would crowd the longest label in a 310 px column). Hover
   highlight and mouse hit-test are per-column (the same y is two rows now — one per
   column); Previous Menu keeps its full-width strip and centered text. Geometry:
   last row of each column spans boxTop+0xf0..0x10a, back row returns to boxTop+0x124,
   box height 0x151 = **337 px** — every stop fits whole again, 640x480 included
   ((480−337)/2 = 71 top margin; R142's one-pitch clip is gone).
2. **The twelfth row — FPS Interpolation** (`{ "unlock_fps_interp", L"FPS
   Interpolation", 1 }`, directly under Unlock FPS; label = the key's plain reading,
   rename on request): rides the same `D2VP_*` write bridge as every row — flip →
   `WritePrivateProfileStringA` → `D2VP_ConfigReloaded` busts `dwUnlockFpsLastCheck` →
   the next `UNLOCKFPS_Active` poll re-reads within the same ~2 s window an external
   ini edit gets. With Unlock FPS off the row is inert (the interp consumers all poll
   the gate first); it stays a plain toggle, no graying.
3. **Per-row read default** (`VanillaPlusCore.{h,cpp}`): new
   `D2VP_GetFeatureToggleDefault(szKey, nDefault)`; `D2VP_GetFeatureToggle` is now its
   `(…, 0)` wrapper. Why: the gate reads a **missing** `unlock_fps_interp` key as **1**
   ("missing key = 1, the intended look"), but `ALTTOGGLE_EnsureIni` writes the
   template only for a missing FILE and never backfills keys — a pre-R143 ini carries
   no key, so the generic 0-default read would display **Off while the interp runs**,
   and `VANILLAPLUS_ToggleRow` inverting that 0 would make the first click write 1 (a
   silent no-op) instead of the real 0. The `VPPFeatureRow` struct grew `nReadDefault`
   (0 on the eleven older rows, 1 on interp); both the draw and the toggle read
   through it — the display always matches what the gate acts on.
4. **Ini template wording**: the interpolation block no longer says "no Options row" —
   it names the **FPS Interpolation** row. `D2VP_ROUND "143" → "144"`: self-ID
   **`Vanilla+0.0.8-R144`**, title screen + automap tag `D2VP-R144`.

Deliberately untouched: the interp sites, the pacer, vsync, the esc-menu row-10 label.

## R144 files (4 code + docs)

- `Source/D2Client/UI/VanillaPlusOptions.cpp` — the reflow: `VPPAGE_COL_WIDTH`,
  box 0x1f1→0x151, back row 0x1c4→0x124, `VPP_ROWS_PER_COL`, `VPPAGE_RowBand`,
  column-aware `VPPAGE_RowTopY`/`VPPAGE_HitTest`, right-aligned
  `VPPAGE_DrawValueText`, `gaVanillaPlusRows[12]` + `nReadDefault`, the interp row,
  the layout-check note.
- `Source/D2Client/UI/VanillaPlusCore.cpp` — `D2VP_GetFeatureToggleDefault` (+ the
  wrapper), the template wording fix.
- `Source/D2Client/UI/VanillaPlusCore.h` — the new bridge declaration + the interp
  comment updated (ini-only through R143, an Options row since R144).
- `Redivivus/d2_boot.h` — `D2VP_ROUND "144"`.

Docs: CHANGELOG (R144 entry), README (feature table + the FPS Interpolation row,
status tail, the ships-OFF exception note), FEATURES v10 (header, row 17, §17,
shipped-as-of, the twelve count), SMOKE item 2 + 64 + 65 notes + the new **item 66**
(issue template synced by hand), plan status, handoff rev 159 (**H62** = the
R143/R143.1/R143.2 verbatim header, offloaded; H60/H61 = the R140/R141 records,
indexed for the first time), ROUND-SUMMARY (this file).

## Verification

- Sandbox static checks (the compile gate of record stays your Win11 MSYS2 build —
  `tools/check-build.sh` stops at the sandbox's missing cmake as documented):
  12 rows keyed and 3-field initialized (`unlock_fps_interp` the only `nReadDefault`
  1); no stale generic `D2VP_GetFeatureToggle` call left in the page TU (both sites —
  draw + toggle — read through the Default form); no stale `0x5a` value x; braces
  balanced in all three touched TUs; geometry arithmetic re-checked in Python
  (0xf0/0x10a/0x124/0x151, column bands inside the box, 480p top margin 71,
  600p 131).
- Behavior invariants: 0/1 semantics unchanged everywhere (the bridge only changes
  the *missing-key default*, and only for the interp row); every other row reads
  exactly as before (Default form with 0 = the old generic read); the write path is
  byte-identical; a flip of FPS Interpolation lands on the same 2 s hot-read the ini
  edit uses.

## Field state + asks

- GREEN: §S-61 telemetry, the §S-64 mid-session menu flip, §S-65's glide look
  (owner: "Ok, I think it looks good now", R143.2 build).
- Open: §S-62 (stopwatch sim identity), §S-63 (the 60 Hz feel + NOVSYNC spot-check),
  §S-64's boundary walk (Esc pause / loading / cutscenes), §S-65's d2boot
  tick-count + boundary sub-checks, and the new **§S-66** (two columns draw +
  per-column hover/click, 480p unclipped, FPS Interpolation under Unlock FPS, a
  pre-R143 ini without the key displays On, the flip lands within ~2 s with the gate
  on and is inert with it off).
