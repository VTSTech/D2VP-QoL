# Inventory Sort — feature plan

*Drafted 2026-11-09 on owner ask (chat, no issue). The ask, verbatim:* "How hard would it be
to add an inventory sort?" *The same-session assessment answered **Medium — easier than the
shared stash**: unlike row 9 this needs zero server-side code, because every move it makes is
a packet a human drag also sends. Owner constraints not yet set — the four open calls are §9.
Slate row 16 (`[features] inventory_sort`). Status: **PLANNED — no code yet.**

---

## 1. Goal and non-goals (v1)

**Goal.** With `[features] inventory_sort=1`, one trigger (button or hotkey — §9 call 1)
re-arranges the **personal inventory grid** into a deterministic, category-ordered layout.
The sort is a visible sequence of real item moves paced over a few seconds, indistinguishable
from a fast player dragging — same packets, same server validation, same broadcasts.

**Non-goals for v1** (each is a clean follow-up, deliberately deferred):
- No stash-side sort (personal inventory only; a stash sort would sort the active page —
  retail or shared — and is a small follow-on once the sequencer is field-proven).
- No cube or belt sorting (belt is its own container with its own semantics; cube contents
  are excluded from the snapshot entirely).
- No stack splitting or merging (that is rows 12/13's surface — untouched here).
- No client-only "fake" reorder (relabeling the grid without moving items server-side) —
  instant desync on any realm, the one implementation class flatly rejected.
- No balance surface: sorting moves items, never changes them.

## 2. The hard invariants (pillars)

1. **Every packet is a retail packet.** The replay stream is only `0x19` pick-to-cursor +
   `0x18` insert-into-buffer pairs — byte-identical to a manual drag. Zero D2Game changes,
   zero new opcodes, nothing realm-facing to defend.
2. **Every intermediate grid state is legal.** The GS validates each insert against the live
   grid, so a place may only target a cell free *at that moment*. The sequencer (§5) owns
   this; a refused move aborts the queue, never retries.
3. **The user's hand always wins.** Any item click/drag while the queue is live kills the
   queue first (the manual pick must not race a queued pick). An aborted sort leaves a legal
   grid and no residual cursor item.
4. **Default OFF = vanilla byte-for-byte.** Gate predicate-false at every touched site; with
   the gate off not a single packet path is reachable. `.d2s`/`.d2x` untouched (rule 5).

## 3. Where things live today (recon, this tree — all "verify", no tree-verify pass yet)

- **The move primitive is shipped code**: the row 0c branch in `D2Client/UI/Inv.cpp` —
  `ITEM_GetContainingInventoryPage` picks the direction, `INVENTORY_FindItemGridPosition`
  finds a free cell, then the `0x19`+`0x18` pair rides
  `D2Client::GAME::Msg::NET_D2GS_CLIENT_Send_SHORT_SHORT_SHORT` (the shared-stash tab's
  send helper). R140's first field build proved `0x18` alone is a dead send — the `0x18`
  handler (`SCMD_0x18_ItemToInventory`) opens with `SERVER_IsItemOnCursor`, so the pair is
  mandatory. A sort reuses the pair and pins its own x/y instead of first-fit.
- **The grid model**: `D2Common/INVENTORY/Inventory.*` — occupancy, item x/y and size
  records; the same API the server-side fit check reads. Snapshot = walk the player unit's
  inventory, project each item's footprint into a 10×4 occupancy map.
- **Category fields**: the item records the row 0b slot resolver already reads (ItemTypes
  rows via `ITEM_GetTxtItemTypes_*`, item-code families like the gcv/gcs gem codes and
  r01–r33 runes cited in §12/§13); ilvl and quality are on the item unit data (row 3's
  offset 0x2C precedent).
- **The affordance**: the shared-stash tab pattern in `VanillaPlusCore.cpp`
  (`VPSHAREDUI_DrawTab` — outlined text, saved/restored font, rect hit-test, page-aware
  layout math) if the trigger is a button; row 0b's cmd-key read idiom
  (`GetAsyncKeyState(vk) < 0` per frame, CmdTbl remap honored) if a hotkey.
- **The pacing hook**: a per-frame tick state machine beside the alt_toggle frame hook; the
  queue lives in `VanillaPlusCore.cpp` — no new TU (the nine-TU fork surface grows only for
  cause).

## 4. Design (recommended shape)

**Trigger** (§9 call 1) → **snapshot** → **plan** → **replay**. One shot per trigger; a
second trigger while live is ignored (or re-plans — owner call folded into §9 call 1).

- **Snapshot**: occupancy map + item list (guid, x/y, w×h, category score). Excludes
  equipped, belt, cube contents; refuses if the cursor holds an item (cube path's refuse
  sound 0x13, no queue).
- **Plan**: score every item (§4a), compute the target layout by shelf-packing top-left
  first, larger footprints before smaller within a category; diff current → target into an
  ordered move list via the sequencer (§5). If current layout == target, do nothing.
- **Replay**: a three-state tick (`IDLE → PAIR(pick) → PAIR(place)`) sending one packet per
  tick at ~100–150 ms, so a full grid sorts in ~3–5 s and the GS never sees a burst. The
  next pair may gate on the server's placement echo if the tree-verify round pins one;
  fixed pacing is the v1 fallback.
- **Gate + surface**: `[features] inventory_sort` (ini template per rule 3, options-page
  eleventh row per the R125/R140 pitch pattern), hot-re-read on the 2 s gate tick.

**4a. Sort order (default, §9 calls 2–3 may reorder)**:
potions (rejuv > heal > mana, by tier) → keys/tomes/scrolls → gems/runes/jewels → charms →
rings/amulets → everything else (weapons/armor by type then name). Within a category,
larger footprint first, then score. Quest-key items sort with keys; an item that matches no
category sorts last by name.

## 5. The sequencer — legal moves on a packed grid

The one genuinely hard part. Pick-first frees the source cell before the place, so a plain
insertion order works while the grid has slack. The degenerate case is a **cyclic
displacement** on a packed grid (A's target holds B, B's target holds A). Draft answer:

1. Compute free cells after the snapshot. If zero, refuse politely (no scratch cell
   available) — sound + d2boot.log mark, no queue.
2. Otherwise earmark one scratch cell (rightmost-bottom of the free set), never a target.
3. Emit places in target order; when a place's target is occupied, first rotate the chain
   it belongs to through the scratch cell (pick occupant → scratch, place → freed cell, …).
4. Refill the scratch cell's own item last.

Properties worth stating: the sequence length is bounded (each item is picked at most
twice); every intermediate state is legal by construction; determinism is testable (same
snapshot → same packet stream — an exit-criterion item). A fit-check reject mid-replay
(the server disagrees with our occupancy map — should be impossible, must be survivable)
aborts the remaining queue, logs the guid + tick to d2boot.log, leaves the grid legal.

## 6. Failure and recovery matrix (R61 pattern — degrade to mark, never halt)

| Event | Behavior |
|---|---|
| Cursor occupied at trigger | Refuse sound 0x13, no queue, log mark |
| Zero free cells at trigger | Refuse sound, log mark (no scratch cell to cycle through) |
| Item click / drag while live | Queue killed before the pending tick; manual action proceeds |
| Panel close / death / TP / game leave | Queue killed; state reset; no packets in flight |
| Fit-check reject from server | Queue aborted at that pair; guid + index logged; grid stays legal |
| Queue length cap hit (defensive) | Abort + log (a bug in the planner would show here, not in play) |
| Gate flipped OFF mid-sort | Queue killed on the next tick (gate read per tick, the 2 s cache is fine) |

## 7. Roadmap — three code rounds, each field-verifiable

- **Round 1 — tree-verify**: the §3 anchors → exact pins; the pair shape re-confirmed
  against the live `0x18`/`0x19` handlers; the placement echo identified (or fixed pacing
  confirmed); the inventory-walk + occupancy API pinned; TU list frozen.
- **Round 2 — implement**: gate + snapshot + planner + sequencer + paced replay + abort
  surface + trigger + the Options page's eleventh row ("Inventory Sort"; Previous Menu
  moves down one pitch, the R125/R140 pattern) + ini template; self-ID roll per the R139
  one-line define (`D2VP_ROUND` in `Redivivus/d2_boot.h`).
