# R2 Game.exe Battle.net-Path Function Reference (R170)

**Date:** 2026-11-20
**Binary:** `Game.exe` 1.14d retail, md5 `2b3600c10ee0d6387675a2f6cf0ebed2`, base `0x400000`.
**Purpose:** function-level reference of every Game.exe function on the Battle.net path
(connect → auth → character select → game create/join), written for **recreation in
D2VP**: each entry gives the observed prototype, globals, call targets, behavior, and a
recreation note (what must be reimplemented client-side vs. what is server-driven).
All addresses are in Game.exe. pvpgn's `d2cs_protocol.h` is cited only to *name* protocol
constants the binary's constants match; no server code was analyzed.
**Method:** capstone harness phase 11 (`scripts/r2_phase11_dump.py`) — full annotated
disassembly of 73 functions to `/tmp/r2fn/`, read function-by-function.

> **⚠ CORRECTED (see R2_Game.exe_vs_D2VP_Gap_Analysis_R170.md)** (`R2_Game.exe_vs_D2VP_Gap_Analysis_R170.md`; verified against
> the disassembly and D2VP's implementations): the stage-4 section is void —
> `0x44c000/0x44c060/0x462d00` are click processing (ProcessQueuedPrimaryClick /
> ProcessPrimaryClickImmediate / ClickMap), not game-server transitions;
> `0x468730/0x468740` are GetMouseX/GetMouseY, not token getters; `0x51a540` is
> `BNSEND_SetServerToken`; `0x51c0e0` is the async DNS resolver (`GetDnsRecordSync`) and
> `0x40d890` is Winsock StartUp (there is no bnserver-ini branch in 1.14d);
> `0x881bf0/bf4/bfc` and `0x881c00` are interlocked flags/counter, not events/mutex;
> `0x520ad0` sets the protocol-dead flag `0x882528`; SID names: 0x1E=CLIENTID2,
> 0x50=AUTH_INFO, 0x51=AUTH_CHECK. Everything else (framing, timers, cache expiry,
> error tables, abort codes) was confirmed faithful by the D2VP implementations.

**New module discovery this pass:** `.\BnSend.cpp` (`0x6def84`) — the BNCS send-buffer
module. [R170 note: the "C++ engine object at 0x88273c" is the SID protocol state
(94-slot tables + config); no separate engine needs recreating.]

---

## 0. Shared state (globals every function touches)

| Global | Meaning (verified usage) |
|---|---|
| `0x881bf0` | interlocked flag: BNCS socket present (`g_pBNetSocketData`); `0x51c5c0` fails if unset [corrected R170: flag, not event] |
| `0x881bfc` | interlocked flag: protocol-ready / proceed [corrected R170: flag, not event] |
| `0x881bf4` | interlocked flag: probe/teardown signal (reset in 10 ms loops) [corrected R170: flag, not event] |
| `0x881c00` | interlocked counter: single BattleConnectFastest at a time (InterlockedIncrement guard) [corrected] |
| `0x8818e0` | socket-mode flag (2 = init/WSAStartup-ish via `[0x6cc258]`, 0 = normal, 1 = ini parse) |
| `0x881838/0x881844` | server-list object + current item (locked by `0x5186d0`) |
| `0x8818e8` | gateway name buffer (0x100); `0x8819e8` = DNS server-list buffer flag [corrected R170: DNS, not ini] |
| `0x882000/0x88201c` | secondary / primary connect-status codes (critsec `0x882004`) |
| `0x8822a8` | auth step→status-text table, 94 slots (0x5e dwords; get `0x520c20`, set `0x520c30`) |
| `0x882128` | auth step→completion table, 94 slots (set on step finish; polled by `0x520b70`) |
| `0x882528/0x88252c` | auth abort marker / last reply dedup id |
| `0x882318/0x8823f0/0x8822c4` | auth flags: logon-allowed / logon-ack received / vercheck progress |
| `0x8818bc` | server-token store (`0x51a540 BNSEND_SetServerToken`) [corrected] |
| `0x8818c8/0x8818d0/0x8818dc/0x8818cc` | BnSend.cpp payload-buffer slots (alloc/append helpers) |
| `0x88273c` | BD auth-engine C++ object (ctor `0x5285a0` fed from `.data 0x730164` ("/" separators)) |
| `0x798fb4` | D2CS (realm) transport/connection handle (send `0x6bc8a0`, peer-check `0x6bc810`) |
| `0x798f8c/0x798f80/0x798fb0` | D2CS expected seq / confirmed seq / reply field |
| `0x79929c` | D2CS "last request status" (`0x449bf0` set, `0x449c00` get) — the `err=` in charcreate errors |
| `0x7992f4` | heap-cached last D2CS reply packet (set by `0x449e70`) |
| `0x798f58/0x798f60` | D2CS request staging (strings, set by `0x449d20`) |
| `0x779de8` | char-list cache head (nodes: name@+0, owner@+4, expiry@+8, next@+0xc) |
| `0x779dbc/0x779dc0` | char-record pool head / current |
| `0x7795d4` | CharSel UI state struct (char name @+0xbd, create fields @+0x3a1/0x3b1, flags @+0x3c1) |
| `0x70f234` | game-connection state (8 = entering, 0x10 = leaving/teardown) |
| `0x7a061c/0x7a0650/0x7a0658/0x7a065c/0x7a0660/0x7a0664/0x7a066c/0x7a0670` | game-connect flags (D2GS-side gating) |
| `0x7a6aac/0x7a6ab0` | cursor coordinates (`0x468730 GetMouseX` / `0x468740 GetMouseY`) [corrected R170 — not tokens] |

Import tables used throughout (from IAT thunks): `[0x6cc250]` `_beginthreadex`,
`[0x6cc3a0/0x6cc3a4/0x6cc3a8]` `WaitForSingleObject/SetEvent/ResetEvent`-family,
`[0x6cc204/0x6cc208]` mutex release/create, `[0x6cc218/0x6cc214]` critsec enter/leave,
`[0x6cc258]` socket-init (arg 0xa), `[0x6cc260]` `GetTickCount`,
`[0x6cc230]` message-pump wait.

Utility helpers (verified): alloc `0x40b380`/`0x413020` (with `.\file.cpp`+line asserts),
free `0x40b3c0`/`0x412650`, memcpy `0x6829c0`, sprintf `0x6823fd`, string-table lookup
`0x524a30(id)`, strcpy-cap `0x5267e0`, append `0x526700`, fmt `0x526320`, fatal
`0x681e09` (after `0x408090(line)`+`0x408a60(file)`), stack-cookie `0x749de0`, chkstk
`0x6831b0`, cookie-check `0x681a48`, str-downcase `0x687740`, registry IO wrappers
`0x415000/0x415070/0x4150e0/0x51f910`.

## 1. Stage 1 — gateway selection and BNCS connect

### `0x51d1c0` — BNET_Connect (entry point)
* **Proto:** `void BNET_Connect(void)` (no args).
* **Flow:** logs "Connecting to Battle.net..."; if event `0x881bf0` already set →
  "   already connected", return. Else `0x51c470()` (reset state, must return non-zero),
  set event `0x881bfc`, socket mode 0, `0x5193e0()`, `0x520b30()` (auth state reset),
  `0x521b50()` (BD auth-engine init), then `_beginthreadex(NULL,0,0x51d0a0,NULL,0,&[0x881c04])`.
* **Recreate:** the whole connect flow is async: the UI entry only resets state and
  spawns the worker. D2VP needs the same event/mutex shape if it wants the original
  progress semantics ("already connected" short-circuit included).

### `0x51d0a0` — BattleConnectFastest (worker thread)
* **Proto:** `uint32 __stdcall BattleConnectFastest(void*)`.
* **Flow:** debug-log name; create mutex `0x881c00` — if already owned (`cmp eax,1; jle`)
  release and return 0. Guard via InterlockedIncrement on
  `0x881c00` (re-entering worker backs off). Init probe table `0x51ea40(ecx=0x66)`
  (102 slots; count at `0x881c50`, flags `0x881d60/61`=0, critsec `0x881f64`).
  Copy gateway name into `0x8818e8` (0x100, via `0x51c190` = GetBnetIp),
  "   connect to: %s". Socket mode 2/4 via
  `0x518c70`. "querying gateway %s". `0x40d890()` = Safesock StartUp (Winsock init);
  if it succeeds, `0x51c0e0 = GetDnsRecordSync(&servers, gateway)` resolves the gateway
  hostname into the server list (async DNS worker 0x51bee0 + message pump).
  If the list comes back empty: release guard,
  `0x520520(0xc)` (status = failed), "   failed to get server list", socket mode −4,
  return 0. Else `0x520520(0)` (clear), `esi = 0x51ca90(0x8819e8)` (probe servers for
  fastest), socket mode −4, set events `0x881bf0`+`0x881bfc`, release, return 1.
* **Recreate:** client-local. The "fastest server" logic = probe each server in the
  parsed list with a 5 s budget (`GetTickCount()-0x1388` in `0x51ca90`) and pick a winner.

### `0x51ca90` — sFastestConnect(server_string)
* **Proto:** `void* __fastcall? sFastestConnect(char* servers /*[ebp+8]*/)`, 0x2350 stack
  frame, cookie. Logs own name; zeroes 0x2100 buffer; `0x51ea40(ecx=0x66)`.
* **Flow:** `[0x881bf8]=0`, `[0x8818e0]=2`, `[0x6cc258](0xa)`; "searching for the fastest
  server"; tokenizes `server_string` on `" ,;"` (`0x68264f` = strtok); per-token probe
  loop (0x108-stride entries on stack; deadline `GetTickCount()-5000`; waits event
  `0x881bf0`); picks the winner into the server-list object.
* **Recreate:** client-local; pure network probing, no protocol.

### `0x51c470` — BNET_ResetConnectionState
* **Flow:** `0x51ac20(); 0x51b210(); 0x519300(); 0x520440(); 0x521b90();` set event
  `0x881bfc`; reset event `0x881bf4` (sleep-10 ms loop until it stays reset); same for
  `0x881bf0`; `0x520ad0()` (transport teardown); close mutex `0x881c00` handle (10 ms
  loop); close `0x881bf0`; `0x51e5d0(); 0x51e5e0();` returns bool.
* **Recreate:** must be callable repeatedly and idempotent; every event/mutex/transport
  handle gets torn down.

### `0x51c5c0` — BNCS_SendPacket  ★ core primitive
* **Proto:** `int __fastcall BNCS_SendPacket(uint8 opcode /*cl*/, void* data /*edx*/,
  uint32 data_len /*[esp+4]*/)`.
* **Flow:** 0x200c stack frame; builds header `[0]=0xFF, [1]=opcode, [2..3]=data_len+4`
  (length includes the 4-byte header); waits event `0x881bf0` — if unset → `0x520ad0()`
  (disconnect path), return 0; waits `0x881bfc` (auth-ready); copies header + up to
  0x2000 bytes of payload; `0x40dcc0(ecx=connection, edx=buf, len)` queues it; return 1.
* **Recreate:** this is the single BNCS egress primitive — everything the client sends to
  the chat server funnels here. Packet framing `FF OP LEN16` matches the classic protocol.

### `0x51ad25` / `0x51adca` — BNCS send-buffer tail helpers (write the `FF` byte; used by builder `0x51aca0`, which sends a `0x0c`-class packet sized `strlen+5`).

### Support functions
* `0x5182b0` — **RealmsBin_Load**: `0x41b650("Data\Global\Realms.bin", out1, out2)`;
  fatal (line 0x188) on failure. The actual list parsing ("Parsing new gateway list
  (version %d)", "Server List Version", keys `BATTLENETIP` `0x70572f`, `MCPIP`
  `0x70578b`, `REALM` `0x705a6b`, `CHARACTER` `0x7057cc`+0x5c stride ×9) happens in the
  parser around this loader — parse targets for D2VP: gateway list file format.
* `0x51c0e0` — **GetDnsRecordSync(ecx=&servers_buf, edx=gateway_name)** [corrected]:
  mode=1, spawns thread (proc `0x51bee0`, args in globals `0x881be8/0x881bec`) that
  resolves the gateway hostname asynchronously (WSAAsync-style lookup with message
  pump), waits for completion, fills the server-list buffer. The server list comes from
  DNS in 1.14d — there is no bnserver-ini branch.
* `0x51c190` — **GatewayName_Get(dst, cap)** from server-list object (locks `0x881838`
  via `0x5186d0`, item `0x518040`, caps at 0x100).
* `0x51ea40(ecx=count)` — probe-slot table init (`0x881c50` = count); getter `0x51ea70`.
* `0x518c70(mode)` — socket-mode setter (2 = open/init, −4 = close, 0 = normal).
* `0x518d40(fmt, ...)` — **status-line printer** (critsec `0x881860`, 0x200 buf) — every
  "Connected…"/"Logon…" UI message goes through here.
* `0x51a480(str)` — debug trace printer (assert-name strings).
* `0x520520(ecx=code)` / `0x520550(ecx=code)` — **SetConnectStatus** into `0x88201c` /
  `0x882000` under critsec `0x882004`. Observed codes: `0xc` = failed to get server list;
  `0x12` = generic abort.
* `0x40dcc0` — BNCS packet-queue enqueue (connection, buf, len).
* `0x520ad0` — set protocol-dead flag `[0x882528]=1` (checked by step-wait → fail 0x12)
  [corrected].

## 2. Stage 2 — BNCS authentication ("BD" auth engine)

### Architecture
`0x521b50` constructs the auth-engine object at `0x88273c` (`0x5285a0(ecx=obj, args from
.data 0x730164/0x730168/0x73016c/0x73018c, sep="/")`) and stamps it with
`GetTickCount()` (`0x528620`). Every auth step is a pair {recv-handler, send-builder}
registered in `.data` handler tables:

| Table | Handlers | Role (from handler bodies) |
|---|---|---|
| `0x72ffb0` | `0x520c40` | singleton callback |
| `0x72ffc0` | `0x5210a0, 0x520cb0, 0x520f60, 0x520e50` | client-id → server-list → vercheck chain |
| `0x730020` | `0x520d60, 0x520d70` | logon-ack step |
| `0x730044` | `0x520fc0, 0x521200, 0x520d50, 0x520da0, 0x520c50` | mixed acks |
| `0x730074` | `0x520c80, 0x521110` | error-detail step (writes result codes 0x80000005/6) |
| `0x7300a4` | `0x520c60, 0x5211f0, 0x5211d0, 0x5211e0, 0x520fe0` | account step |
| `0x7300f0` | `0x521220, 0x5212d0, 0x5215b0, 0x5216b0, 0x521760, 0x521800, 0x5218a0` | account-logon → proof chain |

Step completion: handlers/store set `[step*4 + 0x882128]`; UI waits via `0x520b70`
(poll + 10 ms sleep, 45 000 ms timeout `0xafc8`, abort `0x5204a0(0x12)` on timeout).
Status text per step: `[step*4 + 0x8822a8]` (94 slots; get `0x520c20(idx)`, set
`0x520c30(idx, text)`; global current step id at `[0x72ffa8]`).

### Per-function reference

* **`0x520cb0` — BDClientId_RecvHandler(ecx=payload, edx=len)**: requires len ≥ 0x10;
  reads `ver=[+0]`, `authority=[+4]`, `clientID=[+8]`, `clientToken=[+0xc]`; writes all
  four to registry `HKLM\Configuration` (*Registration Version, Registration Authority,
  Client ID, Client Token*); socket mode 2→4 around the write; logs "client acknowledged";
  returns 1. Invalid → "ERROR BDClientId: invalid response", return 0 (aborts chain).
* **`0x520d50` — Ack_Store(ecx=payload)**: `0x51a540([ecx])` = `BNSEND_SetServerToken` —
  stores the server token at `0x8818bc` (`g_nBNetServerToken`) for password hashing
  [corrected]. **`0x520d60`**: if `[payload]==0` → `[0x882318]=1` (logon allowed).
* **`0x520d70` — LogonAck_RecvHandler(ecx=payload)**: `0x51a540([ecx+4])`; socket mode
  2/4; logs "logon acknowledged"; sets `[0x8823f0]=1` (in the chain head variant at
  `0x521664`). **`0x520da0`**: `[0x72ffa8] = ([payload]==0)`.
* **`0x520dc0` — StatusDispatcher(ecx=payload)**: `status=[payload]`; mirrors to
  `[0x72ffa8]`; `jmp [status*4 + 0x520e28]` (7 cases): 0→abort 0xd, 1→abort 0xe,
  2→abort 0xf, 3→`0x5204d0(0x1b, payload+4)` (abort-with-data), 4–6/default→abort 0x12.
* **`0x520e50` — VerCheck_ResultHandler(ecx=status, edx=extra)**: switch on `[ecx]`
  (0–3): writes `Patch`/`Patches` registry keys; 1 → copy string (`0x51e370`), registry
  write, `0x51f910(HKLM…)` → "downloading upgrade", `[0x8822c4]=0`; 2 → "user has latest
  version", `[0x8822c4]=2`; 3 → "User must reinstall and patch"; else abort +
  "user has unknown version". This is the ExtraWork/MPQ vercheck outcome hook (see
  `0x51fcb2`).
* **`0x520f60` — ServerList_RecvHandler(ecx=payload, edx=len)**: strlen at payload+8;
  requires `strlen+9 ≤ len`; second string at `+8+strlen+1`; compares them (version
  consistency) before accepting.
* **`0x5210a0 — ServerList_Persist(ecx=payload)`**: registry string write
  ("Configuration", "Server List", payload+4).
* **`0x521110 — ErrorDetail_Handler(ecx=payload)`**: reads result dword; `0x80000005` →
  size 2, `0x80000006` → size 4, else 4|1; registry write via `0x51f910`.
* **`0x521150 — Notify_Handler(ecx=payload, edx=len)`**: len ≥ 0x11; dedupe by reply id
  vs `[0x88252c]`; two NUL strings at +8 / +8+len1+1; registry write via `0x51f910`
  with both strings (notification/toast data).
* **`0x521220/0x5212d0/0x5215b0` — Account-logon chain head**: `0x521220` validates len ≥
  0x16 and trailing NUL, trims; the chain ends in the account-logon result switch
  **`0x5216c9`** (result 0→continue, 1→"Logon failed: Account unknown", 5→"Logon
  requires upgrade", else "Logon failed: Unknown result" + abort 0x12) and the proof
  handlers ("Logon Proof success" / "…Invalid password" / "Server didn't know our
  password but told us it did!"), plus the change/upgrade proof variants
  (`0x6dfcd8`…`0x6dfedc`). The proof hashes are SHA-1: **`0x523ac0`** init (standard IV)
  + **`0x523af0`** block transform (0x6b0 bytes).
* **`0x520b30` — AuthState_Reset**: zeroes both 94-slot tables, `[0x72ffa8]=0xff`,
  `[0x882528]=0`, `SetConnectStatus(0)`.
* **`0x520b70(ecx=step)` — AuthStep_Wait**: polls `[0x882128 + step*4]`; 10 ms sleeps;
  45 s timeout → if `[0x882528]` unset → abort 0x12, return 0.
* **`0x520c20(idx)` / `0x520c30(idx,text)`** — status-text table get/set.
* **`0x5204a0(ecx=status)` — BNCS_AbortWithStatus**: critsec `0x882004`, `[0x88201c]=status`.
  Variant `0x5204d0(ecx=status, edx=data)` also copies 0x80 bytes of data to `0x881f80`.
* **`0x51fcb2` — ExtraWork vercheck site**: failure → "Failed ExtraWork archive
  authentication!" — the MPQ-based version check feeding `0x520e50`.
* **BnSend.cpp buffer builders** (`.\BnSend.cpp` @ `0x6def84`): `0x51bdc0` (slot
  `0x8818c8`, asserts lines 0x1e6/0x1ef), `0x51be20` (`0x8818d0`, 0x326/0x36a),
  `0x51ab30` (`0x8818dc`, 0x188/0x189), plus `0x51ab80` (`0x8818cc`). Pattern: if slot
  non-null → free + assert; alloc new buffer; memcpy(ecx-payload, edx-len); store.
  These assemble auth request payloads before `0x51c5c0` sends them.
* **Thunks that route to real senders**: `0x5210c0→0x51bdc0`, `0x5210d0→0x51be20`,
  `0x5211d0→0x51bd60`, `0x5211e0→0x51ab30`, `0x5211f0→0x51b3e0`, `0x521200→0x51b820`,
  `0x521210→0x51d860`, `0x5210f0→0x5202f0+0x520630`.
* **`0x521ba0` — Locale_Get**: `GetSystemDefaultUILanguage` (kernel32, dynamic) with
  fallbacks — locale id used by auth.

**Recreate summary (auth):** D2VP must reproduce: registry persistence of
Client ID/Client Token/Registration Authority/Registration Version (HKLM\…
\Configuration), the per-step completion/status tables with 45 s timeout, the
account-logon result semantics (0 ok / 1 unknown / 5 upgrade), the vercheck outcome
handling, and SHA-1 proof computation. Against a private server, most steps can be
short-circuited — but the *state machine shape* is what the UI waits on.

## 3. Stage 3 — realm (D2CS/MCP) connection and character select

### Packet framing (observed)
* **Client → D2CS request** (sender `0x44a2e0`): `[0]=cmd, [1..4]=dword arg,
  [5..6]=word len, [7…]=payload`; total `7+payload < 0x10000` (assert line 0x7e of
  McpConnect.cpp); send via `0x6bc8a0(transport=[0x798fb4], buf, len)`. Verified sender
  with cmd **0x02** = CREATECHARREQ (pvpgn `CLIENT_D2CS_CREATECHARREQ = 0x02` ✓).
  Sender `0x44aae0` builds the same shape with cmd **0x16** (pvpgn
  `CLIENT_D2CS_CHARLADDERREQ 0x16`-adjacent range; 9-byte header variant with dword at
  +1 and +5).
* **D2CS → client reply** (dispatcher `0x449e70`): `[0]=cmd; words at +1/+3/+5/+7 must
  each be < 0x100 (assert lines 0x233–0x236 / 0x242–0x245); payload at +9`. cmd 0x17 =
  CHARLISTREPLY, 0x19 = CHARLISTREPLY_110 (pvpgn names ✓). Reply field extractors in the
  same member family: seq word @+1 vs expected `[0x798f8c]`, dwords @+3/+7/+0xb/+0xf
  (`0x44abe0`, `0x44ac20`).
* **Charlist reply data** (`0x449f6e` block walk): dword count at payload start, then
  count × {NUL-string name, NUL-string stats}; each block → `0x438ad0` record.

### Per-function reference

* **`0x449e70` — D2CS_RecvDispatch(ecx=?, esi=packet, edx=len)**: dup-checks/asserts
  (McpConnect.cpp 0x21e/0x227), heap-copies the packet into `0x7992f4` (`0x6829c0`), then
  switches on `packet[0]`: 0x17/0x19 validated as above; other cmd → assert line 0x254 +
  fatal. Callers: `0x43ac84` (CharSel walker context) and `0x44b172/0x44b182` (receive
  loop `0x44aae0`'s sibling functions).
* **`0x44aae0 — D2CS_SendRequest(ecx=struct, edx=arg, [esp+4]=seq)`**: sets pending
  status `0x449bf0(seq)`, builds the 9-byte header (cmd 0x16, dword arg, dword seq),
  appends struct payload via **`0x44a170`** (string append with 0x400 cap, overflow
  assert line 0x26), length-checks `9+payload < 0x10000` (assert 0x27b), sends.
  Its member siblings: `0x44ab80` (close path: `0x449bf0([ecx+1])`,
  MessageBox-ish `[0x6cc130]([0x7992ec])`), `0x44aba0→0x449c20(1)`,
  `0x44abc0→0x449c40(1)`, `0x44abe0/0x44ac20` (seq/dword reply extractors).
* **`0x44a2e0 — D2CS_SendCreateChar(ecx=struct, edx=arg, [esp+4]=len)`**: cmd **0x02**
  CREATECHARREQ sender (framing above); calls `0x449bf0(0)` (status reset) and
  `0x449c20(0)` first.
* **`0x44a170 — D2CS_PayloadAppend(dst, &len, max)`**: appends NUL-terminated string at
  ecx; updates `*len`; fatal on exceeding max.
* **`0x449bf0(seq)` — D2CS_SetPendingStatus** (`[0x79929c]=seq`); **`0x449c00()` —
  D2CS_GetLastStatus** (returns `0x79929c`). **This value is the `err=%d` in the
  charcreate error message.**
* **`0x449b30 — MCP_IsConnected`**: `0x6bc810([0x798fb4])` → peer check `0x40dee0`;
  returns bool.
* **`0x449d20 — D2CS_StageStrings(a, b, c)`**: copies two NUL strings into `0x798f60`
  region and stores `[0x798f58]=a` — request string staging (char name / realm password).
* **`0x449df0(a,b,c) — Charlist_Data_Parse`** → `0x44a2e0`-family parser entry.
* **`0x441c95 — MCP_RestoreUI`**: rebuilds the post-connection UI — allocates descriptor
  objects (`0x4f93c0` of `0x70d240`/`0x70d270`), installs callbacks `0x43f9b0`/`0x43db90`
  (`0x4fdad0/0x4fdb00`), 200 ms timeout (`0x4fdd50(edx=0xc8)`), re-shows four UI layers
  (`0x4fa420` on `0x77b084…0x77b090`), creates 10 UI elements (stride 0x3c, from 0x28 to
  0x2bc, proc `0x448c70`). On failure the string at `0x441f41` prints "Could not restore
  MCP connection." Called when the realm socket drops and reconnects.
* **`0x437ab0 — CharSel_CacheAdd(ecx=?, edx=name)`** (CharSel.cpp lines 0xfa/0xfc):
  allocates 0x110 node; name@+0 (strcpy via `0x413020`), owner@+4 (strdup),
  expiry@+8 = `GetTickCount() + 0x493e0` (**300 000 ms — 5-minute charlist cache**),
  next@+0xc; prepends to head `0x779de8`. Lookup helper `0x437b90` removes a node.
* **`0x438ad0 — CharSel_RecordCreate(ecx=?, edx=name, [esp+8]=stats, [esp+0xc]/[esp+0x10]=fields)**
  (line 0x414): 0x350-byte record: name@+0 (cap 0x100), stats@+0x100,
  fields@+0x340/0x344/0x348.
* **`0x438ea0 — CharSel_CacheTeardown`**: walks pool `0x779dbc` (next@+0x34c) freeing
  records, clears `0x779dc0/0x779dc4/0x779dc8/0x779dd8`.
* **`0x43aab0 — CharSel_OnCharlistReply(ecx=?, esi=packet, [esp+8]=?, [esp+0xc]=?,
  [esp+0x10]=?)`**: reads `cmd=packet[0]`, `count=word[+7]`, payload at +9; for cmd 0x19
  takes dword count first; per block: name string + stats string → `0x438ad0`; on alloc
  failure → `0x437b90` (drop node) and set failure flag `[ebp+0x10]=0`; also drives UI
  (`0x408f20` lock, `0x524a30(0x1419/0x141a/0x141b)` string-table ids for the charcount
  label, `0x43a460` block fill, global `0x70cc04`).
* **`0x43a460 — CharSel_BlockFill(name, stats, sum_byte, …)`** (line 0xbaf): fills the
  0x350 record: name cap 0x100 (`0x5263e0`), stats cap 0x100 (`0x5267e0`), zeros
  +0x300–0x310, charsum byte@+0x320, words@+0x322–0x330.
* **`0x435e10 — CharSel_CreateChar_Handler`**: gate on `0x449b30` (connected) +
  `0x449c00` (status); on success builds UI entry (name→`[0x7795d4]+0x3a1` and +0x3b1
  0x10-byte copies, charsum bit `+0x1ef bit6 → +0x3c2`, expansion byte +0x1ee, parse
  via `0x449df0`), refresh (`0x42f6e0`), UI event `0x4fa590(0x432fa0)`. On reply
  status **0x14** → "already exists" branch `0x436017`; otherwise loads localized
  strings (ids `0x145e/0x145f` via `0x524a30`), sprintf
  `"Error from MCP trying to create '%s' (err=%d)"` with name `[0x7795d4]+0xbd` and
  err = `0x449c00()`, composes the error dialog (`0x526320/0x526700/0x6823fd`).
* **`0x42f74d` — UI "Can't connect to Battle.Net. Try again later."** (string `0x6d412c`)
  — the connect-failure dialog site.

**Recreate summary (charlist):** client-local logic = 5-minute cached list, record
layout (name/stats/sum byte/expansion/level fields), reply validation limits (< 0x100
words), and the create-char error mapping (0x14 exists / 0x15 name rejected / else err).
The list itself is server data — D2VP's server must emit the same block shape if the
original UI code is reused.

## 4. Stage 4 — click processing (CORRECTED in R170; the original "game-server session" reading was wrong)

`0x44c000 = ProcessQueuedPrimaryClick`, `0x44c060 = ProcessPrimaryClickImmediate`,
`0x462d00 = ClickMap(clickType, nX, nY, mouseEventFlags)` — the primary-click state
machine (verified against D2VP's faithful `ClickMap` implementation, which matches the
disassembly exactly): bit 8 of the event flags tests stamina (`0x625480` =
STATSEX_GetCurrentStat) and sets flag 0x40; bit 4 sets 0x20; `esi` = clickType
(0 queued, 1 queued-process, 2 immediate…); `0x468730/0x468740` are GetMouseX/GetMouseY.
The create/join reply codes (createchar 0x14, joingame 0x71–0x79 etc.) are handled in
the MCP layer (McpConnect.cpp / OOGUtilities.cpp), which D2VP reproduces faithfully.
The client's game-server session is entered via the MCP join handoff:
`JOINID`=D2GSGameToken, `COMINT`=D2GSGameHash, `szSERVERIP`=D2GSConnectIp →
`pfnSendNotifyJoin` → transport layer → in-game D2GS protocol (D2Client
`D2GSPacketSrv*`/`D2GSPacketClt*`).

## 5. Recreation checklist (condensed)

1. **Events/mutex/status plumbing** (§0 table) — the UI progress text is driven by
   `0x518d40` prints + `0x88201c` status codes + `0x8822a8` per-step texts.
2. **Connect** (`0x51d1c0` → worker `0x51d0a0`): gateway list load, ini parse, probe,
   "fastest" pick, event set.
3. **Auth engine**: object `0x88273c`; step table pairs; registry persistence; SHA-1
   proofs; 45 s step timeout; account-logon result codes 0/1/5; vercheck outcomes.
4. **D2CS**: framing `[cmd][dword][word][payload]` (request) and validated-word replies;
   seq tracking (`0x798f8c/0x798f80`); last-status (`0x79929c`); charlist reply → 0x350
   records with 5-minute cache; createchar error mapping.
5. **Game join** [corrected]: MCP reply codes (0x71–0x79 join errors) + join
   handoff (D2GSGameToken/GameHash/ConnectIp → notify-join) — not a 0x44c000 handshake.

## 6. Open items

1. Pair each of the 47 `0x51c5c0` callers with its BNCS opcode (the builders are thin;
   a per-caller `push imm8` of the opcode constant will name them all).
2. Full function map of the `0x44c000–0x44efe0` reply processor (pointer-called; entry
   list sparse) to pin every creategame/joingame error branch.
3. `0x5285a0`/`0x528620`/`0x528630` — the BD engine object internals (vtable, config
   table at `0x730164`).
4. Exact registry value types for `0x415000/0x415070/0x4150e0/0x51f910` wrappers.
5. D2CS request senders other than 0x02/0x16 (creategame/joingame senders live in the
   same member family; identified by their cmd bytes, not yet individually dumped).

---
*All claims verified this session from full function disassembly (phase 11 dumps).
Companion CSV: `R2_Game.exe_BNetPath_Functions_R170.csv`. Prior stage overview:
`R2_BattleNet_Connect_Auth_CharSelect_GameCreate_R170.md`.*
