# Round summary — R168 (rev 181, 2026-11-18): the retail 30-vs-37 palette-window fix, gated — [features] palette_37, Options row 17 "Palette 37 Tints"

## The ask

Owner: "lets fix the 30 vs 37 retail bug, and make it a gated fix in the Options
Panel" — on the R167 plan (plans/R168-30v37-Palette-Plan.md, marked implemented
this round).

## The bug (verified anatomy, all constants read from the tree)

| Component | Site | Constant | Effect |
| --- | --- | --- | --- |
| Roll | `PALSHIFT_GenerateShiftDuration` (PalShift.cpp) | `nSeedLow % 0x1e + 9` | 30 values, **9..38** |
| Global table | `PalShift_RandTransform[7680]`, 30 rows x 256 B from `RandTransforms.dat` | serves `(idx-8)*0x100` | indices **8..37** |
| Draw-time guard | `PALSHIFT_GetMonsterTransform` default arm | `idx - 8U <= 0x1d` | 8..37 resolve; else **halt 0x160 + exit(-1)** |
| Create-time clamp | `UNIT_CreateMonster` | `>= 0x1e` (30) | discarded **30..38** |

Net: the eight loaded rows 30..37 could never render (~27% of roll outcomes
collapsed into untinted), and the roll's top value (38) pointed one row past the
table — kept from the draw-time halt only by the clamp it shares a site with.

## The fix (gated OFF = byte-retail)

- New gate `PALSHIFT37_Active()` (VanillaPlusCore.cpp): the standard 2 s hot-read
  on `[features] palette_37`, tick `dwVpPalshift37LastCheck` defined above
  `D2VP_ConfigReloaded` and busted by it; declared in VanillaPlusCore.h.
- **Roll site** (PalShift.cpp, `PALSHIFT_GenerateShiftDuration`): gate on ->
  `% 0x1e + 8` (8..37, the full window); gate off -> retail `+ 9`. The roll
  re-seeds per unit from the name seed — no gameplay-RNG entanglement; the
  domain shift only moves which tint a given seed picks (cosmetic).
- **Clamp site** (CUnit.cpp, `UNIT_CreateMonster`): gate on -> `>= 0x26` (a pure
  safety net, no live-row discard); gate off -> retail `>= 0x1e`.
- **Write-site audit** (plan P0.2): the only other palshift-index writers are
  Monster.cpp's death-palette path (idx 3/4/5 or < 8, else 2) and RosterPets'
  ownership recolors (0/1) — all provably in-window, so the create clamp is the
  sole >= 38 gatekeeper; the draw-time halt stays retail-faithful as the canary.

## The Options Panel (row 17 + the 2x9 reflow)

- `gaVanillaPlusRows[17]`: `{ "palette_37", L"Palette 37 Tints", 0 }` — column 1
  band 7, directly under Extended Stats; rides the generic VANILLAPLUS_ToggleRow
  -> D2VP_Get/SetFeatureToggle bridge (ini write + cache flush + chime).
- Geometry (the R147 move, one pitch up): `VPP_ROWS_PER_COL` 8 -> 9; column 0
  bands 0-8 (rows 0-8), column 1 bands 0-7 (rows 9-16, band 8 empty — the R147
  precedent); back row 0x164 -> 0x184; box 0x191 -> 0x1b1 (433 px; 640x480 top
  margin 23 px, the same 0x13 bottom margin).
- Ini template block ships in the same round (fresh d2vp.ini documents the key;
  an existing ini reads 0 = retail until flipped).

## Docs + gates

- Version self-ID `0.0.8-R167` -> `0.0.8-R168`. CHANGELOG +R168; handoff rev 181
  header + self-ID + the seventeen-toggles line; README badge/version; FEATURES
  v23 + table row 21 + §21 (the shipped audit record); SMOKE §Z items 102-103;
  this file; the plan doc's status line -> IMPLEMENTED.
- `tools/docs_gate.py` + `tools/check-vanilla-plus.sh`: ALL GREEN (row-table
  invariants, geometry arithmetic, ini-template key coverage, round identity).

## Field-verify asks (owner)

1. Rebuild (R168); first `d2boot.log` line must read `Vanilla+0.0.8-R168`.
2. SMOKE §Z **102**: the Options page shows seventeen rows in two columns, the
   box fits 640x480 unclipped, the new row toggles + writes the ini.
0. **FIELD PASS 2026-11-18: owner confirms R168 GREEN ("It works!") — §Z 102–103 recorded as passed; the release-notes file RELEASE-NOTES-R162-R168.md ships the R162–R168 window.**
3. SMOKE §Z **103**: with `palette_37=1`, champion/unique tints span new hue
   families (the former rows 30..37); champions still tinted (R167 preserved),
   Griswold still untinted (#34), ownership tints unchanged (R51/R89). With the
   gate off, tints match the pre-R168 build exactly.

## Verification boundary

Sandbox-side: docs gate ALL PASS, VP-marker census balanced, brace/paren balance
green on touched files. The compile gate of record stays the owner's Win11 MSYS2
build (handoff §8); the tint-span and geometry asks need the in-game pass (SMOKE
§Z) as the deciding field check.
