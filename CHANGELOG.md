# Changelog — D2LoD Vanilla+ (`d2vp.exe`)

**For humans — one short entry per shipped release; keep it that way.** The
engineering-level detail (root causes, gate counts, merge resolutions, verification
digests) lives in [`handoff.md`](handoff.md) (current round journal) and
[`HISTORY.md`](HISTORY.md) (verbatim archive) — the full pre-restructure entries are
archived in [HISTORY.md](HISTORY.md) **H41**. Working rounds between releases carry
build self-IDs `Vanilla+0.0.x-Ryy`; the first `d2boot.log` line pins any shipped exe to
its source round. Per-tag notes: the
[releases page](https://github.com/VTSTech/D2VP-QoL/releases).

- **0.0.9-R203 — Battle.net works: closed realms (PvPGN/D2GS) and Open Battle.net fully
  playable.** Character list, character creation/login, game creation and game join all
  field-verified against VTSTech/D2VP-D2GS; the whole fix chain is R169–R203. The
  individual rounds below shipped as working builds during the campaign.

- **R241 — gamble Refresh on the Act 4/5 vendors, behind an ini gate** (Jamella (0x195) and
  Larzuk (0x200/0x202) complete the six vendors `UseMenu`'s gamble-open accepts, and the whole
  feature now needs `[features] gamble_refresh=1` in `d2vp.ini` — default 0 = retail. It is
  **Options row 18 ("Gamble Refresh")**, filling column 1's band 8 so the page is two full
  columns of nine with no geometry change. Both ends poll the same file on the same 2 s cadence —
  `GAMBLEUI_FeatureActive` builds the button, and the server re-polls in
  `SUNITNPC_GambleRefreshEnabled` and refuses the re-roll when unset, so a stock ini cannot reach
  it even if a packet arrives. On a realm the D2GS needs the key in its own `d2vp.ini`. Acts 1-3
  field-confirmed 2026-11-26; Acts 4/5 and the row itself awaiting a field run.)

- **R240 — Act 4/5 start crash fixed** (a character created with `-act4`/`-act5` crashed the
  client on the first rendered frame: `0xC0000005`, read of `0x14`, in `DrawFloorTile` ←
  `DRAW_DrawEdgeFloorTile` ← `DrawFloorByViewAndFloor`. `D2DrlgStrc::sTileCache` is left
  all-zero for acts 3 and 4 — `InitCache` fills only acts 0-2 and no town DT1 is loaded for
  them — so the cached `pTileLibraryEntry` is NULL. Retail never reads that cache; its only
  consumer is the fork's wide-stop edge-void fill, which is why the NULL stayed invisible
  until the R117/R121 band fill reached it. `DrawFloorTile` now skips a tile with no library
  entry, i.e. acts 3/4 get no edge fill, exactly as retail at 640x480. Detail: HISTORY H84.)

- **R239 — Refresh on the Act 3 gamble panel** (the owner asked for Act 3 next: "R239 Act3
  0xfe". Alkor joins Gheed (0x93, R237) and Elzix (0xc7, R238) in
  `INV_IsGambleRefreshNpc` — one predicate, one line per vendor, no other change. All three
  gambling vendors now show Buy | Sell | Refresh | Close and re-roll all 14 offerings.
  Field-confirmed 2026-11-26.)

- **R238 — a Refresh button on the gamble panel** (R237 added it for Gheed (0x93); the owner
  then asked for "the same for the Act 2 NPC Elzix 0xc7", which R238 adds by generalising the
  class predicate to `INV_IsGambleRefreshNpc`. The gamble panel's 3rd slot is blank in retail;
  it now carries a Refresh button that re-rolls all 14 offerings. Retail only ever re-rolls the
  single slot a purchase frees, and a second gamble-open is a no-op because the server refuses
  to regenerate while the player still owns a proxy-inventory node — so the button sends the
  existing gamble-open packet with a magic params dword and the server drops the node,
  regenerates and pushes the deltas. Field-confirmed 2026-11-26.)

- **R236 — closed-realm account creation fixed** (creating an account on a PvPGN
  realm always answered "Account name contains illegal characters". The SID 0x3D
  `CREATEACCOUNT2` sender built its packet from a bare `D2XSHA1Strc` local and relied on
  the username buffer happening to sit immediately after it on the stack; the compiler
  put other locals in between, so the wire packet carried `[sha1][1A 00 00 00][05 00]`
  instead of `[sha1]"name\0"` and the server parsed control bytes as the name. Hash and
  username are now one struct, as in the 0x31/0x06 senders. Field evidence:
  `d2vp-illegal-acct.pcapng` — request `ff 3d 1e 00 …`, reply `ff 3d 08 00 02 00 00 00`.)

- **R205 — shared stash realm wire (configurable toggle id + pad magic)** (the D2GS port
  showed our `0x4F` toggle arriving byte-exact but being dropped by `d2server.dll`'s
  closed-source cheat validator, which whitelists retail button ids per opcode — before
  D2Game ever sees it. The toggle's button id and a marker in the packet's pad word are
  now ini-configurable on **both** ends (`[features] shared_stash_button`,
  `[features] shared_stash_pad_magic`): realms ride a whitelisted retail id (default
  candidate: 3, trade-accept — inert outside an active trade) with a magic in the pad
  word the validator doesn't inspect, and a genuine retail click of that id (no magic)
  still reaches the retail trade path untouched. Defaults `0x7D`/`0` keep single-player
  byte-identical to R204.)

- **R204 — shared stash works online: per-account pools** (the shared stash was v1
  single-player-only — the server refused the page toggle for every other game type, so
  online the tab flipped its label but nothing moved. Restructured to per-account pools
  per the owner's design: "each player to have their own shared stash, not shared with
  other players, just shared for that one account" — the pool key is the player's
  account name and the pool file is `_d2vp_shared_<account>.sss` on the realm host;
  single-player keeps the legacy global file. Each character materializes its account's
  page into its own stash with its own snapshot, so any number of players can use the
  shared stash simultaneously. Adds a post-save re-activation hook (realm saves fire
  per-player on intervals/waypoints; without it every server save would kick the holder
  back to their personal page). Requires the realm host to run this D2Game build — a
  vanilla d2gs ignores the toggle harmlessly.)

- **R202/R203 — realm login + character list** (d2cs rejected the realm LOGINREQ with
  "got bad account name": the account string was only in the packet when the compiler
  happened to place two stack locals adjacently — now built as one contiguous buffer.
  Then the missing MCP CHARLISTREQ (0x19) after a successful login: d2cs has no
  auto-push charlist, so the realm charselect sat at "Connecting" forever. Both
  d2cs-log-confirmed: login accepted, charlist delivered, realm characters render.)

- **R196/R198 — the two crash-class bugs behind the Battle.net connection death** (the
  "recv 10053 on a healthy connection" root cause was our own packet parser: the
  decompiled "bytes remaining" arithmetic returned NULL, so the second recv ran as
  `recv(sock, NULL, ~163 MB)` and Winsock aborted the socket — recv#1 had worked since
  R169 by luck of the pointer math. Independently, a reconstructed SHA-1 wrote its
  W-schedule over its own loop counter on certain CD-key data, crashing at "Checking
  Versions"; rewritten as clean standard SHA-1 and harness-verified against FIPS test
  vectors. ETW + crash-map symbolization closed both.)

- **R200 — realm packet dispatch actually wired** (the full MCP handler table had zero
  callers and the QSSocket handler was invoked with the wrong argument, so realm packets
  were received and dropped; charlist/charlogon/gamelist handlers now run).

- **R199 — retail-faithful socket connect restored** (the R195 connect-in-read-thread
  experiment was obsoleted by the R196 root cause and silently prevented the realm
  socket from ever connecting; retail's connect-in-Create restored, plus the
  TCP_NODELAY/KEEPALIVE/SNDTIMEO options un-bisected back to retail parity).

- **R190 — CheckRevision bypass: closed-realm auth can complete** (the version check
  required a Blizzard-signed CheckRevision DLL no 1.14d rebuild install carries, so
  SID_AUTH_CHECK was never sent to any server; bypass supplies the 1.14d version id and
  empty statstring, real CD-key hashes still sent — d2cs-accepted).

- **R191–R195, R201 — forensic elimination rounds** (probe batteries, ETW traces, pcap
  forensics and a crash-map symbolization workflow that exonerated Windows Firewall,
  WFP, Defender Web Threat Defense, Winsock LSPs and the 24H2 kernel one layer at a
  time; every diagnostic mark kept in `d2boot.log`).

- **R187/R188 — shatter crumble fixed; issue #39 fully closed** (the final vanish strand
  was the client shatter effect: `MONSTER_SpawnDeathEffectMissiles` set
  `pOwner`/`pMissile` **before** `memset(&sSkillArg, 0, 0x5c)`, erasing them — retail
  0x4afc10 memsets first. Both missiles were rejected (`MSN-FAIL-NOPMSL`), so shatter
  deaths vanished with no crumble. Fixed both paths; field-confirmed playing
  (`MSN-VEL raw=0`, live missiles). The remaining `DROP-SKIP` loot skips were decoded as
  retail-intended: the wrapper's bit-17 check is `UNITFLAG_NOTC` (no-treasure-class, set
  by resurrect AI / Necro revive / special spawns) — revived monsters legitimately drop
  nothing on their next death. Full forensics: `R2/ColdFrozen_R182_IdleRoom_Race_Verified.md`,
  `[CF39]`/`[CF39] MSN-*` marks in d2boot.log.)

