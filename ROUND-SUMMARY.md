# Round summary — R147 (rev 163, 2026-11-10): the automap gains a Session/System clock

Owner field: **"Good job. works."** (R146 confirmed) followed by **"Let's add an in
game clock to the Automap. Session Time and System Time"** — a display-only HUD ask,
the speedrun/PD2-overlay staple.

## What changed

1. **The overlay lines** (`Automap.cpp` + the clock block in `VanillaPlusCore.cpp`):
   `AUTOMAP_Draw` calls `VPCLOCK_Draw(&AUTOMAP_TextHeightOffset, SpellSel::WindowSizeX)`
   right after `AUTOMAP_DrawVersion` — the retail automap text-overlay stack (font 1,
   color 4, one 0x10 row per line, right-aligned at WindowSizeX − width − 0x10) gains
   two lines under the build tag: **Session** `H:MM:SS` (hours unpadded) and **System**
   `HH:MM` 24-hour. Strictly insert-after: gate off = nothing drawn, the offset
   untouched; on, the stack reads version → clock → difficulty.
2. **Session anchor** (`Draw.cpp`): `InitializeUI`'s wholesale per-game UI reset (the
   same site that starts the stash page mirror on the personal page) takes ONE
   `GetTickCount()` capture per game via `VPCLOCK_ResetSession`. Real elapsed
   wall-clock, not play-time: pause menus and cutscenes count — a desk stopwatch.
   GetTickCount's 49.7-day wrap is unsigned-subtraction-correct.
3. **System line**: Win32 `GetLocalTime` re-read every drawn frame — always current;
   a paused menu holding the map frozen is fine to lag.
4. **Gate = the charter**: `[features] automap_clock`, default 0, missing
   file/section/key or garbage = 0 = vanilla byte-for-byte; the same 2 s hot-read
   cache every gate uses (`dwAutomapClockLastCheck`), busted by the Options-page
   write bridge (`D2VP_ConfigReloaded`) — a flip lands on the next automap frame,
   no restart, no map toggle.
5. **Options page**: the **Automap Clock** row takes the thirteenth slot; the page
   reflows to two columns of SEVEN (`VPP_ROWS_PER_COL` 6 → 7: col 0 = Show Items
   Toggle … No Panel World Shift, col 1 = Mana Burn Fix … Automap Clock), back row
   0x124 → 0x144, box height 0x151 → 0x171 (369 px, the same 0x13 bottom margin —
   every stop fits whole, 640x480 included, top margin ≈ 55 there). Col 1's empty
   band 6 stays dead — the hit-test only accepts bands rows actually occupy.
6. **Ini template**: the `automap_clock` block ships in the same round (fresh inis
   carry the key; a pre-R147 ini reads 0 until the row flips once).
7. **Text path**: pure-ASCII `SStrPrintf` into char buffers widened 1:1 into the
   uint16_t buffers D2WinFont draws (the item-ilvl digit idiom — no locale, no
   wide-format pitfalls).
8. **Round bookkeeping**: `D2VP_ROUND "146" → "147"` — self-ID
   **`Vanilla+0.0.8-R147`**, title screen + automap tag `D2VP-R147`.

## R147 files (5 code + 1 define + docs)

- `Source/D2Client/UI/Automap.cpp` — the gated `VPCLOCK_Draw` site in `AUTOMAP_Draw`
  (between the build tag and the difficulty line).
- `Source/D2Client/UI/Draw.cpp` — `VPCLOCK_ResetSession()` in `InitializeUI`'s
  per-game reset.
- `Source/D2Client/UI/VanillaPlusCore.cpp` — the clock block (gate + reset + draw,
  the statics above `D2VP_ConfigReloaded`, the reload bust) + the ini template block.
- `Source/D2Client/UI/VanillaPlusCore.h` — the two declarations.
- `Source/D2Client/UI/VanillaPlusOptions.cpp` — the thirteenth row + the 7/6 reflow
  (box 0x151→0x171, back row 0x124→0x144).
- `Redivivus/d2_boot.h` — `D2VP_ROUND "147"`.

Docs: CHANGELOG (R147 entry), README (the Automap Clock feature row), FEATURES (row
18 + §18, thirteen ship), SMOKE §T (items 67–71; issue template synced verbatim),
handoff rev 163 (the rev-162 R146 header offloaded verbatim to **H66**), ROUND-SUMMARY
(this file).

## Verification

- Sandbox static checks (the compile gate of record stays your Win11 MSYS2 build —
  `tools/check-build.sh` stops at the sandbox's missing cmake as documented): 13
  rows keyed + 3-field init'd, keys unique, all 13 keys present in the ini template;
  the geometry arithmetic re-derived (col-0 band 6 = 0x110..0x12a, back row
  0x12a+0x1a = 0x144, back bottom 0x15e, box 0x15e+0x13 = 0x171 = 369 px, 480p top
  margin (480−369)/2 = 55); braces/parens balanced in all five touched files; the
  gate/reset/draw/bridge call sites grepped; no stale `"146"` round literal.
- Behavior invariants: strictly insert-after (gate off = the overlay stack draws
  every line at its retail spot); the session anchor rides an existing per-game
  reset (no new game-state hooks); display-only — no packets, no files, no server
  half, no d2boot marks.

## Field state + asks

- GREEN (standing): §S-61 telemetry, §S-64's mid-session menu flip, §S-65's glide
  look; R145/R146 confirmed by the owner ("Good job, works" / "Good job. works.").
- Open: §S 62–64, §S-65's d2boot tick-count + boundary sub-checks, §S-66's
  behavioral remainder, §H items 33–35 (the hundred-page walk).
- New field asks for R147 (SMOKE §T): **item 67** — the two lines appear under the
  `D2VP-R147` tag, right-aligned, Session counting from ~0:00:00 and System
  matching the taskbar clock; **item 68** — Session resets per game (new game →
  ~0:00:00) but NOT on automap toggle within one game; **item 69** — System tracks
  the wall clock across a minute boundary; **item 70** — the 7/6 reflow (Automap
  Clock last in the right column, Previous Menu below, nothing clipped at
  640x480) and the same-frame Options flip; **item 71** — gate off = the retail
  overlay, every other line in its retail spot.
