# Round summary — R146 (rev 162, 2026-11-10): the shared stash widens to a hundred pages

Owner field: **"Good job, works"** (R145 confirmed) followed by **"Let's increase the
Shared Stash pages to 100"** — the "Then 100." leg of the original R63 go message
("Give us 1 page … If that works we'll do 10 pages. Then 100."), the leg the R82 note
explicitly reserved: *"The 'Then 100' leg remains a pure widening of the same two
constants + the pager bounds."* It was.

## What changed

1. **Server constants** (`VanillaPlusStash.cpp`): `VPSTASH_PAGE_COUNT` 10 → **100**
   (the count the `.sss` v2 header declares and writes) and `VPSTASH_PAGES` 10 →
   **100** (the runtime array/guard width). Everything derives from them: the
   per-page `'JM'` block pointers and sizes, the activation bounds check, the
   0x4F/0x7D param guard, and the header math `12 + 8 * VPSTASH_PAGE_COUNT` — the v2
   header grows 92 → **812 bytes** (one size/checksum pair per page; an empty page
   costs 8 file bytes, so only used pages cost real space).
2. **Client constants** (`VanillaPlusCore.cpp`): `VPSHAREDUI_PAGE_COUNT` 10 → **100**
   and the word table grew to **101 script-generated literal labels** `Shared 1` …
   `Shared 100` (the R82 no-runtime-formatting rule kept — the D2Win measure/draw
   still sees plain static wide strings). The last page still drops the `>>` arrow.
3. **The reader rides unchanged, by design**: any width the build holds loads — a
   v1 one-page file, a pre-R82 two-page file and an R82-R145 ten-page file all read
   back with the missing pages empty (BSS zero) and the next save rewrites a
   hundred; the narrower-file log tag is reworded (`pre-R82 file` → `narrower file`)
   since a ten-page file is no longer necessarily pre-R82. A WIDER file
   (`pageCount > 100`) is still rejected whole — it needs a newer binary; the
   failed read never touches the file on disk, and if an old build then saves, the
   full hundred-page generation survives in the one-generation `.bak`.
4. **Memory stays bounded**: the 16 KB block cap is per non-empty page — worst case
   ~1.6 MB at 100 full pages in the SP game pool, plus the header arrays
   (+1.6 KB BSS). No allocation changes.
5. **Untouched by design**: the `<<`/`>>` pager stepping (one page per click), the
   word toggle (personal↔shared-1), the 0x4F/0x7D packet (the param word always had
   room for the planned 100), the goldbank borrow, and every `.d2s` path (I1: the
   flush parks all hundred pages before any `.d2s` is built).
6. **Round bookkeeping**: `D2VP_ROUND "145" → "146"` — self-ID
   **`Vanilla+0.0.8-R146`**, title screen + automap tag `D2VP-R146`.

Known UX cost, kept from R82 (no behavior change unasked): the arrows step ONE page,
so page 100 is 99 clicks from personal; a faster affordance (page-number entry,
jump-to-page) is a future ask.

## R146 files (4 code + docs)

- `Source/D2Game/PLAYER/VanillaPlusStash.cpp` — the two server constants, the
  format-sketch comment (812-byte header, `pages 3..100`), the `narrower file` log
  tag, the 1..100 click-guard comment.
- `Source/D2Game/PLAYER/VanillaPlusStash.h` — the API comment block (1..100, "all
  hundred pages").
- `Source/D2Client/UI/VanillaPlusCore.cpp` — `VPSHAREDUI_PAGE_COUNT = 100` + the
  101-entry word table + the composite/guard comments.
- `Source/D2Client/UI/VanillaPlusCore.h` — the shared-stash API comment block
  (1..100, `<<`/`>>`, "all hundred shared pages").
- `Redivivus/d2_boot.h` — `D2VP_ROUND "146"`.

Docs: CHANGELOG (R146 entry), README (the Shared Stash feature row), FEATURES (row 9
+ §9), SMOKE §H (header, items 21/33/34/35 + item 27's block-offset guidance; issue
template synced), plan status (the R146 leg appended), ini template wording, handoff
rev 162 (the rev-161 R145 header offloaded verbatim to **H65**), ROUND-SUMMARY (this
file).

## Verification

- Sandbox static checks (the compile gate of record stays your Win11 MSYS2 build —
  `tools/check-build.sh` stops at the sandbox's missing cmake as documented): the
  word table parsed programmatically — exactly 101 entries, `Shared 1`…`Shared 100`
  in order, commas right (the generator's first pass dropped the trailing commas —
  caught and refixed); the array is declared `[VPSHAREDUI_PAGE_COUNT + 1]` = 101, so
  the initializer matches exactly; braces/parens balanced in all three changed
  TUs/headers; the header-size expression still reads the constant (12 + 8×100 =
  812); the v2 reader still accepts `1..VPSTASH_PAGE_COUNT` and rejects wider; no
  single-glyph arrow literal and no stale `1..10` left in `Source/`.
- Behavior invariants: page math unchanged (±1 step, clamped by the existing
  bounds checks on both sides of the packet); the word-click toggle unchanged; the
  `.sss` v2 magic + version unchanged (a ten-page file and a hundred-page file
  differ only in `pageCount` and the record-pair count); I1 unchanged.

## Field state + asks

- GREEN (standing): §S-61 telemetry, §S-64's mid-session menu flip, §S-65's glide
  look (owner: "Ok, I think it looks good now", R143.2 build); R145 confirmed by the
  owner ("Good job, works").
- Open: §S 62–64, §S-65's d2boot tick-count + boundary sub-checks, §S-66's
  behavioral remainder.
- New field asks for R146 (SMOKE §H): **item 33** — the labels walk `Shared 2` …
  `Shared 100` with a log line per step, `<< Shared 100` shows no right arrow, and
  the R145 geometry watch holds at every video stop; **item 34** — the upgrade
  ladder: a ten-page file loads with pages 11–100 empty (`v2 file has 10 pages -
  pages 11..100 start empty (narrower file)`) and rewrites as a hundred after the
  next tab-back, and (reverse direction) a hundred-page file on a pre-R146 build is
  rejected whole with the `.bak` carrying the full generation; **item 35** — the
  hundred-page rewrite: sample pages 1/2/50/99/100 render, an item on page 100
  survives Save & Exit → re-enter, one gold pool across all pages.
