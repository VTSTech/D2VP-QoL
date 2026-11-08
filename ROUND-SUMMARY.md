# Round summary — rev 150 (2026-11-08): R137 landed — issue #33 closed, field-verified

Owner: **"R137 confirmed closes Issue #33."** The "Chance to Cast Level 0" proc-tooltip
defect (track W) is fixed and owner-verified on the Win11 build. Display-only: the
tooltip formatter's descfunc-15 branch passed a literal 0 in the Level `%d` slot and the
level int in the `%s` (skill-name) slot, where the issue-18 format-args guard silently
ate the line — generation, the `.d2s` saves and the item-packet round-trip were verified
correct end to end, so server-side proc stats were never wrong. Third instance of the
#16/#18 varargs family, the first to degrade quietly instead of faulting. Full story:
`plans/ISSUE33-PROC-TOOLTIP.md`, record HISTORY **H54**.

This overlay syncs the sandbox tree to the owner's R137 state (the fix, applied verbatim
from the verified proposal patch — `git apply --check` clean) **plus the closure
bookkeeping**. If your R137 build already bumped the self-ID, the only new takes are the
docs; `SkillDesc.cpp` and the two gate rows should byte-match what you applied.

## What changed (code, mirrors the owner's R137)

- **`Source/D2Client/UI/SkillDesc.cpp`** — case 0xf drops the `uVar2 = 0` vararg and
  passes `(chance, level, name)` = `(nGroupValue, nRepairCostFinal, wszLocaleStr)`;
  marks `// VP-BEGIN/END [proc-level0 #33]` with the family note. One call site, one
  behavior line.
- **`Source/Game/Main.cpp`** — self-ID `Vanilla+0.0.8-R136` → **`Vanilla+0.0.8-R137`**
  (code round; d2boot.log line 1 must pin it).
- **`tools/check-vanilla-plus.sh`** — two NEW gate rows (`proc-line: level in the level
  slot (R137)` = 1; `proc-line: literal-zero vararg gone` = 0 — the latter reads 1 on
  the unfixed tree, red-verified) + the self-ID rows re-pinned R137=1 / R136-gone=0.
  Suite **522 → 524**.

## What changed (docs, the closure bookkeeping)

- **handoff.md → rev 150**: the R137/closure record (outgoing rev-149 paragraph
  offloaded verbatim to HISTORY **H53**, proof-enforced by the swap script
  `scripts/rev150_handoff_swap.py`); §1 self-ID/gates; §7 the owner-closes line gains
  #33, the closed-tracks list gains `W proc-level0 R137 [#33] (H54)`; §8 H-index gains
  H53/H54; §9 manifest — stays **three open** (R #25 / Q #18 / V #31; #33 was never an
  open row — triaged and fixed inside the same window).
- **HISTORY.md**: **H53** = the rev-149 paragraph verbatim; **H54** = the dedicated
  issue #33 record (symptom, generation/save/packet walkthrough, the display-site
  transcription defect, classification, fix, gates, field verification) — the H52 twin.
- **SMOKE-CHECKLIST.md**: new **§O** — item 51 (the hover check = the fix proof) +
  item 52 (ride-along: charged item lines); build line and item 0 pin **R137**; the
  issue template regenerated (`tools/gen-smoke-issue-template.py`, 60 open boxes, both
  files committed together per the header rule).
- **CHANGELOG.md**: the R137 entry (human lane, short, above the R136 entry).
- **plans/ISSUE33-PROC-TOOLTIP.md**: the triage doc versioned in the tree, with the
  LANDED addendum.

## Suggested commit split

1. `fix: proc tooltips pass (chance, level, name) - R137, closes #33` — SkillDesc.cpp,
   Main.cpp, tools/check-vanilla-plus.sh (skip what your R137 build already carries)
2. `docs: R137 landed, issue #33 closed - rev 150; rev-149 offloaded to H53; H54 record` —
   handoff.md, HISTORY.md, ROUND-SUMMARY.md, plans/ISSUE33-PROC-TOOLTIP.md
3. `docs: SMOKE §O proc-tooltip regression check; CHANGELOG R137` — SMOKE-CHECKLIST.md,
   .github/ISSUE_TEMPLATE/smoke-test.md, CHANGELOG.md

## Verification

- `tools/check-vanilla-plus.sh`: **524/524** (incl. the handoff 16 KB budget:
  14,630/16,384 B after the H53 offload).
- No new compile risk sandbox-side: `SkillDesc.cpp` is byte-identical to the owner's
  built and field-verified R137; `Main.cpp`/gates are string pins.
- H53 offload verbatim-proofed (3,773 B re-read byte-identical); template regen 60 open
  boxes (§O added, item 49 stays checked in the repo copy, reset to an open box in the
  template per the generator).

## Still owed (unchanged from rev 149, plus §N item 50)

- §M2 `gamble:` log-chain discriminators, §M3 one purchase, §M4 the same-session
  re-open (the §M1 click is green on the salvage build).
- The explicit 16-bit gossip log check (the `msgType0=` mark + the 0x31 echo).
- SMOKE §N item 50 ride-along on any future build (cheap, rides any session); §O item 52
  likewise on the R137 build (one charged-item hover).
- §7 triages: #31 (track V, 1080p D3D-windowed), #25 (track R, shrine XP buff), #18
  (track Q, attribute-point crash); the R116 `-w` 1366x768 eyeball.
- Shelf (consult-don't-merge): `92f67c4f`, `a633e56a` (the scroll-box suspect) per
  plans/ELEVENTH-SYNC-SALVAGE.md. Upstream sync stays **PAUSED**.
