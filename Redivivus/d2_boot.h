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
#define D2VP_ROUND "225" // [R225] FIRST-FAULT ROOT CAUSE (field report D2-261123: EIP 0x6FF5DF6F in Fog.dll, stack carrying our 0x4F toggle, no [cheat], no log). It was MY argument bug, not the game's: the gated SerializeItem adapter called `(pItem, pOut, nMax, nullptr, 0, 0)` while our own WORKING single-player call is `(pItem, pCursor, remaining, (D2StatListExStrc*)0x1, 1, 0)` - the 4th arg is a required statlist sentinel, so passing NULL made the serializer walk a NULL stat list and fault inside Fog's bit-buffer, exactly the crash shape reported. Sentinel restored. ALSO: because the crash happened DOWNSTREAM of the R224 sanity guard, the guard passed - i.e. **the hook arg mapping (ECX=pGame, EDX=pUnit) is now CONFIRMED correct**, and pUnit->pInventory@+0x60 is valid. Logging hardened per the agent's ask: resolved log path cached once with a C:\d2gs\d2vpstash.log fallback, setvbuf(_IONBF) + fflush per line (a crash cannot eat the tail), plus a HOOK entry marker (proves the hook fired even if everything below dies) and per-item SERIALIZE progress marks. Ordinal resolution already fails closed (nullptr/-1 -> caller bails), and the serialize buffer is a 0x4000 block so the 0xF4 minimum is satisfied. [R224] probe + sanity guard. [R223] extern C. [R222] resync gated. [R221] d2_flags. [R220] header. [R219] gate fix. [R218] dll target. [R217] zero unpinned. [R216] item-flag pair. [R215] state-only. [R214] dematerialise. [R213] iteration. [R212] Route B. [R211] walker. [R210] Fog 10115. [R209] 0x9C header. [R208] queue shim. [R207] own heap. [R206] 0x9C adapter. [R205] wire keys. [R204] per-account pools. [R203] charlist. [R196] bn-handlerloop-arith. [R190] CheckRevision bypass. RELEASE: Vanilla+0.0.9-R203 shipped Battle.net closed realms + Open Battle.net playable.
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
