# BNET-CONNECT-FINDINGS — R169 audit of the Battle.net connect path

## R176 field result: HARD ABORT confirmed — and SO_TYPE now fails where the R175 sample said type=1

**The rideout answered its question (checklist item 3).** Both attempts spent the full
budget: `retry 1/20` through `retry 20/20` at a steady ~15 ms cadence (`Sleep(10)` + the
failing recv), then `recv failed (-1) - WSA err 10053 -> protocol dead`. Attempt 1: first
abort 169347234 → dead 169347453 = **219 ms of continuous 10053, zero healing**. Attempt 2
replays it identically (169347515 → 169347734). No `recv RECOVERED` mark anywhere. The
"stall vs hard abort" question is closed forever: **hard, instant, deterministic abort** —
the filter class (whatever it is) does not ride out; it kills on first sight and keeps
killing for as long as the process keeps asking. The 200 ms workaround class (retry budgets)
is dead as a fix; the remaining levers are identifying/removing the aborting agent (checklist
items 1/2/4) or bypassing the data path it chokes on (item 6).

**Death-point precision (tid tags paying off).** All death-stage lines carry the read
thread's tid (16736 / 15644 per attempt) while the 0x50 wait and teardown lines carry the
auth thread (1456) — no misattribution possible. The sequence inside one millisecond:
`recv 8 bytes (FF 25 08 00 ...)` → `SID send op=0x25 len=4 -> 8` (echo) → recv →
`autopsy` → `retry 1/20`. So the abort lands on the **first recv after our echo** — the
recv that would have delivered the AUTHREQ the R175 pcap shows was already in the host
buffer (arrival preceded death by ~750 µs, our `win=0` RST followed 753 µs later). Also
recorded: attempt 2's autopsy handle (0x00000df0) equals attempt 1's — handle-value reuse
after `Free`, benign and expected, but now visible in the log instead of assumed.

**New contradiction between autopsy samples.** R175's two autopsies: `soerr=0`,
`type=1`, `sel=1(r1,e0)`, `peek=-1/10053`, `send0=-1/10053`. Today's two autopsies: same
except **`type=-1`** — the SO_TYPE getsockopt FAILED, microseconds after SO_ERROR on the
same handle succeeded and while select still reports the handle readable. SO_TYPE reads a
static property of the socket object; it cannot legitimately differ between sessions.
Possibilities: the aborting agent's interaction with the option layer varies run to run, or
the probe pair straddles the death instant differently per run (probe order is
SO_ERROR → SO_TYPE → select → peek → send). Either way the one-instant snapshot cannot
answer it — R177 turns the autopsy into a per-probe-error timeline and adds the two probes
that split the remaining classes cleanly (see below).

**R177 changes (all VP-marked; MSYS2 llvm-mingw build is the compile gate):**

