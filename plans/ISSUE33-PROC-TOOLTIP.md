# Issue #33 triage — Items drop with x% "chance to cast lvl 0" (track W)

> **LANDED — R137, field-verified, owner-closed (2026-11-08).** The proposal patch
> (`issue33-proc-tooltip-fix-proposal-R137.patch`) applied verbatim (`git apply
> --check` clean); owner: **"R137 confirmed closes Issue #33."** Gates 522 → 524;
> self-ID `Vanilla+0.0.8-R137`. Records: HISTORY **H54**, standing check SMOKE **§O**
> (item 51). The triage below is preserved verbatim from the investigation (R136
> tree state).


**Investigated:** 2026-11-08, against the R136 tree (base `86cd10cd` + the R136 overlay, gates 522/522 untouched).
**Verdict:** root cause found, sandbox-side, no artifacts needed. It is a **baseline regeneration transcription defect in the tooltip formatter** — display-only, third instance of the #16/#18 varargs family. Generation, the `.d2s` save and the item-packet round-trip were all verified correct: **the items' server-side proc stats were never wrong.** Fix proposal + gates ready: `issue33-proc-tooltip-fix-proposal-R137.patch` (verified `git apply --check` clean; suite **522 → 524** with the patch, verified apply → 524 / reverse → 522).

---

## 1. What the owner sees

Magic/rare items drop carrying a "Chance to Cast" affix and the tooltip line reads
`<x>% Chance to Cast Level 0` — the percent renders, the **level is 0**, and the line
**ends there**: no skill name, no "When You Strike"-style event tail. (The skill name
slot is silently eaten by the same defect — see §3.)

## 2. The proc pipeline, walked end to end

**Generation (correct).** Affix/uniques/set proc properties dispatch through
`ITEMPROP_AddSkillOnEvent` (`D2Common/ITEMS/ItemMods.cpp:2315`): skill from the
property row, level ≥ 1 guaranteed on every path (level-0 is a sentinel that scales
from the item level, clamped ≥ 1), then `ITEMMOD_AddStatToItem(..., nSubIndex =
skill*0x40 + (level & 0x3f), nValue = chance)` — in-memory `{subindex = skill<<6|level,
value = chance}`.

**Save/packet round-trip (correct).** The stat serializer (`SerializeComplexItem`,
`Items.cpp:7496+`) and the packet writer write the generic `{param: saveParamBits,
value: saveBits}` pair; the reader twin (`ITEMS_DeserializeSingleStat`,
`Items.cpp:2559`) reads `saveAdd[idx+2]` bits of param — which aliases `saveParamBits`
exactly (packed struct: `saveAdd[2]` starts at 0x24, `saveParamBits` is 0x24) — then
`saveBits` bits of value. Faithful both ways; the 21-bit packed
`skill|level<<9|chance<<14` word is only the dead ≤0x5c legacy path (`ExtractItemDetails`
hardcodes version 0x60).

**Display (broken).** The item tooltip's descfunc dispatcher
(`ItemStatCost_DescFunc_DGrpFunc`, `D2Client/UI/SkillDesc.cpp:1226`) walks the stat
pairs (param = wSubIndex, value = stat value — both verified at the call site) and the
proc family (stats 195–202, descfunc 15) lands in **case 0xf**, which decodes
`skill = param >> nChargedSkillLevelBits`, `level = param & mask` correctly — then
formats:

```cpp
uVar2 = 0;
nRepairCostFinal = nStatParam;                    // the decoded LEVEL, in hand
UNICODE_FormatWideString(0x100, wszOutputBuffer, GetLocaleString(descstrpos),
                         nGroupValue, uVar2, nRepairCostFinal, wszLocaleStr);
//                              chance      ^ literal 0   level    skill name
```

The descstrpos format consumes `%d` (chance) `%d` (level) `%s` (skill name). **The
literal `0` occupies the Level `%d` slot** — every proc line renders "Level 0" — and
the level **int** falls into the `%s` slot, where the `[issue-18 fmt-args]`
sub-0x10000 guard (`UNISYS.cpp:541`, added in the #18 round) **silently terminates
the line** instead of dereferencing a bad pointer. Net rendering: `5% Chance to Cast
Level 0` and stop. Exactly the issue title, byte for byte.

## 3. Classification

- **Baseline regen transcription defect** (a Ghidra register-reuse artifact
  materialized as a real vararg), same family as #16 (R84: missing varargs in the
  elemental single-value branches) and #18 (the dropped monster-name vararg at the
  `charmontohit2X` call site — the `VP [issue-18 fmt-args]` marks in
  `ui.cpp:8952` and `UNISYS.cpp` are that round). Case 0xf is the family's third
  instance and the first one that degrades quietly instead of faulting.
- Not a salvage regression: the R135/R136 delta never touches SkillDesc.cpp (the file
  had zero uncommitted changes before this investigation) and predates R128.
- Likely present upstream (jaenster/redivivus) too — reporting upstream is the
  owner's call (sync PAUSED).
- Server-side stats, actual proc behavior on hit, and saved characters were always
  correct — tooltip-only.

## 4. Proposed fix (R137, one call site + gates)

`issue33-proc-tooltip-fix-proposal-R137.patch`:

1. **`SkillDesc.cpp` case 0xf** — drop the `uVar2 = 0` vararg and pass
   `(chance, level, name)`, VP-marked `// VP-BEGIN/END [proc-level0 #33]` with the
   family note. 1 line of behavior, one call site.
2. **`tools/check-vanilla-plus.sh`** — two new gates:
   - `proc-line: level in the level slot (R137)` — the corrected argument spelling = 1
   - `proc-line: literal-zero vararg gone` — `nGroupValue, uVar2, nRepairCostFinal` = 0
     (reads **1** against the unfixed tree, so the gate catches the defect class)
   - Verified: suite **524/524** with the patch; **522/522** after reverse; patch
     `git apply --check` clean against the current worktree.
3. Round bookkeeping at landing: self-ID bump `Vanilla+0.0.8-R137`, CHANGELOG entry,
   handoff rev line (track W triaged/fixed), SMOKE check (hover a proc item), HISTORY
   H-record per the trim pattern.

## 5. Owner field check (after applying + building R137)

Hover any magic/rare item with a "Chance to Cast" affix: the line must read the real
level and skill name — e.g. `5% Chance to Cast Level 3 Dim Vision When You Strike` —
no `Level 0`, no truncation. Ride-along: a charged staff/wand's charge line renders
as before (same descfunc block, untouched shapes).

## 6. Verification done sandbox-side

- Full chain walked in source: `AddSkillOnEvent` → `AddStatToItem` →
  `SerializeComplexItem` / packet writer → `DeserializeSingleStat` (param-aliasing
  check `saveAdd[2]` == `saveParamBits` at 0x24) → descfunc dispatcher → case 0xf →
  `UNICODE_FormatWideString` (the #18 guard).
- All proc in-memory writers/readers audited — the display call is the only
  non-faithful site.
- Gate suite: 522/522 before, 524/524 with the patch, 522/522 after reverse; patch
  `git apply --check` clean.
