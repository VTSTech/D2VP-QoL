# R2 Game.exe ↔ D2VP Gap Analysis (R170)

**Date:** 2026-11-20
**Question:** what gaps remain between Game.exe's Battle.net-path behavior and the D2VP
(`D2VP-QoL/Source`) implementation that we need to close?
**Method (phase 12):** (a) map-vs-cpp scan of all 6 Battle.net-path modules
(`scripts/r2_phase12_gapscan.py`), (b) address-coverage diff of 303 Game.exe call targets
in the BNet ranges `0x435000–0x44c200` and `0x518000–0x52c000` against every D2VP `.map`,
(c) body-level semantic comparison of 25+ core functions against the Game.exe
disassembly from R170, (d) call-wiring trace from UI to sockets.

## 1. Verdict

**The Battle.net path (connect → auth → charlist → game create/join) in D2VP is
functionally complete and faithful to Game.exe. No behavioral gaps were found on this
path.** What this round produced instead:

1. **R170 contained five wrong conclusions**, now corrected against both the raw
   disassembly and D2VP's implementations (§3). The worst was stage 4.
2. Three **misleading D2VP function names** worth renaming (§4).
3. A short list of **confirmed-faithful divergences** — no action (§5).
4. R170's open items §6 are mostly now **resolved** by D2VP's own naming (§6).

## 2. Coverage evidence

