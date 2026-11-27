# main branch — handoff (rev 191, 2026-11-26 — **R241: gamble Refresh on all six vendors + `[features] gamble_refresh` gate; R240 Act 4/5 start crash fixed**): the previous handoff (rev 187, R204–R235) is preserved verbatim in **HISTORY.md H83**. Everything below is the live state.

## 1. Branch topology (unchanged from H83)

- **Repository** `D2VP-QoL`; `main` is the only integration branch; the upstream merge line is RETIRED (rev 152).
- Build self-ID **`Vanilla+0.0.9-R241`** (`Redivivus/d2_boot.h` `D2VP_VERSION_TAG` + `D2VP_ROUND`); the first `d2boot.log` line pins any shipped exe.
- **Compile gate of record**: the owner's MSYS2 MINGW32 (i686-w64-mingw32-gcc 16.2) build via `tools/build-local-win11.ps1` → `build\`. The Linux tree is where edits are made; they are synced to the Windows tree.

## 2. Field status

Eighteen `[features]` toggles ship on the Options page, every one OFF by default (`gaVanillaPlusRows[18]`).

| Area | Status |
| --- | --- |
| SP + Open Battle.net + closed-realm login / charlist / creategame / join (R169→R203) | **WORKING — field-verified twice** (`docs/BNET-CONNECT-FINDINGS.md`; agent memory `bnet-connect-10053-investigation`) |
| Vanilla+ 17 opt-in features (SP) | **SHIPPED + FIELD-PROVEN** (H83 §2) |
| **Gamble Refresh button (R237–R241)** | **WORKING — field-confirmed 2026-11-26** for Gheed (0x93), Elzix (0xc7) and Alkor (0xfe). R241 adds Jamella (0x195) and Larzuk (0x200/0x202) and puts the whole thing behind `[features] gamble_refresh` (default 0). Acts 4/5 **awaiting field**. See §4. |
| **Act 4/5 start (R240)** | **FIXED — field-confirmed 2026-11-26** ("no longer crashes on -act4 or -act5"). See §5. |
| **Shared stash on closed realms (R204–R235)** | **IN FIELD BRING-UP — not yet working.** The 0x4F frame is recovered in the field (one dereference). Remaining: read `pGame`/`pUnit` from the dispatch struct (register mapping is wrong) and settle the post-click Fog crash. See §3.6. |
| Release `Vanilla+0.0.9-R203` | shipped (README / CHANGELOG / RELEASE-NOTES-R188-R203.md) |

## 3. OPEN TRACK — shared stash on closed realms (R204–R234)

### 3.1 Goal (owner)
Each **account** gets its own shared pool, shared across that account's characters and **never** visible to other players. Single-player keeps the existing global pool file.

### 3.2 Architecture (built, compiles, deployed)
- All logic is **server-side D2Game** in `Source/D2Game/PLAYER/VanillaPlusStash.cpp` (fork-owned TU). The client only draws the tab and sends `C->S 0x4F` (ClickButton): button u16, pad u16, page u16.
- Realms run D2Game inside the **D2GS**, so the stash ships as **`d2vpstash.dll`** (hook DLL) plus **one 6-byte patch** of the stock `D2Game.dll`.
- The DLL is **import-free** (verified: imports are KERNEL32 + msvcrt only; single export `VPSHARED_HookPacket@12`). All game-side calls resolve at runtime via `GetModuleHandleA` + `GetProcAddress`(ordinal) or RVA.
- **The export is CDECL** even though the `.def` names it `@12` (disassembly: `pop ebp; ret`, no `ret 12`) — **the stub must `add esp,12` after the call**.
- **Deployment: the DLL goes on the D2GS only.** `d2vp.exe` must not carry it. The client needs `[features] shared_stash=1` in its own `d2vp.ini`; the D2GS needs `C:\d2gs\d2vp.ini` with the same key (hard gate: without it `VPSHARED_FeatureActive()` is 0 and our code never runs).
- Hook contract: patch `D2Game.dll` file offset **0x6cf6f** (VA `0x6fc8cf6f`); displaced bytes **`8B 44 24 04 56 57`**; resume VA **`0x6fc8cf75`**; section-append stub; call `VPSHARED_HookPacket(pGame=ECX, pUnit=EDX, pPacket)`; return 1 → `xor eax,eax; ret 8` (no `[cheat]` line); return 0 → replay + `jmp`. The stub saves EDX/ECX/EBX (12 bytes) before building arguments, so its `[esp+0x10]` / `[esp+4]` / `[esp+0xC]` operands are correct.
- Pools: `VPPool aPools[8]` (key = account name → `_d2vp_shared_<key>.sss`) and `VPActive aActive[8]` (per character). Fork-private buffers use the module's own heap (`HeapAlloc`), never the game pool.
- Persistence: `SFile::GetAndCreateSavePath` (Fog 10115) → `.sss` beside the D2GS saves. Save-flush parks the shared page before any `.d2s` (invariant I1); `VPSHARED_AfterSave` re-activates it afterwards.

