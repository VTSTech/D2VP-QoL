# Round summary — R165 (rev 178, 2026-11-18): issue #34 "Griswold is Green" — the explicit-Utrans entry-less neutralizer

## The ask

Issue #34 (owner, 2026-11-08): "**Bug** Griswold is Green", re-confirmed
2026-11-16 with "Still present R161". Griswold — the Tristram super-unique
zombie — renders with a green unique tint instead of retail's untinted base
palette.

## Root cause

The tint chain, traced end to end (client create → wire → draw):

1. Retail applies a super-unique's tint from `superuniques.txt` `Utrans` —
   Griswold's row says **17** on every difficulty (the pre-1.10 data has no
   `Utrans` at all; the columns were added in 1.10 to carry these hardcoded
   colors). Transform 17 resolves through the global `RandTransforms.dat`
   table as `PalShift_RandTransform + (17-8) * 0x100` — the green-family row
   (the file holds 37 palettes; the retail client only ever reaches 30 of
   them because the drawing-function lookup subtracts the 8-table base — the
   never-fixed Game.exe bug; d2mods.info t=40413).
2. Retail's draw-time lookup, `PALSHIFT_GetMonsterTransform`, **early-outs to
   null for any monster whose class has no `palshift.dat` entry**. One-off
   boss classes — Griswold's monster code `gz` among them — ship no
   `Data\Global\Monsters\<code>\Cof\palshift.dat` in the MPQ: they are not
   revivable, so they never needed the ownership/blood sub-tables the file
   carries. Net retail behavior: an explicit `Utrans` on an entry-less class
   is **dead data** — the boss renders its base palette. (The entry pointer
   is loaded per created monster by the on-demand
   `PALSHIFT_LoadMonsterPalShift` call in the 0xAC handler — retail reads the
   file for every spawn precisely because most *revivable* classes do ship
   one.)
3. The fork's R49 round (issue #9, "unique mobs have no colors") removed that
   early-out so entry-less monsters could resolve the **global** tint table —
   field-verified as the fix for issue #9. Correct for the *rolled* tint path
   (random unique/champion tints), but it also leaked the global table onto
   entry-less classes carrying *explicit* `Utrans` data. Griswold then landed
   on transform row 9 — green — every game, on every difficulty.

The two behavior witnesses are two sides of the same mechanism: issue #9
needs the global table reachable for rolled tints; issue #34 needs explicit
`Utrans` tints to stay dead on entry-less classes, exactly as retail.

## Fix (source round — R164 → R165)

Create-time neutralization, no draw-path change:

- `PALSHIFT_HasMonsterPalShift(nMonsterId)` (`PalShift.cpp` + header): TRUE
  when the class has a registered `palshift.dat` entry — the same slot+4 word
  `PALSHIFT_GetMonsterTransform` reads at draw time.
- `UNIT_CreateMonster` (`CUnit.cpp`): a `[R165] bExplicitUtransTint` flag is
  set wherever the tint index comes from explicit txt data (MonStats2
  `byUtrans`, including the 0xFF ownership reassignment; superuniques.txt
  `Utrans`), cleared by the `[R89]` skeleton ownership override (those
  classes resolve sub-tables through their own entries). After the
  `>= 0x1e` clamp, an explicit tint on an entry-less class neutralizes back
  to idx 2 — the untinted base the TransLvl path stores.
- Rolled tints keep the R49 fallback (issue #9 stays fixed);
  entry-bearing classes keep their tint (retail resolves idx 8+ through the
  global table for them regardless of the entry).

## Docs + gates

- `PalShift.cpp`'s R49 comment carries the `[R165]` supersession note.
- Version self-ID bumped `0.0.8-R164` → `0.0.8-R165`
  (`Redivivus/d2_boot.h` `D2VP_ROUND`; one define feeds the boot mark, title
  and automap tags). CHANGELOG +R165; handoff rev 178 header + self-ID;
  README badge + version line.
- `tools/docs_gate.py`: ALL PASS (0 fail).

## Field-verify asks (owner)

1. Rebuild (R165); first `d2boot.log` mark must read `Vanilla+0.0.8-R165`.
2. Tristram: Griswold renders his base zombie palette (no green tint) on all
   three difficulties.
3. Regression: random outdoor uniques still show rolled tints (issue #9
   behavior unchanged); necro skeleton/revive ownership tints unchanged
   (issues #8/#51 class); act bosses unchanged.
4. If Griswold is still green on R165, the remaining suspect is the
   deployed-MPQ `superuniques.bin` feeding a different `Utrans` — dump
   `data\global\excel\superuniques.bin` provenance; the discriminator is a
   regular zombie vs Griswold side by side in Tristram.

## Verification boundary

Sandbox-side: docs gate ALL PASS, VP-marker census balanced, brace/paren
balance checks pass on all touched files. The compile gate of record stays
the owner's Win11 MSYS2 build (handoff §8); the palette behavior is
MPQ-asset-dependent, so the Tristram eyeball (asks 2–4) is the deciding
field pass.
