# R2 D2GS Protocol Depth + Storm/Fog Transport Certification (R170, phase 14)

**Date:** 2026-11-20
**Binary:** `Game.exe` 1.14d retail, md5 `2b3600c10ee0d6387675a2f6cf0ebed2`.
**Closes §7 items 1–4 of `R2_Game.exe_vs_D2VP_Gap_Analysis_R170.md`.**

## 1. Renames applied (§4 → done)

- `NET_SID_CLIENT_LogonSequence` → **`NET_SID_CLIENT_ConnectFastest`** (0x51ca90) — 5 refs across `Bnclient/BnSend.{h,cpp}`.
- `SID_SetReadyFlag` → **`SID_SetProtocolDeadFlag`** (0x520ad0) — 3 refs across `Bnclient/net_sid.{h,cpp}` + `BnSend.cpp`.
- Map files renamed to match (`BnSend.cpp.map`, `net_sid.cpp.map`), so the regression scan stays at 0 missing. §4's third entry (`0x51d0a0 BattleConnectFastest`) was a no-change note, not a rename.

## 2. Regression scan re-run (§4 item 4 → done, and it earned its keep)

`scripts/r2_phase12_gapscan.py` immediately flagged the two renamed functions as missing
(the cpp no longer matched the old map names) — proof the test catches drift. After the
map update, baseline restored: **Bnclient 0, D2MCPClient 0, D2Multi 0, D2Launch 0,
D2Net 0 missing; D2Client 88 (the known boilerplate set, unchanged).**

## 3. In-game D2GS layer certified (§7 item 1 → done)

Method: for every tiny/stub/missing cpp body in D2Client + D2Net (842 functions), disassemble
the Game.exe function at the same address (disasm to `ret`) and compare sizes
(`scripts/r2_phase14_body_cert.py`, full table `R2/phase14_body_cert_results.txt`).

| Class | Count | Verdict |
|---|---|---|
| Game.exe body ≤ 10 insns (getters/thunks) | 674 | faithful-trivial |
| Tiny cpp body, large Game.exe body | 100 | **all faithful** — see breakdown below |
| Missing cpp body | 68 | container/UI/Warden boilerplate (re-implemented, standing parity check unchanged) |

The 100 "tiny cpp, big exe" rows resolve into four benign shapes (verified by reading the
bodies, `scripts/r2_phase14b_forwarders.py`):

1. **Pure computations** — coordinate bounds checks (`PANEL_IsMouseInMenuButton`,
   `INV_IsMouseOverAcceptTradeBtn`, `CheckSomeCoords`), fixed-point math
   (`SKILLS_ApplyDamageReduction`, `CalculateHypotenusePowered` = x²+y², `SOUND_CalculateAngle2`
   = min). 7-14 lines of C++ legitimately expand to 20-50 machine instructions.
2. **Inlined forwarders** — the D2GS dispatch entries forward to real named handlers:
   `Incoming0x77` → `UI::Trade::NET_D2GS_CLIENT_Incoming0x77_TRADEREQUEST` (full 0x10/0x0c/0x0d/0x15
   switch in `Trade.cpp:212`), `Incoming0x8F` → heartbeat handler, `PLRSKILLS_DrawChargeTrailIfPrimary`
   → `PLRSKILLS_CheckSkillWhipAndLeap`. Game.exe inlined the callee into the entry point; D2VP
   keeps the logic one call deeper, where it exists and is scanned.
3. **Inlined getter chains** — e.g. `NET_D2GS_CLIENT_Incoming0x52` body calls
   `INV_GetItemDescDirtyFlag` (3 lines): Game.exe inlined the whole chain; each link exists.
4. **External targets** — `SRegSaveValue`/`SRegLoadValue` are fully implemented in
   `3rdparty/Storm/Source/SReg.cpp` (with `@basis`/`@oracle` provenance comments; the scan
   corpus originally didn't include `3rdparty/` — false positive, fixed in interpretation),
   `sqrt` is libm, `GetInstructionPointer` is the Fog helper (0x408090), `D2BOOT_Mark` is
   D2VP's own boot-log facility (`[d2boot]` annotation), deliberately not in Game.exe.

**Verdict: zero gutted bodies in the D2GS layer.** Together with phase 13's 0% uncovered
call targets in `0x44c200–0x516000` and the fully-populated 175-entry
`NET_D2GS_CLIENT_INCOMING` table, the in-game D2GS protocol layer is certified to the same
standard as the Battle.net path.

## 4. Storm/Fog transport leg certified (§7 item 2 → done)

| File | Map fns | Missing | Tiny | Tiny verdicts |
|---|---|---|---|---|
| `Fog/Src/Safesock.cpp` | 21 | **0** | 5 | CS wrappers, `closeSocket`, blocking-create thunk |
| `Fog/Src/D2QSSocket.cpp` | 20 | **0** | 0 | — |
| `Fog/Src/QServer/QServer.cpp` | 76 | **0** | 13 | nullsubs, `QSERVER_LOG`, ref-count/thunk wrappers |
| `Fog/Src/D2QSQueue.cpp` | 5 | **0** | 0 | — |

Body checks on the paths every BNet/MCP sender bottoms out in:

- `WSASocket2_Send` — CS-locked, three branches (full send / partial / error) with
  `WSAEWOULDBLOCK` (0x2733) mapped to a retry-0 return and everything else to error 4; faithful.
- `WSASocket2_SendOrCloseOnTimeout` — `WSAETIMEDOUT` (0x274c) → closesocket with the exact
  log line "ERROR: Disconnecting socket due to WSAETIMEDOUT"; faithful.
- `Safesock_PollReadReady` — `select()` with 100 ms timeout, except-fd check, original
  assert line 0x22e preserved; faithful.
- `QSSOCKET_SendWithTimeout` — null/connected guards, CS around the safesock send,
  full-length equality return; faithful.
- `ReadBytesFromSocket` → `D2QSQUEUE_Dequeue`; `QSSOCKET_ThreadEntryPoint` /
  `NET_MCP_CLIENT_CreateConnectionThread` — real event+thread lifecycle; faithful.
- Layout guards present: `sizeof(D2NetDataStrc) == 0x78`, `sizeof(D2QSSocketStrc) == 0x194`
  static_asserts pin the 32-bit structures to the 1.14d allocations.

**Verdict: the transport leg is closed.** Every BNet/MCP sender now terminates in verified
bodies end to end: sender → `NET_SID_CLIENT_Send`/MCP framing → Safesock/QSSocket → Winsock.

## 5. Remaining open items (unchanged)

1. Container/UI/Warden boilerplate parity check (88 D2Client bodies re-implemented as
   D2VP classes) — still the only substantive verification left, unchanged since phase 13.
2. Warden client ctors (0x47fc30, 0x4801c0) only if realm handshake parity ever matters.
3. Keep `scripts/r2_phase12_gapscan.py` in the post-decompile checklist (see §2).

## 6. Regression tooling added

- `scripts/r2_phase14_body_cert.py` — tiny/stub/missing vs Game.exe body-size audit.
- `scripts/r2_phase14b_forwarders.py` — forwarder-target existence check.
- Results table: `R2/phase14_body_cert_results.txt`.