- **R184 — cold/frozen vanish closed: field log pins the last case as retail shatter**
  (R182's idle-room race fix landed and the residual "cold monster rarely vanishes on
  melee" case is now field-confirmed as the retail shatter roll — the vanish kill's
  `[CF39] DEATH` line shows `shatter=2048` active, ApplyCold's 20%-per-cold-application
  roll is byte-identical to retail 0x57b0fa, loot drops before any shatter handling.
  R183's [CF39] instrumentation initially read zero because `LOGMANAGER_TraceToModuleLog`
  doesn't write d2boot.log and `D2TraceFmt` is a release no-op; R184 re-points all 15
  marks to `D2BOOT_MarkFmt`.)

- **R182 — issue #39 residual fixed: idle-room race, not a completion bug** (fresh r2
  pass verified the whole death path retail-faithful — RegenAndPsnDecreaser kill tail +
  deferral, KillMonster guards (`killable` bit 15 IS retail `fcn.00457490(classId,0xf)`),
  lifecycle halt, SetUninterruptableState incl. its AITHINK-removal tail, and the retail
  event-queue guard `0x5416f5` the fork already had. The vanish was a race: a mid-cast
  kill strands at 0 HP with `STATE_death_delay` until freeze/cold expiry, but
  `CleanupIdleRooms` untiles the room inside that window once the killer walks away and
  `UNITS_ProcessDeadOrInactiveMonster` — with no death_delay awareness — freed the unit
  silently, no loot. `CleanupIdleRooms` now completes pending deferred deaths before the
  save/free walk; the redundant R166 cold/frozen branch in RegenAndPsnDecreaser was
  removed — r2-verified byte-identical to the general path.)

- **R181 — final probe pair: big buffers and the logging gap** (the R180 field run
  survived all five probes on both attempts with clean probe data while the real
  socket died at the same byte again — every in-process structural delta is now
  exonerated. The last two behavioral deltas get probes: BnetReadFunction's
  0x2000-byte stack receive buffers vs the probes' small ones (large-receive-request
  filter-bug class), and the ~15 ms echo-to-AUTHREQ logging gap vs the probes' ~1 ms.
  If both survive, the discriminator is kernel/image-name level: rename test,
  `netsh wfp capture`, torrent-free run.)

- **R180 — probe-split round: our own SO_TYPE probe was the "anomaly"** (the R179 field
  log cleared the race shape, the blocking shape, handle identity and the Winsock
  refcount in one run — the reuse probe completed auth ON the freed auth handle itself
  while the real socket died on it — and the git record showed the `type=-1/10042`
  driving two rounds of forensics was a probe bug: R176 reverted R175's fix of
  0x1004→0x1008; 0x1004 is SO_RCVLOWAT and fails on every healthy Windows socket.
  R180 fixes the probe, adds the last two structural-delta replays — Create's exact
  socket options, and the real flow's two-thread shape — and preps the kernel-layer
  endgame: exe-rename test, `netsh wfp capture`, torrent-free run.)

- **R179 — private-server BNCS connects skip the fastest-server race** (the R178 field
  run + packet capture settled the 10053 question: the R178 conversation-replay probes,
  started from the same process milliseconds after each dying auth socket, completed the
  whole auth conversation twice while the auth sockets were kernel-RSTed 1.2 ms after
  the AUTHREQ arrived — byte-identical wire, server accepted all five connections
  identically, so the trigger is the auth socket's own kernel state, not the packet
  content and not the D2VP image; Windows Firewall fully off changes nothing). The one
  shape that provably completes auth is the probes': one plain blocking socket. A
  single-entry server list (any private D2GS/PvPGN realm) now connects exactly that way
  — no race, no FIONBIO/select dance; multi-entry gateway lists keep the retail race.
  Two new forensic probes (race-shaped replay + post-Free handle-reuse replay) and a
  Winsock startup/cleanup refcount trail ride along for the case it still dies.

- **R173 — BNCS recv hex dump + race-winner peer port** (the owner's R172 Battle.net log
  finally named the failure: `SID 0x50 AUTH_INFO` sent ok (51 bytes), the peer sent one
  8-byte frame parsed as a 0x25 PING, the echo sent ok, then `recv failed (-1) - WSA err
  10053 -> protocol dead` — all within one millisecond. A real BNCS server answers
  AUTH_INFO before pinging; a ping-then-abort with no AUTH_INFO reply means
  192.168.0.14:6112 is either not a Battle.net service (a Diablo II TCP/IP game host
  also listens on 6112, as do other services) or a middlebox (firewall/AV) aborts the
  flow). Two marks settle which: the first 16 bytes of every fresh-frame BNCS recv are
  hex-dumped (real SID frames start FF <id> <len lo> <len hi>; foreign protocol garbage
  is instantly recognizable), and the fastest-server race winner now logs peer
  IP:port (confirms which service actually accepted on 6112). No behavior changes.

- **R172 — step-by-step d2boot.log tracing for Battle.net and TCP/IP, driven by the R171
  field logs** (owner sent three R171 logs: offline SP and TCP/IP host complete healthy,
  but the Battle.net log died silently — `SID 0x50 AUTH_INFO stage` then nothing for
  ~2.8 s until exit, no 0x05 CLIENTID, no 0x51 AUTH_CHECK — i.e. the death is between
  the 0x50 send and the first reply, on paths that had zero marks). 28 new marks close
  every blind spot on those paths: `SID_SetProtocolDeadFlag` (the flag itself — if it
  fires now, the log says so directly), `NET_SID_CLIENT_Send` (op/len/result, plus the
  refused-send fallback), `NET_SID_CLIENT_WaitForPacketId` (entry, reply-arrived,
  45 s-timeout, protocol-dead-abort), the BNCS recv loop (per-read byte counts, read
  failure + WSA error), `NET_SID_CLIENT_IncomingPacketHandler` (every SID packet
  received), the fastest-server race win point, the BNCS protocol-byte send, the 0x51
  AUTH_CHECK result code, the 0x29/0x3A/0x3D/0x3E login/realm senders, all five
  `g_nBNetConnectionState` transitions, and the UI state machine
  (`SID_SetStateFlag1`/`BNMESSQUEUE_SetStateIfZero`). TCP/IP side: D2GS `connect()`
  result + WSA error, recv-thread lifecycle (running/exit), and the QServer host path
  (listen on port, client accepted). A Battle.net death can now only be silent above
  the socket API itself. No behavior changes; passwords never logged.

