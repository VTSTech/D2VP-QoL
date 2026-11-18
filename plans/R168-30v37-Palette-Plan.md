# R168 Plan — the original retail 30 vs 37 palette bug

Status: **plan** (no code change in R167). Target round: **R168**, single commit, same
round conventions as every prior round (boot self-ID bump, CHANGELOG, ROUND-SUMMARY,
handoff rev bump, docs gate + vanilla-plus gate green, zips + tmpfiles upload).

## 1. The ask

Owner: "create a plan to fix the original retail 30 vs 37 palettes bug for R168."
The palette-shift (tint) system's three components disagree about how many random
monster tints exist: the roll can produce 30 values (9–38), the global transform table
serves 30 rows (indices 8–37), and the create-time clamp caps the index at 29. The
result: **8 fully-loaded transform rows (30–37) can never render**, the roll's top
value (38) points outside the table entirely, and any non-create write site that
delivers 38 reaches a draw-time **halt** (`ERROR_UnrecoverableInternalError_Halt` +
`exit(-1)`) — a crash-class fault.

## 2. Verified bug anatomy (all constants read from the fork's current tree)

| Component | Site | Constant | Effect |
| --- | --- | --- | --- |
| Roll | `PALSHIFT_GenerateShiftDuration` (PalShift.cpp:477) | `nSeedLow % 0x1e + 9` | 30 values, **9..38** |
| Global table | `PalShift_RandTransform[7680]` (PalShift.cpp:153), loaded from `Data\Global\Monsters\RandTransforms.dat` with a `0x1e00`-byte read | 30 rows × 0x100 B | serves indices **8..37** via `(idx - 8) * 0x100` |
| Draw-time guard | `PALSHIFT_GetMonsterTransform` default arm (PalShift.cpp:444) | `idx - 8U <= 0x1d` | 8..37 resolve; anything else → **halt 0x160 + exit(-1)** |
| Create-time clamp | `UNIT_CreateMonster` (CUnit.cpp, `if ((int)nPalShiftIdx >= 0x1e) nPalShiftIdx = 2;`) | `>= 0x1e` (30) | discards **30..38** |

Consequences, in order of visibility:

1. **Dead rows:** rolled tints 30..37 are discarded by the clamp even though the table
   loads and serves them. 8 of 30 rolled outcomes (~27%) collapse into "untinted"
   (idx 2) — rolled champions/uniques are visibly less varied than the data supports.
2. **Off-by-one roll:** the roll's top value (38) is outside the table window (8..37).
   It survives only because the clamp catches it first; every path that skips the
   create-time clamp exposes the draw-time halt.
3. **Halt exposure:** `SetPalShiftIndex` / `GFX_SetPalShiftIndex` write sites outside
   `UNIT_CreateMonster` (roster recolors, event/packet-driven recolors — the R50/R51
   family) do not pass through the clamp. Any of them writing ≥ 38 halts the game.
4. The "30 vs 37" name: 30 is both the roll modulus and the row count; 37 is the
   highest index the table serves. Retail's three constants (9, 0x1e-as-modulus,
   0x1e-as-clamp) were never mutually consistent.

Phase 0 must first settle **retail-genuine vs transcription defect**: diff these four
sites against upstream's `asm_matching/msvc8/translated` copy of PalShift.cpp/CUnit.cpp.
If upstream carries the same constants, the bug is retail-genuine (as the owner
labels it) and R168 is a deliberate fork divergence — document it as such in the
supersession notes. If upstream differs anywhere, that site is a transcription bug
and gets the standard `[Rn]` provenance treatment instead.

## 3. Goal

One consistent index domain end to end:

- **Roll:** `nSeedLow % 0x1e + 8` → **8..37** — every roll lands on a live table row.
- **Clamp:** `nPalShiftIdx >= 0x26` → 2 — a pure safety net (38+ only reachable from
  explicit txt data or rogue write sites); it no longer discards live rows.
- **Explicit tints:** byUtrans / superuniques Utrans values in 8..37 resolve through
  the global table; values ≥ 38 clamp to 2 (same as today).
- **TransLvl path untouched:** `TransLvl < 8 → TransLvl + 2` (idx 2..9) is retail
  behavior and overlaps the rand window (8/9 = rows 0/1) exactly as retail does.

## 4. Non-goals

- No palette art changes, no MPQ/RandTransforms.dat data edits, no per-monster
  palshift.dat additions.
- No change to the R49 global-table fallback, the R165/R167 neutralizer scoping, or
  the R50/R51 ownership polarity fixes.
- No re-roll of existing units' tints beyond the inherent seed-offset shift (below).

## 5. Phases

