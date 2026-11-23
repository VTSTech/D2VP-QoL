# BNET-CONNECT-FINDINGS — R169 audit of the Battle.net connect path

## R178 field result: DECISIVE — the replay probes completed the whole conversation; the death is per-flow kernel state, not content, not image (and with Windows Firewall fully off, not the firewall either)

**The R178 pcap (`game-d2vp-bnet-R178.pcapng`, 2026-11-20 ~23:23 bnetd time) caught all
five BNCS flows.** Retail first (client :35777), then the D2VP run with both replay
probes firing:

| flow | who | wire outcome |
|---|---|---|
| :35777 | retail | full auth (0x50→0x25 echo→AUTHREQ→0x51→0x33), clean FIN at +4.2 s |
| :35829 | D2VP attempt 1 | 0x01→0x50→echo ok → **client kernel RST 1.24 ms after the 103-byte AUTHREQ arrived** (RST `ack=AUTHREQ_end`, `win=0`, TTL 128, IP-ID continuous) |
| :35830 | replay probe 1 | **identical conversation, started 0.4 ms after the RST, same process** → read the AUTHREQ, graceful FIN |
| :35831 | D2VP attempt 2 (R174 reconnect) | same conversation → **RST again, same signature** |
| :35832 | replay probe 2 | → read the AUTHREQ, graceful FIN |

bnetd (`bnetd.log` in the d2gs container) accepted all five identically — same
versioncheck selection (`D2XP_114D`, "no upgrade is available"), same AUTH_INFO parse;
the two auth flows end with pvpgn's own `sd_tcpinput: read returned -1` (it saw the
client's RST), the probe flows simply closed. IP-IDs 51890-51926 continuous across the
whole window: **nothing injected, no middlebox** — every RST is emitted by the client's
own kernel, with the AUTHREQ fully ACKed inside it.

**What this kills (decision-tree branch 1 taken):** the conversation content (probe = the
verbatim 0x50, same server, same second → survives twice), the process image (the
surviving socket is created by the same D2VP.exe, same thread that just watched the auth
socket die), and the server. What remains: **the original auth flow's kernel state** —
something about HOW that socket was created/connects, not WHAT it says.

**The user-side eliminations this round (owner actions):**
- **Windows Firewall turned off completely** → still dies. The stock WFP BLOCK-filter
  class is out; a third-party filter/WFP callout remains possible but now has no
  firewall involvement at all.
- **"It's what D2VP does with the 0x25" (client-side close) — checked and refuted:**
  the pcap shows our 0x25 echo *send succeeded* (server ACKed it) before the abort; the
  10053 lands on the recv AFTER the echo. No `shutdown()` call exists anywhere in the
  rebuild; every `closesocket` path is trapped since R177 and the only closers in the
  death window are the sanctioned post-death teardowns; a WSACleanup-to-zero imbalance
  is impossible in the auth window (refcount only grows — Create/StartUp add, nothing
  frees in between; R179 now logs the count to prove it with data). And decisively: the
  replay probes DO understand the 0x25 (they echo it from the same code shapes) and
  complete. The user's instinct that the kill is client-host is correct — but it is the
  kernel per-flow state, not our user-mode packet handling.

