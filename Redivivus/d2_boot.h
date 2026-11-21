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
#define D2VP_ROUND "179" // [R179] bn-auth-blocking-connect + probe split: the R178 pcap + bnetd logs settled the content question - both replay probes (started ~0.4ms after each auth RST, SAME process, byte-identical 0x50, same server) read the AUTHREQ and closed gracefully while the auth socket died with a kernel RST 1.2ms after AUTHREQ arrival, twice (attempt 1 + the R174 reconnect), so the trigger is per-flow kernel state, NOT the 0x25/AUTHREQ conversation and NOT the image; the only structural difference left between dying and surviving sockets is the race creation pattern (non-blocking connect + select + FIONBIO(0) toggle) vs the probes' plain blocking socket(), confounded with handle value (the reconnect landed on the freed handle and died too) - so R179 (a) connects BLOCKING when the server list has a single entry (the private-server case; multi-entry lists keep the retail race) - the exact socket shape that provably completes auth on this machine, (b) splits the confound with two new probes: race-pattern replay (FIONBIO(1) -> connect -> select writable -> FIONBIO(0) -> full conversation) and a reuse replay run AFTER Free so its socket lands on the just-freed auth handle value, (c) logs the WSAStartup/WSACleanup refcount at every site to close the in-process-cleanup question with data
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