### P0 — audit and provenance (read-only)
1. Diff the four sites in §2 against upstream `asm_matching/msvc8/translated`
   (`Source/D2Client/GAME/PalShift.cpp`, `Source/D2Client/UNIT/CUnit.cpp`); record
   which constants are retail-genuine.
2. Enumerate **every** palshift-index write site: `GFX_SetPalShiftIndex`,
   `CUnit::SetPalShiftIndex`, and their callers (roster recolors, mon-prop recolors,
   any packet-driven path). For each: can it produce ≥ 38 today? Record in a table
   that lands in the R168 ROUND-SUMMARY.
3. Confirm the roll's RNG isolation: `InitSeed_Param_666(pSeed, nSeedInput + classId)`
   re-seeds per unit from the name seed, so shifting `+9` → `+8` only moves which
   tint a given seed picks — verify it shares no sequence with gameplay-relevant RNG
   (loot, AI) before touching it.

### P1 — align roll and clamp (the core fix, ~4 lines)
- `PALSHIFT_GenerateShiftDuration`: `% 0x1e + 9` → `% 0x1e + 8`; docstring updated
  ("indices 8-37, the full RandTransforms window"); `[R168]` provenance comment.
- `UNIT_CreateMonster`: `>= 0x1e` → `>= 0x26`; comment documents it as a safety net
  for explicit/rogue values, not a live-row discards.
- Update the stale "Halts if the random index is out of the 30-entry range"
  docstrings on `PALSHIFT_GetMonsterTransform` / header.

### P2 — write-site guards
For each audited write site that can deliver an out-of-window index, route the value
through a single shared clamp helper (new small inline in PalShift.cpp,
`PALSHIFT_SanitizeShiftIndex`: `< 2 → 2` for the base/blood arm is NOT enforced here —
only `>= 0x26 → 2` and an `8..37` assert-comment for the rand arm) so future write
sites inherit the guard. Keep the draw-time halt retail-faithful (fork policy) now
that every write path is provably in-window; note in the supersession block that the
halt remains the canary if a future site skips the helper.

### P3 — verification
- Static: `tools/docs_gate.py` + `tools/check-vanilla-plus.sh` ALL GREEN; VP marker
  census balanced; brace/paren balance on touched files.
- Numeric: a tiny Python harness asserting the new roll domain ⊂ [8, 37] and the
  clamp table (2..37 pass, 38+ → 2) — printed, not assumed.
- Field asks (owner): (a) rolled champion/unique tints visibly span the full range —
  tints that never appeared before (former rows 30..37) show up; (b) old logs show no
  `0x160` halt line after long sessions with heavy champion traffic; (c) Griswold
  still untinted, champions still tinted (R167 behavior preserved); (d) Merc/skeleton
  ownership tints unchanged (R51/R89 paths).

### P4 — round mechanics
Boot self-ID `0.0.8-R167` → `0.0.8-R168`, CHANGELOG entry, ROUND-SUMMARY (audit table
+ field asks), handoff rev bump, README badge, commit "Vanilla+0.0.8-R168 - the retail
30 vs 37 palette window alignment", zips + tmpfiles upload, issue #34/#9 follow-up
comment if the owner tracks this under one of them.

## 6. Risks and mitigations

| Risk | Likelihood | Mitigation |
| --- | --- | --- |
| Rows 30..37 are rarely-seen palettes (retail never rendered them) and may look off | medium | owner QA matrix over all 30 rows before merge; rows are straight RandTransforms.dat data — worst case is cosmetic |
| Seed-domain shift changes existing units' rolled tints | certain, cosmetic | document in CHANGELOG; no gameplay RNG entanglement (verified in P0.3) |
| A write site missed in P0.2 still delivers ≥ 38 | low | draw-time halt stays as the canary; P2 helper makes new sites safe by default |
| Upstream turns out to differ at one of the four sites (transcription bug, not retail bug) | low-medium | P0.1 settles it first; fix becomes a standard transcription repair, plan steps unchanged |
| The clamp change re-opens a leak the clamp accidentally masked (e.g., an explicit Utrans ≥ 38 that used to clamp) | low | P0.2 table lists explicit data ranges; the `>= 0x26` clamp still catches them |

## 7. Acceptance criteria

1. Roll domain ≡ table window ≡ [8, 37]; clamp discards only ≥ 38.
2. Every palshift-index write site either provably in-window or routed through the
   sanitize helper.
3. All gates green; field asks (a)–(d) confirmed by the owner.
4. Docs complete: provenance comments at every changed constant, ROUND-SUMMARY audit
   table, CHANGELOG, handoff rev bump.
