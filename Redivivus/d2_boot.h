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
#define D2VP_ROUND "177" // [R177] bn-sock-death-forensics: the R176 field run spent its full 20 x 10 ms budget and never healed (both attempts, ~220 ms, identical to the millisecond pattern) - the abort is hard, instant, deterministic at the exact same conversational point (first recv AFTER the SID_PING echo send succeeds) - and the R174 autopsy gained a contradiction worth chasing: soerr=0 yet SO_TYPE FAILS (type=-1) while select still claims readable, i.e. the Winsock option layer lost the handle microseconds after SO_ERROR succeeded, so R177 turns the one-instant snapshot into a timeline: every closesocket in the process is now tagged with its handle + owning tid (Free / closeSocket wrapper / SendOrCloseOnTimeout), the 10053 retry loop probes h-value + SO_TYPE + kernel-handle validity each iteration and logs only on drift, the autopsy adds getsockopt error codes + GetHandleInformation + FIONREAD, a one-shot Winsock catalog census names any layered provider (LSP), and a one-shot CONTROL SOCKET replays connect -> 0x01 -> recv against the same server while the auth socket sits dead (ctl alive => death is handle-scoped/in-process; ctl dead => app-connection-wide, image-keyed filter)
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
}
