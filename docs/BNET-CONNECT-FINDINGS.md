# BNET-CONNECT-FINDINGS — R169 audit of the Battle.net connect path

## R174: the abort is deterministic — autopsy probe + auth-stage reconnect

**What the R173 field run added.** The R173 build's one-shot recovery fired exactly as
designed and answered the open question: the abort is **not transient**. The retry recv
failed with the *same* 10053 in the *same* millisecond
(`recv aborted (WSA 10053) but socket still open - retrying once` → `recv failed (-1) -
WSA err 10053`), so a poisoned connection cannot recover in place; and the R173 teardown
telemetry showed no free in the fatal window (the only mark is the post-failure
"auth-stage teardown", after the fact). The same run also confirmed the pre-death wire
flow is byte-correct: `FF 25 08 00 05 63 4C 00` is PvPGN's SERVER_ECHOREQ (server uptime
ticks), our 0x25 echo (`SID send op=0x25 len=4 -> 8`) is the correct BNCS ping reply, and
the "0x50 before 0x25" ordering is D2's real flow — D2VP sends AUTH_INFO immediately
after the protocol byte and PvPGN accepts it before echoing.

**Code audit closure (R174).** Every remaining in-process suspect was eliminated by
direct reading: `WSASocket2_Read` captures `WSAGetLastError` immediately after `recv`
under `pSync`, so 10053 is genuinely the recv's own error on a blocking socket; the read
thread is provably single-instance (`Destination_00881bf4` refcount, decremented on
exit); `NET_SID_CLIENT_WaitForPacketId` is a pure flag/Sleep(10) poll — no second
reader; `WSASocket2_SendOrCloseOnTimeout` only closes on WSAETIMEDOUT; no shutdown/flush
call touches the socket in the window. Conclusion stands: the abort comes from outside
our code — local stack/WFP/AV/LSP class — consistent with retail Game.exe connecting
fine on the same box and with the pcap's healthy-wire evidence.

**R174 changes (all VP-marked; MSYS2 llvm-mingw build is the compile gate):**

- `Safesock.cpp/.h` — new `SAFESOCK_LogRecvAutopsy()`: one-shot forensic probe of the
  poisoned handle, run by the read thread before the R173 retry. Logs peer/local
  addresses, `SO_ERROR`, `SO_TYPE`, a zero-timeout `select` on read/except, a
  `MSG_PEEK` recv and a zero-length send probe. Interpretation: send probe fails too →
  the local TCP TCB is aborted (stack/firewall class); send OK but recv/peek still
  10053 → receive-only poisoning (LSP/filter class); select readable + recv failing →
  readiness and receive paths disagree (layer-conflict class). `SO_TYPE`≠SOCK_STREAM
  would finally prove handle reuse.
- `BnSend.cpp` — **bn-auth-reconnect**: the wait-0x50 failure path now gets one
  budget-guarded reconnect (`s_nBncsReconnectBudget`, re-armed in
  `ConnectingToBattleNet`): free the dead socket (`Safesock::Free` waits for the read
  thread, so the refcount is re-armable), clear `g_pBNetSocketData`, reset SID protocol
  state (`SID_InitializeProtocolState` clears the dead flag), release
  `BattleConnectFastest`'s `Destination_00881c00` single-instance guard, recursively
  re-enter `BattleConnectFastest()` on the same thread (fresh DNS resolve, fresh race,
  replay of 0x01/0x50/0x51), re-take the guard so the outer frame's tail decrement
  stays balanced. Recursion is hard-capped at 1 by the budget: if the fresh connection
  dies the same way, its own failure path finds the budget spent and tears down
  normally.
- `Redivivus/d2_boot.h` — `D2VP_ROUND` → `174` + round note.

**Field-verification checklist for the next run:** expect
`recv aborted (WSA 10053) ... - retrying once` preceded by the
`BNet: autopsy recvErr=10053 h=0x... peer=... soerr=... sel=... peek=... send0=...`
line, then `recv failed ... protocol dead`, then
`auth-stage reconnect -> freeing dead socket, replaying handshake on a fresh
connection` followed by a second `connection state -> 2` race. Send us the whole log:
the autopsy line decides the suspect class; either the reconnect completes the login
(workaround holds) or it dies again at the same byte (then the failure is per-process,
not per-connection — AV/LSP becomes the prime suspect and the next lever is a
retail-vs-D2VP socket API trace on the same box).

## R173: pcap forensics vs PvPGN 1.99.8 — the 10053 disconnect

**Symptom.** D2VP reaches "SID 0x50 AUTH_INFO stage" against the D2GS/PvPGN stack
(VTSTech/D2VP-D2GS, bnetd on 6112), receives `FF 25 08 00 ...`, echoes it, then
`recv failed (-1) - WSA err 10053 -> protocol dead`, wait 0x50 aborts, teardown frees the
socket, the game exits the mode loop. Retail Game.exe reaches the login screen on the same
server, same machine, same path.

