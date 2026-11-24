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
#define D2VP_ROUND "234" // [R234] INDIRECTION PROBE. Field: VPLOG.TXT = **DHX34FD3002802900** - p[0]=0x34 and the first dword 0x0230FD34 is a STACK ADDRESS whose target (per the D2GS crash dump) holds the real `4F 7D 00 00 00 01 00` frame. The dispatcher was independently disassembled (0x6fc873a0: opcode->table 0x6fd1a698, pGame=ECX pUnit=EDX buf=EBX) and the stub frame arithmetic is byte-verified, so the remaining unknown is ONE DEREFERENCE. VPSHARED_HookPacket now, on p[0]!=0x4F: logs the raw 7 bytes, logs the dword at pPacket (`>`+8 nibbles), dereferences it ONLY when it is a plausible user-mode pointer (0x10000..0x7FFF0000), logs the target bytes (`<`), and adopts that target as the frame (`R`) when it starts with 0x4F. Expected next trace: DHX34FD3002802900>0230FD34<4F7D0000000100R... after which the stash logic runs for the first time in the field. Also: handoff.md rewritten for this track (rev 186) with the old rev-185 text preserved verbatim in HISTORY.md **H83** - the new handoff carries the full pinned-value inventory (1.13d profile, every ordinal/RVA, the hook contract, deployment rules, the CDECL finding) so a fresh session does not re-derive any of it, plus the method lessons (export names lie about ABI; diagnostics must use raw Win32 and report real results; named imports are invisible to ordinal scans; re-verify #if gating; byte-assert every RVA). [R233] 7-byte dump; 0x34 = SCMD_0x34_CainIdentify in our map. [R232] R231 retraction. [R230] cdecl export. [R229] relative paths. [R228] honest WriteFile. [R227] beacon/fail-closed/skew. [R226] Win32 logger. [R225] SerializeItem sentinel. [R224] probe + guard. [R223] extern C. [R222] resync gated. [R221] d2_flags. [R220] header. [R219] gate fix. [R218] dll deliverable. [R217] zero unpinned. [R216] item-flag pair. [R215] state-only. [R214] dematerialise. [R213] iteration. [R212] Route B. [R211] walker. [R210] Fog 10115. [R209] 0x9C header. [R208] queue shim. [R207] own heap. [R206] 0x9C adapter. [R205] wire keys. [R204] per-account pools. [R203] charlist. [R196] bn-handlerloop-arith. [R190] CheckRevision bypass. RELEASE: Vanilla+0.0.9-R203 shipped Battle.net closed realms + Open Battle.net playable.
#define D2VP_BUILD_TAG "D2VP-R" D2VP_ROUND
// [R161 MAINT-06] the version prefix composes here ONCE: the boot self-ID
// ("Vanilla+" D2VP_VERSION_TAG, Game/Main.cpp) and the title/automap tag
// (D2VP_BUILD_TAG) both derive from this header, so a release bump touches
// exactly one place - the old hardcoded "0.0.8" literal in the self-ID
// would have drifted from CHANGELOG reality at the next bump.
#define D2VP_VERSION_TAG "0.0.9-R" D2VP_ROUND

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
