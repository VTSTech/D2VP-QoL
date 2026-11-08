# Round summary — rev 151 (2026-11-09): R138 landed — issue #31 closed, field-verified

Owner: **"R138 confirmed closes #31."** The `-w` (windowed) 1080p select crash (track V)
is fixed and owner-verified on the Win11 build — the round's own-code AV WRITE was a
**static table overread**, not a renderer defect: every software tile blitter indexes
`gaYBufferRowOffsets[nScreenY + 0x20]` and the vertical clip lets `nScreenY` reach
`height - 0x30`, so the maximum index ever read is `height - 16`; retail sized the table
for its tallest stop (732 = 700 + 0x20) and the 1080p `-w` stop read **332 slots past the
end** — the garbage row offsets became wild write destinations in `D2GFX_DrawWallTileBasic`
(write to `0B88BF60` = surface + surface read back from an overread slot, frame #0
`+0x66B` in the symbolized 22-frame dump). Fullscreen D3D was immune by construction
(hardware vertex tiles never touch the table) and 720p `-w` stayed inside — why the field
passes never caught it. "D3D windowed" resolves to the `-w` GDI Windowed renderer;
Direct3D.cpp was never in the path. Full story: `plans/ISSUE31-WINDOWED-1080P.md`,
record HISTORY **H56**.

This overlay syncs the sandbox tree to the owner's R138 state (the fix, applied verbatim
from the verified proposal patch — `git apply --check` clean) **plus the closure
bookkeeping**. The proposal overlay carried no self-ID bump; this one takes `Main.cpp` to
R138. If your R138 build already bumped the self-ID, the only new takes are `Main.cpp`
(verify) and the docs + the two gate self-id rows; `D2GfxHelperStrc.cpp` and the iss31
gate row should byte-match what you applied.

## What changed (code, mirrors the owner's R138)

- **`Source/D2gfx/D2GfxHelperStrc.cpp`** — `gaYBufferRowOffsets[732]` → **`[1112]`**
  (1080 + 0x20, the tallest ladder stop plus the same slack retail carried); the
  VP marker block `[windowed-1080p rowtable #31]` carries the whole index arithmetic.
  One constant; `D2GFX_ClearYBuffer` fills via `sizeof` and adapts automatically.
- **`Source/Game/Main.cpp`** — self-ID `Vanilla+0.0.8-R137` → **`Vanilla+0.0.8-R138`**
  (code round; d2boot.log line 1 must pin it).
- **`tools/check-vanilla-plus.sh`** — one NEW gate row (`iss31: y-row table covers
  1080p` = 1 — red-verified against the unfixed tree: exactly this row FAIL, 524+1F) +
  the self-ID rows re-pinned R138=1 / R137-gone=0. Suite **524 → 525**.

## What changed (docs, the closure bookkeeping)

- **handoff.md → rev 151**: the R138/closure record (outgoing rev-150 paragraph
  offloaded verbatim to HISTORY **H55**, swap script `scripts/handoff_rev151.py`);
  §1 self-ID/gates; §2 the resolution-ladder row shows #31 FIXED; §7 the owner-closes
  line gains #31, the closed-tracks list gains `V windowed-1080p R138 [#31] (H56)`, the
  R116 line notes the fix covers the 1366x768 `-w` stop by construction; §8 H-index
  gains H55/H56; §9 manifest — **two open** (R #25 / Q #18); handoff
  15,825/16,384 B.
- **HISTORY.md**: **H55** = the rev-150 paragraph verbatim; **H56** = the dedicated
  issue #31 record (symptom, the terminology resolution, the symbolicated stack, the
  index arithmetic + per-stop table, the `.bss` overread walk, classification, fix,
  gates, field verification) — the H52/H54 twin.
- **SMOKE-CHECKLIST.md**: new **§P** — item 53 (the `-w` 1080p select = the fix proof) +
  item 54 (ride-along: the smaller stops; the 1366x768 `-w` select doubles as the
  still-owed R116 eyeball); build line and item 0 pin **R138**; the issue template
  regenerated (`tools/gen-smoke-issue-template.py`, 62 open boxes, both files committed
  together per the header rule).
- **CHANGELOG.md**: the R138 entry (human lane, short, above the R137 entry).
- **README.md**: badges (R138, 525/525), current build, the since-R128 list gains the
  windowed-1080p fix, open issues 3 → 2, #31 to recently-closed.
- **plans/ISSUE31-WINDOWED-1080P.md**: the triage doc stamped FIXED R138 —
  field-verified (R4/R5 updated; the 1366x768 in-game pass stays an explicit ask).

## Suggested commit split

1. `fix: y-buffer row table sized for the 1080p windowed stop - R138, closes #31` —
   D2GfxHelperStrc.cpp, Main.cpp, tools/check-vanilla-plus.sh (skip what your R138
   build already carries)
2. `docs: R138 landed, issue #31 closed - rev 151; rev-150 offloaded to H55; H56 record` —
   handoff.md, HISTORY.md, ROUND-SUMMARY.md, plans/ISSUE31-WINDOWED-1080P.md
3. `docs: SMOKE §P windowed-1080p regression check; CHANGELOG/README R138` —
   SMOKE-CHECKLIST.md, .github/ISSUE_TEMPLATE/smoke-test.md, CHANGELOG.md, README.md

## Verification

- `tools/check-vanilla-plus.sh`: **525/525** (incl. the handoff 16 KB budget:
  15,825/16,384 B after the H55 offload).
- No new compile risk sandbox-side: `D2GfxHelperStrc.cpp` is byte-identical to the
  owner's built and field-verified R138; `Main.cpp`/gates are string pins.
- H55 offload verbatim (the rev-150 line 1 moved byte-identical); template regen
  62 open boxes (§P added; §N item 49 stays checked in the repo copy, reset to an open
  box in the template per the generator).

## Still owed (rev 150's list minus the #31 triage)

- §M2 `gamble:` log-chain discriminators, §M3 one purchase, §M4 the same-session
  re-open (the §M1 click is green on the salvage build).
- The explicit 16-bit gossip log check (the `msgType0=` mark + the 0x31 echo).
- SMOKE §N item 50 ride-along on any future build (cheap, rides any session); §O item 52
  likewise (one charged-item hover); §P item 54's 1366x768 `-w` select is also the
  R116 eyeball — one pass covers both.
- §7 triages: #25 (track R, shrine XP buff), #18 (track Q, attribute-point crash).
- Shelf (consult-don't-merge): `92f67c4f`, `a633e56a` (the scroll-box suspect) per
  plans/ELEVENTH-SYNC-SALVAGE.md. Upstream sync stays **PAUSED**.
