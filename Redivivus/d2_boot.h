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
#define D2VP_ROUND "176" // [R176] bn-recv-abort-budget + boot-log tids: the R175 field run (fresh-socket reconnect died at the same byte; pcap re-read confirms the echo precedes death in both attempts, wire healthy, RST is ours with win=0) certifies in-process parity with retail end to end (read loop 0x51c7e0, Send 0x51c5c0, Ping 0x520fc0, transport leg) and leaves the WFP/AV/LSP filter class as the only suspect - so the R173 one-shot 10053 retry became a bounded 20 x Sleep(10) ~ 200 ms budget with one-shot autopsy and a RECOVERED mark if the condition ever clears (either it rides the condition out or it writes the definitive did-not-heal-in-200-ms data point), and every d2boot line now carries [tid N] so read-thread vs auth-thread marks can never again be misattributed
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