- **R171 — d2boot.log now traces the full pre-game journey** (follow-up to the R161 boot
  trace, which stopped at the main loop). 46 new `D2BOOT_Mark`/`D2BOOT_MarkFmt` milestones
  across the four flows a silent-exit report needs: offline character select (main-menu
  Single Player / Create Hero / commit-new-character, char-select screen shown with the
  selection mode, OK → start game), Battle.net (connect click, BNCS connect thread,
  fastest-server race + candidate connect, DNS failure, SID 0x50/0x51 auth stage, 0x40/0x1D
  challenges, 0x29/0x3A logon results, 0x05/0x09 first responses), MCP realm protocol
  (connection/charlogon/create/join/list state setters, 0x02/0x07/0x03/0x04 senders,
  0x03/0x04/0x07 replies with result codes and the D2GS IP) and TCP/IP (host select with
  local IP, join screen, resolve+connect, host connection verified/failed, D2GS connection
  pump and client connection thread). Game names and IPs are logged; passwords are not.
  No behavior changes — the marks are extern-C calls into the existing diagnostic writer.

- **R170 — issue #39 closed: cold/frozen monsters can no longer strand at 0 HP and vanish**
  (owner report after R166: "still noticing some disappearing monsters when cold or frozen";
  r2 verification of Game.exe 1.14d retail in
  [R2/ColdFrozen_DeathPath_r2_Verified_R170.md](R2/ColdFrozen_DeathPath_r2_Verified_R170.md)).
  Binary evidence: retail completes a `STATE_death_delay` kill in exactly one place —
  `SetUninterruptableState(unit, 0)` (retail `0x5544b0`) — and the R166 expiry-callback
  completion was still guarded by an early return while `STATE_uninterruptable` was set.
  For a monster killed mid-cast that guard never lifts in time: `MONSTER_EndOfFrameAiDispatch`
  skips cast modes entirely for frozen-alive units, so the cast-end clear is unreachable
  until unfreeze, and every re-applied freeze/chill keeps pushing the state timeout out —
  the unit sat at 0 HP with the deferred kill pending until a room update dropped it.
  `SUNITDMG_CompleteDeathDelayIfPending` now routes that guard through retail's own
  completion primitive (`SUnit::SetUninterruptableState(pUnit, 0)` — clears the state and
  finishes the pending kill atomically, byte-for-byte the retail order), so sustained
  freeze/chill can no longer strand a deferred death. H2 hardening in the same round: both
  expiry callbacks now drop a stale `STATE_death_delay` when the unit is already dying/dead
  (the retail early return stands; the stale flag no longer survives it). Corrects the R2
  doc address tables in the same pass (`0x0058abc0` is an A5Q3 quest timer, not
  `RegenAndPsnDecreaser`; the sole retail deferral site is `GetHitCalculation` `0x57cf7b`;
  `SetUninterruptableState` pinned at `0x5544b0`). Field ask: freeze/chill kills must always
  drop loot and leave the normal death flow, including mid-cast kills under repeated
  freeze application.

- **R169 — the Battle.net title-screen click crash fixed + private-server gateway tools**
  (owner ask: "Currently, Clicking 'Battle.net' from the Title Screen crashes the game.
  Can we fix that?"). Root cause: the rebuild never calls `D2Net::SRP::SRP_GlobalInit`, so
  the SRP context global was still NULL on the first Battle.net click and `SRP_SetParameters`
  dereferenced it — an instant crash before the connect thread even started. Fixed with a
  create-on-demand in `NET_SID_CLIENT_InitializeSRP` (retail-identical). Three more defects
  on the same connect path fixed in the same round: the Battle.net read thread died right
  after the first server response (a pointer used where the packet loop wants a byte offset
  — no Battle.net connection could ever complete), an unbounded CD-key owner copy in the
  SID_AUTH_CHECK expansion packet, and gateway-list lookup failures returning the gateway
  struct pointer as a string. New tools: `tools/D2VP-SetPrivateGateway.ps1` points the
  Battle.net button at any private server (PvPGN etc., TCP 6112) via the registry gateway
  override + `BNETIP`, and `tools/D2VP-RestoreGateway.ps1` puts real Battle.net back.
  Field ask: the Battle.net click must now show the connect screen and a connect result,
  not a crash.

- **R168 — the retail 30-vs-37 palette-window bug, gated** (owner ask: "lets fix the 30
  vs 37 retail bug, and make it a gated fix in the Options Panel" — on the R167 plan
  `plans/R168-30v37-Palette-Plan.md`). Retail's rolled unique/champion tint roll
  (`nSeedLow % 0x1e + 9`) spans 9..38 while the global RandTransforms table serves only
  rows 8..37, and the create-time clamp (`>= 0x1e`) discarded everything from row 30 up —
  eight fully loaded tint rows never rendered and the roll's top value pointed one row
  past its own table (kept from the draw-time halt only by that clamp). The fix ships
  OFF behind `[features] palette_37` (Options row 17, **Palette 37 Tints**):
  `PALSHIFT37_Active()` (2 s hot-read, cache busted in D2VP_ConfigReloaded) gates the
  roll (`+ 8` — the full 30-row window) and the clamp (`>= 0x26` — a pure safety net);
  gate off = byte-retail. Write-site audit (plan P0.2): Monster.cpp's death-palette path
  and RosterPets' ownership recolors are provably in-window, so the draw-time halt stays
  retail-faithful as the canary. Options page reflows to two columns of NINE (box
  0x191 → 0x1b1, back row 0x184, 640x480 top margin 23 px, col 1 band 8 empty — the R147
  precedent); ini template block ships in the same round.
  **Field-verified 2026-11-18** (owner: "It works!"). A consolidated release-notes file for
  R162 → R168 ships this round too: [RELEASE-NOTES-R162-R168.md](RELEASE-NOTES-R162-R168.md).

- **R167 — issue #34 regression: champion/unique tinting restored** (owner report after
  R166: "Unique Monsters (Regular Champions) are not tinted anymore"). The R165
  create-time neutralizer flagged BOTH explicit tint branches — superuniques.txt Utrans
  AND the MonStats2 byUtrans branch. The byUtrans branch is shared: it overrides the
  rolled unique/champion tint whenever the class carries byUtrans data, and
  champion-eligible classes routinely ship no palshift.dat — so every such champion was
  neutralized back to the untinted base. The neutralizer now covers the superuniques.txt
  branch only (one-off fixed bosses, Griswold among them — still untinted, issue #34
  stays fixed); rolled tints and byUtrans tints resolve through the R49 global
  RandTransforms arm as field-verified since R49. Also adds
  `plans/R168-30v37-Palette-Plan.md`: the full plan for the original retail 30-vs-37
  palette bug (roll `%0x1e+9` yields 9-38, the global table serves 8-37, the create-time
  clamp discards >=30 — rows 30-37 are dead data and the roll value 38 is out of range).

- **R166 — issue #39 residual: chilled/frozen monsters no longer strand at 0 HP** (the
  rare "disappears without loot" after R163): the poison-tick kill for cold/frozen monsters
  no longer runs while the victim is uninterruptable — retail's invariant is that such units
  take the `STATE_death_delay` deferral (`MONSTERAI_ExecuteLifecycleTransition` halts on
  uninterruptable victims), and a chilled monster killed mid-cast was being fed straight
  into that halt path. The deferred death now also **completes**: the freeze/cold expiry
  callbacks finish any stranded `STATE_death_delay` kill
  (`SUNITDMG_CompleteDeathDelayIfPending`), so the unit dies and drops loot at state expiry
  even when the cast-end completion path never runs. Non-uninterruptable cold/frozen kills
  stay immediate (R163 behavior unchanged).

