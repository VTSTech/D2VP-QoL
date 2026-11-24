// [d2boot] Diagnostic boot trace — TEMPORARY, diagnostic build only.
// Writes d2boot.log next to the executable at every milestone between CRT
// init and the main loop, so a silent exit (no window, no crash report)
// names the exact stage it died at. D2BOOT_Fail additionally pops a
// MessageBox at the known silent-exit paths.
#pragma once

// [R139] visible build tag - one line to bump each round. The title screen
// (D2Launch UIMENU_MainMenu's version text box) and the in-game automap
// version line (D2Client AUTOMAP_DrawVersion) print D2VP_BUILD_TAG instead of
// the retail "v 1.14d", and the boot self-ID (Game/Main.cpp) composes its
// R-number from D2VP_ROUND, so all three stay in lockstep per round.
#define D2VP_ROUND "236" // [R236] CLOSED-REALM ACCOUNT CREATE FIX. Field (pcapng d2vp-illegal-acct): "Create Account" always answered "Account name contains illegal characters"; the capture shows the request `ff 3d 1e 00` (SID_CREATEACCOUNT2) with payload [20-byte sha1][1A 00 00 00][05 00] and the server replying `ff 3d 08 00 02 00 00 00` (status 2). Root cause: NET_SID_CLIENT_Send_0x3D_CreateAccount2 passed `&sha1Password` (a bare 20-byte local) with a length spanning into the NEXT stack slot, relying on szUsername being adjacent - the compiler placed nUsernameLen (0x1A) and nLen (5) there instead, so the username never entered the packet and PvPGN parsed control bytes as the name. Fixed by grouping the hash and the username into one `__frame0_t` (hash at +0, name at +20), same pattern as the 0x31/0x06 senders. [R235] ARGUMENT SHAPE + CRASH ISOLATION. Field (R234, SUCCESS): VPLOG.TXT = `DHX34FD3002802900>0230FD34<4F7D0000000100R` and d2vpstash.log = `HOOK 0x4F pGame=0230fd34 pUnit=025274b5 btn=0x007d page=1` then `REFUSED: pUnit=025274b5 type=146955615`. So the one-dereference theory is CONFIRMED: the frame (btn 0x7D, page 1) is reachable through *(void**)pPacket, and the logger works. But the argument mapping is WRONG: pGame as forwarded == dword0 of the struct == the packet pointer, and pUnit is not a unit. R235 therefore: (a) dumps the first 24 bytes of the dispatch struct as six dwords (`#` + 48 nibbles), (b) writes ECX/EDX exactly as the stub forwarded them (`[` + 16 nibbles) so they can be matched against the struct dwords, (c) dumps 24 bytes at the adopted frame (`T` + 48 nibbles) where the handler's own pGame/pUnit should sit, and (d) the REFUSED line now carries pPacket and all six dwords. New build mode `VPSTASH_113D_PASSTHROUGH`: logs `H` then returns 0 without touching any argument, to prove whether the post-click Fog crash is our code or the stub's replay/return path. Audit answer: the REFUSED path performs only reads and file writes - no game-state write precedes the return 0. [R234] indirection probe + handoff rev 186 (rev-185 text offloaded to HISTORY.md H83). [R233] 7-byte dump; 0x34 = SCMD_0x34_CainIdentify. [R232] R231 retraction. [R230] cdecl export. [R229] relative paths. [R228] honest WriteFile. [R227] beacon/fail-closed/skew. [R226] Win32 logger. [R225] SerializeItem sentinel. [R224] probe + guard. [R223] extern C. [R222] resync gated. [R221] d2_flags. [R220] header. [R219] gate fix. [R218] dll deliverable. [R217] zero unpinned. [R216] item-flag pair. [R215] state-only. [R214] dematerialise. [R213] iteration. [R212] Route B. [R211] walker. [R210] Fog 10115. [R209] 0x9C header. [R208] queue shim. [R207] own heap. [R206] 0x9C adapter. [R205] wire keys. [R204] per-account pools. [R203] charlist. [R196] bn-handlerloop-arith. [R190] CheckRevision bypass. RELEASE: Vanilla+0.0.9-R203 shipped Battle.net closed realms + Open Battle.net playable.
#define D2VP_BUILD_TAG "D2VP-R" D2VP_ROUND
// [R161 MAINT-06] the version prefix composes here ONCE: the boot self-ID
// ("Vanilla+" D2VP_VERSION_TAG, Game/Main.cpp) and the title/automap tag
// (D2VP_BUILD_TAG) both derive from this header, so a release bump touches
// exactly one place - the old hardcoded "0.0.8" literal in the self-ID
// would have drifted from CHANGELOG reality at the next bump.
#define D2VP_VERSION_TAG "0.0.9-R" D2VP_ROUND

extern "C" {
void D2BOOT_Mark(const char* pszStage);
void D2BOOT_MarkFmt(const char* pszFormat, ...);
void D2BOOT_Fail(const char* pszStage);
// VP-BEGIN [bn-ctl-replay R178]: the exact outbound SID_AUTH_INFO (0x50) frame, stashed by
// NET_SID_CLIENT_Send at send time and replayed verbatim by the R178 control socket onto a
// fresh connection. Forensic-only: written by the auth thread, read by the read thread, no
// locking (a torn read would just make the replay server-invalid, which is itself logged).
extern char g_chCtlReplayAuthInfo[0x2000];
extern int g_nCtlReplayAuthInfoLen;
// VP-END [bn-ctl-replay R178]
}
