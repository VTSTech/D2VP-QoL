# Round summary — R167 (rev 180, 2026-11-18): issue #34 regression — champion/unique tinting restored; + R168 30-vs-37 palette plan

## The ask

Owner (field report on R165/R166): "Unique Monsters (Regular Champions) are not
tinted anymore" — a regression introduced by the R165 issue #34 ("Griswold is
Green") fix. Plus: a plan for the original retail 30-vs-37 palette bug, targeted
at R168.

## Root cause

The R165 create-time neutralizer flagged BOTH explicit tint branches in
`UNIT_CreateMonster` via `bExplicitUtransTint`:

1. the **superuniques.txt Utrans** branch (one-off fixed bosses — Griswold, the
   issue #34 case), and
2. the **MonStats2 byUtrans** branch — which is shared: it overrides the rolled
   unique/champion tint whenever the class carries byUtrans data, and
   champion-eligible classes routinely ship no `palshift.dat`.

For any champion/uniqued monster on an entry-less class whose class carries
byUtrans data, the neutralizer then forced the index back to 2 (untinted) —
killing exactly the tints the R49 global-table fallback (issue #9, "no unique
colors") was field-verified to render.

## Fix (R166 → R167)

Single scoping change in `Source/D2Client/UNIT/CUnit.cpp`:

- the byUtrans branch no longer sets `bExplicitUtransTint` — its tints (and the
  rolled tints it overrides) resolve at draw time through the R49 global
  RandTransforms arm, restoring pre-R165 tinting for champions and uniques on
  entry-less classes;
- the neutralizer keeps firing for the **superuniques.txt branch only** — Griswold
  and the other one-off fixed bosses with entry-less classes stay untinted
  (issue #34 stays fixed);
- comments updated at all three sites (flag declaration, byUtrans branch,
  neutralizer) with the [R167] supersession rationale.

## R168 plan (delivered, no code change)

`R2/R168-30v37-Palette-Plan.md` — full plan for the original retail palette-window
bug, with the verified anatomy: roll `% 0x1e + 9` yields **9..38**, the global table
(`PalShift_RandTransform[7680]`, 30 rows from RandTransforms.dat) serves **8..37**,
the create-time clamp discards **>= 30** — so rows 30..37 are dead data and roll
value 38 is out of the table window (halt-reachable from non-create write sites).
Plan: P0 upstream provenance + write-site audit, P1 roll `+8` / clamp `>= 0x26`
alignment, P2 shared sanitize helper for write sites, P3 gates + owner field QA,
P4 round mechanics.

## Docs + gates

- Version self-ID `0.0.8-R166` → `0.0.8-R167`. CHANGELOG +R167; handoff rev 180
  header + self-ID; README badge/version; this file; R168 plan doc added.
- `tools/docs_gate.py` + `tools/check-vanilla-plus.sh`: ALL GREEN.

## Field-verify asks (owner)

1. Rebuild (R167); first `d2boot.log` mark must read `Vanilla+0.0.8-R167`.
2. Champions and uniques on entry-less classes are tinted again (pre-R165
   behavior); tints that previously appeared are back.
3. Griswold (and other superuniques.txt-Utrans bosses on entry-less classes) still
   render their base palette — issue #34 stays fixed.
4. Mercs and raised skeletons keep their ownership tints (R51/R89 paths untouched).

## Verification boundary

Sandbox-side: docs gate ALL PASS, VP-marker census balanced, brace/paren balance
green on touched files. Compile gate of record stays the owner's Win11 MSYS2 build
(handoff §8); tint behavior needs the in-game repro (asks 2–4) as the deciding
field pass.
