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
#define D2VP_ROUND "198" // [R197] R196 FIELD: the 10053 war is WON - recv#2 pulled the 103 B AUTHREQ, 0x50 dispatched, bypass fired (first field log ever past recv#2). New crash at 'Checking Versions': AV READ 8A61CD26, eip 00FD32DC, auth thread, inside the VIRGIN region right after PerformCheckRevision (DecodeAndLoadKeys -> D2CheckExpansionGame -> key hashing -> owner copy -> 0x51 send) that NEVER executed in any earlier build (pre-R196 PerformCheckRevision returned 0 first). R197 = bn-authcheck-marks: step marks (key decode classic/owner/expansion results, 0x51 key-state line, per-key hash ok/fail via BNSEND_HashCDKeyByType now returning PrepareCDKeyHash's result, pre-send len mark) + NULL guards (PrepareCDKeyHash top guard for missing key file, owner string fallback to "") so a missing/undecodable key yields a clean server-rejected 0x51 instead of a crash. Awaiting user's d2crash-20261122-181430.txt 12-frame report to pin the exact faulting function. [R196] bn-handlerloop-arith CLOSED the 10053 case: handler-loop endpoint arith returned NULL on full consume -> nBufPos = -(int)chRecvBuf -> recv(sock, NULL, huge) -> AFD 8007 endpoint abort; + bn-patchwait-bypass-skip (skip 0x51 patch wait under bypass). [R195] bn-threadconnect. [R194] bn-skip-select. [R192] bn-recvbuf-lock. [R190] CheckRevision bypass ver=0x00010064 checksum=0 statstring=empty return 1. [R188] Issue #39 chain FULLY CLOSED.
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