- `Safesock.cpp` — **bn-sock-ops-trap**: every closesocket path in the process now logs
  handle + owning tid: the `closeSocket` wrapper (game-server/MPQ-download sockets),
  `Free` (the sanctioned BNCS closer — the R173/R174 call-site marks name WHO ordered the
  teardown, this line proves the handle's state at execution time), and
  `SendOrCloseOnTimeout` (the one close that does NOT wait for the read thread — a
  candidate for a racing close).
- `Safesock.cpp` — autopsy v3: every failing probe logs its own error code
  (`soerr=%d/%d type=%d/%d`), plus **`GetHandleInformation`** (`hvalid=%d/%d` — kernel-side
  handle validity: an in-process closesocket frees the kernel handle → `kvalid=0 err=6`)
  and **`FIONREAD`** (`fion=%lu/%d` — does AFD still queue the AUTHREQ?).
- `Safesock.cpp` — **bn-ctl-socket**: the differential. One-shot at autopsy time: a
  brand-new socket connects to the captured peer, sends the 0x01 protocol byte, waits
  1 s, MSG_PEEKs. **ctl alive** ⇒ fresh sockets from this process sail through the
  opening leg while the auth handle sits dead ⇒ the death is handle-scoped (in-process
  close/alias/poison) or conversation-triggered — the drift watch + close traps name it.
  **ctl dead** ⇒ the block is app-connection-wide at the opening leg ⇒ image-keyed filter,
  and the retail-vs-D2VP exe difference is the lever.
- `Safesock.cpp` — **bn-lsp-catalog**: one-shot `WSAEnumProtocolsA` census at the first
  `Create` (provider count, layered-provider count, up to 4 LSP names). A legacy LSP shim
  is exactly the provider class that reports 10053 from recv/send on an otherwise-open
  socket while select (which bypasses LSP data hooks) stays green; an empty LSP list
  pushes the WFP-callout/kernel class further up. Cross-checks `netsh winsock show
  catalog` for free.
- `BnSend.cpp` — **bn-drift-watch**: each retry iteration probes h-value + SO_TYPE +
  kernel validity and logs ONLY on change (`retry %d/20 drift h=0x%08x type=%d/%d
  kvalid=%d/%d`); state is function-scoped and reset per thread start so each attempt
  starts clean. The three verdicts it can deliver: `type` flipping back to `1` mid-budget
  = the handle was **recycled** by a new socket (aliasing bug in-process); `kvalid`
  flipping to `0` = an **in-process closesocket ran** (and the traps will show which one);
  all three stable for the whole budget = the handle is intact and the **provider layer**
  is refusing the data plane (WFP-callout class).
- `Redivivus/d2_boot.h` — `D2VP_ROUND` → 177.

**How to read the R177 log (the decision tree):**

1. `winsock catalog: N providers, M layered` — M > 0 with an AV/VPN name = prime suspect
   found before any test runs.
2. `ctl-socket connect=0/0 send=1/0 sel=...` (error slots `0`) = alive; any nonzero error
   slot = dead. Compare across both attempts.
3. Drift lines during the 20-retry budget, three shapes:
   - stable `type=-1/<err> kvalid=1/0` all budget → handle valid kernel-side but Winsock's
     option layer lost it → LSP/handle-table poisoning class (matches today's `type=-1`);
     the `typeErr` code (expect 10038 WSAENOTSOCK vs 10053) narrows it further;
   - `kvalid=0/6` at some retry → in-process close — the `SAFESOCK ... closesocket` trap
     lines name the culprit and tid;
   - `type=1/0` stays green while peek/send keep 10053 → WFP-callout class (matches
     R175's `type=1`) — go straight to checklist item 4 (`netsh wfp capture`).
4. Everything else (DLL diff, exclusion test, WFP capture, API Monitor) — unchanged from
   the R176 checklist below; the exclusion test is still the single fastest decisive
   experiment a user can run.

## R176: the sandbox half of the retail-vs-D2VP trace is done — filter class confirmed by elimination

**What the R175 field run proved (d2boot.log + `game-d2vp-bnet.pcapng` re-read).** The
R174 auth-stage reconnect fired and **died at the same byte on a brand-new socket**
(`race winner peer 192.168.0.14 port 6112` → `0x50` ok → `recv 8 bytes` → 0x25 echo ok →
`recv failed (-1) - WSA err 10053` → `auth-stage teardown freeing socket (wait failed)`),
so the condition is **per-process, not per-connection** — exactly the branch the R174
checklist named. Both attempts show the identical single-threaded pattern: recv#1 (the
8-byte `FF 25` echo-req) → inline echo send (`-> 8`) → recv#2 → 10053 → autopsy → retry →
dead. (One correction to this session's working notes: attempt 1's echo did NOT go out
late from a second thread — its `SID send op=0x25 -> 8` sits at the same millisecond
between `SID pkt received` and `autopsy`; there is exactly one 0x25 sender,
`NET_SID_CLIENT_Incoming_Ping` at net_sid.cpp:549, dispatched only from the read loop.)

**Pcap forensics this round (own parser, `scripts/parse_pcapng.py`).** Two flows on
6112: retail `192.168.0.12:9640` (t=+9.2..+11.0 s) and D2VP `192.168.0.12:9676`
(t=+18.7..+18.8 s, 21 ms total). New facts beyond R173:

- **The trigger is the AUTHREQ arriving, not our echo.** Retail: echo-req at +9.254627,
  echo at +9.254847 (0.22 ms), AUTHREQ at +9.254893, client ACK at +9.295, AUTH_CHECK
  120 ms later, clean FIN at +10.995. D2VP: echo at +18.757277 (3.17 ms — the delta is
  D2BOOT logging, not a thread hop), AUTHREQ at +18.757390, then **RST `win=0` 753 µs
  later with no ACK and no read**. Attempt 1 of the R175 log died on the same schedule.
- **The RST is ours.** `win=0` + client-host IP-ID continuity (retail used ids
  51468-51478, D2VP 51479-51484, same capture, same host) — no middlebox injected
  anything, and the server never sent FIN/RST (R173's finding, reconfirmed).
- **The wire is byte-clean in both directions.** D2VP's 0x50 is byte-identical to
  retail's (`IX86`/`D2XP`, version 0x0e, `CAN`/`Canada`); the server's 0x25 and 103-byte
  `FF 50` AUTHREQ replies are the same shape in both flows.

**The retail-vs-D2VP socket API trace — static half complete (r2 6.1.5 portable, project
`/tmp/game114d.r2`).** Retail disassembly vs D2VP source, function by function:

| Retail | D2VP | Verdict |
|---|---|---|
| `BnetReadFunction` 0x51c7e0 | `BnSend.cpp:1595` | identical: single-instance guard, `gbBNetSocketConnected` Sleep(10) arm, double re-fetch of `g_pBNetSocketData`/dead-flag between poll and read, `recv(chRecvBuf+nBufPos, 0x2000-nBufPos)` (buffer at `ebp-0x2008`, 0x2014 frame), `IncomingPacketHandlerLoop` offset contract, 180000 ms keepalive (`cmp 0x2bf20`), dead-flag on read failure |
| `NET_SID_CLIENT_Send` 0x51c5c0 | `BnSend.cpp:1507` | identical: header `FF|id|size+4`, guards on `g_pBNetSocketData` + ready-flag, `SMemCopy` with `nSize+4 < 0x2000` bound, `WSASocket2_Send`, dead-flag on refusal |
| `NET_SID_CLIENT_Incoming_Ping` 0x520fc0 | `net_sid.cpp:547` | identical 18-byte inline echo (`mov cl,0x25; call Send`) |
| `NET_SID_CLIENT_ConnectFastest` race | `BnSend.cpp:1771` | identical: non-blocking candidates via `Create(...,flags=1)`, losers `Free()`d post-race, winner CAS'd into `g_pBNetSocketData` with `bConnected=1`, **`SAFESOCK_SetSocketNonBlocking(0)` before the 0x01 protocol byte** |
| Safesock transport (Create/Free/Read/Send/select) | `Fog/Src/Safesock.cpp` | already certified faithful (R170 phase 14); `SO_SNDTIMEO`=3000 ms explains `WSASocket2_SendOrCloseOnTimeout`'s WSAETIMEDOUT close |

With this, **every in-process suspect is eliminated by direct parity**: one reader
(refcount), no closer in the window (R174 audit), faithful read/send/ping/race/transport.
What remains sits strictly *between* `recv()` and the TCB.

**Autopsy interpretation (now with two samples).** `sel=1(r1,e0)` + `peek=-1/10053` +
`send0=-1/10053` + `soerr=0` + `type=1` on both attempts = the R174 matrix's
**layer-conflict class**: the readiness layer still reports the queued AUTHREQ, the data
plane refuses everything with WSAECONNABORTED, the TCB carries no queued error, and the
handle is a live stream socket. Combined with a provably clean wire, byte-identical
server behavior toward retail on the same box, and full in-process parity, the aborting
agent is the **local Winsock filter stack (WFP callout / AV content scanner / LSP)
keyed on the process image** — the unsigned rebuild gets its sockets' data plane
aborted at AUTHREQ-delivery time; signed retail does not. This is now a diagnosis by
elimination at every layer we can inspect from this side, not a guess.

**R176 changes (all VP-marked; MSYS2 llvm-mingw build is the compile gate):**

- `BnSend.cpp` `BnetReadFunction` — **bn-recv-abort-budget**: the R173 one-shot 10053
  retry became a bounded budget (20 × `Sleep(10)` ≈ 200 ms) with the select-poll re-arming
  between attempts; the autopsy probe stays one-shot (first failure only); each retry logs
  `retry %d/20`, and a post-budget healthy recv logs `recv RECOVERED after %d x 10053
  retry (condition cleared)` before re-arming. Either the condition clears and login
  proceeds (workaround holds), or the next log writes the definitive "did not heal in
  200 ms" data point and tears down as before — both outcomes close a question.
- `Redivivus/d2_boot.cpp` — every boot-log line now carries `[tid %lu]` (the R175 session
  burned real time on thread attribution; marks come from several threads, the prefix
  should say which). `D2VP_ROUND` → 176.

**Field-verification checklist for the next run (decisive → cheap):**

1. **Injected-module census (30 s, do this first):** with D2VP sitting at the failed
   connect, open Process Explorer / Process Hacker → D2VP.exe → DLL list, and diff it
   against retail Game.exe's. Any AV/WFP user-mode hook DLL present in D2VP but not
   retail is the suspect. Also `netsh winsock show catalog` — any non-default layered
   provider explains a per-socket data-plane abort that `select` (which bypasses LSP
   data hooks) does not see.
2. **Exclusion test:** add `F:\GAMES\Diablo II\` (or D2VP.exe) to the AV/Defender
   exclusions (or disable the web/behavior shield for one run) and connect. If it logs
   in, the filter class is confirmed and the fix is distribution guidance (installer
   adds the exclusion / signed builds).
3. **R176 rideout:** if the filter remains enabled, expect
   `recv aborted (WSA 10053) but socket still open - retry %d/20` up to 20 lines, then
   either `recv RECOVERED ...` + login, or `recv failed ... protocol dead` ~200 ms after
   the first abort — that timestamp spread decides "stall" vs "hard abort" forever.
4. **WFP capture while reproducing:** `netsh wfp capture start` / `stop` around a failed
   connect names the callout that aborts the flow (look for the BLOCK with
   `.abortive` on the 192.168.0.14:6112 flow).
5. Optional dynamic API trace (the R174 lever's other half): API Monitor on ws2_32 in
   both processes — expect identical sequences (static parity is now proven); any extra
   caller at the moment of death is the injected module from step 1.
6. If a hard (non-healing) abort is confirmed and the AV cannot be excluded for users,
   the fallback lever is switching the BNCS auth stage to non-blocking + select-driven
   reads (the race phase's pattern), which bypasses the blocking-recv data path the
   filter is choking on — bigger change, own round.

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
