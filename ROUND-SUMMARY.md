# Round summary — R145 (rev 161, 2026-11-10): the shared-stash pager arrows read `<<` / `>>`

Owner field: **"For the Shared Stash, Can we make the navigation label '<<' and '>>'
instead of just < >, i find just one is a little small to click on"** — a fair read of
the R75 pager: the single `<`/`>` glyphs are small, and a click target you use ten
times to walk to page 10 should not ask for pixel precision.

## What changed

1. **The glyphs double** (`VanillaPlusCore.cpp`): `VPSHAREDUI_TabLayout` measures the
   arrow with `DRAW_text0(L">>")` (was `L">"`) and `VPSHAREDUI_DrawTab` draws
   `L"<<"`/`L">>"` — the composite reads **`Shared >>`** on the personal page and
   **`<< Shared N >>`** on a shared page (the last page still drops the right arrow),
   all under the same R75 outline halo.
2. **The click bands double with the pixels** — no second change needed, by design:
   the R75 layout feeds the ONE measured width to both the draw and the hit bands
   (`VPSHAREDUI_HandleStashClick` calls the same `VPSHAREDUI_TabLayout`), so the
   arrow bands widen exactly as far as the glyphs do, 2 px margins included. The
   complaint was about the click target; the band is what got bigger.
3. **Geometry unchanged**: the composite stays right-anchored at
   `VPSHAREDUI_TAB_RIGHT_EDGE`, so the wider arrows cost only leftward room on the
   R77 band — empty retail space out to the X +0x49..+0x96 gold-area hit rects (the
   gold display itself sits 0x2d below the row since R77), and the right tip stays inside
   the +0x140 art-seam rule. Stepping (one page per click), the word toggle
   (personal↔shared-1), the last-page arrow drop, the 0x4F/0x7D packet and every
   `.sss` format: untouched.
4. **Round bookkeeping**: `D2VP_ROUND "144" → "145"` — self-ID
   **`Vanilla+0.0.8-R145`**, title screen + automap tag `D2VP-R145`.

Archaeology note (pre-existing, owner call, untouched this round): the constant reads
**0x130** at HEAD while its comment still tells the R76 `+0x13c` story — web edit
`e0dd97ce` (2026-11-01) reverted the R76 +0x0c nudge without updating the comment.
Harmless since the R77 row lift separated the tab row from the gold text vertically,
but one of the two should give.

Deliberately untouched: the pager bounds (ten pages), the word-click behavior, the
Options rows, everything outside the tab composite.

## R145 files (2 code + docs)

- `Source/D2Client/UI/VanillaPlusCore.cpp` — the arrow measure (`L">>"`), the draw
  literals (`L"<<"`/`L">>"`), the R145 composite comment + the per-site notes.
- `Redivivus/d2_boot.h` — `D2VP_ROUND "145"`.

Docs: CHANGELOG (R145 entry), README (the Shared Stash feature row), FEATURES §9,
SMOKE items 21/33/35 re-glyphed (+ the item-33 R145 geometry watch), plan status,
handoff rev 161 (the rev-160 docs-only header offloaded verbatim to **H64**),
ROUND-SUMMARY (this file).

## Verification

- Sandbox static checks (the compile gate of record stays your Win11 MSYS2 build —
  `tools/check-build.sh` stops at the sandbox's missing cmake as documented): no
  single-glyph arrow literal left in the TU; the click handler reads the same layout
  outputs (`nLeftX`/`nRightX`/`nArrowW`) as before, so bands and pixels cannot drift;
  braces balanced; the width string is measured with the same font-1 `DRAW_text0`
  call the word uses.
- Behavior invariants: page math unchanged (`±1` step, clamped by the existing
  `nNewPage` bounds check); a click on the word still toggles personal↔shared-1; a
  miss still falls through to retail (`return FALSE`); no packet or file change.

## Field state + asks

- GREEN (standing): §S-61 telemetry, §S-64's mid-session menu flip, §S-65's glide
  look (owner: "Ok, I think it looks good now", R143.2 build).
- Open: §S 62–64, §S-65's d2boot tick-count + boundary sub-checks, §S-66's behavioral
  remainder — and the new **item-33 R145 watch**: the wider `<< Shared 10` composite
  stays clear of the gold display row and inside the art seam at every video stop,
  640x480 included; the `>>` band clicks first-try.
