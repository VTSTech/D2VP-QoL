# Round summary — rev 145 (2026-11-08): the rollback round, syncs PAUSED, salvage review

Base: `8067c4a` (R134), tree identical to the rollback commit `86cd10cd`. Docs-only
round — self-ID stays `Vanilla+0.0.8-R134`, no code changes, no smoke owed.

## What this overlay changes (7 files)

| File | Change |
| --- | --- |
| `plans/ELEVENTH-SYNC-SALVAGE.md` | **NEW** — the read-only salvage review of the eleventh sync (`30384f1a..a633e56a`): ranked take-list, do-not-port list, the texture-corruption suspect note, per-item port plans. The main deliverable of the round. |
| `tools/check-vanilla-plus.sh` | Self-ID gate row re-pinned R130 → R134 (R131–R134 bumped the self-ID but missed the re-pin; the suite ran **518/519 red before this round**). Suite is 519/519 green with this overlay. |
| `handoff.md` | Line 1 → rev 145: rollback verified byte-identical, syncs **PAUSED** (may resume when a process bounds the cost), salvage verdicts, R131–R134 recorded (they had no handoff records), owner-owes line. §1/§2/§8 re-pointed to R134 / PAUSED / H47. 14,160/16,384 B, gate-checked. |
| `HISTORY.md` | Appended **H47** = the rev-142 paragraph verbatim (the trim-N pattern). Numbering note inside: the rolled-back R135–R139 chain had already used H47/H48; the numbers re-use after the rollback. |
| `docs/merge-upstream.md` | PAUSED banner at top — archive, not live procedure; points at the salvage review. |
| `CONTRIBUTING.md` | "arrive here at the next sync" → paused wording + salvage pointer; "so the next sync merges clean" → "so hand-ported upstream material diffs clean". |
| `README.md` | Contributing section: the next-sync promise → paused wording + salvage pointer. |

## Verdicts at a glance (full detail in the review doc)

- **TAKE 1:** `d6746a33` — gamble table back to retail's one 0x198-byte layout; supersedes
  the R128 accessor workaround (same defect class, found upstream independently). Keep a
  VP clamp at the diagnostics mark; re-pin the four R128 gate rows; smoke = Gheed gamble click.
- **TAKE 2 (easiest):** `228dfc62` — SREG_InternalSaveEntry flags dword, 8 lines, Storm only.
- **TAKE 3:** `83b045a4` — TEXT/OVERHEAD getters 16-bit (skip its version chores); both client
  TUs are fork-marked; recon for the quest-news scroll-box known issue.
- **TAKE 4:** `c16c2a68` — pointer typing (after #1, SUNITNPC overlap).
- **OPTIONAL 5:** `92f67c4f` — typing contracts; widest fork-conflict surface (D2WinFont
  shiftcmp counts, ui.cpp dismiss table).
- **CONSULT, don't merge:** `a633e56a`'s statement-order rounds (text scroll box, shrines)
  when triaging the scroll-box known issue or #25.
- **PARK:** the x64 typing (`272e075e` + half of `a633e56a`).
- **DO NOT PORT:** `b62994ba` (include rewrite), `7b702558` (public-header split), and the
  sync's Glide texture-cache re-expression — the prime suspect for the corrupted textures;
  R134's fix-2/fix-3 are field-proven.

The paused window is exactly those nine upstream commits: `origin/jaenster-main` has
moved only CI commits since `a633e56a`.

## Suggested commit split (owner applies as own commits per §8)

1. `fix: self-ID gate row re-pinned R130 -> R134 (R131-R134 missed the re-pin)` — check-vanilla-plus.sh
2. `docs: rev 145 - rollback round, syncs PAUSED, salvage review; rev-142 offloaded to H47` — handoff.md, HISTORY.md, plans/ELEVENTH-SYNC-SALVAGE.md
3. `docs: paused wording for the sync process (merge-upstream.md banner, CONTRIBUTING, README)` — the three remaining docs

## Verification on this overlay

- `tools/check-vanilla-plus.sh`: **519 passed** (was 1 FAILED / 518 passed at `86cd10cd`).
- handoff budget: 14,160 / 16,384 B.
- Rollback identity re-verified before editing: `git diff 8067c4a HEAD` empty.
