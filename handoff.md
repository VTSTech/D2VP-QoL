# main branch — handoff (rev 186, 2026-11-23 — **R204–R234: Vanilla+ shared stash on closed realms — server-side hook DLL, mid field bring-up**): the previous handoff (rev 185, R168/R173-era) is preserved verbatim in **HISTORY.md H83**. Everything below is the live state of the realm work.

## 1. Branch topology (unchanged from H83)

- **Repository** `D2VP-QoL`; `main` is the only integration branch; the upstream merge line is RETIRED (rev 152).
- Build self-ID **`Vanilla+0.0.9-R234`** (`Redivivus/d2_boot.h` `D2VP_VERSION_TAG` + `D2VP_ROUND`); the first `d2boot.log` line pins any shipped exe.
- **Compile gate of record**: the owner's MSYS2 MINGW32 (i686-w64-mingw32-gcc 16.2) build via `tools/build-local-win11.ps1` → `build\`. The Linux tree is where edits are made; they are synced to the Windows tree.

## 2. Field status

| Area | Status |
| --- | --- |
| SP + Open Battle.net + closed-realm login / charlist / creategame / join (R169→R203) | **WORKING — field-verified twice** (`docs/BNET-CONNECT-FINDINGS.md`; agent memory `bnet-connect-10053-investigation`) |
| Vanilla+ 17 opt-in features (SP) | **SHIPPED + FIELD-PROVEN** (H83 §2) |
| **Shared stash on closed realms (R204–R234)** | **IN FIELD BRING-UP — not yet working.** Our function is passive on reject paths; the 0x4F toggle reaches the handler but the buffer pointer is one indirection short. See §3. |
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

`cmake --build build --target d2vpstash` → `build/d2vpstash.dll` (the target links `d2_flags`; never restate flags — CMake de-duplicates repeated `-include`).

### 3.4 What is PROVEN in the field
- The DLL loads in the D2GS (module list, base `0x76F00000`); `DllMain` runs (beacon: `BEAC dll=C:\d2gs\ log=…`); relative-path writes from inside the DLL work (`VPLOG.TXT` starts with `D` at load).
- The stub executes and calls our export; the dispatcher was independently disassembled (`0x6fc873a0`: opcode byte → table `0x6fd1a698` → `call [edi*8+…]`, pGame=ECX, pUnit=EDX, buf=EBX).
- Trace alphabet in `VPLOG.TXT`: `D` load, `H` entered, `N` null argument, `X` not-0x4F, `S` serialize, `M` materialize, `>` candidate pointer, `<` dereferenced bytes, `R` adopted dereferenced frame.

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
| ~~INVENTORY_Create 10888~~ | **never bind for removal** (allocates + writes the magic) | body traced |
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
| 0x4F handler (patched) | `0x6fc8cf60`; dispatch table at file `0xfa698`, entry `0x4f` → this address, and it is the only entry referencing it | table read |

### 3.6 CURRENT BLOCKER — one indirection short
Latest field trace: **`VPLOG.TXT = DHX34FD3002802900`**.
- `D` load, `H` entered, `X` (p[0] != 0x4F), bytes `34 FD 30 02 80 29 00`.
- The first dword, `0x0230FD34`, is a **stack address**, and the D2GS crash dump shows the real `4F 7D 00 00 00 01 00` frame **at exactly that address** → the pointer we receive points at a *pointer to* the frame.
- Dispatcher verified independently (`0x6fc873a0`), stub frame arithmetic verified — so the remaining unknown is a single dereference, not a dispatch or ABI problem.
- The Fog fault (`0x6FF5DF6F`, reached from `D2COMMON+0x4704E`) accompanies these clicks.

