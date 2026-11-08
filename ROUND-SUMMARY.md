# Round summary — rev 147 (2026-11-08): docs-only — CONTRIBUTING carries the three-goal charter

Owner: "Update CONTRIBUTING, There is 3 goals. The 3rd is porting to Linux in the linux
branch of the repo." No code, no gate-row changes, self-ID stays `Vanilla+0.0.8-R135`.
This overlay re-carries the full working-tree delta vs `86cd10cd` (the R135 salvage code
round plus this docs round), so it applies cleanly whether or not the earlier zips were
applied.

## What changed (CONTRIBUTING.md only, plus round bookkeeping)

- **Intro: two goals → three.** Goal 3 is **port to Linux — `d2linux`**, the native
  Linux ELF that *is* the game host (no Windows, no Wine, no PE), living on the
  **`linux` branch** of this repo; `main` stays the Windows/PE line.
- **New section "If you want to help port the game to Linux"** (before License),
  grounded in the branch's own docs: SDL3 host layer under `platform/linux/`, game
  code stays i386 (the decompiled logic assumes the 32-bit x86 ABI), verified by
  behavior — no byte-match possible for a port; charter/milestones/build live in
  `docs/linux-port.md` on that branch; the two-tracks ground rule (Linux-driven
  game-source changes are `#ifdef`-guarded and must leave the PE build
  byte-identical); branch off `linux`, start from its docs.
- **Housekeeping:** the house-rules suite count 519 → **520** (stale since rev 146's
  gamble retirement row).
- Branch facts verified before writing: `linux` forked at `c0a9a6e2` (2026-11-03,
  R101-era tree — its README badge still shows 0.0.7-R101 / 380 gates), tip `15fac275`
  (M2-R3), 25 ahead / 158 behind `main`; carries `docs/linux-port.md`,
  `platform/linux/`, `tools/check-build-linux.sh`.
- Round bookkeeping: handoff rev 147 (rev 146 offloaded verbatim to HISTORY **H49**);
  README's gates badge (519/519) left for the release-refresh cadence, per the
  rev-138 pattern.

## Suggested commit split

1. `docs: CONTRIBUTING three-goal charter - goal 3 is the Linux port (linux branch)` —
   CONTRIBUTING.md
2. `docs: rev 147 - three-goal charter round; rev-146 offloaded to H49` — handoff.md,
   HISTORY.md, ROUND-SUMMARY.md

## Verification

- `tools/check-vanilla-plus.sh`: **520/520** (no gate rows pin CONTRIBUTING; suite
  unaffected by docs).
- handoff budget: checked by the suite (16 KB cap).
- No code TUs touched — no syntax sweep needed this round.

## Owner owes

Unchanged from rev 146: the MSYS2 build + link + the R135 smoke belt (Gheed gamble
click §M2 discriminators, one NPC-gossip click for the re-typed 16-bit path,
d2boot.log line 1 `Vanilla+0.0.8-R135`).
