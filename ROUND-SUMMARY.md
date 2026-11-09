# Round summary — R148 (rev 164, 2026-11-10): one-line automap clock + the area level

Owner field: **"lets change the Automap clock display to 1 line instead of 2. SESS:
hh:mm:ss SYS: 12:01PM. Also for the first line, Area Name, Can we also add (aLvl) as
well ?"** — two display tweaks on top of the not-yet-field-tested R147 clock.

## What changed

1. **The clock merged to one line** (`VanillaPlusCore.cpp`, `VPCLOCK_Draw`): the
   R147 `Session …` / `System …` pair is now **`SESS: hh:mm:ss SYS: h:MMAM`** —
   one draw, one `0x10` row advanced (with the gate on, the stack below sits one
   row higher than R147). Session keeps the per-game `GetTickCount()` anchor in
   `InitializeUI`'s wholesale UI reset and goes zero-padded `hh:mm:ss` per the
   ask's literal sketch (hours wrap at 100, `% 100`); System is `GetLocalTime`
   converted to 12-hour with a trailing AM/PM and no space — `h % 12`, `0 → 12`
   (midnight `12:xxAM`, noon `12:xxPM`), minutes padded, re-read every drawn
   frame. Gate untouched: `[features] automap_clock` (default 0), the 2 s
   hot-read cache busted by the Options write bridge, strictly insert-after.
2. **The area level on the level-name line** (`Automap.cpp`,
   `AUTOMAP_DrawLevelName`, VP-marked): the overlay's first line appends
   ` (N)` — N = `TXT_Levels_GetMonLvlForDifficulty(eLevel, difficulty,
   expansion)`, the same levels.txt MonLvl/MonLvlEx-per-difficulty lookup the
   drop code uses (D2Game already calls it cross-DLL; D2Client now imports it
   too — `LvlTbls.h` include added). The tail is pure ASCII widened 1:1 into a
   composed name buffer (the item-ilvl digit idiom; bounds-guarded, name + 8 <
   0x60), then the line measures and draws as one string. **Always-on — no
   toggle, no ini key, no Options row** (the R145 pager precedent); suppressed
   where the table reads ≤ 0, so towns keep the plain retail name. Works with
   the clock gate on or off.
3. `Redivivus/d2_boot.h` — `D2VP_ROUND "148"` (title + automap tag `D2VP-R148`).
   Options page untouched (two columns of seven, thirteen rows).

## R148 files (2 code + 1 define + docs)

- `Source/D2Client/UI/VanillaPlusCore.cpp` — the one-line clock (format string,
  12-hour fixup, single draw pair, single `0x10` advance) + comment sync.
- `Source/D2Client/UI/VanillaPlusCore.h` — declaration comments only.
- `Source/D2Client/UI/Automap.cpp` — the ` (aLvl)` suffix block in
  `AUTOMAP_DrawLevelName` + the `LvlTbls.h` include.
- `Redivivus/d2_boot.h` — `D2VP_ROUND "148"`.

Docs: CHANGELOG (R148 bullet), README (feature row + note), FEATURES (v12
header, row 18, §18), SMOKE §T rewritten (items 67–72) with the issue template
synced verbatim (`smoke_sync copy --section T`), handoff rev 164 (the rev-163
R147 header offloaded verbatim to **H67**), ROUND-SUMMARY (this file).

## Verification

- Sandbox static checks (the compile gate of record stays your Win11 MSYS2
  build — `tools/check-build.sh`): the 35-check R148 suite — boot define + note,
  the new format string with the old `Session %u` / `System %02u` strings gone,
  the 12-hour modulo + midnight/noon fixup + AM/PM ternary, exactly one
  `DRAW_text` pair and one `0x10` advance in `VPCLOCK_Draw`, the MonLvl call
  site with `GetDificulity` + `CLIENT_CheckExpansion`, the compose guards
  (`> 0`, name length, `+ 8 < 0x60`), VP markers paired, the version → clock →
  difficulty order intact, the two-columns-of-seven Options geometry + the 2 s
  gate cache + reload bridge regressed green, braces/parens balanced; docs_gate
  `--round 148` green (round identity, 16 KB handoff gate, smoke-sync contract,
  options page, feature counts, zip byte-identity).
- Behavior invariants: gate off = no clock line and the offset untouched (the
  ONLY always-on delta is the level-name suffix, called out in item 71); the
  session anchor rides the existing per-game reset; display-only — no packets,
  no files, no server half, no d2boot marks.

## Field state + asks

- GREEN (standing): §S-61 telemetry, §S-64's mid-session menu flip, §S-65's
  glide look; R145/R146 confirmed by the owner ("Good job, works" / "Good job.
  works.").
- Open: §S 62–64, §S-65's d2boot tick-count + boundary sub-checks, §S-66's
  behavioral remainder, §H items 33–35 (the hundred-page walk).
- New field asks for R148 (SMOKE §T 67–72): **67** — ONE line under the
  `D2VP-R148` tag reading `SESS: 00:00:12 SYS: 12:01PM`-style; **68** — SESS
  resets per game, not on map toggle; **69** — SYS tracks the wall clock in
  12-hour form across a minute boundary and flips AM/PM (and `12:xx`) at
  noon/midnight; **70** — the 7/6 Options reflow still whole at 640x480, the
  same-frame flip; **71** — gate off = retail except the level-name suffix;
  **72** — the level-name line reads `Name (N)` per difficulty, plain name in
  town, gate-independent.