| Check | Result |
|---|---|
| Map functions vs cpp bodies (Bnclient 372, D2MCPClient 96, D2Multi 257, D2Launch 318, D2Net 1, D2Client 302) | **0 missing bodies** in all six modules (only trivial `operator=` 0x517e40 in D2Client) |
| Game.exe call targets in BNet ranges vs D2VP map addresses | **303/303 covered** (within 0x10 bytes) |
| Tiny/constant-return functions (102+42+37+74+1+98 across modules) | All inspected ones are genuine getters/setters/delegators — **none are gutted logic** |
| assert-line preservation | D2VP reproduces Game.exe's exact assert lines: MCP `0x7e`/`0x9a`/`0xc1`, charlist `0x21e/0x227/0x233–0x236/0x242–0x245/0x254` |
| Timers | auth step-wait timeout 45 000 ms ✓; charlist cache expiry `dwTick + 300000` ✓ (5 min) |
| Framing | BNCS `FF OP LEN16` (len incl. 4-byte header, 0x2000 cap) ✓; MCP request `[cmd][dword][word][payload]` with `len+7`/`len+1`/`len+10` ✓ |
| Wire details | keepalive = SID 0x02 empty (Game.exe `0x51b1b0` identical) ✓ |
| Error tables | LogonResponse2 → states 0xd/0xe/0xf/0x1b(str)/0x12 ✓; joingame errors 0x71–0x79 → string ids 0x2770–0x2776 ✓; charcreate 0x14 exists-branch + `"Error from MCP trying to create '%s' (err=%d)"` + ids 0x145e/0x145f ✓ |
| Registry | SRegSaveValue "Configuration" × {Registration Version, Registration Authority, Client ID, Client Token} ✓ |
| Dispatch tables | SID incoming table = 94 entries ✓ (matches R170's 94-slot finding); MCP charlist validation for cmd 0x17/0x19 incl. word<0x100 checks ✓ |

**Wiring (all connected):**
UI → `gpBNetCallbacks.pfnConnectToBattleNet` = `ConnectingToBattleNet` (registered at
`Bnclient.cpp:72`) → thread `BattleConnectFastest` → `NET_SID_CLIENT_LogonSequence`
(sFastestConnect, probe + protocol byte + `Send_0x50_AuthInfo` + `Send_0x51_AuthCheck` +
`WaitForPacketId(0x51)`) → realm via `MAINMENU_ConnectToRealm`
(`pfnSendLogonRealmEx` → `MCPCONNECT_CreateThread` → `McpConnectionLoop`) → charcreate
(`NET_MCP_CLIENT_HandleCharCreation` → `MCPCONNECT_SendCharCreate` = send 0x02) →
join handoff: `JOINID`=D2GSGameToken, `COMINT`=D2GSGameHash,
`szSERVERIP`=D2GSConnectIp → `pfnSendNotifyJoin` into the transport layer (in-game
D2GS protocol lives under D2Client's `D2GSPacketSrv*`/`D2GSPacketClt*`, already present).

## 3. Corrections to R170 (this round, verified against disasm + D2VP bodies)

R170 stage 4 and several labels were **wrong** and must not be used for recreation:

| R170 claimed | Correction (verified) |
|---|---|
| `0x44c000/0x44c060/0x462d00` = game-server enter/leave/transition | They are the **click-processing state machine**: `0x44c000 ProcessQueuedPrimaryClick`, `0x44c060 ProcessPrimaryClickImmediate`, `0x462d00 ClickMap(clickType, x, y, flags)`. My "flags bit8→0x40 / bit4→0x20" reading was ClickMap's stamina/mouse-event flags; "class 1/2" was clickType 1/2. R170 stage 4 (incl. the "token pair handshake") is void. |
| `0x468730/0x468740` = client-token pair getters | `GetMouseX` / `GetMouseY` (globals `0x7a6ab0/0x7a6aac` = cursor coords, not tokens). The real client/server token storage is `g_nBNetClientToken` / `g_nBNetServerToken` (`0x51a560`/`0x51a540`). |
| `0x51a540` = generic "ack payload store" | `BNSEND_SetServerToken` — stores the server token from SID_LOGONCHALLENGE (0x28) / LOGONCHALLENGEX (0x1D) for password hashing. |
| `0x51c0e0` = bnserver-D2DV.ini parser, `0x40d890` = file-exists check | `0x51c0e0 = GetDnsRecordSync` — async DNS resolution of the gateway hostname into the server list (thread proc 0x51bee0 + message pump is the WSAAsync-style lookup). `0x40d890 = Safesock StartUp` (Winsock init). The "bnserver ini" framing in R170/R170 stage 1 is wrong for 1.14d: the server list comes from DNS, gated on Winsock startup. |
| `0x881bf0/0x881bf4/0x881bfc` events, `0x881c00` mutex | They are **interlocked flags/counter** (`[0x6cc3a0]` = InterlockedCompareExchange — the 3-arg push pattern matches): `0x881bf0` = connected (`g_pBNetSocketData`), `0x881bfc` = protocol-ready, `0x881c00` = connect-in-progress counter (InterlockedIncrement guard). |
| `0x520ad0` = BNCS disconnect/teardown | `0x520ad0` = set protocol-dead flag `[0x882528] = 1` (checked by the step-wait to fail with 0x12). D2VP calls it `SID_SetReadyFlag` (name misleading — see §4). |
| SID opcodes cited as "BD auth" 0x1e/0x50/0x51 | Correct names: 0x1E = SID_CLIENTID2, 0x50 = SID_AUTH_INFO, 0x51 = SID_AUTH_CHECK; the recv-side handler `0x520cb0` = `NET_SID_CLIENT_Incoming_ClientId` (registry persistence confirmed exactly as R170 described). |

Everything else in R170 (framing, timers, cache, error tables, abort codes, status
dispatcher 0x520dc0 = LogonResponse2, vercheck 0x520e50 = ReportVersion) **was
confirmed correct** by the D2VP implementations.

## 4. D2VP naming fixes recommended (cosmetic, no behavior change)

1. `NET_SID_CLIENT_LogonSequence` (0x51ca90, `Bnclient/BnSend.cpp:1704`) — its own body
   begins `DEBUG_Trace("sFastestConnect()")`; it is the fastest-server probe + connect
   sequence, not the logon sequence. Rename to `NET_SID_CLIENT_ConnectFastest`.
2. `SID_SetReadyFlag` (0x520ad0, `net_sid.cpp:287`) — sets the protocol-dead/abort flag
   (`g_bSIDProtocolReady`, Game.exe `0x882528`), checked to fail waits with state 0x12.
   Rename to `SID_SetProtocolDeadFlag` (or `Abort`); "Ready" inverts the meaning.
3. `0x51d0a0 BattleConnectFastest` — fine; but note it uses an interlocked counter guard,
   not a mutex, in both binaries.

## 5. Divergences checked, faithful — no action

- D2VP models Game.exe's events/mutexes as interlocked flags (§3 row 5) — same
  semantics, better for a portable codebase.
- `BNCS_SendPacket` failure path: Game.exe `0x51c5c0` → `0x520ad0` (set dead flag) →
  return 0; D2VP `NET_SID_CLIENT_Send` → `SID_SetReadyFlag()` → return 0. Identical.
- D2Multi/McpConnect.cpp (57 lines) is only the `GetMCPThread` getter (0x449ac0) — not a
  duplicate of D2MCPClient/McpConnect.cpp.
- VP-annotated divergences in BNet-path bodies (e.g. `VP-BEGIN [camera-zoom R155]` in
  ClickMap) are intentional QoL hooks and are cleanly fenced.

## 6. R170 §6 open items — status after this round

| R170 open item | Status |
|---|---|
| Pair all 47 `0x51c5c0` callers with BNCS opcodes | **Resolved by D2VP naming** — BnSend.cpp senders: 0x1E ClientId2, 0x50 AuthInfo, 0x51 AuthCheck, 0x28/0x29 Logon, 0x3A/0x3D LogonResponse2/NLS, 0x07 ReportVersion, 0x06 StartVersioning, 0x0B/0x0C channel, 0x0E chat, 0x10 leavechat, 0x15 gamelist, 0x1F leavegame, 0x40/0x41 extra/ad work… |
| Map `0x44c000–0x44efe0` reply processor | **Reinterpreted** — that range is the click/Game-view state machine (§3); the create/join reply codes live in McpConnect/OOGUtilities (verified: 0x71–0x79 handled with string ids). |
| BD engine object `0x88273c` internals | Effectively resolved: it is the SID protocol state (94-slot tables + config), not a separate engine; no recreation work left. |
| Registry value types for wrappers | Resolved: `SRegSaveValue(..., 2, ...)` (REG-style type 2 = string slot per Storm SReg) and `SRegSaveData("Patch","Patches",0x82,...)`; behavior reproduced faithfully. |
| Other D2CS request senders | Resolved: 0x01 StartUp, 0x02 CharCreate, 0x03 CreateGame, 0x04 JoinGame, 0x07 CharLogon, 0x16 CharRank (all in D2MCPClient/McpConnect.cpp with exact framing). |

## 7. What to actually work on next (priority order)

Since the BNet path is closed, the remaining recreation risk is **outside** it:

1. **In-game D2GS protocol depth** (D2Client `D2GSPacketSrv*`/`Clt*`) — the join handoff
   ends at `pfnSendNotifyJoin`; the same body-level gap scan should be run over
   D2Client/GAME and D2Net to certify that layer the way this round certified Bnclient.
2. **Storm/Fog transport layer** (`Safesock`, `D2QSSocket`) — every BNet/MCP sender
   bottoms out there; one round of body checks against the Storm imports would close the
   last unverified leg.
3. Apply the §4 renames (3 functions) so future readers don't inherit inverted names.
4. Re-run `scripts/r2_phase12_gapscan.py` after each new decompilation drop to keep the
   missing-body count at zero (it is the cheap regression test).

---
*Companion: `R2_Game.exe_BNetPath_Function_Reference_R170.md` (now corrected — see its header banner), `R2_Game.exe_BNetPath_Functions_R170.csv`, scripts
`scripts/r2_phase12_gapscan.py` / `r2_phase12_reconcile.py`.*
