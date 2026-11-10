# Round summary — R150 (rev 166, 2026-11-10): the R149 field pass — the #37 deposit crash closed for real + the #38 perspective character glide

Owner field on the R149 build: **"ok, it build without error, perspective mode still
jittery with interpolation. Ctrl+Click Xfer seemed to work withdraw from stash, but when
I try to Ctrl+Click the items back into stash from Inventory crash as soon as I
ctrl+click the first item that was just withdraw"** —
[#37](https://github.com/VTSTech/D2VP-QoL/issues/37) + [#38](https://github.com/VTSTech/D2VP-QoL/issues/38).
The R149 fixes were each half right: the pair guard covered the wrong window, and the
origin glide left the character behind.

## What changed

1. **#37 — the window is the round trip, not the park** (`VanillaPlusCore.cpp/.h`,
   `Item.cpp`, `Inv.cpp` comments): the 0x19 pick echo HANDS the item client-side
   (`HandleItemRemoveFromContainer` → `CopyInventoryOfPlayerOrFree` sets
   `pInventory->pCursorItem`, `ITEM_SetItemOnHand` draws it under the cursor) until the
   0x18 place echo lands — several latency-dependent frames, far past R149's 2-frame
   park window. A Ctrl+Click inside THAT window never reaches the empty-hand xfer branch
   (`ClickItemLeft` dispatches on the hand item first): it takes the retail hand-full
   path — place/swap with a mid-flight locked item and hover-context globals from
   another panel — the desync that turned the intermittent crash deterministic on the
   deposit leg. Two halves:
   - **`VPCTRLXFER_EchoSuppressHand`** (new core export): Item.cpp's 0x19 echo skips the
     `INVPAGE_None` re-page AND the hand-set for the pair's picked item — the item stays
     node-linked, visible and unlocked in its source cell until the parked 0x18's echo
     relinks it the normal `PutItemInInventory` way. No cursor hop; a pair stranded by a
     panel close leaves the item visible in its cell (better recovery than the R141
     on-hand strand). Retail echoes never match the parked GUID: byte-for-byte retail.
   - **`VPCTRLXFER_PairInFlight`** now spans the whole 500 ms round trip (park + send +
     both echoes + the server hand window; ~3–5x margin over a 25 Hz SP tick, ~200 ms
     TCP headroom). Clicks inside the window are silent no-ops (the pre-R140 retail dead
     click) — mashing throttles to ~2 transfers/s, the deliberate correctness trade.
2. **#38 — the character rides its own glide** (`Wall2.cpp`, `Engine/Gfx.cpp`): R149
   glided the warp ORIGIN (`View.cpp` `SetPerspectiveScale`), but the local player's
   sprite input stayed the raw tick-snapped precision position — the character's
   projected spot was `(P_cur − origin)`: a full step at every tick gliding back to
   zero, so the character (and the ground under it) seesawed at the tick rate while the
   distant world scrolled smoothly — the jitter the owner still saw. The local player's
   warp input now rides the SAME two-sample precision delta
   (`UNLOCKFPS_GetPlayerRenderOffsetOrigin`) at three sites — **Wall2
   `DRAW_WORLD_Unit`** (the body: `input − origin ≡ 0`, the character screen-fixed at
   its exact retail spot while the world glides — the field-accepted top-down shape),
   **Gfx.cpp `DrawUnitShadows`**, **`DRAW_UNIT_States`** (auras/buffs). Other units keep
   their raw 25 Hz cadence, same as top-down. Known residual: the hover selection shape
   (`GFX_DrawUnitSelectionShape`) still projects raw — transient, mouse-hover only.

## Verification

Static only — the R150 suite + `docs_gate` green (compile gate of record: the owner's
Win11 MSYS2 build). Field asks: SMOKE §U rewritten — 73 (mash: no crash, no cursor hop,
the settle-window no-op, the strand stays visible), 74 (the world glides AND the
character is screen-fixed), **75** the withdraw→deposit rhythm (the exact #37 repro),
**76** deposit into a Shared page.

Round identity: self-ID `Vanilla+0.0.8-R150`, automap tag `D2VP-R150`
(`D2VP_ROUND "149"→"150"`).