### 3.3 Build configurations (d2vpstash target)

| Define | Meaning |
| --- | --- |
| `VPSTASH_D2GS_113D` | 1.13d realm code paths (required) |
| `VPSTASH_113D_DLL` | export + local logging + DllMain beacon |
| `VPSTASH_113D_NO_0X9C` | v1 state-only: no client repaint (grid refreshes on stash reopen) |
| `VPSTASH_113D_PROBE` | zero-risk: logs arguments, always returns 0 |
| `VPSTASH_113D_PASSTHROUGH` | [R235] isolation: logs `H`, touches nothing, returns 0 |

`cmake --build build --target d2vpstash` → `build/d2vpstash.dll` (the target links `d2_flags`; never restate flags — CMake de-duplicates repeated `-include`).

### 3.4 What is PROVEN in the field
- The DLL loads in the D2GS (module list, base `0x76F00000`); `DllMain` runs (beacon: `BEAC dll=C:\d2gs\ log=…`); relative-path writes from inside the DLL work (`VPLOG.TXT` starts with `D` at load).
- The stub executes and calls our export; the dispatcher was independently disassembled (`0x6fc873a0`: opcode byte → table `0x6fd1a698` → `call [edi*8+…]`, pGame=ECX, pUnit=EDX, buf=EBX).
- Trace alphabet in `VPLOG.TXT`: `D` load, `H` entered, `N` null argument, `X` not-0x4F, `S` serialize, `M` materialize, `>` candidate pointer, `<` dereferenced bytes, `R` adopted frame; [R235] `#` = 24 bytes of the dispatch struct, `[` ECX+EDX `]`, `T` = 24 bytes at the adopted frame.

### 3.5 Pinned inventory (do NOT re-derive)
| What | Value | Evidence |
| --- | --- | --- |
| Game profile | **1.13d** (D2Game/D2Common), ordinal base 10000, `d2server.dll` resolves 43 fixed ordinals | PE timestamps, export shape, no `<Inspector.*>` strings |
| `D2UnitStrc::eUnitType` / `pInventory` | `0x00` / `0x60` | deref evidence |
| `D2ClientStrc::pPlayer` | `+0x174` (validated by `eUnitType == PLAYER` before use) | struct shape at `0x6fc34ff4` |
| goldbank / INVPAGE_STASH | `15` / `4` | immediates + page gate |
| SerializeItem | D2Common **10665**, 6-arg (ret 0x18) — **4th arg is the statlist sentinel `0x1`, isSave = 1** | our SP call + call sites |
| STATSEX_GetCurrentStat | D2Common **10518** | 57 call sites |
| ITEM_GetContainingInventoryPage | D2Common **10445** (page returned in AL) | item-type assert |
| INVENTORY_GetFirstItem / GetNextItem | **11040** (validates magic `0x01020304`) / **10897** | byte-verified |
| INVENTORY remove (unlink) | **10903** `(pInventory, pItem)`, ret 8; slot search `0x6fd89940`; `[unit]==4`; engine mode 1 | body traced |
| Item-flag getter / setter | **10120** (ret 4) / **10876** (ret 0xC) | item-type assert |
| GetItemComponent | **10523** (feeds the 0x9C/0x9D category byte) | confirmed by its consumer |
| ReadInfoFromStreamVersioned | D2Common **named export** — `GetProcAddress(hCommon, "ITEMSReadInfoFromStreamVersioned")` | D2Game imports it by name (IAT rva 0xf809c) |
| ITEM_CreateFromSaveData | D2Game RVA **0xE2B70** (8-arg fastcall, ret 0x18) | stock loader call order |
| PLRSAVE_RestoreItemToUnit | D2Game RVA **0x6FC70** | called immediately after create |
| Item stream format | **0x60** (1.13d accepts 0x47 legacy and 0x48..0x60 modern; both are 'JM' containers) | dispatcher `0x6fc5a990` |
| SCMD_QueuePacketForClient | D2Game RVA **0x0DB780** — pClient in **EAX**, `[esp+4]`=size, `[esp+8]`=buf, ret 8 | 286 call sites |
| SFile::GetAndCreateSavePath | Fog **10115** (fastcall: ECX=buf, EDX=0x400) | exactly 2 callers, both `%s%s.d2s` builders |
| 0x9C packet | 8-byte header: `+0` id, `+1` action/page, `+2` TOTAL size, `+3` category, `+4` GUID, `+8` bitstream | our own `D2GSPacketSrv0x9C` |
| 0x9D removal builder | D2Game RVA **0xDCDA0** (client in ECX, action in AL) | emission only |
| `PlayerTrade::ClickButton` | D2Game RVA **0x9b8210** | call site |
| 0x4F handler (patched) | `0x6fc8cf60`; dispatch table at file `0xfa698`, entry `0x4f` → this address | table read |

