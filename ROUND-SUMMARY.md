# Round summary — R173 (rev 185, 2026-11-20): the R172 field log named the BNet death (recv -1 WSA 10053 right after the 0x25 ping echo, no AUTH_INFO reply) — 2 marks to identify the peer: BNCS recv hex dump + race-winner peer IP:port



## The R172 field log and the R173 follow-up

The R172 marks did their job. The owner's Battle.net log now reads: connection state 0-4
in sequence, race won, protocol byte ok, 0x50 AUTH_INFO sent (51 bytes), then — same
millisecond — the peer sent one 8-byte frame (parsed 0x25 PING), the client echoed it ok,
and the next recv returned -1 with WSA error 10053 (WSAECONNABORTED) -> protocol dead ->
wait aborted -> UI state 18 -> exit.

Two facts matter. First, the 0x50 AUTH_INFO **reply never arrived** — a real BNCS server
answers AUTH_INFO before it pings. Second, the peer aborted the connection immediately
after we sent AUTH_INFO (or a local middlebox did). Both point away from a client bug:
192.168.0.14:6112 is either not a Battle.net service (a Diablo II TCP/IP game host also
listens on 6112 and would accept + close on foreign bytes), or something on the path
(firewall/AV) kills the flow. R173 adds the two marks that settle it: a hex dump of the
first 16 bytes of every fresh-frame BNCS recv (SID frames start `FF <id> <len lo>
<len hi>`; foreign garbage is instantly recognizable) and the race-winner peer IP:port.

## The follow-up ask (field logs, R172)

Owner sent three R171 logs. Offline SP (`d2boot-offline-join.log`) and TCP/IP host
(`d2boot-tcpip-host.log`) complete healthy end to end. The Battle.net log
(`d2boot-d2gs.log`) dies silently: `BNet: SID 0x50 AUTH_INFO stage` at T+609, then
nothing for ~2.8 s until `mode loop exited`. No 0x05 CLIENTID, no 0x51 AUTH_CHECK,
no recv/send marks — the death is between the 0x50 send and the first reply, exactly
where R171 had no instrumentation. The wait aborts through the protocol-dead branch
(not the 45 s timeout), so `SID_SetProtocolDeadFlag` fired somewhere with no mark.

R172 closes every blind spot on those paths (28 marks, 5 files):
- `SID_SetProtocolDeadFlag` itself now logs — the next silent BNet death names itself.
- `NET_SID_CLIENT_Send`: op/len/result per send, plus the refused-send fallback.
- `NET_SID_CLIENT_WaitForPacketId`: entry, reply-arrived, 45 s timeout, dead-abort.
- BNCS recv loop: per-read byte counts; read failure logs the WSA error before dying.
- `NET_SID_CLIENT_IncomingPacketHandler`: every SID packet received (id + size).
- Race win point, BNCS protocol-byte send result, 0x51 AUTH_CHECK result code.
- Login/realm senders 0x29/0x3A/0x3D/0x3E; all five `g_nBNetConnectionState` steps;
  the UI state machine (`SID_SetStateFlag1`, `BNMESSQUEUE_SetStateIfZero`).
- TCP/IP: D2GS `connect()` result + WSA error, recv-thread lifecycle, QServer host
  listen (port) + client-accepted marks (QServerNT path).

Expected next BNet log shape: race won -> protocol byte -> 0x50 sent -> either "recv N
bytes" + "SID pkt 0x51 received" (server answered) or "recv failed (WSA err)" /
"send refused" / "DEAD FLAG set" — the failing step names itself.

## The R171 round (previous ask, table retained)

Owner: "Let's add more entries to d2boot.log covering character select (offline),
connecting to battle.net, login, auth, create/join game. do tcp/ip as well" — the
R161 boot trace stops at the main loop, so a silent exit past the title screen
lands in a log that names nothing. This round extends the same diagnostic writer
(`Redivivus/d2_boot.cpp`, extern-C `D2BOOT_Mark`/`D2BOOT_MarkFmt`) into the four
pre-game flows.

## What shipped (46 marks in 10 files, all `// [d2boot]` annotated)

| Flow | File | Marks |
| --- | --- | --- |
| Offline char select | `D2Launch/MainMenus.cpp` | Single Player pressed; Create Hero pressed; Commit new character (mode=%d, class=%d) |
| | `D2Launch/CharSel.cpp` | screen shown (selection mode + expansion) |
| | `D2Launch/Launcher.cpp` | OK → starting game with character index |
| Battle.net connect | `Bnclient/BnSend.cpp` | ConnectingToBattleNet entered; BNCS connect thread started; DNS resolve failed; fastest-server race (gateway %s); BNCS candidate server connected; SID 0x50 AUTH_INFO stage; SID 0x51 AUTH_CHECK (accepted=%d) |
| Login/auth results | `Bnclient/net_sid.cpp` | SID 0x05 CLIENTID; 0x09 SERVERLIST; 0x40 LOGONCHALLENGE (server token); 0x1D LOGONCHALLENGEEX; 0x29 logon result; 0x3A logon result (status codes 0/1/2/3/6) |
| MCP realm protocol | `D2MCPClient/McpConnect.cpp` | 7 state setters (connection established, charlogon/charcreate/creategame/joingame/list success, D2GS connect ready); 0x02 CHARCREATE / 0x07 CHARLOGON / 0x03 CREATEGAME / 0x04 JOINGAME sent; 0x03/0x04/0x07 replies with result codes + the D2GS IP |
| Create/join handoff | `D2Multi/OOGUtilities.cpp` | D2GS join result; MCP success (GS IP + game name); notify join sent (D2GS handoff); charlogon pump result |
| TCP/IP | `D2Launch/D2Launch.cpp` + `MainMenus.cpp` | host game selected (local IP); TCP/IP options opened; connecting to host %s; host connection verified / join failed; server pump result |
| D2GS connection | `D2Net/Game/Server.cpp`, `D2Net/SRC/Client.cpp` | connection pump (single-player=%d); client connection thread started; ConnectToServer type + IP |

## What was deliberately NOT changed

No behavior: every mark is a call into the existing diagnostic writer, placed at
points the surrounding code already computed the logged value. Game names and IPs
are logged; passwords are never passed to `D2BOOT_Mark*`. The `[d2boot]` marks on
retail-faithful code paths follow the existing convention (see `D2GFX.cpp`,
`Game/Main.cpp`).

## Verification

- All 46 insertions applied through an anchor script that validates every anchor
  is unique before writing (`scripts/r2` sandbox copy; 10 files, 55 lines added).
- `tools/docs_gate.py --round 171`: round identity (boot define, handoff header,
  §1 self-ID, CHANGELOG top bullet, ROUND-SUMMARY title) — see gate output.
- Compile gate of record stays the owner's Win11 MSYS2 build (handoff §8); no
  cross-compiler in this sandbox, so the Win11 build is the syntax check.

## Field-verify asks (owner)

1. Rebuild (R171); first `d2boot.log` line must read `Vanilla+0.0.8-R171`.
2. Walk each flow once and eyeball the log: offline SP (menu → char select → OK
   → in game), Battle.net (click → fastest-server race → auth → realm → char
   select → create/join game → notify join), TCP/IP host and join. A silent exit
   in any of these should now name the last milestone reached.
3. Confirm no log spam on the idle paths (pings/ads deliberately not marked).

## Verification boundary

Sandbox-side: docs gate, anchor-validated edits. The compile + in-game passes are
the owner's (SMOKE belt §V/§W field QA still owed from earlier rounds).
