# Round summary — rev 146 (2026-11-08): R135, salvage round 1 — the four TAKE commits hand-ported

Base: `86cd10cd` (tree == `8067c4a`, R134) + the rev-145 docs overlay. Owner go:
"Salvage what you can, whatever is least likely to break anything." All four TAKE
items of plans/ELEVENTH-SYNC-SALVAGE.md landed; `92f67c4f` (widest fork-conflict
surface) stays deferred; the DO-NOT-PORT list untouched — above all **no Glide
texture-cache edits** (R134's fix-2/fix-3 remain the field-proven state). Self-ID
`Vanilla+0.0.8-R134` → **`Vanilla+0.0.8-R135`**. This overlay also re-carries the
rev-145 docs files (handoff/HISTORY/plans/merge-upstream/CONTRIBUTING/README and
the gate script) with their rev-146 updates, so it applies cleanly whether or not
the rev-145 zip was applied yet.

## The ports (each is an independent drop in the commit split)

1. **`228dfc62` — SREG_InternalSaveEntry takes its flags as a dword.** Applied
   verbatim (+8/−8; SReg.cpp has no fork marks). Zero behavior change: the tested
   flags 0x10/0x4/0x8 all fit a byte; `SRegGetBaseKey` keeps its cast. Storm only.
2. **`d6746a33` — the gamble table is one 0x198-byte table again.** `D2GambleTableStrc`
   regains `int anLevelLimit[100]`, `pTable` typed `int*` (ItemTbls.h); the loader
   writes the per-level row counts in place (ItemTbls.cpp); the SUNITNPC roll site
   reads `pGambleTable->anLevelLimit[level]` — the vanilla expression again,
   in-bounds by construction (the reused `pPlayerUnit` slot is clamped 5..99
   immediately above). The R128 workaround (file-scope static + bounds-checked
   accessor `GetGambleLevelCache`) retired with dated notes in ItemTbls.{h,cpp}.
   The salvage plan's one required VP guard is in: the diagnostics mark clamps
   `nCacheLevel` 0..99 because its `nPlayerLevel` comes straight from
   `STATSEX_GetCurrentStat` unclamped. Gamble gate rows re-pinned R128→R135
   spellings (5 state rows re-pointed; adjacency-gone + 3 field-mark rows carry;
   one NEW retirement row — suite 519→520).
3. **`83b045a4` — TEXT/OVERHEAD message getters return a 16-bit id.** Text.{h,cpp}
   verbatim; npcmenu.cpp hunks applied clean (no fork marks inside them); ui.cpp
   hand-threaded — the `[quest-gossip-mark]` boot-log marks (the msgType0
   branch-decider and the c2s 0x31 echo) now log `(uint16_t)nMessage`. The one
   other reference, D2Client/UI/Text.cpp:7591's s2c 0x27 mark, needs no edit
   (implicit widening, `%04x` prints the same low 16 bits). Version chores
   (.recomp.json / README / ghidra_version.cpp / .map sidecars) skipped per the
   salvage verdict.
4. **`c16c2a68` — seed, affix, NPC and inactive-item pointer typing.** All 11 real
   files applied clean: `RNG_GetRandomInRange` takes `D2SeedStrc*` (word-array call
   casts gone), `SKILLITEMS_SetDispatchTableEntry`'s EDX is `D2MagicAffixTxt*`,
   `SUNITNPC_ValidateNpcInteraction`'s game-data is `D2GameNpcStrc*`,
   `SUNITNPC_UpdateNpcInventory`'s stackability is BOOL,
   `SUNITINACTIVE_SerializeAndDeactivateItem`'s record is `D2InactiveItemRecordStrc*`,
   Item.cpp keeps its skill snapshot as `int[7]` (the big 104-line hunk).
   Cross-sweep: `D2Client.cpp`'s `RNG_GetRandomInRangeWithMin` is self-contained
   (does not call the D2Common one); every caller of the retyped names is in a
   patched TU; zero VP marks clobbered.

## Scroll-box recon (the free pass port 3 bought)

The fork's scroll chain (`NPCMENU_FindNextUnhandledMessage` /
`NPCMENU_ScrollCompleteShowNextMessage` / `NPCMENU_OnScrollComplete`) is
structurally IDENTICAL to upstream's post-byte-match state — the handled-id
asymmetry (gossip-path callback never appends to `gaNpcMenuHandledMessageIds`;
interaction-path `OnScrollComplete` does) is faithful retail behavior, so the
README's "quest news scroll box does not appear" bug is **not in the
queue-recording logic**. New prime suspect: `a633e56a`'s byte-match round reordered
**`TEXT_UpdateScrollingText`** (Source/D2Client/UI/Text.cpp) — the fork's
early-return form skips the shared cleanup tail whenever `TEXT_DrawScrollingTextBox`
returns false, upstream's nested form falls through. Diff-vs-jaenster and port when
triaging (consult-don't-merge, same shelf as #25). Recorded in handoff rev 146.

## Files in this overlay (27)

| Commit-drop | Files |
| --- | --- |
| 1. SREG dword | `3rdparty/Storm/Source/SReg.cpp` |
| 2. TEXT getters 16-bit | `Source/D2Common/Text.h`, `Source/D2Common/Text.cpp`, `Source/D2Client/UI/npcmenu.cpp`, `Source/D2Client/UI/ui.cpp` |
| 3. Pointer typing | `Source/D2Client/UNIT/Item.cpp`, `Source/D2Common/Seed.h`, `Source/D2Common/Seed.cpp`, `Source/D2Game/Monster/AI/AI.cpp`, `Source/D2Game/SKILLS/Items.h`, `Source/D2Game/SKILLS/Items.cpp`, `Source/D2Game/SKILLS/SkillItem.cpp`, `Source/D2Game/UNIT/SUnitDmg.cpp`, `Source/D2Game/UNIT/SUnitInactive.cpp` |
| 4. Gamble in-struct + self-ID + gate rows | `Source/D2Common/DATATBLS/ItemTbls.h`, `Source/D2Common/DATATBLS/ItemTbls.cpp`, `Source/D2Game/UNIT/SUNITNPC.h`, `Source/D2Game/UNIT/SUNITNPC.cpp`, `Source/Game/Main.cpp`, `tools/check-vanilla-plus.sh` |
| 5. Docs | `handoff.md`, `HISTORY.md`, `plans/ELEVENTH-SYNC-SALVAGE.md`, `docs/merge-upstream.md`, `ROUND-SUMMARY.md`, `CONTRIBUTING.md`, `README.md` |

Split rules honored: every prefix is suite-green; `SUNITNPC.h` rides with
`SUNITNPC.cpp` (decl/def pairing — the header carries the `c16c2a68` typing
decls, the .cpp carries both the typing defs and the gamble sites, one commit);
the gate script lands with commit 4 (its rows + the self-ID rows + the code all
land atomically). `SUNITNPC.cpp` noting: the `c16c2a68` typing hunks
(ValidateNpcInteraction/UpdateNpcInventory) ride commit 4 with the gamble sites —
typing-only, no behavior.

## Suggested commit split (owner applies as own commits per §8)

1. `fix(storm): SREG_InternalSaveEntry takes its flags as a dword (228dfc62)`
2. `fix: TEXT/OVERHEAD message getters return a 16-bit id (83b045a4)`
3. `fix: type seed, affix, NPC and inactive-item pointers (c16c2a68)`
4. `fix: gamble table is one 0x198-byte table again (d6746a33); R128 accessor retired; self-ID R135`
5. `docs: rev 146 - salvage round 1; rev-145 offloaded to H48`

## Verification on this overlay

- `tools/check-vanilla-plus.sh`: **520/520** (one new retirement row; was 519/519).
- r135-salvage syntax sweep: **13/13 TUs clean** (baseline-diff vs `86cd10cd`, host
  shims, the merge-site v2 method — `scripts/r135_salvage_syntax_sweep.sh`).
- d2boot visibility audit: **ALL VISIBLE** (gamble marks re-checked after the edits).
- handoff budget: 15,982 / 16,384 B (suite-checked).
- Cross-sweep: retyped-name callers all patched; VP marks intact (ui.cpp 13,
  SUNITNPC.cpp 4 blocks); README release line stays at the last tag (R128) by design.

## Owner owes (the compile + smoke gate of record)

- MSYS2 build + link (the sandbox has no cross build this round; the full compile
  is the gate of record).
- In-game smoke: the standing **Gheed gamble click belt** (§M2 discriminators:
  `cache=` sane count, `bound=` 2..total, `pick=` item id — the marks survive
  unchanged) + **one NPC-gossip click** (the re-typed 16-bit path: the
  `msgType0=` mark and the 0x31 echo must still log) + d2boot.log line 1 self-ID
  `Vanilla+0.0.8-R135`. The gamble belt can ride the same session.