- [`dh`/`R234`] **The frame is reachable — field-proven.** Trace `DHX34FD3002802900>0230FD34<4F7D0000000100R`: `*(uint32_t*)pPacket` targets the real `4F 7D 00 00 00 01 00` frame (button `0x7D`, page 1).
- **The receive shape is a dispatch struct, not the frame.** `pGame` as forwarded equals `*(uint32_t*)pPacket` (the packet pointer), and `pUnit` reads `type=146955615`. So `(ECX, EDX)` at `0x6fc8cf6f` are **not** `(pGame, pUnit)` (R235: dispatcher pushes `ebx`, sets `ecx=ebp, edx=esi`).

### 3.6 CURRENT BLOCKER — argument mapping (dispatch is settled)
Latest field trace: **`VPLOG.TXT = DHX34FD3002802900>0230FD34<4F7D0000000100R`** — `D` load, `H` entered, `X` (p[0] != 0x4F), struct bytes `34 FD 30 02 80 29 00`, `>` candidate `0x0230FD34`, `<` target `4F 7D 00 00 00 01 00`, `R` adopted.
- The one-dereference theory is correct; the remaining unknown is **which dwords of that struct are `pGame`/`pUnit`**.
- The Fog fault (`0x6FF5DF6F`, from `D2COMMON+0x4704E`) follows the refused click and is replayed into stock. Our function's REFUSED path performs **only reads and file writes**; no game-state write precedes the `return 0`. That is audited, not assumed.

### 3.7 NEXT EXPERIMENT (R235 — built, not yet field-run)
- On `p[0] != 0x4F` the trace also carries: `#` + 48 nibbles = **the first 24 bytes (six dwords) of the dispatch struct**; `[` + 16 nibbles = **ECX, EDX as the stub forwarded them**; `T` + 48 nibbles = **24 bytes at the adopted frame**. The `REFUSED` line in `d2vpstash.log` logs `pPacket` plus all six dwords.
- Expected trace shape: `DHX34FD3002802900>0230FD34#<6 dwords>[<ecx><edx>]<4F7D0000000100T<6 dwords>R`.
- **Read-out:** identify the dword equal to a validated `UNIT_PLAYER` (or to `0x025274B5`, the current EDX) — that is the real unit; the dword holding a pointer that validates as `pGame` is the other.
- **Crash isolation:** build with `VPSTASH_113D_PASSTHROUGH` (logs `H`, touches nothing, returns 0). If stock then handles the click cleanly, our function is implicated; if it still crashes in Fog, the fault is the stub's replay/return path. Run passthrough first.