- **R165 — issue #34 "Griswold is Green"** (spawn-time palette fix): Griswold (and any
  one-off boss class whose monster code ships no `palshift.dat`) rendered with a green
  unique tint instead of retail's untinted base palette. Root cause: the R49 global-table
  fallback (issue #9) let **every** entry-less monster resolve the RandTransforms tint
  table, including classes whose tint came from explicit `superuniques.txt` Utrans data
  (Griswold = transform 17) — data retail never renders for those classes because the
  draw-time lookup early-outs for them. The create path now neutralizes explicit-Utrans
  tints back to the untinted base when the class has no palshift entry
  (`PALSHIFT_HasMonsterPalShift` + the `[R165]` guard in `UNIT_CreateMonster`); rolled
  unique/champion tints keep the R49 fallback, entry-bearing classes are unchanged.

## [0.0.8] — 2026-11-06 ([`R128`](https://github.com/VTSTech/D2VP-QoL/releases/tag/R128))

- **R161 — Audit closure** (every remaining safe fix; the ledger at [AUDIT-FIXES.md](AUDIT-FIXES.md)
  resolves all 22 audit findings): crash/boot diagnostics no longer overrun their buffers on long
  report lines and near-260-char install paths (ROB-05/SEC-03); the fork's tripwires run at boot
  instead of dying mid-game (ROB-08); a failed settings write (read-only dir) now plays a distinct
  sound + logs a mark instead of pretending the flip took, and the Windows profile-cache caveat is
  documented (ROB-06); the remaining control-channel commands parse remote input with guards;
  explicit buffer bounds on the item-name suffix and ground labels (ROB-10); the reduced gate suite
  is revived (`tools/check-vanilla-plus.sh`). Deferred items and reasons: the ledger.

- **R160 — Shared-stash data safety** (the audit's two High findings, `VanillaPlusStash.cpp` only): a
  transient read failure (the `.sss` locked by an antivirus scan or a sync client) no longer pins an
  empty session — the toggle is refused with a `d2boot.log` mark and retried on the next click, and
  saves are refused until a read succeeds, so the real stash file can never be overwritten with empty
  state (ROB-01). A page whose items fail to decode (partial materialization) keeps its stored copy
  authoritative — toggling away no longer re-serializes the subset over it (ROB-02). No file-format
  or healthy-path behavior change. Detail: [ROUND-SUMMARY.md](ROUND-SUMMARY.md).

- **R158 — Extended Stats: the character screen's second view** (`[features] extended_stats`, the
  sixteenth Options row — owner ask 2026-11-16): an outlined **Extended** tab flips the character
  screen to a second view listing the twelve stats retail hides — Magic Find, Gold Find, +% XP,
  vendor price reduction, Faster Run/Walk, IAS, FCR, FHR, FBR, light radius, poison length
  reduction, half freeze duration — read live off the statlist every drawn frame. Display-only:
  zero packets, zero server code; the covered +stat buttons never arm, the Done button stays
  clickable, the view resets when the screen closes. Six field-driven rounds (R158.1–R158.6) fixed
  the value getter, the tab position/draw order and the FRW stat id; field-verified 2026-11-16
  ("R158 works. Stats appear accurate"). Per-round detail: [RELEASE-NOTES.md](RELEASE-NOTES.md).

- **R157 — Ctrl+Click transfer covers the Horadric Cube** (`ctrl_click_xfer`, owner ask): with the
  cube panel open the cube owns the direction — inventory/stash items Ctrl+Click INTO the cube,
  cube items OUT to the inventory; the stash views keep the R140 directions; a full cube plays the
  "won't fit" sound. Field-verified 2026-11-16 ("Ctrl+Click working fine for Cube in R157").

- **R156 — Camera Zoom on Glide: the same Z zoom now renders on the Glide renderer**
  (plans/CAMERA-ZOOM-PLAN.md, FEATURES row 0a — owner field-verified the R155 D3D zoom and
  ordered the port 2026-11-11): the R155 D3D site set mirrored into `Glide.cpp` — the two
  zoom helpers (defined up front in the Glide namespace), the two perspective funnels, the
  top-down sprite quads and the two flat subtile builders. Ground tiles ride the shared
  client light grid the R155 round already scales; the cursor inverse and the world-phase
  bracket are renderer-agnostic. DDraw/Gdi still clamp to 1.0 with a `d2boot.log` note.
  Known v1 limit: the Glide lightmap halftone lattice stays at retail screen spots while
  zoomed (soft glow offset — field QA decides if a rebuild rides a later round).

- **R155 — Camera Zoom: Z steps the world zoom 1.0/1.25/1.5, the UI stays 1:1**
  (plans/CAMERA-ZOOM-PLAN.md, FEATURES row 0a, owner ask 2026-11-09 + the four owner calls
  settled 2026-11-11 — in-steps only, Z only, D3D only first, level 0 each join): with
  `[features] camera_zoom=1`, **Z** steps the world zoom 1.0 → 1.25 → 1.5 → 1.0 anchored on
  your character. A world-phase bracket around the `ViewDraw2` call keeps the scaling inside
  the world pass — the bottom bar, panels, tooltips, automap and cursor stay pixel-1:1 — and
  the raw cursor pixel is inverse-zoomed before the retail projection (hover/selection at
  `SELECT_HandleMouseInput`, walk/cast at `ClickMap`), so clicks land exactly where the
  zoomed world shows them. D3D renderer only this round; Glide/DDraw/Gdi clamp to 1.0 with a
  `d2boot.log` note. Gate = the charter (2 s hot-read, Options-page write bridge), the
  fifteenth Options row (**Camera Zoom** — the page re-balances to two columns of eight),
  ini template key, d2boot marks on activation and every step.

- **R154 — Transparent HUD: the bottom bar's background goes fully transparent on ask**
  (FEATURES.md row 4, owner ask 2026-11-11 — "it is just the bottom bar i want to make
  transparent, and even then, not the globes, belt bar or skill buttons", then the XP/stamina
  keep and "fully transparent"; plan at `plans/TRANSPARENT-HUD-PLAN.md`, Round 1 pinned in-tree):
  with `[features] transparent_hud=1` the bar's console frame art (`PANEL_DrawCharacterInfo`'s
  six `CtrlPnl7`/`800CtrlPnl7` `DRAW_SOLID` pieces — the stone behind the globes and belt,
  including the chat area background and the orb sockets/button wells baked into the frame DC6s),
  the run button, the menu button and the hostile indicator are **not drawn at all** — the world
  shows through. Everything functional keeps its vanilla draw and stays interactive: globes
  (+ orb text + gold), belt (all states), skill buttons, XP bar, stamina bar; every panel,
  tooltip, the automap and chat text are untouched; the hidden run/menu buttons keep their hit
  areas and tooltips (invisible is not intangible). Zero renderer code — the skip is four guarded
  call sites in `Panel.cpp`/`Draw.cpp`, upstream of every renderer, so there is no Glide/D3D
  axis and 8-bit DDraw needs no exception. Gate = the charter: missing ini/section/key/garbage
  = 0 = vanilla byte-for-byte; 2 s hot-read busted by the Options-page write bridge; ini template
  key + the fourteenth Options row (**Transparent HUD** — the page stays two columns of seven,
  col 1 fills its last band, geometry unchanged); one `hud-transparent:` d2boot.log mark on
  first activation. Self-ID `Vanilla+0.0.8-R154`. Field QA (Round 3) pending.