**The two remaining structural differences between dying and surviving sockets** (both
confounded, R179 splits them):
1. **Creation pattern.** Auth sockets: race → non-blocking `connect` → `select`
   write/except → winner → `FIONBIO(0)` toggle → blocking conversation. Replay probes:
   plain `socket()` + blocking `connect` + conversation. Retail completed auth THROUGH
   the race pattern on the same machine though (flow :35777) — so the pattern alone
   cannot be sufficient cause; it must combine with something else (state left by the
   first attempt, handle identity, or a per-flow classifier that trips only when the
   conversation is driven by the auth thread's raced socket).
2. **Handle identity.** R176 saw the reconnect's own socket land on attempt 1's freed
   handle value (0xdf0) and die like it. If the kernel/classifier poisoned the HANDLE
   TABLE entry, every future socket reusing that value inherits the death — which would
   also explain why the replays (created BEFORE `Free`, different handles) survive.

## R179 changes (all VP-marked; MSYS2 llvm-mingw build is the compile gate)

- `BnSend.cpp` `NET_SID_CLIENT_ConnectFastest` — **bn-auth-blocking-connect**: a
  **single-entry server list** (the private D2GS/PvPGN case) skips the race entirely and
  connects exactly like the probes: `Create(..., flags=0)` — one plain blocking socket.
  This is the only shape that has provably completed auth on the failing machine.
  Multi-entry gateway lists keep the retail race. The blocking path re-uses the shared
  post-race tail via `goto bn_race_won` (non-winner cleanup is a no-op with count 0).
- `Safesock.cpp` — the R178 inline replay became **`sRunAuthConversationReplay(saPeer,
  mode)`** (step coding unchanged; verdict line now tagged `ctl-replay` / `race-replay` /
  `reuse-replay`) plus two new probes:
  - **probe B — race-replay** (mode 1, in the autopsy right after probe A): the same
    conversation through the race's exact shape — `FIONBIO(1)` → connect → `select`
    writable → `FIONBIO(0)` → conversation. If A survives and B dies, the pattern is the
    trigger; if both survive, pattern AND content are exonerated from a clean handle.
  - **probe C — reuse-replay** (mode 2, `SAFESOCK_RunReuseReplayProbe`, called from the
    R174 reconnect path **after** `Free`): fresh socket created immediately after the
    dead auth socket is freed, so `socket()` lands on the just-freed handle value. If C
    completes while the real reconnect dies at AUTHREQ → handle identity is the
    discriminator; if C completes and the reconnect now survives → the blocking-shape
    change alone fixed the round; if C dies like the real socket → the death rides on
    the Free/reconnect sequencing itself.
  - The autopsy now stashes the peer sockaddr into `g_saCtlReplayPeer` (d2_boot globals)
    so probe C can reach the server after the auth socket's `getpeername` is gone.
- `Safesock.cpp` — **bn-wsaref**: every `WSAStartup`/`WSACleanup` site in the process
  funnels through a counter (`BNet: wsaref -> N (site) [tid]`): startup, create, and the
  cleanup wrapper (the ErrorManager fatal-path sites all call the wrapper). Closes the
  refcount question with data — the autopsy's "data-plane fails, ws2_32-internal probes
  succeed, SO_TYPE=10042" shape is also the documented count-hit-zero signature, and
  the count must be provably >0 through the whole auth window.
- `Redivivus/d2_boot.{h,cpp}` — `D2VP_ROUND` → 179; peer-stash globals.

**How to read the R179 log (decision tree):**

1. `single-server list -> blocking connect <ip> (race skipped)` then login proceeds →
   **FIXED**: the race shape was the co-trigger; private-server connects stay blocking
   forever. (If retail lists ever need the race, the multi-entry path is untouched.)
2. Blocking connect, still `autopsy recvErr=10053` → shape alone is not it; read
   `ctl-replay`/`race-replay` verdicts: if the plain-shape replay (A) survives but the
   race-shape replay (B) dies at/after connect → the FIONBIO/select pattern is the
   trigger and the fix is dropping the race for ALL list shapes.
3. `reuse-replay verdict step=0` while the real (blocking) connect dies → handle-value
   poisoning; next round re-seeds the handle table (burn sockets until the value moves)
   or switches the whole BNCS stage off the racing path permanently.
4. `reuse-replay` dies AND blocking connect dies → the remaining lever is the
   `wsaref` trail + `netsh wfp capture` on a fully-clean boot (the P2P-storm hygiene
   factor matters: the desktop runs heavy torrent/DHT traffic during every failing run;
   try one run with the torrent client off — retail survived it, but the classifier
   theory says concurrent P2P flows may arm per-flow inspection that a plain-block
   firewall rule never exercises).

## R177 field result: two suspect classes died, the block is conversation-scoped — SO_TYPE fails with WSAENOPROTOOPT(10042)

**Machine-side classes eliminated by the user's Process Hacker dump + the in-log census.**

- **No LSP.** `winsock catalog: 12 providers, 0 layered` — the stock provider set, zero
  layered entries. A legacy LSP shim (the classic "recv returns 10053 on an open socket,
  select stays green" producer) is ruled out.
- **No in-process hook.** The full DLL list of the failing process contains only system
  DLLs and AMD graphics (`aticfx32`/`atiu9pag`/`atiumdag` = Radeon driver;
  `amdihk32` = AMD's Radeon *input/overlay* hook, graphics-side only; `mswsock` thread at
  `sethostname+0x25d0` = the stock Winsock async worker, present in every networked
  process). No AV/content-filter DLL injected. An "extra caller in our process" is dead as
  a theory; whatever remains runs in the kernel (WFP callout driver) — invisible to
  Process Hacker by design.

**The R177 log itself.** Autopsy, both attempts, identical: `soerr=0/0` (SO_ERROR
works, empty), `type=-1/10042` (**SO_TYPE fails with WSAENOPROTOOPT** — now with its error
code), `hvalid=1/0` (kernel handle VALID — no closesocket ever ran on it), `fion=0/0`
(AFD reports **zero bytes pending** — the receive queue is purged, i.e. the TCB was
already aborted at autopsy time; the AUTHREQ's 103 bytes never sat readable), `sel=1(r1,e0)`,
`peek=-1/10053`, `send0=-1/10053`. Drift: exactly one drift line per attempt, then nothing
— handle value, SO_TYPE and kernel validity stable across the whole 20-retry budget. Close
traps: the only `SAFESOCK ... closesocket` lines in the entire log are the two sanctioned
`Free` calls by the auth thread (tid 12244) *after* death — no rogue closer exists.

**The ctl-socket came back ALIVE on both attempts** (`ctl-socket connect=0/0 send=1/0
sel=0 recv=-1/0`): a brand-new socket from the same process, same millisecond, same server —
connect and send succeed; the server stays silent for the 1 s window because it pings only
after receiving 0x50, exactly as both real attempts show. Combined with the catalog and the
module dump: the block is **NOT app-wide and NOT keyed on the image wholesale** — it is
scoped to the auth conversation/flow. (UI note: "Accessing your account" appearing before
"Unable to connect" is the mess queue posting the next status string optimistically —
`mess queue state -> 18` fires the instant the protocol-dead flag is set; the protocol
itself was already dead. Not a hidden progress step.)

**The one remaining anomaly: SO_TYPE → 10042.** A retail-class MSAFD TCP provider answers
SO_TYPE on any live or aborted socket — R175's autopsies did exactly that (`type=1`). Since
R176 the same probe on the same machine returns WSAENOPROTOOPT, "bad protocol option", on a
handle that simultaneously answers SO_ERROR and FIONREAD and passes GetHandleInformation.
Either the provider/option layer for THIS flow's kernel object is corrupted or delegated
(flow-scoped state), or the machine's WFP/provider state changed between the R175 and R176
runs (AV definitions update silently). The R178 A/B battery settles it empirically: the
same probes run against a live control socket in the same process, same instant.

**R178 changes (all VP-marked; MSYS2 llvm-mingw build is the compile gate):**

- `BnSend.cpp` `NET_SID_CLIENT_Send` — **bn-ctl-replay**: the exact outbound 0x50 frame
  (47 bytes, header included) is stashed into `g_chCtlReplayAuthInfo` (defined in
  `d2_boot.cpp`, declared in `d2_boot.h`) before every send. Auth thread writes, read
  thread reads, forensic-only, no locking.
- `Safesock.cpp` — the R177 opening-leg ctl probe became the **full conversation replay**:
  fresh socket → connect → **0x01 → stashed 0x50 verbatim** → wait 2 s for the server ping
  → echo it → wait 3 s for the AUTHREQ. Step-coded verdict line
  (`ctl-replay verdict step=%d err=%d ping=%d [...] authreq=%d [...]`; step 0 = replayed
  the whole conversation). Plus **`sLogCtlProbeBattery`** — the autopsy's probes
  (soerr/type/hvalid/fion/sel) run against the replay socket before start (`battery
  ctl-live`) and after the verdict (`battery ctl-post`), so the A/B against the dead auth
  socket's autopsy line is one-to-one.
- `Redivivus/d2_boot.h` — `D2VP_ROUND` → 178.

**How to read the R178 log (decision tree):**

1. `ctl-replay verdict step=0` (replay survives ping + echo + reads the AUTHREQ) → the
   death is bound to the ORIGINAL flow's kernel state, not the conversation content and
   not the image. Forensics then hinge on the battery pair: if `ctl-live`/`ctl-post` show
   `type=1` while the autopsy shows `type=-1/10042`, the original flow's option layer is
   demonstrably corrupted per-flow → kernel-side per-flow state (WFP flow context) is the
   killer; `netsh wfp capture` (below) names it.
2. `ctl-replay verdict step=3/4/5` (died at/after the ping echo, before AUTHREQ) or
   `step=6/7` (died at AUTHREQ) → the trigger IS the conversation content — a
   content-inspecting kernel classifier aborts any flow of this process that replays this
   byte sequence. Still testable: `netsh wfp capture`.
3. Any `battery ctl-*` line showing `type=-1/10042` on a HEALTHY socket → SO_TYPE=10042 is
   machine-wide behavior (provider state changed between rounds), the datum loses
   per-flow power, and R175's `type=1` was the anomaly worth re-checking.
4. Machine-side decisive test, independent of 1–3: **`netsh wfp capture`**
   - `netsh wfp capture start` (needs admin; records WFP events + state)
   - reproduce one failed connect in D2VP
   - `netsh wfp capture stop` → writes a `.cab`; inside, `wfpstate.xml` lists filters and
     callout providers — search for the `192.168.0.14` / port `6112` flow and any
     **non-Microsoft provider name** (an AV's kernel driver aborting flows shows up here).
   - quick alternative without a cab: `netsh wfp show state file=wfp.xml` then search the
     XML for provider names, and `netsh wfp show filters` for BLOCK filters on 6112.
5. The AV exclusion test stays on the list as the fastest user-side lever (Windows
   Defender's network inspection / a third-party web-shield module both run WFP callouts;
   exclude `F:\GAMES\Diablo II\` or disable network protection for one run).

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

---

## R190 — CheckRevision bypass: the 10053 rounds' real root cause (2026-11-21)

The WFP capture exonerated the kernel (all 15 providers Microsoft, zero third-party
callouts, the `nsr`/NsrSubLayer `STREAM_FILTER`/`TCP_FILTER` entries are Win11 Web
Threat Defense **inspection** callouts — cannot block; zero TCP classify-drop events for
the auth flow), and the pktmon capture (port 6112, one failing run) showed the actual
death: **15 full auth conversations in ~430 ms** (bursts of 7+8), each one: SYN → 1 B
protocol byte → 51 B 0x50 → server 8 B ECHOREQ → client echoes 8 B (read thread alive,
framing parser fine) → server 103 B AUTHREQ_109 → **client FIN 1.4–1.7 ms later** (data
consumed, graceful) → immediate re-SYN. **SID_AUTH_CHECK (0x51) was never sent on any
attempt**; retail sends it ~120 ms after AUTHREQ on the same server. The rename-to-
Game.exe test behaved identically (not image-name-keyed); adapter census = Npcap +
VBoxNetLwf + MS components only; Defender = only AV. The old "WFP STREAM-layer abort"
theory (R178–R181) is dead: nothing ever aborted the flow from below.

**Root cause (client, deterministic):** `NET_SID_CLIENT_Send_0x51_AuthCheck` gates on
`BNDOWNLOAD_PerformCheckRevision` (BnDownload.cpp), whose retail chain —
`SFileOpenArchive` on the AUTH_INFO MPQ → authenticate → extract CheckRevision DLL →
`D2FILE_VerifyFileSignature` → `LoadLibrary` → call — can never succeed in the 1.14d
rebuild (no Blizzard-signed CheckRevision DLL in a D2VP install), so it returned
0 = failure and 0x51 was never sent, against any server. The Strategy-1 bypass from
`R2/CheckRevision_Analysis_and_Fix_Strategies.md` was designed on 2026-11-19 but never
integrated. The Nov-18/19 session's leftovers (the `D2VP-D2GS_Compatibility*` docs, the
false "IMPLEMENTATION/INTEGRATION_COMPLETE" notes, the test/build scripts, and the
never-wired `compatibility_patch.{cpp,h}` packet-whitelist — dead code whose whitelist
would have broken the protocol anyway) were removed after R190 landed.

**R190 changes (all VP-marked; MSYS2 llvm-mingw build remains the compile gate):**

- `BnDownload.cpp` `BNDOWNLOAD_PerformCheckRevision` — Strategy-1 bypass fast path
  (static `s_bCheckRevisionBypass = TRUE`): ver=0x00010064, checksum=0x00000000, empty
  exe-info, **return 1** (both callers treat 0 as failure — the strategy doc's "return 0"
  snippet contradicts the callers and was not used). Retail chain kept below the toggle
  for a future full-Storm install. Added `Redivivus/d2_boot.h` include for the marks.
- `net_sid.cpp` `NET_SID_CLIENT_Send_0x51_AuthCheck` — the `HasPatchFilename` wait had
  **no timeout** (BNFTP failure on a server that serves no version MPQ = auth thread
  stuck forever); bounded at 20 s with a logged proceed (bypass needs no MPQ).
- Flag note (correction to the R190 planning notes): `g_bSIDProtocolReady` /
  `SID_GetReadyFlag` looks like a collapsed dead/ready pair but every consumer treats
  1 as abort/failure coherently, including the auth-stage site that sets terminal state
  0xb on dead — it is the protocol-dead flag under a recon misnomer. Not touched.

**Field tuning:** if the server rejects 0x51 (result 0x101/0x102), read the expected
ver/checksum/exeinfo from pvpgn's versioncheck log (or the D2GS agent's retail pcap) and
adjust the two constants in the bypass — or set `exactmatch = 0` server-side
(VTSTech/D2VP-D2GS owns that side).

**Expected d2boot.log on success:** `SID pkt 0x50 received (99 bytes)` → `CheckRevision
bypass (R190): ver=0x00010064 checksum=0x00000000` → `SID 0x51 AUTH_CHECK sent
(accepted=1)` → `wait 0x51 -> reply arrived` → `SID 0x51 AUTH_CHECK result=...` →
`Connected to Battle.net`.

**Riders:** all 90 pktmon drops are the server's UDP 6112 probes hitting an unbound
client port ("transport endpoint was not found") — D2VP never binds UDP 6112; game
play will need that after auth lands (allow inbound UDP 6112 / LAN=Private profile).
Open question: which thread emitted the +1.4 ms FIN (auth thread is timing-excluded;
primary suspect: launcher-poll `BNSEND_Disconnect` after `BNMESSQUEUE_SetStateIfZero(1)`)
— the R173 teardown marks in the next d2boot.log will name it.

## R196 — Mode A root cause closed: the read loop recv()s through a NULL buffer (2026-11-22)

The tracerpt CSV landed (`etw-analysis/tracerpt/bnet.txt`, from `bnet.etl` 2026-11-21
18:00, R190-era build, PID 5860 / read TID 2412 / auth TID 5128), and it names the
killer in one event. Per auth flow on 192.168.0.14:6112:

```
Bound -> connect -> send 1 B (0x01) -> send 51 B (0x50 AUTH_INFO)
-> recv 8 B ECHOREQ (recv IRP completes at the user buffer)
-> send 8 B echo -> AUTHREQ 103 B lands in the kernel buffer
-> +9.6 ms: Winsock event 1032 "Aborted": "Invalid buffer specified on fastio receive"
-> {Virtual Circuit Closed} (event 4001) x3   <- AFD ABORTS THE ENDPOINT ITSELF
-> Freed/Closed by the read thread -> reconnect ~2.6 ms later -> repeat (15x)
```

**The abort is AFD rejecting OUR recv#2 buffer.** `recv#1` completes at
`0x9BCDE74` (the 8192-byte `chRecvBuf`); `recv#2` is issued ~9.6 ms later with a
buffer AFD considers invalid, and on Win11 24H2 AFD's fast-io receive path answers
an invalid buffer by **aborting the whole endpoint** — hence every subsequent
op failing with WSAECONNABORTED while `SO_ERROR` stays 0 and the handle stays
kernel-valid: the exact R175–R177 autopsy signature (`sel=1, peek=-1/10053,
send0=-1/10053, soerr=0, hvalid=1, fion=0`) that we spent R173–R191 blaming on
WFP/Defender/LSP/kernel state. Game.exe "working fine" was the tell: retail never
passes a bad buffer. The R192 `VirtualLock` theory is dead — no page-fault story
produces a NULL buffer; the pointer itself was NULL.

**The arithmetic bug (BnSend.cpp `NET_SID_CLIENT_IncomingPacketHandlerLoop`, in the
rebuild since R169):** the "bytes remaining" endpoint was decompiled as

```c
pBytes + (nSize - (int)pScanPtr)        // = B + nSize - P  (WRONG)
pScanPtr + (nSize - (int)(pScanPtr - pBytes))  // = B + nSize  (retail, invariant)
```

The wrong form evaluates to a tiny integer (0..nSize) reinterpreted as an ADDRESS,
and it doubles as the memmove SIZE and the RETURN VALUE. On the normal path — a
lone 8-byte ECHOREQ recv'd, dispatched, fully consumed (`pScanPtr = B+8`, nSize=8)
— the next pass computes the endpoint as `B + (8 - (B+8))` = **0**, memmoves 0
bytes and **returns NULL**. `BnetReadFunction` then does
`nBufPos = (int)NULL - (int)chRecvBuf = -(int)chRecvBuf` (≈ −163,000,000), so
recv#2 is `WSASocket2_Read(pSafesock, chRecvBuf + nBufPos /* = NULL */,
0x2000 - nBufPos /* ≈ 163 MB */)` — the exact invalid-buffer recv the ETW shows.
Deterministic on every attempt (nBufPos resets to 0 per thread start), which is
why the pcap/pktmon always died at the same byte with the AUTHREQ unread, and why
the ETW shows the later attempts' only *successful* 103-byte reads coming from the
R178 ctl-replay probe's own buffers, never the real socket. Harness-validated
(consumed/pipelined/compaction/reassembly/scan scenarios): the corrected loop
returns `pBytes + 0` after a full consume → `nBufPos = 0`; offsets stay in
`[0, nSize]`; the recv call is always in-allocation. `memcpy` → `memmove` for the
compaction copy (overlapping ranges since R169).

**Rider (`bn-patchwait-bypass-skip R196`):** with the loop fixed, the auth flow
reaches `Send_0x51_AuthCheck`, but `NET_SID_CLIENT_Incoming_AuthInfo` seeds the
patch filename as `""` and only a successful BNFTP version-MPQ download replaces
it — on servers that serve none (VTSTech/D2VP-D2GS) every login would pay R190's
full 20 s bound before proceeding. The bypass hardcodes ver/checksum/exe-info and
never opens the MPQ, so when `BNDOWNLOAD_CheckRevisionBypassActive()` (new getter;
`n-checkrev-bypass` flag moved to file scope in BnDownload.cpp) is true the wait
is skipped with a log mark; the retail chain keeps the bounded wait.

**R196 changes (all VP-marked; MSYS2 llvm-mingw build is the compile gate — edits
hand-checked, the loop arithmetic additionally harness-validated):**

- `BnSend.cpp` `NET_SID_CLIENT_IncomingPacketHandlerLoop` — **bn-handlerloop-arith**:
  correct endpoint arithmetic (`pEnd = pBytes + nSize`, remaining = `pEnd - pScanPtr`),
  memmove for the compaction copy, scan/dispatch order untouched.
- `BnDownload.cpp` — `s_bCheckRevisionBypass` hoisted to file scope + new
  `BNDOWNLOAD_CheckRevisionBypassActive()`.
- `BnDownload.h` — getter declaration.
- `net_sid.cpp` `NET_SID_CLIENT_Send_0x51_AuthCheck` — **bn-patchwait-bypass-skip**:
  the 20 s-bounded patch-filename wait runs only when the bypass is off.

**Expected d2boot.log on success (fast path):** `SID pkt 0x50 received (99 bytes)`
→ `0x51 patch-filename wait skipped (CheckRevision bypass needs no MPQ)` →
`CheckRevision bypass (R190): ver=0x00010064 checksum=0x00000000` → `SID 0x51
AUTH_CHECK sent` → `wait 0x51 -> reply arrived` → `SID 0x51 AUTH_CHECK result=0x000`
(or the server's versioncheck verdict) → `Connected to Battle.net`. No autopsy,
no `recv aborted`, no reconnect loop: one connection, one conversation.

**Field checklist:** 1) if the server rejects 0x51 with 0x101/0x102, read the
expected ver/checksum from pvpgn's versioncheck log and set the two constants in
`BnDownload.cpp` (or relax `exactmatch` server-side); 2) confirm the winsock
events in a fresh ETW/pktmon show **no** `Aborted/1032` on the auth flow; 3) after
auth lands, the R190 riders still apply — UDP 6112 inbound for game play.

## R196 field result + R197 — auth flow advanced 2 stages; new crash in virgin key-decode region (2026-11-22)

**R196 field log — the 10053 war is won.** First field log ever past recv#2:
`recv 8 bytes` → 0x25 echo → **`recv 103 bytes (buffered 103)`** → `SID pkt 0x50
received (99 bytes)` → `wait 0x50 -> reply arrived` → `0x51 patch-filename wait
skipped (CheckRevision bypass needs no MPQ)` → `CheckRevision bypass (R190):
ver=0x00010064 checksum=0x00000000`. No autopsy, no 10053, no reconnect loop. The
handler-loop arithmetic fix + patch-wait skip are confirmed in the field.

**New crash, further down the same call:** `VEH: fatal exception 0xC0000005 at
addr 00FD32DC`, `AV READ at 8A61CD26`, auth thread (tid 14384), right after the
bypass mark and before any `SID send op=0x51` mark — i.e. inside the region that
**never executed in any earlier build**: pre-R196, `PerformCheckRevision`
returned 0 at net_sid.cpp:1548 and the auth thread returned before
`DecodeAndLoadKeys()`. The R196 run is the first execution ever of:
`DecodeAndLoadKeys` (loads disguised key files font\font.gid/.clh/expansion
from MPQ/dir, XOR+IDEA+SHA1-chain decode) → `D2CheckExpansionGame` → per-key
`BNSEND_HashCDKeyByType` → `PrepareCDKeyHash` (`SStrLen`/decode/XSHA1 or
TransformCdKeyToken) → CD-key owner `SStrCopy` → `NET_SID_CLIENT_Send(0x51)`.

Audited so far, no AV found with valid inputs: `DecodeAndLoadKeys` (paths and
permute tables in-bounds; `aKeyPathTable[0]` never set but never read),
`PrepareCDKeyHash` (switch on strlen, rejects other lengths), `DecodeCDKeyString`
(NULL guard, bounded copies, checksum gate), `TransformCdKeyToken` (bounded
locals). Suspects remaining: the virgin crypto helpers
(`XSHA1`/`BNSEND_HashCDKeyData`/`SHA1Compress`/`IDEAEncryptDecrypt`/
`SetupIDEAKey`/`EncodeCDKeyHash` — note `EncodeCDKeyHash`'s decompiled XOR loop
reads up to 3 bytes past its 196-byte stack work buffer, benign-looking but a
transcription-bug flag) and the possibility that a key file simply failed to
load in the D2VP install (missing `font\font.gid`/`.clh` → NULL key →
`SStrLen(NULL)`/`SStrCopy(dst,NULL,n)` fault, or decoded garbage used as data).

**R197 changes (bn-authcheck-marks; MSYS2 llvm-mingw is the compile gate):**

- `net_sid.cpp` `DecodeAndLoadKeys` — result marks after each store:
  `key decode: classic=%p` / `owner=%p` / `expansion=%p` (a NULL says the key
  file is not in the MPQ/dir).
- `net_sid.cpp` `Send_0x51_AuthCheck` — `0x51 key state:` line (classic/expansion/
  owner pointers + expansion-game), per-key `hash FAILED` marks, `0x51 AUTH_CHECK
  sending len=%u` before the send. Owner pointer NULL-guarded (empty owner).
- `BnSend.cpp` `BNSEND_HashCDKeyByType` — returns `PrepareCDKeyHash`'s result
  (was discarded), logs `cd-key hash type=%d key=%p len=%u -> ok=%d`; header
  decl updated.
- `BnSend.cpp` `BNSEND_PrepareCDKeyHash` — NULL key top guard (retail never sees
  a missing key file here; the SStrLen would fault).

**Decision tree for the next run:** a NULL in `key decode:` lines → the key file
is missing in the D2VP install → guards keep us alive, 0x51 goes out with zeroed
hashes → server rejects 0x201 (invalid key) → the fix is install-side (ship/copy
the key files) or a registry-based key reader. All pointers non-NULL but the
crash persists → the fault is data-dependent in the decode/hash crypto → diff
the named function (marks + crash-report frames) against retail Game.exe via the
r2 project (D2-Game.exe-1.14d-Retail) and fix the transcription. Crash report
`d2crash-20261122-181430.txt` (12 frames) requested — if d2exe.map was present
at build time the report may already symbolize the faulting frame.

## R198 — 'Checking Versions' crash root-caused via the crash zip: Sha1_Pump self-clobber (2026-11-22)

The user's `r196-crash.zip` (d2boot.log + **d2exe.map** + d2vp.exe) closed it without
needing the R197 field run. Symbolization chain: VEH eip `00FD32DC`, runtime base
`00F70000` (VEH line), PE ImageBase `0x400000` (d2vp.exe) → map VA `0x004632DC`
→ **`Storm::Source::SSignature::Sha1_Pump` +0x1E8** (nearest symbols: starts
0x4630F4, next `Sha1_Append` at 0x463C04). Reached via
`BNSEND_HashCDKeyData` → `Sha1_Append` → `Sha1_Pump` — the 26-char expansion
CD-key hash inside `PrepareCDKeyHash` case 0x1a, i.e. the virgin region R197
instrumented.

Disassembly of the faulting instruction (`mov edx, [eax]` at 0x4632DC, eax =
`[ebp-0x14]+0x2C`) and the full loop tail (`0x46330E add eax,0x38; 0x463313 mov
[eax],edx; 0x463315 add [ebp-0x14],4; 0x463319 cmp [ebp-8],0x4f; 0x46331D jle`)
expose the mechanism: the W-expansion loop keeps `P = ebp-0xA4 + ctr*4`, reads
`[P+0x2C]/[P+0x18]/[P-8]/[P]` (= W[j-3]/W[j-8]/W[j-14]/W[j-16] — standard SHA-1
read set), stores the new word at `P+0x38`, and keeps its counter at `[ebp-8]`.
**When ctr = 25, `P+0x38` == `ebp-8`: the freshly computed W word is written over
the loop counter itself.** A W word with bit 31 set reads as negative → the
signed `jle` keeps iterating from a garbage counter → billions of iterations →
`P` wraps through the address space → AV READ at wherever it lands
(`8A61CD26` in the field run). Bit 31 clear → silent early exit with a corrupted
schedule (wrong hash, no crash). Which case fires depends on the key data —
a data-dependent, deterministic-per-install crash.

The reconstruction's read set, big-endian load, round constants
(0x5a827999/0x6ed9eba1/0x8f1bbcdc/0xca62c1d6) and H+= tail are standard SHA-1,
so **R198 `bn-sha1pump-rewrite` replaces `Sha1_Pump` with a clean standard SHA-1
block function** (one 80-word schedule, strictly bounded loops, no pointer
windows). Harness (`/tmp/r198-test`) validates the composite
`Sha1_Initialize`+`Sha1_Append`+`Sha1_Pump`+`Sha1_Finalize` against the FIPS
test vectors and an independent reference across lengths 0..200 × 11 chunk
splits at -O0, -O2 and -fsanitize=address: ALL PASS.

**Rider (`bn-sha1append-counter R198`):** the -O2 harness build segfaulted where
-O0 and ASan passed — `Sha1_Append`'s full-block loop used a `uint8_t*` as the
counter (`(uint8_t*)(nLen >> 6)`, decrementing a pointer through byte address 0:
UB the optimizer exploits). Fixed with an integer counter; harness -O2 clean.
The same UB existed in the shipped build's SSignature.cpp (llvm-mingw -O2).

R198 build also carries R197 (`bn-authcheck-marks` + NULL guards) — the key
decode / hash-failure marks remain useful if anything else in the virgin region
misbehaves. `WardenClient.cpp` shares Initialize/Append/Pump/Finalize and gets
the same fixes.

**Expected d2boot.log on success:** `SID pkt 0x50 received (99 bytes)` →
`0x51 key state: classic=0x… expansion=0x… owner=0x… expansion-game=1` →
`cd-key hash type=0 … ok=1` / `type=1 … ok=1` → `0x51 AUTH_CHECK sending
len=…` → `SID send op=0x51 …` → `wait 0x51 -> reply arrived` →
`SID 0x51 AUTH_CHECK result=0x000` → `Connected to Battle.net`. If a hash
reports `ok=0`, the corresponding key file is missing/undecodable in the D2VP
install (guards keep us alive; 0x51 goes out with zeroed hashes and the server
rejects 0x201 — install-side follow-up).

## R198 field result + R199 — BNCS auth COMPLETE; realm/MCP socket never connects (2026-11-22)

**R198 field log — the auth gauntlet is fully won.** `AUTH_CHECK result=1`
(versioncheck accepted!), `cd-key hash type=0/1 ok=1` (key files decode fine —
classic/expansion/owner all non-NULL: the R197 marks show `02CB00F0/02CB0148/
02CB01A0`; the R196 crash was purely the broken SHA-1), `LOGONRESPONSE2 result=0`,
**`CharSelect: screen shown`**, `QUERYREALMS2` answered, `LOGONREALM (realm=D2CS)`
answered with the 81-byte realm reply. Every fix R169→R198 confirmed live.

**New failure, next stage:** right after LOGONREALM, the realm/MCP socket is
created (`wsaref -> 2 (create)`, h=0xd5c) — and then **nothing**: no
`thread-side connect ok` mark, zero sends/recvs, 11.8 s of silence, then the main
thread gives up: `SAFESOCK Free closesocket h=0x00000d5c` → protocol dead →
`BNSEND_Disconnect` frees the BNCS socket (h=0xd34).

**Cause: the R195 `bn-threadconnect` deferral.** R195 moved the blocking-path
connect out of `Create` into the socket's read thread via
`SAFESOCK_ThreadConnect` — but only `BnetReadFunction` calls that. The realm
socket's thread proc is `D2QSSocket::ReceiveLoopCallback`, which exits its loop
on `!pSafesock->bConnected` (D2QSSocket.cpp:255) — so with the staged sockaddr
never picked up, **the MCP socket never performs connect() at all**, the MCP
stage's own retry/timeout (~12 s) expires, and the flow tears down. This also
explains why the BNCS socket logged `thread-side connect ok` twice while the
realm socket logged nothing.

R195's reason to exist (Mode A probe-parity: "connect-thread != recv-thread is
the last structural difference") is void — R196 proved Mode A was the
handler-loop NULL recv. Retail connects in `Create`, on the calling thread, and
sets `bConnected` (`WSASocket2_Send` guards on it; pre-R195 builds sent 0x50
straight after `Create`, so retail semantics are certain).

**R199 changes (Safesock.cpp only; MSYS2 llvm-mingw compile gate):**

- `bn-threadconnect-revert`: the blocking path connects directly again
  (failure → nError=3, cleanup, nullptr as the non-blocking path already does),
  sets `bConnected = 1`. `SAFESOCK_ThreadConnect` + the staging statics remain
  as inert no-ops (nothing stages; `dwThreadParam` returns to 0x0 for the BNCS
  Create calls).
- `bn-options-restore`: `s_bApplySocketOptions` back to TRUE (retail parity:
  TCP_NODELAY / SO_KEEPALIVE / SO_SNDTIMEO=3000 on every Safesock socket). R193
  exonerated the options empirically (no-option socket still died; the killer
  was the buffer bug), and NODELAY is wanted for gameplay latency.
- Kept: R194's 100 ms SO_RCVTIMEO (harmless for poll-then-read sockets;
  `BnetReadFunction`'s R194 idle-wake depends on it).

**Expected d2boot.log on success:** after `wait 0x3e -> reply arrived`, a new
socket connects immediately (`race winner`-style mark absent but no dead flag),
then the MCP handshake marks (`MCP: connection established=1`, …). NOTE: the
MCP/charlist stage (`McpConnect.cpp`, `D2QSSocket.cpp`) is virgin code like the
0x51 region was — the next run may surface its own decompilation defects. The
R197-style marks + the crash-map workflow (zip with d2exe.map + d2vp.exe +
fault address) will close anything that surfaces. Rider still standing from
R190: the client never binds UDP 6112 — allow inbound UDP 6112 on the server
side for game play once realm charlist works.

## R199 field result + R200 — realm/MCP connects; charlist delivered and dropped (2026-11-22)

**R199 field log:** auth identical (all wins replay). The realm socket now CONNECTS
(R199 revert confirmed live) — and the same-millisecond `closeSocket h=0x00000d4c`
turned out to be a different socket's teardown, not the MCP socket: the pcap proves
the MCP connection lived and talked.

**R199 pcap decode (`game-d2vp-bnet-R199.pcapng`, client-side capture):**

- BNCS 6112 flow: full healthy auth, real 0x51 (139 B) with the decoded CD-key
  hashes, versioncheck accepted, LOGONRESPONSE2 (user "test"), realms list
  (`D2CS` / "PvPGN Closed Realm"), LOGONREALM → 81 B reply:
  `u32 result=1 … u32 0x10 … IP c0 a8 00 0e … port 17 e1 … token da526925 …
  "PX2D" … ver 0x0e … "Ntest"` — the MCP dial parameters parse CORRECTLY
  (state+16 = 192.168.0.14, state+20 = 0x17E1 = 6113 after the u16 truncation).
- MCP 6113 flow: SYN → SYNACK (+102 ms) → connected. Client sent the MCP
  protocol byte + MCP_STARTUP (72 B, echoing the 0x3E-provided cookie/chunks:
  server token, "PX2D", ver 14, unique name) → **d2cs replied `07 00 01 00 00
  00 00` = MCP_STARTUP result 0 (ACCEPTED)** → client sent CHARLIST2 request
  (0x19) → **d2cs delivered the charlist (0x36 00 19 …) containing the "Test"
  character** → the client sat on it and FIN'd ~1.66 s later.

The closed-realm handshake works end-to-end on the wire; **the client dropped the
charlist in user mode.** Two stacked rebuild defects (`bn-mcp-dispatch R200`):

1. `ProcessReceivedData` (D2QSSocket.cpp) called the registered QSSocket handler
   as `pfnPacketHandler(pHandlerContext)` — passing the registration context
   (nullptr for the MCP socket) where the PACKET pointer belongs.
   `MCPCONNECT_SocketCallback_Dispatch(pPacketBuf, nPacketSize, pContext)` then
   received garbage for its packet/size arguments, bailed at the first guard, and
   every MCP packet was routed nowhere.
2. `HandleIncomingPacket` — the full 26-entry INCOMING table (0x02..0x19:
   charcreate, charlogon, **charlist 0x19 → `CopyCharListToGlobal`**, gamelist
   0x11, …) — had **zero callers** in the entire tree (dead code since
   reconstruction). Only the sparse `gaMcpSocketCallbackHandlers[0x01]`
   join-result signal was ever reachable, and even that never ran (defect 1).

**R200 changes:**

- `bn-mcp-dispatch`: `ProcessReceivedData` now invokes the handler with
  `(pMessage, wMsgSize, pHandlerContext)` — the retail 3-arg contract
  (`fpD2QSSocketPacketHandler` typedef and the 0x168/0x16C field comments in
  D2QSSocketStrc.h updated; the MCP client is the only registrant). Dispatch
  keeps its 3-arg signature and now chains `HandleIncomingPacket` for any packet
  the sparse table doesn't consume; consumed packets return 1 so
  `ProcessReceivedData` skips enqueueing (the UI pumps' `ReadAndParsePacket`
  path re-runs the full table on queued messages — avoid double-processing,
  e.g. the 0x11 gamelist append offset). Unhandled packets still enqueue.
- `bn-mcp-marks`: ReceiveLoopCallback gate-break reasons, recv result/error and
  close-site marks; `MCP: pkt 0x%02x received` dispatch marks.
- MILESTONE (user field report): **'Other Multiplayer → Open Battle.net'
  connects all the way and creates a game** — the R196/R198/R199 fixes
  unblocked the whole direct-play path.

**Expected d2boot.log on success:** `MCP: pkt 0x01 received (5 bytes)` →
`MCP: pkt 0x19 received (52 bytes)` (CopyCharListToGlobal) → the realm
character list renders the "Test" character → char select → charlogon (0x04) →
`MCP: connection established=1` → D2GS join. If anything else stalls here, the
MCP marks + the crash-map workflow close it.

## R200 field result + R201 — Open Battle.net PLAYABLE end-to-end; closed-realm MCP EOF is server-side (2026-11-22)

**R200 field log — Open Battle.net is fully playable.** Second connect in the log:
auth perfect → `CharSelect: screen shown (mode=3)` → chat enter + channel list
(0x0b/0x0a, channel "Diablo II-1") → news (0x46 ×2, 335/346 B) → chat ops (0x15/0x21)
→ `enter-world: qserver start (gametype=6)` → `D2GS: ConnectToServer
192.168.56.1:4000 → 0` → `c2s 0x67 create-game` → SP create-game → DRLG act
generated → **character "TestMage" loaded** → 0x5B playerjoin → **~2.5 minutes of
real gameplay** (quest gossip: kashya/akara, NPC menus 0x27/0x31, in-game packets,
unlock-fps 61/s) → clean exit to charselect → 0x0b/0x0a again → exit. Connect →
charlist → **create game → play**: the Open Battle.net path works end-to-end.

**Closed realm (Battle.net button) — new signature, server-side:** auth perfect →
LOGONREALM answered → MCP socket created **and connected** (R199 revert confirmed) →
`MCP: recv failed/EOF: bytes=0 socketStatus=4 wsaErr=0` at +16 ms — **d2cs closed the
fresh connection before any client data** (no MCP protocol byte, no STARTUP had gone
out; the R200 marks caught it). The R199 pcap already proved the full MCP handshake
works against this same server when the session is fresh (STARTUP accepted, charlist
delivered) — so the instant close is d2cs-side state, prime suspect a **stale d2cs
session** from the earlier run (that session ended with a client FIN mid-charlist;
d2cs may hold the account/IP session and close the new MCP connection on sight).
This side is VTSTech/D2VP-D2GS: check the d2cs log at the close moment / restart
d2cs and retry closed realm before any further client work.

**R201 (`bn-wsaref-quiet`):** the R200 run flooded the log with `wsaref -> -974/-975/…`
(cleanup) lines ~60/s during gameplay — a game-loop site calls the cleanup wrapper
every frame (harmless to the real Winsock refcount; the count is our instrumentation
delta). Cleanup marks are now suppressed except a one-time first-negative-crossing
line (the R179 imbalance tripwire stays armed); create/startup keep their marks.

Sync for R201: `Source/Fog/Src/Safesock.cpp` + `Redivivus/d2_boot.h` only.

## R201 field + R202 — closed-realm LOGINREQ fixed: stack-adjacency luck (2026-11-22)

**The D2GS agent's d2cs log corrected our reading:** `recv failed/EOF bytes=0` did NOT
mean "no client data" — d2cs's own log shows `on_d2cs_initconn` (client connected and
sent), `on_client_loginreq: got bad account name`, `d2cs_conn_destroy` — **the client's
D2CS LOGINREQ arrived and was rejected; d2cs closes without replying**, which our socket
observed as a 0-byte read. Retail works end-to-end on the same config, so: client packet
defect.

**pvpgn ground truth** (pvpgn-server `src/d2cs/d2cs_protocol.h` + `handle_d2cs.cpp`,
fetched verbatim): `t_d2cs_client_header` = size(u16)+type(u8) = 3 bytes;
`t_client_d2cs_loginreq` = header, seqno@3, u1@7, bncs_addr1@11, sessionnum@15,
sessionkey@19, cdkey_id@23, u5@27, clienttag@31, bnversion@35, bncs_addr2@39, u6@43,
secret_hash[5]@47, **account-name string @67** — `on_client_loginreq` reads
`packet_get_str_const(packet, sizeof(t_client_d2cs_loginreq) /* = 67 */, MAX_CHARNAME_LEN)`.

Our LOGINREQ shape matches pvpgn exactly — the R199 pcap packet carries "test\0" at
offset 67 and d2cs **accepted it then** (LOGINREPLY 0 + charlist). The R201 rejection
means the bytes changed. **The defect (`bn-mcp-loginreq R202`):**
`NET_MCP_CLIENT_Send_0x01_StartUp` sent `pStrLen+0x41` bytes starting at
`&mcp0x01.nPacketId` — the 65-byte struct **plus whatever sat behind it on the stack**.
The account string lands in the packet only when the compiler places `payloadBuffer`
(where `STRING_GetStringLength` copied the name) immediately behind `mcp0x01`. The
R199-era build had that adjacency by luck; the R200/R201 builds don't — d2cs read stack
garbage as the account name → reject → close. (The struct even declares
`char BnetName[0]` at 0x41 — the reconstruction knew the string must follow; the code
just never guaranteed it.)

**R202 fix:** build one contiguous buffer — `memcpy(aLoginReqBuf, &mcp0x01, 0x41)` +
`memcpy(aLoginReqBuf + 0x41, payloadBuffer, pStrLen)` — and send exactly
`pStrLen + 0x41` bytes; new mark `MCP: 0x01 LOGINREQ sending len=%u account='%s'`.

**Expected d2boot.log on success:** `MCP: 0x01 LOGINREQ sending len=70 account='test'`
→ `MCP: pkt 0x01 received (5 bytes)` (LOGINREPLY 0) → `MCP: pkt 0x19 received` → the
realm character list renders → charlogon → realm game join. If the mark ever prints
`account='<EMPTY>'`, the 0x3E state copy is the next suspect (state+0x48 should hold the
account name bnetd embedded in the reply).

## R202 field + R203 — LOGINREQ accepted server-side; the missing CHARLISTREQ trigger (2026-11-22)

**R202 confirmed by the D2GS agent's d2cs log:** account `test` accepted three times
(02:12:39, 02:13:04, 02:13:40) — no more "bad account name". The contiguous-buffer
LOGINREQ fix works.

**The last gap:** after LOGINREPLY (the `MCP: recv 7 bytes` in the log), D2VP sends
**nothing** — no MCP CHARLISTREQ (0x19). d2cs has **no auto-push charlist** (pvpgn
`handle_d2cs.cpp`: charlists only go out for 0x17/0x19 requests) and requires
`conn_state_authed` (set by the login reply) before accepting one — so d2cs correctly
goes quiet, and the realm charselect sits at "Connecting". The agent's proof: d2cs logs
zero charlistreqs for D2VP sessions vs retail sessions.

**The user's charcreate detour proved everything else works:** create `test` → 0x14
"already exists" (correct!), create `testt` → ok, CHARLOGINREQ 0x07 (authed),
CREATEGAMEREQ 0x03 (game "Test" created on the D2GS), JOINGAME 0x04 (GS IP translated
to 192.168.0.14), joined, loaded the act, played, exited clean, char saved to d2dbs.
Closed realm is functionally complete except the charlist trigger.

**R203 (`bn-mcp-charlistreq`) — wire formats per the agent (big-endian d2cs):**
- CHARLISTREQ_110 (what 1.14d retail sends): h(size u16 + type u8) + u16 maxchar +
  u16 u1 = **7 bytes**; send right after LOGINREPLY result==0. (Legacy 0x17 also works.)
- CHARLISTREPLY_110: h + u16 maxchar + u16 currchar + u16 u1 + u16 currchar2 = 11 bytes
  fixed, then per char: NUL-terminated name + NUL-terminated portrait block.
- LOGINREQ: 67 fixed + account+NUL — our len=70 + 2 size = 72 exact; **do not touch**.
- CREATECHARREPLY results: 0x00 ok, 0x14 already exists, 0x15 name reject.

**Changes:**
1. `NET_MCP_CLIENT_ReadAndParsePacket` — after `HandleIncomingPacket`, on a queue frame
   `[0]==0x01` (LOGINREPLY payload after the size prefix), send the CHARLISTREQ once per
   MCP session (`s_bMCPCharListRequested`, reset in `MCPCONNECT_CreateThread`). The
   builder `NET_MCP_CLIENT_Send_0x19_CharList2` already existed and produces exactly the
   7-byte request (`19 08 00 00 00` + size prefix) — it was simply never called.
2. `McpConnectionLoop` — the login-wait loop now pumps the queue via
   `ReadAndParsePacket` and breaks once the CHARLISTREQ is out; the MCP thread then
   exits, `PollMCPThreadStatus` proceeds, and the realm charselect pumps consume the
   CHARLISTREPLY (`Incoming0x19` → `CopyCharListToGlobal`). Post-loop safety send covers
   the read-thread event path.
3. `MAINMENU_PollMCPThreadStatusPump` — calls `ReadAndParsePacket` while the "Connecting"
   wait-pump runs (it previously only polled the thread handle; nothing dequeued).
4. Dispatch-path note: the R200 per-packet dispatch marks (`MCP: pkt 0x%02x received`)
   never printed in either field log, while `Incoming0x02` demonstrably ran — packets
   reach the full handler table via the **enqueue → pump → ReadAndParsePacket** path, not
   the read-thread Dispatch hop. R203 deliberately routes the trigger through the proven
   path; the Dispatch question stays open (harmless while the pump path works).

**Expected d2boot.log on success:** `MCP: 0x01 LOGINREQ sending len=70 account='test'`
→ `MCP: recv 7 bytes` → `MCP: LOGINREPLY result=0 -> sending CHARLISTREQ (0x19)` →
(the MCP thread exits, "Connecting" releases) → realm charselect renders the saved
characters → charlogon → realm game join. Sync: `McpConnect.cpp`, `MainMenus.cpp`,
`d2_boot.h`.

## R203 field — CLOSED REALM: FULL VICTORY (2026-11-22)

The R203 build completed the entire closed-realm flow in one session, twice, plus a
char creation, against VTSTech/D2VP-D2GS:

```
LOGINREQ(70B 'test') -> LOGINREPLY result=0 -> CHARLISTREQ(0x19) -> charlist 98B received
-> CHARLOGON 'Test' result 0 -> CREATEGAME 'Test' result 0 token 65537
-> JOINGAME -> GS IP 192.168.0.14 -> D2GS :4000 connected -> STARTACT
-> playerjoin 'Test' -> gameplay -> clean exit
-> re-login -> second realm session (charlogon again) -> CHARCREATE 'VTSTech' ok
-> clean shutdown (BNSEND_Disconnect + mode loop exit)
```

The full fix chain, R169→R203: bn-srp-context + bn-read-offset + bn-authcheck-bound +
bn-nth-failstr (R169), CheckRevision bypass (R190), handler-loop NULL-recv arithmetic
(R196, the 10053 root cause — AFD "Invalid buffer on fastio receive"), Sha1_Pump
self-clobber rewrite + Append counter UB (R198), thread-connect deferral revert + socket
options restore (R199), MCP dispatch contract + HandleIncomingPacket chain (R200),
wsaref quiet (R201), LOGINREQ contiguous buffer (R202, d2cs-accepted), CHARLISTREQ
trigger + queue pumping in the connecting wait (R203). 34 fixes across 9 days; every
one field-verified.

**One cosmetic note:** a hangwatch stall dump fired during the game-exit → realm
charselect transition (no main-loop beat for 12.2s). The main thread was alive throughout
(it logged LOGONREALM/charlogon/chat lines inside the window) — the beat counter just
isn't fed during mode transitions. False positive, process continued normally; worth a
beat-feed touch during transitions someday, not urgent.

**Closed.** The original R169 audit's "no Battle.net connection could ever complete"
rebuild now: authenticates (real CD-key hashes, accepted versioncheck), logs on, lists
realms, connects d2cs, logs in to the realm, lists characters, creates characters,
creates games on the D2GS, joins and plays them, and shuts down cleanly — on both the
closed-realm and Open Battle.net paths.
