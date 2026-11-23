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
#define D2VP_ROUND "199" // [R199] R198 FIELD: BNCS AUTH FULLY WORKS - versioncheck result=1, both key hashes ok=1, LOGONRESPONSE2=0, CharSelect shown, realm list queried, LOGONREALM(D2CS) answered. NEW failure: realm/MCP socket h=0xd5c created after LOGONREALM then NEVER connects (no thread-side connect mark, zero sends/recvs, 11.8 s silence, main-thread teardown + dead flag). CAUSE: R195 bn-threadconnect deferred blocking-path connects into the read thread, but only BnetReadFunction calls SAFESOCK_ThreadConnect - the MCP socket's thread proc (D2QSSocket::ReceiveLoopCallback, breaks on !bConnected) never connects, and nobody else does either. bn-threadconnect-revert: blocking Create connects directly on the calling thread + sets bConnected (retail-faithful; WSASocket2_Send guards on bConnected, pre-R195 builds sent 0x50 straight after Create). bn-options-restore: s_bApplySocketOptions back to TRUE (R193 bisect obsolete - options were never the poison; R193/R194 field runs exonerated them; NODELAY wanted for gameplay). SAFESOCK_ThreadConnect + staging statics remain as inert no-ops. NOTE: MCP/charlist stage is virgin code - the next run may surface its own defects; the R197-style marks will name them. [R198] bn-sha1pump-rewrite + bn-sha1append-counter (Sha1_Pump self-clobber crash, field-CONFIRMED fixed: auth completed). [R197] bn-authcheck-marks + guards (key decode marks all non-NULL in R198 field - key files load fine). [R196] bn-handlerloop-arith (10053 root cause) + bn-patchwait-bypass-skip. [R190] CheckRevision bypass. [R188] Issue #39 chain CLOSED.
#define D2VP_BUILD_TAG "D2VP-R" D2VP_ROUND
// [R161 MAINT-06] the version prefix composes here ONCE: the boot self-ID
// ("Vanilla+" D2VP_VERSION_TAG, Game/Main.cpp) and the title/automap tag
// (D2VP_BUILD_TAG) both derive from this header, so a release bump touches
// exactly one place - the old hardcoded "0.0.8" literal in the self-ID
// would have drifted from CHANGELOG reality at the next bump.
#define D2VP_VERSION_TAG "0.0.8-R" D2VP_ROUND

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
