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
#define D2VP_ROUND "226" // [R226] LOGGING THAT CANNOT SILENTLY FAIL + ITEM VALIDATION. Zero log lines despite the markers being present in the binary meant either fopen failing silently or the file landing somewhere nobody looks. Rewrote VPSHARED_Log113d on RAW WIN32 (CreateFileA + WriteFile + CloseHandle, no CRT stdio at all - a stdio FILE* inside an injected DLL was a plausible silent failure), and it now DISCOVERS its target by trying candidates one per call and dropping the one that fails: (1) the DLL's own directory, (2) hardcoded C:\d2gs\d2vpstash.log, (3) the process working directory ("d2vpstash.log"). That covers a DLL deployed to a subfolder - which is the most likely reason the log never appeared, since GetModuleFileNameA always succeeds and the old code had no reason to use its fallback. ALSO answers the agent's second point (fault signature byte-identical before/after the sentinel fix): the serialise loop now VALIDATES the unit before use - `if (pCarried->eUnitType != UNIT_ITEM) log SERIALIZE SKIP and continue` - and the SERIALIZE mark prints the type, so a bad pItem is named instead of crashing. [R225] SerializeItem sentinel restored; arg mapping confirmed by the guard. [R224] probe + UNIT_PLAYER guard. [R223] extern C. [R222] resync gated. [R221] d2_flags. [R220] header. [R219] gate fix. [R218] dll deliverable. [R217] zero unpinned. [R216] item-flag pair. [R215] state-only. [R214] dematerialise. [R213] iteration. [R212] Route B. [R211] walker. [R210] Fog 10115. [R209] 0x9C header. [R208] queue shim. [R207] own heap. [R206] 0x9C adapter. [R205] wire keys. [R204] per-account pools. [R203] charlist. [R196] bn-handlerloop-arith. [R190] CheckRevision bypass. RELEASE: Vanilla+0.0.9-R203 shipped Battle.net closed realms + Open Battle.net playable.
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
