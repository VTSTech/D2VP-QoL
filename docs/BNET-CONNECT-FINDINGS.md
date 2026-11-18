# BNET-CONNECT-FINDINGS — R169 audit of the Battle.net connect path

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
