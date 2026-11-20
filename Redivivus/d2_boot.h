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
#define D2VP_ROUND "174" // [R174] bn-recv-autopsy + bn-auth-reconnect: the R173 field run proved the BNCS recv abort is deterministic (the one-shot retry recv failed with the same WSAECONNABORTED 10053 in the same millisecond; the 2026-11-20 pcap shows the wire healthy with AUTHREQ_109 already flushed) - Fog::Src::Safesock::SAFESOCK_LogRecvAutopsy now runs a one-shot probe of the poisoned handle in the BNCS read thread (getpeername/getsockname, SO_ERROR, SO_TYPE, zero-timeout select on read/except, MSG_PEEK recv, zero-length send probe) so the next d2boot.log distinguishes a locally aborted TCB from receive-only poisoning, and the auth stage gets one budget-guarded reconnect per user-initiated connect: NET_SID_CLIENT_ConnectFastest's wait-0x50 failure path frees the dead socket, resets SID protocol state, releases BattleConnectFastest's single-instance guard around a recursive re-entry on the same thread (budget hard-caps recursion at 1, re-armed in ConnectingToBattleNet) and replays 0x01/0x50 on a fresh connection
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
