# Round summary — rev 148 (2026-11-08): docs-only — the R135 salvage field pass recorded

Owner: "R135 built on Win11, Talked to Gheed, Gamble doesnt crash." No code, no
gate-row changes, self-ID stays `Vanilla+0.0.8-R135`. This overlay is **docs only**
(4 files) and applies on top of the rev-147 state; the code tree is unchanged.

## What the field pass clears

- **MSYS2 build + link green on Win11** — the compile gate of record passed for the
  whole salvage delta: the four hand-ported TAKE commits (`228dfc62` SREG flags dword,
  `d6746a33` the gamble table back to retail's one 0x198-byte layout, `83b045a4` the
  16-bit TEXT/OVERHEAD getters, `c16c2a68` the seed/affix/NPC/inactive-item pointer
  typing) plus the docs rounds. First real compiler+linker pass over the R135 tree,
  after the sandbox's 13/13-TU syntax sweep.
- **§M1 gamble click green** — talking to Gheed and opening the gamble screen does
  not crash: the `d6746a33` retail single-table supersession **holds the R128 field
  fix**. Track T (issue #32) stays closed. This was the salvage program's one
  gameplay-risk item — it is now field-de-risked.
- The Gheed talk also ran the re-typed npcmenu gossip path (the `83b045a4` 16-bit
  getters) crash-free — encouraging for port 3, though the log marks were not
  captured.

## What changed (docs only)

- **handoff.md → rev 148**: the field-pass record (outgoing rev-147 paragraph
  offloaded verbatim to HISTORY **H50**, proof-enforced by the swap script);
  §2 field table — gamble row and upstream-sync row updated with the pass;
  §8 H-index gains H50.
- **HISTORY.md**: **H50** = the rev-147 paragraph verbatim (the three-goal charter
  round).
- **SMOKE-CHECKLIST.md**: the per-round history line and the §M preamble note the
  R135 re-check (§M1 green; §M2–M4 not yet re-run on R135). No check text changed —
  the issue template does not need a regen.
- **Still owed (narrowed, one session's worth)**: §M2 the `gamble:` log-chain
  discriminators (`cache=` sane count, `bound=` 2..total, `gen done items=14` —
  glance d2boot.log next session), §M3 one purchase, §M4 the same-session re-open,
  the explicit 16-bit gossip log check (the `msgType0=` mark + the 0x31 echo), and
  the item-0 habit (d2boot.log line 1 = `Vanilla+0.0.8-R135`).

## Suggested commit split

1. `docs: R135 salvage field pass - Win11 build green, Gheed gamble click holds` —
   SMOKE-CHECKLIST.md
2. `docs: rev 148 - field pass recorded; rev-147 offloaded to H50` — handoff.md,
   HISTORY.md, ROUND-SUMMARY.md

## Verification

- `tools/check-vanilla-plus.sh`: **520/520** (docs-only round; the suite also
  enforces the handoff 16 KB budget).
- No code TUs touched — no syntax sweep needed this round.