- **R153 — the #37 choke-point fix: every client-side item-unit free unlinks first**
  ([#37](https://github.com/VTSTech/D2VP-QoL/issues/37), owner field crash on the R152 build — the SAME
  AV read `[null+0xC]` at `DRAW_UI_InventoryContent+0x9A`, ~5 s after the last page flip; paired-crash
  frame-shift analysis against the owner's `d2exe.map` proved the same 15-frame draw chain on a build
  that carries the R152 unlink): R152 had fixed one free site — the 0x9D REMOVED branch — but the
  client frees item units at ~30 `RemoveObjectFromServerObjectTable` call sites (RELOAD recreate pairs,
  equip/swap recreates, ground deletes), and any of them can free a chain-linked unit into the panel
  draw's inventory-chain walk. R153 unlinks at the choke point instead: one VP block in
  `CUnit::RemoveObject` runs the same self-guarding `INVENTORY_RemoveItemFromInventory` before
  `FreeAny` — no-ops for ground/already-unlinked units, covers every free path at once, behaviorally
  invisible. Ungated. Self-ID `Vanilla+0.0.8-R153`. Field ask §U-80.
- **R152 — the #37 root fix: the page-switch remove storm stops corrupting the client inventory chain**
  ([#37](https://github.com/VTSTech/D2VP-QoL/issues/37), owner field crash on the R150 build:
  "Ctrl+Click deposit crash, this time no withdraw, just finished countess run, tp to town, open stash to
  deposit, ctrl+click potion in inventory. crash" — symbolized against the owner's `d2exe.map`: AV read
  `[null+0xC]` at `DRAW_UI_InventoryContent+0x9A`, same-ms as a `switched to page 7` mark). The page switch
  dematerializes the whole page-4 set with the retail remove echo (`ITEM_RemoveItemFromCurrentLocation`
  stamps `ITEMFLAG_REMOVED` 0x20; page 4 IS the player's own inventory page), and the client's REMOVED
  branch freed each item unit **without unlinking it from the owning inventory's item chain** — retail never
  bulk-removes list-linked grid items, so the latent dangling node never mattered until a hundred-page pager
  flipped dozens of units per second while the panels kept drawing the same chain. The walk then dereferenced
  freed/reused units — the crash, intermittent by pool-reuse luck, deposit or no deposit. R152 unlinks the
  item first (`INVENTORY_RemoveItemFromInventory` — self-guarding, frees the grid cell, relinks head/tail/
  neighbors, no-ops for cursor or unlinked items), captures the GUID before the free (the retail notify read
  it after the free — a second UAF on the same path), and early-returns; the non-REMOVED retail path is
  byte-identical. One build-gate rider: the generated header keeps `INVENTORY_RemoveItemFromInventory`
  module-private to D2Common (no retail cross-module caller existed), so `Redivivus/d2_hand.h` hand
  re-exposes the real, unchanged signature to D2Client — no wrapper, no new code. Self-ID
  `Vanilla+0.0.8-R152`. Field ask §U-79.
- **R151 — the #38 residual pass: the origin delta shared per present, the selection shape + the WW sprite ride**
  ([#38](https://github.com/VTSTech/D2VP-QoL/issues/38), owner field pass on R150: "improved in perspective.
  still a slight intermittent stutter… was unable to reproduce ctrl+click crash either"). The perspective origin
  delta is now computed once per present and shared by every rider (R150's four call sites sampled their own
  clocks — sites straddling a 1 ms boundary disagreed by delta/40 within one frame), and the two raw-projection
  sites left in R150 join the glide: the hover selection shape (`GFX_DrawUnitSelectionShape` — the residual R150
  named) and the Whirlwind rotated-angle sprite (`SKILL_DrawUnitAtRotatedAngle`). Both player-gated; off/interp
  off = byte-for-byte retail. Self-ID `Vanilla+0.0.8-R151`.
- **R150 — the R149 field pass: the deposit crash closed for real, the perspective glide's second half**
  ([#37](https://github.com/VTSTech/D2VP-QoL/issues/37),
  [#38](https://github.com/VTSTech/D2VP-QoL/issues/38), owner field pass on the R149
  build). **#37 — the real window was the round trip, not the park:** the 0x19 pick echo
  hands the item client-side until the 0x18 place echo lands, so a Ctrl+Click in that
  window never reached the R149 guard at all — it took the retail hand-full path
  (place/swap with a mid-flight locked item), the desync that turned the intermittent
  crash deterministic on the deposit leg. The echo no longer hands the transfer's item
  (it stays in its source cell until the place echo relinks it — no cursor hop), and the
  click guard now spans the whole 500 ms round trip. A click inside the window is a
  silent no-op; the next one transfers. **#38 — the character rides its own glide:**
  R149 glided the perspective warp origin, but the character sprite still projected from
  its raw tick-snapped position — it (and the ground under it) seesawed at the tick rate
  while the world scrolled smoothly. The local player's sprite, shadow and state
  overlays now share the origin's glide: the character sits screen-fixed (its exact
  retail spot) while the perspective world glides. Self-ID `Vanilla+0.0.8-R150`.
- **R149 — smoke-test fallout: the fast-Ctrl+Click crash and the perspective jitter**
  ([#37](https://github.com/VTSTech/D2VP-QoL/issues/37),
  [#38](https://github.com/VTSTech/D2VP-QoL/issues/38), owner field pass on the R148
  build). **#37 — Ctrl+Click transfer, very fast clicks:** a second Ctrl+Click inside
  the R141 park window (the ~2 frames a pick+place pair is mid-flight) passed the
  empty-hand test — the pick echo had not drained yet — and sent a second `0x19` the
  server rejects (the hand still holds item 1) while **overwriting the parked `0x18`**:
  a lost place packet, a client/server desync, and the intermittent crash the echo
  handlers ride. A pair is serialized now — `VPCTRLXFER_PairInFlight` makes the click
  branch consume the click while a pair is in flight (a 500 ms staleness cap
  self-clears a stage stranded by a mid-window panel close). **#38 — FPS Interpolation
  in perspective mode still jittery:** re-derivation found the perspective pipeline
  anchors **every** layer (floor tiles, wall tiles, unit sprites, shadows) on the warp
  origin `D2GFX_SetPerspectiveScale` — the player's tick-snapped precision position —
  and the sprite path neutralizes the screen-unit delta by construction, so R143.2's
  perspective "cancel" was the only thing on screen that moved (the character wobbling
  against a 25 Hz stepping world). The perspective camera now **glides itself**: the
  same two-sample alpha rides the warp origin in 16.16 precision units
  (`UNLOCKFPS_GetPlayerRenderOffsetOrigin` → `SetPerspectiveScale`), floor/walls/
  sprites/shadows glide together through the warp they already share, the character
  needs no cancel, and the screen-unit delta (camera, MouseOffsets, the Wall2 cancel)
  is top-down-only. Self-ID `Vanilla+0.0.8-R149`.
- **R148 — Automap clock on one line; the area level joins the level-name line**
  (owner ask: "lets change the Automap clock display to 1 line instead of 2. SESS:
  hh:mm:ss SYS: 12:01PM. Also for the first line, Area Name, Can we also add (aLvl)
  as well?"). The clock overlay is now a single line — **`SESS:`** the session
  stopwatch (`hh:mm:ss`, zero-padded, same per-game anchor as R147) and **`SYS:`**
  the PC's local clock in 12-hour form with a trailing AM/PM (`12:01PM`, `1:05AM`).
  The automap's level-name line gains the area's effective level — `Blood Moor (3)`
  style, the same levels.txt MonLvl/MonLvlEx-per-difficulty lookup the drop code
  uses; always-on (no toggle), plain name in town. Self-ID `Vanilla+0.0.8-R148`.
- **R147 — Automap clock: Session + System time** (owner ask: "Let's add an in game
  clock to the Automap. Session Time and System Time"). With `automap_clock=1` the
  automap overlay gains two lines under the build tag, right-aligned with the other
  overlay text: **Session** (real time since you entered the current game, `H:MM:SS`)
  and **System** (your PC's local clock, `HH:MM` 24-hour). Opt-in (default off), a
  **Automap Clock** row joins the Vanilla+ Options page (the page is two columns of
  seven now, box 0x151→0x171 — still whole at 640x480). The five round-tooling scripts (docs gate, overlay
  pack/verify, smoke sync, round bump, handoff offload) move into `tools/` and
  ship in the overlay. Self-ID `Vanilla+0.0.8-R147`.
- **R146 — Shared Stash: 100 pages** (owner ask: "Good job, works. Let's increase the
  Shared Stash pages to 100" — the "Then 100" leg of the original R63 roadmap). The
  R82 design was built for this: the `.sss` v2 header grows to 812 bytes (one
  size/checksum pair per page), only used pages cost file space, and narrower files
  (pre-R75, pre-R82, ten-page) still load with the missing pages empty and rewrite as
  a hundred on the next save. Labels `Shared 1`…`Shared 100`; a hundred-page file on
  an older build is rejected whole (file untouched; the old build's save keeps the
  full generation in `.bak`). Same `<<`/`>>` pager, one page per click. Self-ID
  `Vanilla+0.0.8-R146`.
- **R145 — Shared Stash: the pager arrows read `<<` / `>>`** (owner ask: "Can we make
  the navigation label '<<' and '>>' instead of just < >, i find just one is a little
  small to click on"). Same outlined tab, same right-anchored spot, same one-page
  stepping — the glyphs (and with them the click bands, which come from the same
  measured layout) double in width; the composite still grows leftward into the empty
  band above the gold display. Self-ID `Vanilla+0.0.8-R145`.
- **R144 — Vanilla+ Options page: two columns + the interpolation row lands on the
  panel** (owner ask: "Is unlock_fps_interp on the Vanilla+ Options Panel? … Can we
  make the panel two column instead of one?"). The page was one column of eleven rows —
  the box had grown to 497 px and clipped its empty bottom margin at 640x480. It is now
  **two columns of six rows**: labels left, On/Off right-aligned per column,
  per-column hover highlight and hit-testing, Previous Menu full-width below; the box
  is 337 px again and fits every video stop whole, 640x480 included. **`unlock_fps_interp`
  leaves ini-only** and takes the twelfth row, **FPS Interpolation**, directly under
  Unlock FPS — same write bridge as every row (flip → ini write → 2 s cache flush).
  The row reads a missing key as **1** through a new per-row read default
  (`D2VP_GetFeatureToggleDefault`) so the display always matches what the gate acts on:
  on a pre-R143 ini without the key (the template is only written for a missing file,
  never backfilled) it shows On — the truth — and the first click writes a real `0`
  instead of a silent no-op `1`. The R143.2 glide is field-accepted (owner:
  "Ok, I think it looks good now"). Self-ID `Vanilla+0.0.8-R144`.
- **R143 — Unlock FPS: sub-tick interpolation** (plan Phase 2, owner: "lets continue";
  revised **R143.1** after the owner's field report "the interpolation is jittery, looks
  better at 0").
  With `unlock_fps=1` the world now **glides between game ticks at the present rate**:
  the camera, the sprite world offset and everything world-anchored (floor, monsters,
  missiles, items) glide between the last two tick-snapped samples — applied at draw
  time only, nothing written back to the path; the sim stays byte-exact 25 Hz (the
  d2boot presents/ticks pair still proves it). The character keeps its retail screen
  position (the camera cancels the delta), shadows/auras stay glued. Ships under a
  second, ini-only key **`[features] unlock_fps_interp`** (default 1, no Options row;
  `0` = the R142 step-cadence look, hot-read). Self-ID `Vanilla+0.0.8-R143`. **R143.1**:
  the first cut extrapolated a velocity prediction (`pos + screenVelocity × α`); a
  prediction is never exact, so every tick boundary snapped by its own error — the
  reported jitter. The revision interpolates between two real position samples taken at
  each client world update, which is continuous at every tick boundary by construction;
  one frame-shared offset (camera and every sprite ride the same delta), teleports and
  level changes snap instead of gliding. The d2boot mark now reads
  `+ sub-tick interp (two-sample)` — that string identifies the revised build.
  **R143.2** (owner field: "still looks jittery at 1, looks better at 0"): the two-sample
  math was right, the delivery was not — three wiring defects fixed. (1) The snapshot
  ran AFTER the draw, so on tick+present frames the draw read the live post-tick
  position against the previous segment's pair and the picture sawtoothed a full tick
  at every boundary (a sandbox loop-order harness reproduces an 18 px backward kick on
  a 32 px walking step — the exact reported jitter); the snapshot call now runs before
  the draw block. (2) The sprite world offset's delta sat behind the shake early-return
  in `DRAW_UpdateMouseWorldOffsets`, dead in normal play — unit shadows, state overlays,
  on-screen culling and the mouse world-pick stayed tick-locked against the gliding
  camera; the delta now applies before that check. (3) The player-sprite cancel
  double-added in perspective mode; it now subtracts there (MouseOffsets already carried
  the delta into projected coords). Sandbox wiring harness 10/10 green: sawtooth
  reproduction, monotone bounded glide, kick-free stop and turn.

- **R142 — Unlock FPS: presentation-FPS unlock** (owner ask, no issue). With
  **`d2vp.ini [features] unlock_fps`** (default 0, like every feature; the Vanilla+
  Options page's new **"Unlock FPS"** row) the game **presents at ~60 FPS while the
  simulation keeps the exact retail 25 Hz cadence**. The recon found the 25 FPS
  presentation is only a draw gate in the client frame loop — `CLIENT_GameLoopFrame`
  presents one draw per 40 ms tick window — so R142 moves the draw onto a 60 Hz present
  pacer (skip-never-queue: a slow GPU re-anchors, never queues), paces the message-pump
  idle sleep to the same deadline, and drops `D2GFX_EndScene`'s 5 ms anti-spin budget to
  1 ms under the gate. Server ticks, client world updates, movement, animations and
  timers stay byte-exact; vsync untouched. The immediate wins: a 60 Hz cursor,
  hover/tooltip/target-highlight responsiveness and up to ~40 ms less input-to-photon
  latency. `d2boot.log` carries a presents/ticks-per-second pair once per 5 s while the
  gate is on — the field proof the game speed is unchanged. Retail's own
  `GAMETYPE_SINGLEPLAYER_UNCAPPED` QA path was considered and **rejected** — it speeds
  the *server*, a game-speed change. Sub-tick interpolation (world *motion* at 60) is
  the deliberate follow-on; plan at [plans/FPS-UNLOCK-PLAN.md](plans/FPS-UNLOCK-PLAN.md).
  Self-ID `Vanilla+0.0.8-R142`. Field ask: SMOKE **§S** (items 61–64).
- **R141 — Ctrl+Click: the vacated-slot fix** (field catch on R140,
  [#35](https://github.com/VTSTech/D2VP-QoL/issues/35)). The R140 build sent the `0x19`
  pick and `0x18` place back to back; the two echoes drained in one packet batch and the
  source container's client-side grid entry went stale — the emptied slot refused every
  later placement. R141 parks the place packet and releases it one full frame later (the
  timing of a fast manual drag). Field-verified 2026-11-09 — "R141 confirmed", issue
  closed. Self-ID `Vanilla+0.0.8-R141`.
- **R140 — Ctrl+Click stash transfer** (owner ask, no issue). With the stash open,
  **Ctrl+Click** now moves an item — inventory → stash or stash → inventory, one
  click, no dragging; the shared-stash pages work unchanged, and a full target
  plays the "won't fit" sound. Retail wired Ctrl+Click to sell-in-shop only; with
  no shop open the branch was a dead click (no packet, no cursor), so it is
  re-expressed as the real transfer — the same `0x19` pick + `0x18` place pair a
  manual drag sends, with the free-cell finder the cube path uses (the server's
  `0x18` handler rejects a place unless the item is on the cursor, which is why
  the first field build — `0x18` alone — silently did nothing: sound played,
  item stayed). Gated on **`d2vp.ini [features] ctrl_click_xfer`** (default 0,
  like every feature; the Vanilla+ Options page's "Ctrl Click Transfer" row) —
  off is the retail dead click.
- **R139 — On-screen build tag** (owner ask, no issue). The title screen and the
  in-game automap overlay used to show the retail version line ("v 1.14d"); both
  now show the fork build tag — **`D2VP-R139`** — so the exe identifies itself at
  a glance. One define (`Redivivus/d2_boot.h`) feeds the title screen, the automap
  line and the `d2boot.log` self-ID, so every future round bumps a single line.
- **R138 — Windowed 1080p crash fix** ([issue #31](https://github.com/VTSTech/D2VP-QoL/issues/31),
  owner-closed 2026-11-09). Selecting 1920x1080 while running windowed (`-w`) crashed the
  game on the first world frame — fullscreen was fine. The software tile blitters' row-offset
  lookup table was sized for retail's tallest resolution (700 rows) and read past its end at
  the fork's taller stops, handing the wall-tile blitter wild write destinations. The table is
  sized for the 1080p stop now (one constant). Field-verified 2026-11-09
  ("R138 confirmed closes #31").
- **R137 — Proc tooltip fix** ([issue #33](https://github.com/VTSTech/D2VP-QoL/issues/33),
  owner-closed 2026-11-08). Magic/rare items with a "Chance to Cast" affix showed
  "<x>% Chance to Cast Level 0" with no skill name and no event tail. Display-only —
  the items' hidden proc stats were always correct. The tooltip formatter now passes
  the real level and skill name, as vanilla did. Field-verified 2026-11-08
  ("R137 confirmed closes Issue #33").
- **R136 — Countess-chest crash fix** ([issue #29](https://github.com/VTSTech/D2VP-QoL/issues/29),
  owner-closed 2026-11-08). Killing The Countess crashed the game seconds after death,
  as the scripted smoke-to-chest sequence opened her reward chest — the chest's potion
  reward returned a memory address instead of the potion's item code, so every kill
  halted at the first drop. The potion tags are returned by value now; the issue repro
  is the standing regression check. Field-verified 2026-11-08
  ("R136 confirmed closes #29").
- **R130 — the tenth upstream sync** (merge-only; upstream `dd2cbd94..30384f1a`, Ghidra DB → 1970, 6 commits, 1875 files: the module-directory restructure — every Blizzard module under `Source/<module>/`, Storm under `3rdparty/Storm/`, the shared headers under `Source/D2Headers/`, support under `Redivivus/` — plus one static library per module from `project.json` with a module-gate report, and the accessor refile round). Resolved per docs/merge-upstream.md with git rename detection carrying the fork's files into the new layout; the per-module visibility gates' fallout fixed (the UI globals include, two fork accessors re-homed outside module-private gates, a fork-owned D2Sound bridge TU for the boot verdict); the merge tools re-pinned to the new tree. Compile gate of record: all 492 TUs `-fsyntax-only` clean with the exact cross flags; audits green (519/519, include-path/refile-aliasing/placement/branch/visibility clean). Full link + in-game smoke = owner-side.
- **R129 — the ninth upstream sync** (merge-only; upstream `b8e3087f..dd2cbd94`, Ghidra DB → 1969, 6 commits, 366 files: "globals in the modules that own them" — 558 globals moved into owning-module namespaces and file-local globals refiled into their using TUs as statics, plus msvc8 tooling rounds). All 20 conflicted files resolved per docs/merge-upstream.md; 225 fork lines re-spelled for the namespace move; Direct3D's gadwBitMasks re-regen NOT taken (the field-proven literal-mask forms stay, build-independent); the merge-site syntax sweep caught and fixed a party-switch splice; audits green (519/519, sweep 0, include-path/refile-aliasing/placement/branch/visibility clean).
- **R128 — Gheed gamble crash fix** ([issue #32](https://github.com/VTSTech/D2VP-QoL/issues/32),
  owner-closed 2026-11-06). Clicking **Gamble** crashed the game on every round since the
  source regeneration — the gamble screen's per-level item-count lookup read a table that
  no longer sits where the original binary kept it. Fixed with a bounds-checked accessor;
  new `gamble:` lines in `d2boot.log` pin the fix in the field. Field-verified 2026-11-06.
- **R127 — vpdiag retirement.** The R120 wide-view diagnostic probes leave the tree
  whole (the R103 diag-retirement class); zero behavior change — the wide-view fixes
  the probes shipped with stay. `d2boot.log` no longer carries `vpdiag` lines.
- **R126 — Shift-compare shield-slot fix; release line rolls 0.0.7 → 0.0.8.** With both
  arm slots occupied, a shield/quiver hover now compares the off-hand item and a weapon
  hover the primary — the same-kind slot pick retail's own auto-equip applies.
  Field-verified 2026-11-06 ("Great job, works").

## [R106–R125] — the 0.0.7 line's later rounds (shipped as per-round tags)

- **R125 — Shift Item Compare** (slate row 0b): hold **Shift** over an equippable item
  to draw the equipped item's tooltip beside it — inventory, stash, cube, trade screen
  and the vendor (shop stock included). Options-page row 9.
- **R124 — D3D fullscreen boot-crash fix** — the eighth-sync merge split the
  texture-cache node-base spelling and the first texture re-upload dereferenced a
  near-null base. Field-verified 2026-11-06 ("d3d fullscreen works again").
- **R123 — the eighth upstream sync** (Ghidra DB → 1948: 693 unreferenced functions
  defined, module-local symbols refiled into their using modules, Duriel's death no
  longer crashes — upstream-side).
- **R117–R122 — the wide-view rendering campaign** at the wide stops: edge-void band
  fill (R117), BFS room walk extended to hop 4 (R118/R121), reach-unit fix (R119),
  vpdiag instrumentation (R120 — retired R127), fill un-gate + snapshot triggers
  (R121), cull-rect re-fit (R122).
- **R116 — windowed 1366x768 shear fix** (the GDI DIB renderer rode the logical width
  as scanline pitch).
- **R115 — 1920x1080**, the video-options slider's sixth stop (wrapper pairing:
  Glide3xVK 0.4.8).
- **R114 — Glide3xVK 0.4.7** wrapper entry for 1366x768 (wrapper-side, no game change).
- **R113 — esc-menu hotfix** (R112's struct growth broke mouse navigation and label
  writes).
- **R112 — hosted stops stretch to fill the host** (edge-to-edge, cursor mapped back)
  **+ 1366x768 fifth stop**.
- **R110/R111 — D3D 1344x700 fullscreen presentation** (probe-first mode entry,
  host-sized frame).
- **R109 — 1344x700 fourth stop** (retail's dormant mode 3, first exposure).
- **R108 — Mana Burn Fix joins the Vanilla+ Options page** (eighth row).
- **R106/R107 — `mana_burn_fix`**: Mana Burn uniques drained 256x the intended mana
  with melee hits (512x in Hell); the opt-in fix drains the designed 66–100% of
  monster-level damage (33–50% for Nightmare/Hell minions), matching D2R post-2.4.
- Repo housekeeping: repository renamed **`D2VP-QoL`**; the glide wrapper completed its
  spin-out to [Glide3xVK](https://github.com/VTSTech/Glide3xVK).

## [R105] — 2026-11-04

- The sixth upstream sync (Ghidra DB → 1894; merge round).

## [R104] — 2026-11-04

- Per-TU restructure fallout sweep (SCmd roster mark + TU-private VP accessors
  re-homed). The build also carried the fourth/fifth syncs (R99/R102), the unit-sprite
  pan fix under `no_panel_shift` (R100), the Options row (R101) and the diag retirement
  (R103). **The full smoke belt passed on this build**
  ([#28](https://github.com/VTSTech/D2VP-QoL/issues/28)).

## [R103] — 2026-11-04

- Diag retirement: `d2light-diag` + `d2drop-diag` + hooks out; zero behavior change.

## [R102] — 2026-11-04

- The fifth upstream sync (Ghidra DB → 1892, 459 files; merge round).

## [R101] — 2026-11-04

- `no_panel_shift` leaves ini-only: the **No Panel World Shift** row joins the Vanilla+
  Options page (issue #26 round 9).

## [R100] — 2026-11-04

- `no_panel_shift` unit-sprite pan fix: all 14 unit-anchored consumers ride the border
  neutralizer — "no_panel_shift is working as intended now".

## [R98] — 2026-11-03

- The D3D perspective-toggle crash **root-fixed**: the flat-wall builder's `uint8_t`
  subtile iterator wrapped after 12 subtiles and read +4-shifted pseudo-records —
  latent since the original decompilation; widened to `int`.

## [R97] — 2026-11-03

- D3D texture-cache promote guards + re-armed transition probes (the R96 blind spot).

## [R96] — 2026-11-03

- D3D perspective-toggle triage probes (five one-shot `d2boot.log` marks; issue #26
  round 5).

## [R95] — 2026-11-03

- **No Panel World Shift** (`[features] no_panel_shift`): with one panel open at the
  wide layouts the world — terrain, units, effects — stays exactly where it was; the
  panel floats over an unmoved camera instead of the vanilla quarter-screen slide.

## [R94] — 2026-11-03

- Issue #26 round 3: the border-3 world-draw pass + bottom-band cuts ("both panels
  open, no more blackness").

## [R93] — 2026-11-03

- Issue #26 round 2: non-perspective half-cuts in every renderer + the border-3
  scene-list skip.

## [R92] — 2026-11-03

- Issue #26 round 1: window-fraction panel-cull family (single-panel margins at 720p).

## [R91] — 2026-11-03

- Docs + field-verify round: the third sync field-run green; the necro minion
  skeleton fix confirmed ("Skeletons are finally white again", [#8](https://github.com/VTSTech/D2VP-QoL/issues/8)).

## [R90] — 2026-11-03

- The third upstream sync (Ghidra DB → 1855) + R90b build fixes; FIELD-RUN GREEN.

## [R89] — 2026-11-02

- Monster component-model data fix (owner-applied; skeleton palette-ownership
  hardening).

## [R88] — 2026-11-02

- The second upstream sync (Ghidra DB → 1851): the supersession ledger opens —
  upstream wins on the sound-union fix and the R54/R85 gossip hand-tables (retail-exact
  tables recovered upstream); fork root-cause notes stay in code.

## [R87] — 2026-11-02

- The first upstream sync of the line (Ghidra DB 1806 → 1839); field-run green.

## [R86] — 2026-11-02

- The `-nointro` boot switch: no logo videos, straight to the title screen.
  Field-verified ("R86 verified, -nointro works").

## [R85] — 2026-11-02

- A1Q2 Bloodraven turn-in fix ([#22](https://github.com/VTSTech/D2VP-QoL/issues/22),
  owner-closed 2026-11-02).

## [R84] — 2026-11-02

- Item-tooltip zero fix ([#16](https://github.com/VTSTech/D2VP-QoL/issues/16)): "0
  poison damage over 0 seconds" / "+0 Cold Damage" lines now show the real rolled
  values — tooltip-only, the item stats were never wrong.

## [0.0.7] — 2026-11-01/02

The first 0.0.7 build is the R66 content state (tag `R67`); the R67→R77 block shipped
as tag [`R77`](https://github.com/VTSTech/D2VP-QoL/releases/tag/R77); `R78`–`R86` mark
fix rounds, `R87`/`R88` the sync rounds.

### Added

- **Shared Stash** (SP): an outlined "Shared" tab over retail-shaped stash pages every
  character on the PC shares — items and gold, one pool, own `_d2vp_shared.sss` file;
  `.d2s` files stay retail-compatible. Round 1 (one page) R63–R66; outlined label +
  second page + pager arrows R75; **ten pages** R82. Field-proven end to end.
- **Item Level Tooltips** (`item_ilvl`, R55–R59): ` (N)` appended to every item name
  line.
- **Vanilla+ Options page**: every feature also toggles in-game (Esc → Options).
- The smoke checklist ships as an interactive GitHub issue template (R83+).

### Fixed

- Waypoint panel row icons ~120 px above their buttons at 720p
  ([#21](https://github.com/VTSTech/D2VP-QoL/issues/21), R81).
- Video-options 1280x720 label alignment
  ([#23](https://github.com/VTSTech/D2VP-QoL/issues/23), R83).
- Alt-toggle stale selection: walk-clicking away from a latched label picked the item
  up ([#15](https://github.com/VTSTech/D2VP-QoL/issues/15), R58).
- Stash tab geometry: pager arrow over the gold-amount text (R76 nudge, R77 lift).

### Changed

- Upstream syncs to Ghidra 1806; every merge field-run green, and the full SMOKE belt
  passed on the R83 build ([#24](https://github.com/VTSTech/D2VP-QoL/issues/24)).
- `tools/syntax-check.sh` host syntax net added (R71); shipped client renamed
  `d2vp.exe` (R74/R96).

## [0.0.6] — 2026-10-30 ([`D2LoD-Vanilla-Plus-0.0.6`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.6) @ `0eb2655`)

The 720p release: native 1280x720 widescreen; loading-screen freeze fixed; sound
constants repaired; the Vanilla+ Options page visible; inventory, skill flyouts and
dialogs aligned at 720p. Post-release rounds shipped as tags `R47`–`R62`:

- Glide `-3dfx` first-frame crash (R46, [#11](https://github.com/VTSTech/D2VP-QoL/issues/11)).
- Auto Gold Pickup restored (R47, [#10](https://github.com/VTSTech/D2VP-QoL/issues/10)).
- 720p deposit/withdraw gold dialogs (R48, [#12](https://github.com/VTSTech/D2VP-QoL/issues/12)).
- Unique monster palettes (R49, [#9](https://github.com/VTSTech/D2VP-QoL/issues/9)); necro
  minion skeletons gold (R50/R51, [#8](https://github.com/VTSTech/D2VP-QoL/issues/8)).
- 720p belt hover popup (R52, [#14](https://github.com/VTSTech/D2VP-QoL/issues/14)).
- A1Q1 Den of Evil never completed (R54, [#13](https://github.com/VTSTech/D2VP-QoL/issues/13)).
- Alt-toggle stale selection (R58, [#15](https://github.com/VTSTech/D2VP-QoL/issues/15); fixed for 0.0.7).
- Save-and-re-enter crash (R60/R61, [#19](https://github.com/VTSTech/D2VP-QoL/issues/19),
  field-verified "R61 Confirmed fixes #19").
- Upstream to Ghidra 1783 (R62), field-run green + full smoke pass.

## [0.0.5] — 2026-10-29 ([`D2LoD-Vanilla-Plus-0.0.5`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.5))

Per-class player sounds restored (77 values byte-matched from the retail exe);
full-inventory pickup crash and walk-click mis-pickup fixed; **Persistent Globe Text**
(the 0.0.4 globe-hover readout reworked to retail's own always-on above-orb display).

## [0.0.4] — 2026-10-29 ([`D2LoD-Vanilla-Plus-0.0.4`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.4))

Auto Gold Pickup; globe hover values.

## [0.0.3] — 2026-10-28 ([`D2LoD-Vanilla-Plus-0.0.3`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.3))

Portable saves (`[features] portable_save` — saves read/write `<game folder>\save`,
registry untouched); alt-toggle click-through fix; diagnostics quieted (probe
retirement).

## [0.0.2] — 2026-10-28 ([`D2LoD-Vanilla-Plus-0.0.2`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.2))

First opt-in feature: the alt-key item-label toggle (`[features] alt_toggle`), with
self-provisioning `d2vp.ini`.

## [0.0.1] — 2026-10-27 ([`D2LoD-Vanilla-Plus-0.0.1`](https://github.com/VTSTech/D2VP-QoL/releases/tag/D2LoD-Vanilla-Plus-0.0.1))

First public release. Glide-renderer defect series closed (fixes 1–13), item drops
fixed, sound fixed.

## Wrapper tags

`0.3.1`, `0.4.1`–`0.4.8` (the [Glide3xVK](https://github.com/VTSTech/Glide3xVK) repo —
formerly the branch of the same name here) — `glide3x.dll` wrapper versions; the
current release 7z ships wrapper **0.4.8** (0.4.6 knows 1344x700, 0.4.7 adds 1366x768,
0.4.8 adds 1920x1080).