**What the 2026-11-20 packet capture proved (client 192.168.0.12 → server 192.168.0.14:6112):**

- `FF 25` is PvPGN's `SERVER_ECHOREQ` (`bnet_protocol.h:3444`), payload = server uptime
  ticks; it is sent by `_client_auth_info` **only after** the client's 0x50 passed the size,
  langstr, countryname and client-tag whitelist checks — so PvPGN fully accepted D2VP's
  AUTH_INFO (D2VP's 0x50 is byte-identical to retail's: `IX86`/`D2XP`, versionid 14,
  "CAN"/"Canada").
- The server then queued and flushed **AUTHREQ_109 (103 B) to D2VP exactly like retail**.
- Retail's next action was the 0x38 AUTH_CHECK 120 ms later. D2VP instead **sent RST itself**
  0.75 ms after the AUTHREQ went out, without ever reading it — the RST is the abortive
  close from `Safesock::Free` in the auth-stage teardown path (`BnSend.cpp`, "auth-stage
  teardown freeing socket"), closing a socket with the reply unread. The `recv` 10053 came
  **first** and is the root cause: a WSAECONNABORTED on a connection the capture shows was
  still healthy.
- Static audit of every close path (`Safesock::Free` callers: BNSEND_Disconnect, race
  except-set cleanup, post-race non-winner cleanup, auth-stage teardown; D2QS/BnDownload/
  QServer own their sockets) found **no closer active in that window** — the 10053 is a
  genuine local abort (suspects: AV/LSP touching the modded client's sockets, or an
  unidentified in-process close). pvpgn-side restarts were also ruled in/out via docker
  inspect; bnetd had restarted earlier but the pcap shows no server-side FIN/RST before the
  client RST.

**R173 changes (all VP-marked, hand-checked; MSYS2 llvm-mingw build is the compile gate):**

- `Safesock.cpp`/`.h` — new `SAFESOCK_IsSocketOpen()`: CS-free `getpeername` handle probe.
- `BnSend.cpp` `BnetReadFunction` — one-shot recovery: a `recv` failure with WSA
  10053 while the handle is still open waits 5 ms, re-polls and lets the next recv pick the
  buffered reply up instead of declaring the protocol dead; the retry re-arms after every
  healthy recv, and any second failure or closed handle still tears down as before.
- `BnSend.cpp` — teardown telemetry marks at all four `Safesock::Free` owner sites
  (BNSEND_Disconnect, race except-set, post-race non-winner, auth-stage wait-failure), so
  the next d2boot.log names whoever closes the socket.

**Field-verification checklist for the next run:** expect either "recv aborted (WSA 10053)
but socket still open - retrying once" followed by the normal 0x50 reply handling (bug
worked around, root cause still worth chasing), or a teardown mark naming the closer.


Round R169 (rev 182) fixed the Battle.net title-screen click crash plus three riders
on the same connect path. This file records the connect-path findings that were
**triaged but deliberately NOT fixed in R169** — each needs its own round and its own
field verification. All of them are live defects in the rebuild (found by a full audit
of the click→connect chain: MainMenus/Launcher UI, BnSend, net_sid, SRP, BNGatewayAccess,
BnDownload, Cache, BnMessQueue, Storm SReg), ranked by risk.

## Fixed in R169 (for the record)

- **bn-srp-context** (`Source/Bnclient/net_sid.cpp`, `NET_SID_CLIENT_InitializeSRP`) —
  THE crash. `D2Net::SRP::SRP_GlobalInit` (the only caller of `SRP_CreateContext`) has no
  caller anywhere in the rebuild, so `g_pSrpContext` was NULL on the first click and
  `SRP_SetParameters`'s `(*ppSrp)->bInitialized` read dereferenced it synchronously inside
  `ConnectingToBattleNet`. Fixed by create-on-demand; VP-marked.
- **bn-read-offset** (`Source/Bnclient/BnSend.cpp`, `BnetReadFunction`) — the buffer
  position was a `char*` used where `NET_SID_CLIENT_IncomingPacketHandlerLoop`'s
  contract wants a byte offset into `chRecvBuf`; after the first dispatched packet the
  next `recv` aimed at a wild address and the read thread died — no Battle.net connection
  could ever complete. Fixed: offset = returned pointer − buffer base; VP-marked.
- **bn-authcheck-bound** (`Source/Bnclient/net_sid.cpp`, `NET_SID_CLIENT_Send_0x51_AuthCheck`)
  — the expansion branch computed the CD-key owner copy bound from the 256-byte stack local
  `szExeInfoClassic` instead of the packet's `szTail + 0x200` end → unbounded copy into the
  0x25c packet. Classic branch's `szTail + 0x1dc` is retail and untouched. VP-marked.
- **bn-nth-failstr** (`Source/Bnclient/BNGatewayAccess.cpp`, `Nth`) — every failure path
  returned the gateway struct pointer reinterpreted as a string (Ghidra reused `this` as
  the return slot); raw struct bytes flowed into `gethostbyname` via `GetBnetIp` whenever
  the registry gateway list was missing/truncated. Now returns an empty string. VP-marked.

