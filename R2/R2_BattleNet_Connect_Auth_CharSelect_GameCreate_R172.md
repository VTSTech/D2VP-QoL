# R2 Battle.net Connect / Auth / Character Select / Game Create (R172)

**Date:** 2026-11-20
**Binary:** `Game.exe` 1.14d retail, md5 `2b3600c10ee0d6387675a2f6cf0ebed2`, base `0x400000`.
**Purpose:** the client-side Battle.net path — gateway selection, BNCS connection and
authentication, realm (MCP/D2CS) connection, character list/select/create, and game
create/join — mapped stage by stage with verified addresses.
**Method:** capstone harness phases 10–10g (`scripts/r2/r2_validate10*.py`): string-cluster
mapping, raw-reference resolution, opcode-constant density scan, dispatch-table discovery.
Opcode *names* follow pvpgn's `d2cs_protocol.h` (fetched and checked this session:
`https://raw.githubusercontent.com/pvpgn/pvpgn-server/master/src/common/d2cs_protocol.h`);
the D2 binary's constant usage was verified against it. BNCS SID names in §3 use the
standard community naming (bnetdocs/pvpgn) and are marked accordingly — the binary evidence
is the strings and callback chains, not literal SID enums.

**Key architectural fact:** 1.14d merged the old D2Launch/Battle.net frontend into
`Game.exe` itself. The assert strings name the original modules: `.\Src\CharSel.cpp`
(`0x6d4dbc`), `.\Src\ChatDlg.cpp` (`0x6d5460`), `.\Src\McpConnect.cpp` (`0x6d5af0`),
`.\Src\ComCallback.cpp` (`0x6d5810`), `.\BNetGW.cpp` (`0x6dec6c`), plus the auth module's
function-name strings (`BattleConnectFastest()` `0x6df1c4`, `sFastestConnect()` `0x6df168`).

---

## 0. Module map

| Module (assert string) | Function range (verified ref sites) | Role |
|---|---|---|
| `BNetGW.cpp` | `0x517ed0`, `0x518190`, `0x5182b0`, `0x518921` | gateway/realm list parsing (Realms.bin, bnserver ini) |
| connect/auth (unnamed, ~`0x51a000–0x524000`) | see §2 | BNCS connect, auth state machine, SHA-1, packet send |
| `CharSel.cpp` | `0x437ab0–0x43a730` (14 ref sites) | character list records, select UI, char ops |
| `ChatDlg.cpp` | `0x43e250–0x440d8f` (9 ref sites) | chat dialog |
| `McpConnect.cpp` | `0x449e70` (+ send side `0x44aae0`/`0x44a170`) | D2CS (realm) packet dispatcher |
| D2CS reply processor (unnamed) | `0x44c000–0x44efe0` window | per-opcode reply handling incl. game create/join errors |
| `ComCallback.cpp` | `0x6d5810` (string; refs near `0x6d4xxx` UI) | community/callback glue |

Useful globals seen: char-list head `0x779de8`, char-record pool `0x779dbc`/`0x779dc0`,
UI state `0x7795d4`/`0x7795ec`, D2CS last-packet `0x7992f4`, MCP connection flags
`0x7a061c`/`0x7a0650`/`0x7a066c`/`0x7a0670`, game-connection state `[0x70f234]`,
auth status-text table `0x8822a8`, auth step state `0x882350`/`0x88239c`/`0x8823f0`,
BNCS mutex `0x881c00`.

## 1. Stage 1 — gateway and server selection

