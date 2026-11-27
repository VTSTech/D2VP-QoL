# Round summary — R241 (rev 191, 2026-11-26): gamble Refresh on all six vendors, behind `[features] gamble_refresh`

## What shipped

Two things. First, Jamella (0x195, Act 4) and Larzuk (0x200/0x202, Act 5) join the predicate, so
every class `UseMenu`'s gamble-open accepts is wired — the owner's log confirmed both panels open
and generate (`gen start npc=0x0195` / `npc=0x0202`, 14 items each).

Second, the feature is now gated. `[features] gamble_refresh` defaults to 0, so a stock `d2vp.ini`
is byte-retail: no button, no re-roll. It is **Options row 18 ("Gamble Refresh")** — the row that
fills column 1's band 8, so the page becomes two full columns of nine and no geometry constant
moves (box `0x1b1`, back row `0x184`, 640x480 top margin 23 px all unchanged). Both ends poll the
same file on the same 2 s cadence: `GAMBLEUI_FeatureActive` (`VanillaPlusCore.cpp`, flushed by
`D2VP_ConfigReloaded`) decides whether the button is built, and `SUNITNPC_GambleRefreshEnabled`
(`SUNITNPC.cpp`) re-polls and refuses the re-roll when unset — so a hand-crafted packet cannot
reach it on a default install. On a realm the D2GS needs the key in its own `d2vp.ini`, same as
`shared_stash`.

## Verification

Acts 1-3 field-confirmed 2026-11-26. Acts 4/5 and the gate itself **await a field run**. Compiles
clean; docs gate green.

---

# Round summary — R240 (rev 190, 2026-11-26): the Act 4/5 start crash — a NULL tile in the fork's own edge-void fill, found by symbolicating the owner's d2exe.map

## What shipped

A `-act4`/`-act5` character crashed the client on the first rendered frame. The owner supplied
`d2exe.map` from the same build, which turned the "own-code fault" into a precise answer:
`DrawFloorTile +0xc6` ← `DRAW_DrawEdgeFloorTile +0xde` ← `DrawFloorByViewAndFloor +0x87b`,
`0xC0000005` reading `0x14` with EAX=0.

`D2DrlgStrc::sTileCache` is left all-zero for acts 3 and 4: `InitCache` (`Drlg.cpp:971`) fills
only acts 0-2, and `DRLGACTMISC_LoadTileProjects` (`:371`) loads no town DT1 for them, so the
cached `pTileLibraryEntry` stays NULL and `pTileOther->nOrientation` reads `[0+0x14]`. Retail
never reads that cache — its only consumer is the fork's R117/R121 edge-void band fill, which
runs only at the wide stops. `DrawFloorTile` now returns early on a NULL entry, so acts 3/4 get
no edge fill, exactly as retail at 640x480.

Teaching `InitCache` about acts 3/4 instead is not viable: `apTileProjects` is empty there, so
the lookup would return 0 and hit the `ERROR_UnrecoverableInternalError_Halt` at `Drlg.cpp:993`.

## Verification

Compiles clean; `docs_gate --round 240` green apart from the handoff size cap (the detail lives
in HISTORY H84). **Awaiting the owner's field run.**

---

# Round summary — R239 (rev 189, 2026-11-26): the gamble Refresh button reached Act 3 — Alkor (0xfe) joins Gheed (0x93) and Elzix (0xc7) behind one predicate, field-confirmed

## What shipped

R239 is a one-line change: `INV_IsGambleRefreshNpc` gains `0xfe`, so Alkor's gamble panel
shows Buy | Sell | Refresh | Close and re-rolls all 14 offerings exactly as Act 1 and Act 2 do.
The predicate is the only place a vendor is named — the button setup, the click branch and the
tooltip all route through it.

The server side is untouched: `SUNITNPC_GambleRefresh` already keys off the magic params dword
in the C→S `0x38` action-2 packet, and `UseMenu`'s gamble branch already accepted class 0xfe.
The owner's log confirms the panel opens and generates (`gen start npc=0x00fe … gen done
items=14`).

## Verification

Owner field test 2026-11-26. `tools/docs_gate.py --round 239` green; both changed TUs compile
clean. The pre-existing repo-wide VP-marker census delta (7) is unchanged.

---

# Round summary — R238 (rev 188, 2026-11-26): the gamble panel's blank 3rd slot became a Refresh button that re-rolls all 14 offerings — Gheed (0x93) in R237, Elzix (0xc7) in R238, field-confirmed

## What shipped

The owner asked for a Refresh button in slot index 2 of Gheed's gamble panel, then for the
same on the Act 2 vendor Elzix. The slot is blank in retail, so the button is new; the
re-roll is not — retail re-rolls only the single slot a purchase frees
(`SUNITNPC_DeductGoldForPurchase` → `SUNITNPC_RefreshNpcItem`), and a second gamble-open is
a no-op because `SUNITNPC_ValidateNpcInteraction` returns early while the player still owns a
proxy-inventory node.

The button sends the existing C→S `0x38` action 2 (open gamble) with a magic value in the
params dword retail always leaves 0 (`D2VP_GAMBLE_REFRESH_PARAM`, `Redivivus/d2_hand.h`).
No new packet id — the C→S dispatch table is a fixed array and both ends ship as one build.
`SUNITNPC_GambleRefresh` then queues the old items as shop-removes, flushes, drops the proxy
node, regenerates, queues the new items as shop-adds, flushes, and drains.

## Three silent failures the field runs exposed

1. **Wrong discriminator.** Keying the click branch on the shop-mode global fell through to
   the retail Repair All path whenever it read 0, and `ShopAction`'s `if (!bValidNpc) return 0;`
   is a silent no-op for a gambling vendor. Now keyed on the NPC class.
2. **Shifted parameter names.** `SUNITPROXY_SendProxyItemsToClient`'s names are off by one —
   the retail caller passes `(pGame, pNpc, pPlayer, pClient)`. Reversed, the
   `pNpc != pUnit` check in `Pre0x9DAnd0x9CPacket` drops every packet. The same swap exists in
   `SUNITPROXY_GetPlayerProxyInventory`.
3. **Adds never queued.** Gamble generation places items with
   `SERVER_ItemToInventory(..., bSendUpdate = 0)`, so they only get a unit-update flag —
   enough when the panel opens, not when re-rolling under an open one.

## Verification

Owner field test 2026-11-26: both panels show Buy | Sell | Refresh | Close and the click
clears and refills all 14 offerings. `tools/check-vanilla-plus.sh` green on the round
identity; the pre-existing marker-census delta (232/225 at R236) is unchanged.

---

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