## Open findings (not fixed, ranked)

1. **`MAINMENU_UpdateProgressBar` — deref before null check** (`Source/D2Launch/MainMenus.cpp`
   ~4952). `gpPatchProgressTextBox` (`ClearAllLines`) and `gpCtlPatchProgressAnimImage`
   (`StopAnimation`) are used BEFORE the null guard a few lines later — check/deref order
   transposed. An AV inside the patch-download pump (`LAUNCHER_CheckRevisionAndShowDialog`)
   when either pointer is null. Fix: hoist the guard above both uses, guard the anim image
   too. Runs only after a successful BNCS connect, so R169's field pass will reach it.
2. **`BNDOWNLOAD_InitiatePatchDownload` — server-supplied size unbounded**
   (`Source/Bnclient/BnDownload.cpp` ~1263). `SMemCopy` of the patch list into a 256-byte
   global with no clamp → global-buffer overflow on the BNFTP patch-list download. Fix:
   reject/clamp nSize >= 256. Server-controlled input; matters for any server.
3. **`BNSEND_Disconnect` — timeout arithmetic mismatch** (`Source/Bnclient/BnSend.cpp`
   ~1465). `nAtomicVal += 100; Sleep(10); if (nAtomicVal > 0x3c) return 0;` — counter grows
   100/iter against a limit of 60: whenever the connect/DNS thread is still alive the
   disconnect gives up after one iteration instead of ~6 s. Fix: `Sleep(100); nAtomicVal++;`
   or compare against 600. Symptom class: spurious "disconnect failed" / button no-op on
   a fast re-click.
4. **`LAUNCHER_BnetConnectionPollCallback` — busy-check fallthrough**
   (`Source/D2Launch/MainMenus.cpp` ~9521). After `WaitForSingleObject` signals connect
   success it clears the loop flag but falls through into the busy check; ESC in the same
   frame tears down a just-established socket. Fix: `return 1` right after the signaled
   branch clears the loop flag.
5. **`GetDnsResults` — unbounded "ip;" append** (`Source/Bnclient/BnSend.cpp` ~1187).
   Each resolved address appends with no total bound into the 512-byte
   `EntireDnsRecordOfServers`; > ~32 A records overflow it. Retail-identical behavior, but
   trivially boundable. (A private server that resolves to many A records can trigger it.)
6. **`MAINMENU_ScrollListExUp/Down` — off-by-one over the form list**
   (`Source/D2Launch/MainMenus.cpp` ~5371/5401). `while (i <= FORMS_nListExExCount)` reads
   the one-past slot (stale/destroyed control pointer after `FORMS_DestroyLists`) and the
   helper derefs it — use-after-free read on the ToS/EULA screens, i.e. right after a
   successful connect. Fix: `i < FORMS_nListExExCount` + bound-check.
7. **`LAUNCHER_ClearPatchDownloadFlag`/`GetPatchDownloadFlag` alias the ToS cancel flag**
   (`Source/D2Launch/MainMenus.cpp` ~4970). Both alias `gbTermsOfServiceCancelled`; a ToS
   cancel in a previous session makes the next connect's patch wait read "cancelled"
   immediately and bounce to the main menu. Logic bug, no crash. Fix: dedicated flag.
8. **AuthInfo token naming** (`Source/Bnclient/net_sid.cpp` ~683-715) — verified CORRECT,
   documented to prevent a future "fix": the wire layout of SID_AUTH_INFO 0x50 is
   logon-type@0, server-token@4; the struct field names (`dwServerToken`@0) are misleading
   and the setters compensate (`SetServerToken` gets offset 4, `SetClientToken` gets offset 0
   = the logon type used by `BNCLIENT_SendLogonRequest`'s 0→0x3A / 1→0x53 switch). Do not
   "fix" the apparent swap.
9. **Minor**: `SRP_GenerateAccountCredentials` copies username/password into 32-byte
   credential fields with an unbounded `SStrCopy` (auth garbage, no AV, only with long
   credentials); `BnMessQueue` node alloc unchecked (OOM only); `CACHE_ReadData` alloc
   unchecked (OOM only); `WritingDefaultGatewayList` passes `&localArgSlot` as the out-size
   pointer (works via shadow copies, refactor hazard).

## Verification status

Sandbox: the per-TU compile check could NOT run here (no i686 Windows cross-toolchain in
this sandbox build; host g++ chokes on the Win32-only `d2_platform.h`). The owner's MSYS2
llvm-mingw build remains the compile gate of record — the four R169 edits are small,
self-contained, and hand-checked (types, namespaces, brace balance). The gateway blob
format was validated against `BNGatewayAccess::Load()`'s 3n+1 parse arithmetic with a
script (one-gateway and retail-shaped four-gateway blobs both parse; trailing double-NUL
assert holds).
