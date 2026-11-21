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
#define D2VP_ROUND "182" // [R182] coldfrozen-vanish-final: issue #39 residual closed. Fresh r2 pass (Game114d project, per-function pdf diffs) verified the whole death path retail-faithful - RegenAndPsnDecreaser 0x5a6920 kill tail AND deferral, KillMonster 0x57ccb0 guards (killable bit 15 == retail fcn.00457490(classId,0xf)), lifecycle halt 0x5a7c59 + dead recovery block, SetUninterruptableState 0x5544b0 (incl. its AITHINK-timer removal tail), and the retail event-queue guard 0x5416fa-0x541712 (monster + AITHINK + uninterruptable -> clear) which the fork already had at Event.cpp SetEventInternal - so the vanish was never another completion bug: it is a RACE. A monster killed mid-cast (uninterruptable) strands at 0 HP with STATE_death_delay until its freeze/cold state expires (R166/R170 hooks); once the killer walks away, CleanupIdleRooms idles the room out (idle ticks > 10 at 12-frame cadence) INSIDE that window and UNITS_ProcessDeadOrInactiveMonster - which has no death_delay awareness - freed the unit with no removal packet (silent vanish, no loot) or saved it as a 0 HP live-mode zombie. Fix: CleanupIdleRooms now runs a pre-pass over the room's unit list completing pending deferred deaths (SUNITDMG_CompleteDeathDelayIfPending, the R166/R170 retail-derived completion primitive) BEFORE the save/free walk, so loot/XP/quest credit drop through the normal death flow and the dropped items are still present when the walk saves them with the room (FreeRoom force-unlinks anything left after the walk, so killing inside the walk would have eaten the loot). Also removed the R166 cold/frozen branch in RegenAndPsnDecreaser - r2-verified byte-identical to the general path, retail has no such branch. [R180] bn-replay-split: the R179 field run executed the plan perfectly and killed the remaining structural classes in ONE log - the real socket died while BLOCKING (bn-auth-blocking-connect worked as designed: race skipped), race-replay survived (the race/FIONBIO shape is exonerated), reuse-replay completed the whole conversation ON the freed auth handle value itself (h=0xb9c for both the probe and attempt 2's real socket - handle identity exonerated), wsaref only ever climbed (refcount exonerated; the boot -1 is NET_GetLocalIp's uncounted direct ::WSAStartup vs its counted wrapper cleanup, ret= now logged to prove it) - which exposed that the type=-1/10042 driving two rounds of forensics was OUR OWN probe bug: 0x1004 is SO_RCVLOWAT (unsupported on Windows, fails 10042 on ANY healthy socket), not SO_TYPE=0x1008; R175 had it right, R176 regressed it, every battery/autopsy/drift line since showed the same uniform garbage. R180: fix all three probe sites, add the last two structural-delta probes (opt-replay = Create's exact TCP_NODELAY/SO_KEEPALIVE/SO_SNDTIMEO set; twin-replay = a dedicated reader thread blocked in recv while the creating thread sends - the real flow's two-thread shape), and with shape+content+handle+opts+threading all covered, a full-survival verdict hands the diagnosis to the kernel layer: image-name-keyed classifier (rename test: D2VP.exe -> Game.exe) / WFP callout (netsh wfp capture)
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