| Retail VA | Identity | Evidence |
|---|---|---|
| `0x5182b0` | **Realms.bin reader** | refs `Data\Global\Realms.bin` (`0x6decf0`), "Parsing new gateway list (version %d)" (`0x6dee08`), "Server List Version" (`0x6dee34`) |
| `0x51c0e0` | **bnserver-D2DV.ini parser** | refs `bnserver-D2DV.ini` (`0x6df05c`), "Processing Bnserver.ini file" (`0x6df5c0`) |
| (in `0x518d40`'s callers) | **GetServersList** | error strings "ERROR GetServersList: gethostbyname failed…" (`0x6def98`), "…inet_addr failed to convert IP" (`0x6df020`), "no servers found" (`0x6df104`) |
| `0x51ca90` | **`sFastestConnect()`** | assert-name string `0x6df168` referenced at `0x51caad`; "searching for the fastest server" (`0x6df144`), "   failed to get server list" (`0x6df17c`) |
| `0x51d0a0` | **`BattleConnectFastest()`** | assert-name at `0x51d0a0`; takes mutex `0x881c00`; per-gateway: socket setup (`0x518c70`), "querying gateway %s" (`0x6df19c`), "   connect to: %s" (`0x6df1b0`) |
| `0x517ed0`, `0x518190`, `0x518921` | BNetGW.cpp functions (gateway struct IO) | 5 assert refs |
| registry | gateway persistence | strings "Diablo II Battle.net gateways" `0x6dec8c`, "Override Battle.net gateways" `0x6decac`, "Preferred Realm" `0x6d4c5c`, "Aux Battle.net" `0x6d4164`; .data keys `BATTLENETIP 0x70572f`, `MCPIP 0x70578b`, `REALM 0x705a6b`, `CHARACTER 0x7057cc`(+0x5c stride ×9) |

Flow: Realms.bin → gateway list → fastest-server probe (`BattleConnectFastest`) → server
list connect loop **`0x51cdbb`** (per-server sockets from a 0x108-stride table;
"Connected to server %s" `0x6df0d4` → "Checking for the latest version" `0x6df090` →
"Connected to Battle.net" `0x6df078`).

## 2. Stage 2 — BNCS connection and authentication

### 2.1 Connect status machine
`0x51cdbb` (function `0x51cdbb..0x51d07b`): owns the server table (`[ebp-0x2014]`,
0x108 stride, state field at +0, handle at +4), tears down stale handles via `0x40dbd0`,
and prints the three status strings above. Failure → "Can't connect to Battle.Net. Try
again later." (`0x6d412c`, referenced from UI fn `0x42f74d`).

### 2.2 Auth architecture (registration authority / "BD" module)
1.14d auth is a **registered-callback state machine**, not a flat switch. Handler-pointer
tables live in `.data`:

| Table | Handlers (in chain order) | Notes |
|---|---|---|
| `0x72ffb0` | `0x520c40` | singleton callback |
| `0x72ffc0` | `0x5210a0, 0x520cb0, 0x520f60, 0x520e50` | auth-info/check step chain |
| `0x730020` | `0x520d60, 0x520d70` | logon-ack step: `0x520d70` prints **"logon acknowledged"** (`0x6df7d8`) and advances (`call 0x51a540`) |
| `0x730044` | `0x520fc0, 0x521200, 0x520d50, 0x520da0, 0x520c50` | mixed steps |
| `0x730098` | `0x520dc0` | **status-code dispatcher**: `jmp [idx*4 + 0x520e28]`, 7 cases (0–6); invalid → abort via `0x5204a0` with codes 0xd/0xe/0xf |
| `0x7300a4` | `0x520c60, 0x5211f0, 0x5211d0, 0x5211e0, 0x520fe0` | account step |
| `0x7300f0` | `0x521220, 0x5212d0, 0x5215b0, 0x5216b0, 0x521760, 0x521800, 0x5218a0` | **account-logon → proof chain** |

Key auth functions:
* **Status-text table**: get `0x520c20` (`mov eax, [idx*4 + 0x8822a8]`), set `0x520c30`.
  The `INVALID: AuthCheck/AuthAccountCreate/AuthAccountLogon/AuthAccountLogonProof/
  AuthAccountChange/AuthAccountChangeProof/AuthAccountUpgrade/AuthAccountUpgradeProof`
  strings (`0x6df964`…`0x6dfedc`) are the per-step invalid markers, in the same order as
  the standard SID_AUTH_ACCOUNT* family (SID 0x02/0x29 CHECK, 0x52 CREATE, 0x50 LOGON,
  0x51 LOGONPROOF, 0x54 CHANGE, 0x55 CHANGEPROOF, 0x56 UPGRADE, 0x57 UPGRADEPROOF —
  community naming, not literal in the binary).
* **Account-logon result switch** (`0x5216c9`, inside the `0x7300f0` chain): result 0 →
  continue (`0x521744`), 1 → "Logon failed: Account unknown" (`0x6dfb08`), 5 → "Logon
  requires upgrade" (`0x6dfb28`), else "Logon failed: Unknown result" (`0x6dfb40`) +
  abort `0x5204a0(0x12)`. Proof strings: "Logon Proof success" `0x6dfbc8`, "…Invalid
  password" `0x6dfbdc`, "Server didn't know our password but told us it did!" `0x6dfb80`.
* **Client identity persistence** (`0x520cc0`): on "client acknowledged" (`0x6df7c4`)
  writes registry `HKLM\…\Configuration`: *Registration Version, Registration Authority,
  Client ID, Client Token* (strings `0x6def68/0x6def50/0x6def44/0x6def34`).
* **Crypto**: SHA-1 init `0x523ac0` (IV `0x67452301/0xefcdab89/0x98badcfe/0x10325476/
  0xc3d2e1f0`), block transform `0x523af0`. These back the logon-proof hashes.
* **Version check**: "Failed ExtraWork archive authentication!" (`0x6df70c`) in fn
  `0x51fcb2` — the MPQ vercheck path; "logon acknowledged"/"user has latest version"
  (`0x6df828`) / "User must revert to an older version" (`0x6df8e0`) are its outcomes.
* **Abort helper** `0x5204a0(ecx=status)`: takes BNCS critical section `0x882004`, records
  status at `0x88201c` — the disconnect-with-reason used by every failure branch.
* **Loggers**: `0x51a480` = debug trace printer (assert-name strings go through it),
  `0x518d40` = status message printer (all "Connected…"/"Logon…" strings).

### 2.3 BNCS packet send
`0x51c5c0` = the BNCS packet-queue sender (writes the `0xff` signature at `0x51c5eb`;
**47 request-builder callers** `0x51a480, 0x51a570, 0x51a660, 0x51a740, 0x51a7d0,
0x51a8f0, 0x51ab30, 0x51ac20, 0x51aca0, 0x51adca, 0x51ae10, 0x51aed0, 0x51af20, …` —
one builder per request type; e.g. `0x51aca0` builds a `0x0c`-class packet, sizes it
`strlen+5`, and queues). The two small fns at `0x51ad25`/`0x51adca` are its tail
(signature write `0x51adcd`).

## 3. Stage 3 — realm (MCP/D2CS) connect and character select

### 3.1 Connection
* Realm list / selection UI strings: `%s\UI\CharSelect\RealmSelectbutton` family
  (`0x6d3b4c`, `0x6d3b8c`, `0x6d3ee8`), `Realm: ` (`0x6dcf30`), "Preferred Realm"
  (`0x6d4c5c`).
* **MCP connection restore** `0x441c95`: "Could not restore MCP connection."
  (`0x6d55bc`) — used when the realm socket drops and is re-established (relevant to the
  fork's reconnect work).
* **MCP readiness/status getters**: `0x449b30` (MCP connected, per-caller checks), 
  `0x449c00` (last D2CS reply status), `0x449bfe0`/`0x44bff0` (`0x7a066c`/`0x7a0670`
  flags). CharSel/create code polls these before every request (e.g. `0x435e38`,
  `0x435e45`).

### 3.2 D2CS packet layer (`McpConnect.cpp`)
* **Receive dispatcher `0x449e70`** (`0x449e70..0x449f9e+`): caches the incoming packet
  (heap copy, global `0x7992f4`), reads the opcode byte at packet+0:
  * `0x17` = `D2CS_CLIENT_CHARLISTREPLY` — validates the four words at +1/+3/+5/+7
    (< 0x100 each; assert lines `0x233–0x236` of McpConnect.cpp, abort `0x681e09` on fail);
  * `0x19` = `D2CS_CLIENT_CHARLISTREPLY_110` (next branch `0x449f26`).
  Called from the socket-ready handlers `0x43ac84` (CharSel region) and `0x44b172`/
  `0x44b182` (MCP receive loop `0x44aae0`'s function family).
* **Send wrapper `0x44aae0`**: builds the 9-byte D2CS header — byte 0 = `0x16` (tag),
  dword at +1, dword at +5 — then appends opcode+payload via **`0x44a170`** (string
  append with overflow assert at line `0x26`), sends through `0x6bc8a0` (Storm transport).
  Observed client reply framing (fn `0x43aab0`): opcode@0, word len@3, word count@7,
  payload@9.
* **Reply processor window `0x44c000–0x44efe0`**: per-opcode status/error handling with
  the pvpgn reply codes as literals — char-create "already exists" `0x14` (site
  `0x44cbbf`), create-game "server down" `0x20` (site `0x44dba6`); game-connection state
  machine head `0x44c000` (`[0x70f234] = 8 / 0x10`, transitions `0x462d00` with
  ecx=1/2, D2GS-side calls `0x463dd0`/`0x44bec0`/`0x468740`/`0x468730`).

### 3.3 Character list / select / create (`CharSel.cpp`)
* **Char-list cache** `0x437ab0`: allocates a 0x110-byte node (name @+0, owner @+4 via
  strdup, expiry `GetTickCount()+0x493e0` = 300 000 ms @+8 — a 5-minute cache, next-node
  @+0xc), prepends to head `0x779de8`. Assert lines 0xfa/0xfc.
* **Char record** `0x438ad0` (line 0x414): 0x350-byte record — name @+0 (strcpy cap
  0x100), second string @+0x100, fields @+0x340/0x344/0x348 from args.
* **Char-list reply parser** `0x43a460` (line 0xbaf): fills record (name 0x100 cap,
  stats string @+0x100, zeroed sum area @+0x300–0x310, charsum byte @+0x320 from arg
  [+0x14], extra word fields @+0x322–0x330). The charlist walker `0x43aab0` reads the
  reply count (dword at payload start for opcode `0x19`), then iterates blocks via
  `0x43a460`.
* **List teardown** `0x438ea0`: frees every node (pool `0x779dbc`, next @+0x34c) and
  clears `0x779dc0`-family globals.
* **Char create** `0x435e10` ("Error from MCP trying to create '%s' (err=%d)" at
  `0x435f42`): checks MCP connected (`0x449b30`) + last status (`0x449c00`); on status
  `0x14` → "already exists" branch (`0x436017`); on success builds the UI char-entry
  (name copy at `[0x7795d4]+0x3a1`, charsum bit at +0x1ef bit6 → +0x3c2, expansion byte
  +0x1ee, calls charlist parse `0x449df0`), else reports the D2CS error
  (`D2CS_CLIENT_CREATECHARREPLY_*` per pvpgn: `0x00` success, `0x01` failed, `0x14`
  already exists, `0x15` name rejected).
* Character-select UI strings: `%s\ui\CharSelect\password` (`0x6d3b0c` — realm
  char-password dialog), char-record "CHARACTER" registry keys (§1).

### 3.4 D2CS opcode table (names from pvpgn `d2cs_protocol.h`, binary evidence noted)

| Op | pvpgn name (req / reply) | Binary evidence |
|---|---|---|
| `0x01` | LOGINREQ / LOGINREPLY (`0x00` ok, `0x0c` bad pass) | density site in `0x44c000+` window |
| `0x02` | CREATECHARREQ / CREATECHARREPLY (`0x00`/`0x01`/`0x14`/`0x15`) | `0x14` cmp @ `0x435f10`, `0x44cbbf` |
| `0x03` | CREATEGAMEREQ / CREATEGAMEREPLY (`0x1e` invalid name, `0x1f` name exists, `0x20` server down, `0x32` not available) | `0x20` cmp @ `0x44dba6` |
| `0x04` | JOINGAMEREQ / JOINGAMEREPLY (`0x29` bad pass, `0x2a` not exist, `0x2b` full, `0x2c` level limit, …) | density window |
| `0x05` | GAMELISTREQ / GAMELISTREPLY | density window |
| `0x06` | GAMEINFOREQ / GAMEINFOREPLY | density window |
| `0x07` | CHARLOGINREQ / CHARLOGINREPLY (`0x00` ok, `0x01` failed, `0x46` not found, `0x7b` expired) | density window |
| `0x0a` | DELETECHARREQ / DELETECHARREPLY | density window |
| `0x11` | LADDERREQ / LADDERREPLY | (pvpgn only; not sighted this pass) |
| `0x12` | MOTDREQ / MOTDREPLY | (pvpgn only) |
| `0x13` | CANCELCREATEGAME | (pvpgn only) |
| `0x14` | CREATGAMEWAIT (server→client) | (pvpgn only) |
| `0x17` | CHARLISTREQ / CHARLISTREPLY (0x22-byte blocks) | dispatcher `0x449ecc` |
| `0x18` | CONVERTCHARREQ / REPLY | (pvpgn only) |
| `0x19` | CHARLISTREQ_110 / CHARLISTREPLY_110 | dispatcher `0x449f26`, reader `0x43aab0` |

## 4. Stage 4 — game create / join

* Game-list UI: "Up to %d Players" (`0x6d5770`), "Level %d" / "Level %d to %d"
  (`0x6d5784/0x6d5790`), "Elapsed Time: %d:%02d:%02d" (`0x6d57a0`), diff/level keys
  `DIFF_LEVEL` (`0x6d57dc`), `LVL_REST` (`0x6d57cc`), `MAX_PLAYER` (`0x6d57c0`);
  create/join buttons `%s\UI\BIGMENU\creategamebutton` (`0x6d51c8`) etc.
* Reply error codes handled in the `0x44c000–0x44efe0` processor (see §3.4): create →
  `0x1e/0x1f/0x20/0x32`; join → `0x29/0x2a/0x2b/0x2c` + cross-type rejections
  (`0x71/0x73/0x74/0x78/0x79/0x7d` hardcore/ladder mismatches per pvpgn — code literals
  not individually sighted this pass).
* **Connection state machine to the game server**: `0x44c000` (state `[0x70f234] ← 8`,
  flags `0x7a061c/0x7a0650/0x7a0658`, query `0x463dd0`, socket `0x44bec0`,
  capability encode `0x468740/0x468730`, transition `0x462d00` ecx=1) and `0x44c060`
  (`[0x70f234] ← 0x10`, teardown `0x466ff0`, transition ecx=2). The "entering game"
  UI path is `%s\ui\CharSelect\password` → join. "Open server thread shut down"
  (`0x6d5d2c`) is the TCP/IP (open games) counterpart, alongside the QServer module
  (`.\Src\QServer\QServer.cpp` `0x6fc460`) used for open-hosted games.

## 5. Symptom playbook (connect/auth/charselect/gamecreate)

**"Can't connect to Battle.Net"** → failed before/at server connect: fastest-server probe
(`0x51ca90`/`0x51d0a0`) produced no live server (`0x6df104`) — check Realms.bin parse
(`0x5182b0`, version mismatch in "Parsing new gateway list") and bnserver-D2DV.ini
(`0x51c0e0`); UI message site `0x42f74d`.

**Stuck at "Connecting to Battle.net…"/"Checking for the latest version"** → BNCS auth
chain: find the last printed step ("client acknowledged" `0x520d1c` → "logon
acknowledged" `0x520d81`); the abort reason is in the status table `0x8822a8` (see the
"INVALID: …" markers) and the abort code passed to `0x5204a0` (0xd/0xe/0xf/0x12).
Version-check failures surface as "Failed ExtraWork archive authentication!" (`0x51fcb2`)
or "User must revert to an older version".

**Character list empty / stale** → charlist cache is 5 minutes (`0x437ab0`,
+0x493e0 ms); force teardown via `0x438ea0`. If the realm socket drops, the restore path
`0x441c95` ("Could not restore MCP connection") decides whether the UI survives.

**"Error from MCP trying to create 'X'"** → D2CS CREATECHARREPLY != 0 (`0x435e10`):
`0x14` already exists, `0x15` name rejected, `0x01` generic fail. The err value is the
reply status from `0x449c00`.

**Game create hangs** → CREATEGAMEREPLY codes in the `0x44c000+` processor (`0x1f` name
exists, `0x20` server down = D2GS not accepting, `0x32` not available); successful create
moves to the D2GS connection state machine `0x44c000`/`0x44c060` (`[0x70f234]` = 8 → 0x10)
and the "wait" reply `0x14` (pvpgn CREATGAMEWAIT) gates the countdown.

## 6. Open items

1. Enumerate all 47 BNCS request builders (`0x51c5c0` callers) and pair each with its
   SID; the auth-side names would then be fully literal.
2. Map the `0x44c000–0x44efe0` reply-processor window function-by-function (it is
   pointer-called; entry list is sparse) — this would pin every creategame/joingame
   error branch precisely.
3. Decode the 9-byte D2CS header fields at +1/+5 (tag `0x16`, two dwords) against
   pvpgn's msgnum/seqnum model.
4. The `0x72faa0` blob ("English with Korean Chat", "Unknown", "14", then fn-pointer
   array `0x518bc0, 0x51d1c0, 0x51c240, 0x51c290, 0x51c470, 0x51af40, 0x520650, …`) —
   looks like a locale/handler config pair; not decoded.
5. Client side of SID_CHATEVENT (`0x0f`) and chat command routing (`.\UI\chat.cpp`
   `0x6d7288`, ChatDlg.cpp) — out of scope this pass.

---
*All binary claims verified this session (phases 10–10g); opcode names for D2CS replies
cross-checked against pvpgn `d2cs_protocol.h` (URL above). BNCS SID names are community
convention. See also `R2_Game.exe_Diagnostic_Function_Reference_R171.md` for the in-game
side.*
