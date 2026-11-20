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
#define D2VP_ROUND "178" // [R178] bn-ctl-replay: the R177 field run killed two suspect classes outright (winsock catalog 12 providers / 0 layered = no LSP; Process Hacker module dump = zero third-party hook DLLs, only system + AMD graphics) and the ctl-socket came back ALIVE on both attempts (connect + 0x01 send ok, server silent as expected) so the block is NOT app-wide and NOT image-keyed-wholesale - the death is scoped to the auth flow/conversation, with the dead socket showing hvalid=1, soerr=0, fion=0 (queue purged = TCB already aborted at autopsy) but SO_TYPE failing WSAENOPROTOOPT(10042), a probe retail-class providers cannot produce - so R178 upgrades the control socket into a full conversation replay (stash the exact outbound 0x50 frame in NET_SID_CLIENT_Send, then on a fresh socket: 0x01 -> 0x50 verbatim -> server ping -> echo -> wait for AUTHREQ) with a probe battery run on the live socket before start and again the instant any replay step dies: if the replay survives the full conversation the death is bound to the ORIGINAL flow's kernel state (flow-scoped corruption, forensics thread: 10042/fion=0), if it dies at the same point the trigger is the conversation content itself (content-scoped classifier - netsh wfp capture then names the callout)
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
