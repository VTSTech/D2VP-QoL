# Round summary — R153 (rev 169, 2026-11-10): the #37 choke-point fix — every client-side item-unit free unlinks first

## The crash

The owner rebuilt with R152 (the build that required the `Redivivus/d2_hand.h`
re-exposure — a successful compile IS the proof the fix is in) and smoke-tested:
join → open stash → fast Shared-page flips 2→8→7 → ~5.75 s quiet → crash.
`d2crash-20261109-222517`, build `R150-1-gb12fc932-dirty (release)`: **the same AV
read `[null+0xC]` at `DRAW_UI_InventoryContent+0x9A`** (EAX=0 = pStaticPath after
FreeStaticPath) as the R150-era crash.

Symbolization against the owner's `d2exe.map`, keyed by the paired R148-dirty crash
(`d2crash-20261109-200257`): all 15 frames of both reports are the SAME call chain
(`DRAW_UI_InventoryContent` → `DRAW_UI_Inventory` → `DRAW_UI` → `InGameDraw` →
`CLIENT_GameLoopFrame` → `MessageGameLoop` → `pfModes_EnterGame` → the app-mode
tail) modulo uniform per-region shifts — **+0xC8** (UI/Inv/Draw), **+0x70**
(GAME/Game), **0** (the app-mode tail), **+0x140** (the thread entry). That is the
signature of a rebuild with small code insertions; shift-corrected, every new frame
lands on the exact same symbol+offset.

## Why R152 did not cover it

R152 fixed ONE free site — the 0x9D REMOVED branch. The client frees item units at
~30 `RemoveObjectFromServerObjectTable` call sites (`Item.cpp` alone): the
remove/put RELOAD recreate pairs, the equip/swap recreates, the ground-packet
deletes. None of the others unlinks the unit from the owning inventory's item chain
before the free — any of them can free a chain-linked unit and feed the panel
draw's walk a freed block (the chain nodes live IN the item units). Which echo
carried it this run is unproven (no xfer lines exist in this build's log; the free
happened inside the 5.75 s quiet window after the flips — consistent with a
Ctrl+Click deposit leg or any item-churn echo) — and it does not matter.

## What changed

One VP block in `CUnit::RemoveObject` (`Source/D2Client/UNIT/CUnit.cpp`,
`// VP-BEGIN/END [unit-free R153, issue #37]`), inside the existing `UNIT_ITEM`
branch right before `FreeAny`:

1. **Unlink at the choke point**: the same self-guarding
   `INVENTORY_RemoveItemFromInventory` R152 used (hand re-exposed in
   `Redivivus/d2_hand.h` since R152) — for a chain-linked item it frees the grid
   cell, relinks head/tail/neighbors and clears a matching `pCursorItem`; for
   ground/already-unlinked units the node's `pInventory` is null and the signature
   guard returns 0 before touching anything.
2. **Covers all ~30 free sites at once** — and any future one. R152's explicit
   unlink becomes a harmless second call (the node is already detached → returns 0).
3. **Ungated on purpose** — behaviorally invisible (the unit is being deleted
   either way), and it hardens every retail free path too.

`Redivivus/d2_boot.h`: `D2VP_ROUND "152"→"153"`. No features, no keys, no d2boot
marks.

## Verification

Static — the R153 suite (the unlink lands inside the `UNIT_ITEM` branch BEFORE
`FreeAny`; no-op safety for the ground paths; R152 block intact; boot lockstep) +
`smoke_sync` contract 0 warnings + `docs_gate` green (compile gate of record: the
owner's Win11 MSYS2 build). Field ask: SMOKE **§U +80** — the full storm on R153
(item 79's flips + deposit scenario), then item churn with the panels open
(vendor identify/repair/buy, gamble refresh — the RELOAD recreate path), then the
mash rhythms of 73/75/76. 73–79 stay from R149–R152.

Round identity: self-ID `Vanilla+0.0.8-R153`, automap tag `D2VP-R153`
(`D2VP_ROUND "152"→"153"`).
