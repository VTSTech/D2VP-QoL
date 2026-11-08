# Round summary — rev 149 (2026-11-08): R136 landed — issue #29 closed, field-verified

Owner: **"R136 confirmed closes #29."** The Countess-chest crash (track U) is fixed and
owner-verified on the Win11 build; the smoke-that-opens-the-chest show the owner described
("after you kill the countess there is smoke that drifts to the chest and opens it after a
few seconds.") is the **vanilla scripted sequence** — the crash fired one step later, at
the first potion reward, because the reassembly's potion-tag getters returned a memory
address instead of the 4CC item code. Full story: `plans/ISSUE29-COUNTESS-CHEST.md`,
record HISTORY **H52**.

This overlay syncs the sandbox tree to the owner's R136 state (the fix, applied verbatim
from the verified proposal patch — `git apply --check` clean) **plus the closure
bookkeeping**. If your R136 build already bumped the self-ID, the only new takes are the
docs; `Drop.cpp` and the two gate rows should byte-match what you applied.

## What changed (code, mirrors the owner's R136)

- **`Source/D2Game/ITEMS/Items/Drop.cpp`** — both `UNIT_GetMaxAvailable{HP,MP}CharCode`
  getters return the potion tag **by value** (`*(const int32_t*)HP/MPLIST[n]`); the
  `szHpCode`/`szMpCode` dangling stack locals deleted; marks `// VP: [countess-chest #29]`.
- **`Source/Game/Main.cpp`** — self-ID `Vanilla+0.0.8-R135` → **`Vanilla+0.0.8-R136`**
  (code round; d2boot.log line 1 must pin it).
- **`tools/check-vanilla-plus.sh`** — two NEW gate rows (`countess-chest: tag-by-value
  returns` = 4; `countess-chest: no address/stack returns left` = 0; both verified red
  against the unfixed tree at counts 0 and 14) + the self-ID rows re-pinned
  R136=1 / R135-gone=0. Suite **520 → 522**.

## What changed (docs, the closure bookkeeping)

- **handoff.md → rev 149**: the R136/closure record (outgoing rev-148 paragraph offloaded
  verbatim to HISTORY **H51**, proof-enforced by the swap script
  `scripts/rev149_handoff_swap.py`); §1 self-ID/gates; §7 the #29 bullet lifts into the
  owner-closes line, the closed-tracks list gains `U countess-chest R136 [#29] (H52)`;
  §8 H-index gains H51/H52; §9 manifest — **four open → three** (R #25 / Q #18 / V #31).
- **HISTORY.md**: **H51** = the rev-148 paragraph verbatim; **H52** = the dedicated
  issue #29 record (symptom, scripted-sequence walkthrough, halt-line forensics, root
  cause, classification, fix, gates, field verification) — the H44 twin for #32.
- **SMOKE-CHECKLIST.md**: new **§N** — item 49 (the issue repro inverted = the fix proof,
  CONFIRMED 2026-11-08) + item 50 (ride-along sanity); build line and item 0 pin
  **R136**; the issue template regenerated (`tools/gen-smoke-issue-template.py`, 58 open
  boxes, both files committed together per the header rule).
- **CHANGELOG.md**: the R136 entry (human lane, short).
- **plans/ISSUE29-COUNTESS-CHEST.md**: the triage doc versioned in the tree, with the
  LANDED addendum.

## Suggested commit split

1. `fix: countess-chest potion tags returned by value - R136, closes #29` — Drop.cpp,
   Main.cpp, tools/check-vanilla-plus.sh (skip what your R136 build already carries)
2. `docs: R136 landed, issue #29 closed - rev 149; rev-148 offloaded to H51; H52 record` —
   handoff.md, HISTORY.md, ROUND-SUMMARY.md, plans/ISSUE29-COUNTESS-CHEST.md
3. `docs: SMOKE §N countess regression check; CHANGELOG R136` — SMOKE-CHECKLIST.md,
   .github/ISSUE_TEMPLATE/smoke-test.md, CHANGELOG.md

## Verification

- `tools/check-vanilla-plus.sh`: **522/522** (incl. the handoff 16 KB budget:
  14,854/16,384 B).
- No new compile risk sandbox-side: `Drop.cpp` is byte-identical to the owner's built
  and field-verified R136; `Main.cpp`/gates are string pins.
- H51 offload verbatim-proofed; template regen diffed clean (§N added, item 49 reset to
  an open box, verdicts preserved as text).

## Still owed (unchanged from rev 148)

- §M2 `gamble:` log-chain discriminators, §M3 one purchase, §M4 the same-session
  re-open (the §M1 click is green on the salvage build).
- The explicit 16-bit gossip log check (the `msgType0=` mark + the 0x31 echo).
- SMOKE §N item 50 ride-along on any future build (cheap, rides any session).
- §7 triages: #31 (track V, 1080p D3D-windowed), #25 (track R, shrine XP buff), #18
  (track Q, attribute-point crash); the R116 `-w` 1366x768 eyeball.
- Shelf (consult-don't-merge): `92f67c4f`, `a633e56a` (the scroll-box suspect) per
  plans/ELEVENTH-SYNC-SALVAGE.md. Upstream sync stays **PAUSED**.