### 3.8 Remaining for a working realm stash
1. **Map the dispatch struct** to `pGame`/`pUnit` (§3.7) and read them from the struct.
2. Settle the Fog crash: passthrough run, then stub replay/return-path check if needed.
3. **`Clients::GetAccountName` (#10)** — the pool key is the literal `"smoketest"`; one-line swap once pinned.
4. **`ChangeGoldForPlayer` RVA** — restores the shared-gold pool (v1 skips gold).
5. Then unset `VPSTASH_113D_NO_0X9C` for the full build with client repaint.

## 4. Gamble Refresh button (R237–R241) — WORKING, gated

A Refresh button in the gamble panel's blank 3rd slot for all six gambling vendors — Gheed (0x93), Elzix (0xc7), Alkor (0xfe), Jamella (0x195) and Larzuk (0x200/0x202); the vendor list is the single predicate `INV_IsGambleRefreshNpc` (`Inv.cpp`). Click → C→S `0x38` **action 2** with `D2VP_GAMBLE_REFRESH_PARAM` (`0x5246`, `d2_hand.h`) in the params dword retail leaves 0. `SUNITNPC_GambleRefresh` (`SUNITNPC.cpp`) queues the old items as shop-removes, flushes, drops the proxy node, regenerates, queues the new items as shop-adds, flushes, drains.

**Gate (R241): `[features] gamble_refresh`, default 0 = retail, Options row 18 ("Gamble Refresh") — it fills col 1 band 8, so the page is now two FULL columns of nine and no geometry constant moved.** Both ends poll the same file on the same 2 s cadence: `GAMBLEUI_FeatureActive` (`VanillaPlusCore.cpp`, flushed by `D2VP_ConfigReloaded`) decides whether the button is built; `SUNITNPC_GambleRefreshEnabled` (`SUNITNPC.cpp`) re-polls and refuses the re-roll when unset, so a stock ini cannot reach it even if a packet arrives. On a realm the D2GS needs the key in its own `d2vp.ini`.

Three silent bugs the field runs exposed, all fixed: the click branch keyed on the shop-mode global fell through to the retail Repair All path (a no-op for a vendor) — now keyed on the NPC class; `SUNITPROXY_SendProxyItemsToClient`'s parameter names are shifted by one (retail passes `(pGame, pNpc, pPlayer, pClient)` from `Item.cpp:592`) and the reversed pair makes `Pre0x9DAnd0x9CPacket`'s `pNpc != pUnit` check drop every packet; and gamble generation uses `SERVER_ItemToInventory(bSendUpdate = 0)`, so the adds need explicit `UNITFLAGEX_ISVENDORITEM` + `UpsertToUpdateList` (the `'8'` trigger, mirroring the removals' `UNITFLAGEX_ITEMINIT` = `'9'`). `ITEMCMD_DELETE` frees nothing and `FreeServerUnit` sends no packet — the client learns only through the 0x9C walk, so anything freed before it is a silent ghost.

## 5. Act 4/5 start crash (R240) — FIXED, awaiting field

`-act4`/`-act5` characters faulted on frame 1 (`0xC0000005`, read of `0x14`) in `DrawFloorTile` ← `DRAW_DrawEdgeFloorTile` ← `DrawFloorByViewAndFloor`. `sTileCache` is left all-zero for acts 3/4 (`InitCache`, `Drlg.cpp:971`, fills only acts 0-2), so `pTileLibraryEntry` is NULL. Retail never reads that cache — its only consumer is the fork's wide-stop edge-void fill. `DrawFloorTile` now returns early on a NULL entry (`Draw.cpp:752`). Full writeup: **HISTORY.md H84**.

## 6. Standing watches (carried from H83)

- `roster heal:`; `s2c dispatch guard:`; repeating `s2c pump: drained`; hangwatch 12 s dumps (keep `d2exe.map` beside the exe); `d2boot.log` line 1 must read the current self-ID.
- **Realm failures: always revert the patched stock `D2Game.dll` / `D2Common.dll` from `D2VP-D2GS/gamefiles/` first**, then collect evidence.

## 7. Field checklist — owed

- **Realm run #1** (state-only, `VPSTASH_113D_NO_0X9C`): click the Shared tab, collect `VPLOG.TXT`, `vpstash-beacon.txt`, `d2vpstash.log`, the `D2GS-Hack.txt` diff and any `_d2vp_shared_*.sss`. **Run #2** (full build, client repaint) only after #1 shows `S`/`M`.
- Carried unchanged from H83: SMOKE §M2–M4 gamble/purchase chains, §W 92–96 extended stats, §V 81–87 camera zoom, §U-80 storm pass, §X 97–99, the R116 `-w` 1366x768 eyeball, and triage for issues **#25** (Refilling Shrine XP) and **#18** (attribute-point crash).
- **R240**: start a `-act4` and an `-act5` character; both must reach town.

## 8. Conventions + hard-won method lessons (realm work)

- Conventions unchanged from H83 §8 (docs lanes, `VP-BEGIN`/`VP-END` markers, LF endings, self-ID bump per code round, the 16 KB handoff cap with verbatim offload to HISTORY).
- **An export NAME does not define the ABI** — disassemble the epilogue (`ret N`) before writing a caller.
- **A parameter NAME in a generated header does not define the argument order** — check a retail caller first (R237 lost a round to `SUNITPROXY_SendProxyItemsToClient`'s shifted names).
- **Symbolicate with the owner's own `d2exe.map`** (base `0x400000`: map address = `0x400000` + RVA) — the reporter's "own-code fault" frames are useless without it, and it turned R240 from a guess into a one-line fix.
- **Ordinal scans cannot see named imports; check which DLL you are scanning; require a consumer/assert before binding a number; restore every argument the working SP call passes when adapting it.**
- **Re-verify `#if` gating after every edit** — helpers drifting below their gate and a define used before definition both broke the ordinary build.
- **Never trust an RVA or pointer by inspection** — byte-assert the expected instructions at the address before patching or reporting it.
- **Never trust an RVA or pointer by inspection** — byte-assert the expected instructions at the address before patching or reporting it.
- **Symbolicate with the owner's own `d2exe.map`** (image base 0x400000: map address = 0x400000 + RVA) — the crash reporter's "own-code fault" frames are useless without it, and the map is what turned R240 from a guess into a one-line fix.

## 9. Known-issue manifest (unchanged from H83)

Open: row 12 (**#25** Refilling Shrine XP buff — track R) and row 13 (**#18** attribute-point crash — track Q). All other rows were closed in their H-records.

## 10. History

**H83** holds the rev-185 handoff verbatim (R168-era). Round-by-round detail for the stash work lives in `Redivivus/d2_boot.h` notes (R204–R234), `docs/BNET-CONNECT-FINDINGS.md`, and the agent memory `shared-stash-online-r204.md`.
