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
#define D2VP_ROUND "184" // [R183] cf39-instrument: cold/frozen physical-kill vanish still reported post-R182 ("rarely disappear when hit with physical damage"). Fresh r2 pass closed the remaining cold-path questions - ApplyCold shatter roll (0x57b0fa-0x57b158: seed%100<20 && monster && MonStats2 deadCol bit 0x13 clear -> SetState 0x6b) is BYTE-IDENTICAL to the fork's, the death-drop wrapper (retail 0x5a6830, called from OnDeathDispatch 0x5a6ff0 - the fork's "not called in shipping" provenance note is WRONG) has the same two silent skips (ISDEAD bit 17 / COLLISION_ITEMS 0x801 mask = fork's COLLISION_ITEMS=2049), and the client gib consumer (Client Monster.cpp case-8: STATES_HasMaxState = flag-class tbl[0x1f] -> SpawnDeathEffectMissiles + dwLastHitClass|=0xa0) matches retail. Static analysis exhausted -> instrumented field repro per the r167 pattern: [CF39]-tagged LOGMANAGER_TraceToModuleLog at every silent decision point - DEFER (SUnitDmg post-hit deferral), DEFER-POISON (RegenAndPsnDecreaser), COMPLETE-* (all four CompleteDeathDelayIfPending outcomes), KILL-REJECT-* (all three KillMonster silent guards), DROP-SKIP-* (both loot-wrapper skips), IDLE-PREPASS (R182 room-idle completion), LIFECYCLE-HALT (pre-crash), DEATH (death entry w/ cold/freeze/shatter states). Next session: grep d2boot.log for [CF39] around a repro - the missing/interleaved lines name the divergent path. [R182] coldfrozen-vanish-final: issue #39 residual closed - idle-room race: CleanupIdleRooms pre-pass completes pending deferred deaths (SUNITDMG_CompleteDeathDelayIfPending) before the save/free walk; removed the R166 cold/frozen branch in RegenAndPsnDecreaser (r2-verified byte-identical to the general path, retail has no such branch). [R180] bn-replay-split: the R179 field run executed the plan perfectly and killed the remaining structural classes in ONE log - the real socket died while BLOCKING (bn-auth-blocking-connect worked as designed: race skipped), race-replay survived (the race/FIONBIO shape is exonerated), reuse-replay completed the whole conversation ON the freed auth handle value itself (h=0xb9c for both the probe and attempt 2's real socket - handle identity exonerated), wsaref only ever climbed (refcount exonerated; the boot -1 is NET_GetLocalIp's uncounted direct ::WSAStartup vs its counted wrapper cleanup, ret= now logged to prove it) - which exposed that the type=-1/10042 driving two rounds of forensics was OUR OWN probe bug: 0x1004 is SO_RCVLOWAT (unsupported on Windows, fails 10042 on ANY healthy socket), not SO_TYPE=0x1008; R175 had it right, R176 regressed it, every battery/autopsy/drift line since showed the same uniform garbage. R180: fix all three probe sites, add the last two structural-delta probes (opt-replay = Create's exact TCP_NODELAY/SO_KEEPALIVE/SO_SNDTIMEO set; twin-replay = a dedicated reader thread blocked in recv while the creating thread sends - the real flow's two-thread shape), and with shape+content+handle+opts+threading all covered, a full-survival verdict hands the diagnosis to the kernel layer: image-name-keyed classifier (rename test: D2VP.exe -> Game.exe) / WFP callout (netsh wfp capture)
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
