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
#define D2VP_ROUND "187" // [R186] cf39-msl-vel-trace: R185 field log decoded the shatter vanish completely - on every shatter kill: CLT-DEATH maxState=1 (shatter state synced OK), lightState=1 (the death NODRAW fires - CORRECT: retail's death handler 0x4aff60 NODRAW-checks fcn.0063a320 = pSingleStateDataTbl[2] = the states.txt HIDE flag column, and the shatter state row carries hide=1, so retail ALSO hides the monster instantly - the visible crumble IS the two effect missiles, not the corpse), and CLT-SHATTER-FX spikeMsl=0 smokeMsl=0 - MakeMissile returns NULL for both, so nothing plays and the monster just vanishes. Retail mechanics now fully mapped: shatter death = NODRAW the unit + spawn missiles Monglacialspike(271)+MonStats2 Height(0xb) and Icebreaksmoke(275) with nFlags 0x2020; MakeMissile's >=100 distance guard (0x4cd7d1/0x4cd7e8) is SKIPPED when velocity == 0, and retail passing proves rows 271-275 must have Vel/VelLev == 0 in missiles.txt (static frame-animated effect). The fork computes NONZERO velocity (or fails elsewhere) -> NULL. Diffed byte-level vs retail 0x4cd540: D2SkillArgStrc layout (flags+0/owner+4/missile+8/target+0xc/txtNo+0x10/nX+0x14/nY+0x18/nUnitX+0x1c/nUnitY+0x20/level+0x30), the 0x20-flag destination REPLACE, the velocity math (VelLev*level+.../8+Vel<<8, Vel@0x9a VelLev@0x9b), the 75% nVelFinal conversion, and the guard shape - ALL identical. Static analysis exhausted again -> R186 adds gated [CF39] MSN-* marks in client MakeMissile (bCf39 gate = missile rows 271-276 only): MSN-VEL (raw/final velocity + dest + spawn), MSN-FAIL-DISTX/DISTY (the guard firing), MSN-FAIL-TXT/NOPMSL/CREATE (earlier failure points). Next log read discriminates: raw>0 on rows known-zero in retail -> missiles.txt column binding/loader bug; raw=0 + CREATE fail -> unit creation issue; raw>0 + genuinely nonzero data rows -> the data files themselves differ.
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