### 3.7 NEXT EXPERIMENT (R234 — built, not yet field-run)
On `p[0] != 0x4F`, `VPSHARED_HookPacket` now logs the raw 7 bytes, logs the dword at `pPacket` (`>` + 8 nibbles), dereferences it **only when it is a plausible user-mode pointer** (`0x10000..0x7FFF0000`), logs the target's 7 bytes (`<` + 14 nibbles), and **adopts that target as the frame** (`R`) when it begins with 0x4F.
Expected next trace: `DHX34FD3002802900>0230FD34<4F7D0000000100R…` — after which the stash logic should finally run (watch for `S`/`M` and for `d2vpstash.log`).
If the target is not 0x4F, the next ask is the agent's `ebx` value at the dispatcher call.

### 3.8 Remaining for a working realm stash
1. Resolve the indirection (§3.7).
2. **`Clients::GetAccountName` (#10)** — the pool key is currently the literal `"smoketest"`; a one-line swap once pinned.
3. **`ChangeGoldForPlayer` RVA** — restores the shared-gold pool (v1 deliberately skips gold).
4. Then unset `VPSTASH_113D_NO_0X9C` for the full build with client repaint (all four resync values are already pinned).

## 4. Standing watches (carried from H83)

- `roster heal:`; `s2c dispatch guard:`; repeating `s2c pump: drained`; hangwatch 12 s dumps (keep `d2exe.map` beside the exe); `d2boot.log` line 1 must read the current self-ID.
- **Realm failures: always revert the patched stock `D2Game.dll` / `D2Common.dll` from `D2VP-D2GS/gamefiles/` first**, then collect evidence.

## 5. Field checklist — owed

- **Realm run #1** (state-only, `VPSTASH_113D_NO_0X9C`): click the Shared tab and collect `VPLOG.TXT` (letters), `vpstash-beacon.txt`, `d2vpstash.log`, the `D2GS-Hack.txt` diff, and any `_d2vp_shared_*.sss`.
- **Realm run #2**: full build with client repaint, only after #1 shows `S`/`M`.
- Carried unchanged from H83: SMOKE §M2–M4 gamble/purchase chains, §W 92–96 extended stats, §V 81–87 camera zoom, §U-80 storm pass, §X 97–99, the R116 `-w` 1366x768 eyeball, and triage for issues **#25** (Refilling Shrine XP) and **#18** (attribute-point crash).

## 6. Conventions + hard-won method lessons (realm work)

- Conventions unchanged from H83 §8 (docs lanes, `VP-BEGIN`/`VP-END` markers, LF endings, self-ID bump per code round, the 16 KB handoff cap with verbatim offload to HISTORY).
- **Diagnostics must use raw Win32 file APIs, return the real write result, prefer relative filenames, take `hModule` from `DllMain` (a code address cast to `HMODULE` returns 0 under Wine), and drop a load-time token.** A diagnostic that cannot report failure is worse than none — two field rounds were lost to a logger that returned success after `CloseHandle` regardless of `WriteFile`.
- **An export NAME does not define the ABI** — disassemble the epilogue (`ret N`) before writing a caller.
- **Ordinal scans cannot see named imports; check which DLL you are scanning; require a consumer/assert before binding a number; restore every argument the working SP call passes when adapting it.**
- **Re-verify `#if` gating after every edit** — helpers drifting below their gate and a define used before definition both broke the ordinary build.
- **Before claiming a frame/offset error, account for the stub's own save area** (two of my offset calls were wrong; the agent's byte dumps were right).
- **Never trust an RVA or pointer by inspection** — byte-assert the expected instructions at the address before patching or reporting it.

## 7. Known-issue manifest (unchanged from H83)

Open: row 12 (**#25** Refilling Shrine XP buff — track R) and row 13 (**#18** attribute-point crash — track Q). All other rows were closed in their H-records.

## 8. History

**H83** holds the rev-185 handoff verbatim (R168-era). Round-by-round detail for the stash work lives in `Redivivus/d2_boot.h` notes (R204–R234), `docs/BNET-CONNECT-FINDINGS.md`, and the agent memory `shared-stash-online-r204.md`.