- **Round 3 — field QA**: the exit loop (§8) on the owner's Win11 build; new SMOKE
  section rows land with this round (the row 0c pattern — §R was born with R140).

## 8. Risks and the exit loop

**Risk: Med** — the first slate row that streams a scripted input sequence. The bug classes
are desync-adjacent (a lost/mis-ordered pair leaves an item on the server cursor) and
interaction races (user clicks mid-sort). Both are bounded by the abort surface and the
refuse-don't-retry rule; nothing writes save data.

**Exit** (all must hold on the field build):
- A mixed 30+-item grid sorts to the same layout twice in a row (determinism).
- A mid-sort click aborts; the grid is legal; every item remains reachable; no residual
  cursor item after *any* abort path in the §6 matrix.
- An inventory with only 3 free cells still sorts (the scratch-cell case); a deliberately
  overpacked 0-free-cell inventory refuses gracefully.
- `0` from a fresh start = vanilla byte-for-byte (no packet ever sent).
- Save → load → retail-load `.d2s` all clean; ATMA/GoMule see the same items at new
  coordinates.

## 9. The four owner calls

1. **The trigger** — button on the inventory panel, hotkey, or both (draft leans button +
   optional key later; the tab pattern is proven, and a button needs no keybind surface).
2. **The category order** — §4a's draft is the default; owner may reorder freely (it is one
   score table).
3. **Within a category** — size-first (draft) vs. name-first vs. quality-first.
4. **Stash affordance in v1?** — draft says no (inventory only); a stash-page sort is the
   natural row 16b once the sequencer is field-proven.
