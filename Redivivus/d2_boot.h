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
#define D2VP_ROUND "188" // [R188] cf39-closed: shatter crumble CONFIRMED fixed in the field (R187 log: MSN-VEL msl=272/275 raw=0 final=0, guard skipped as predicted, spikeMsl/smokeMsl live, user confirms animation plays). DROP-SKIP thread resolved as RETAIL-INTENDED: the wrapper's bit-17 check is UNITFLAG_NOTC (131072, no-treasure-class) - set by the self-resurrect AI (AI.cpp:5087/8797), Necromancer revive (SkillNec.cpp:1864 NOXP|NOTC), minion/Baal/Create spawns - resurrected/revived monsters legitimately drop no loot on their next death, retail identical. Mark renamed DROP-SKIP-ISDEAD -> DROP-SKIP-NOTC (my earlier label misread bit 17 as ISDEAD=65536=bit 16). Issue #39 chain FULLY CLOSED: R182 idle-room race fix, R187 shatter memset-order fix, NOTC behavior verified intended. [R187] shatter memset-order fix. [R186] MSN-* marks. [R185] client death marks. [R184] D2BOOT_MarkFmt writer. [R182] idle-room race fix + RegenAndPsnDecreaser cold-branch removal.
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
