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
#define D2VP_ROUND "196" // [R196] BNET 10053 ROOT CAUSE CLOSED: IncomingPacketHandlerLoop's "bytes remaining" endpoint was decompiled as pBytes + (nSize - (int)pScanPtr) = B + nSize - P, a tiny int reinterpreted as an address - on the normal fully-consumed path it is exactly NULL, so nBufPos became -(int)chRecvBuf and recv#2 ran as recv(sock, NULL, huge). ETW bnet.etl 2026-11-21 named it: AFD rejected recv#2 with "Invalid buffer specified on fastio receive" (event 8007) and ABORTED the endpoint itself ({Virtual Circuit Closed}, 4001) - the deterministic recv 10053/soerr=0 auth death since R169, misread as WFP/AV/kernel aborts R173-R191. Fixed in BnSend.cpp (bn-handlerloop-arith, memmove for the overlapping compaction copy), validated by /tmp harness across consumed/pipelined/compaction/scan scenarios. Rider: 0x51 patch-filename wait skipped when the CheckRevision bypass is active (BNDOWNLOAD_CheckRevisionBypassActive, bn-patchwait-bypass-skip) - bypass needs no MPQ, so the old 20 s bound was a pure stall on MPQ-less servers. [R195] bn-threadconnect: the read thread performs its own connect (probe-parity). [R194] bn-skip-select + 100 ms SO_RCVTIMEO idle wake. [R192] bn-recvbuf-lock static VirtualLock'd buffer (kept; its working-set theory was wrong - the real killer was the NULL recv - but harmless). [R191] overlapped-recv probe (failed identically). [R190] CheckRevision bypass: ver=0x00010064 checksum=0 statstring=empty, return 1. [R188] cf39-closed: shatter crumble CONFIRMED fixed in the field; DROP-SKIP-NOTC = retail-intended (UNITFLAG_NOTC bit 17); Issue #39 chain FULLY CLOSED (R182 idle-room race, R187 shatter memset-order, R185 client death marks, R184 D2BOOT_MarkFmt).
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
