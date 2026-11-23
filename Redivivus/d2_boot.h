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
#define D2VP_ROUND "201" // [R201] bn-wsaref-quiet: cleanup-site wsaref marks suppressed (the R200 field run flooded the log ~60/s during gameplay - a game-loop site calls the cleanup wrapper every frame; harmless to the real Winsock refcount). Cleanups now log ONCE on the first negative crossing (R179 imbalance tripwire stays armed); create/startup keep their marks. R200 FIELD: closed realm (Battle.net) auth perfect -> LOGONREALM ok -> MCP socket connected -> d2cs closed it INSTANTLY (recv=0 EOF at +16 ms, before any client data - R200 marks caught it: 'MCP: recv failed/EOF: bytes=0 socketStatus=4'). The R199 pcap already proved the full MCP handshake works against this server when the session is fresh (STARTUP accepted, charlist delivered) -> suspect STALE d2cs session state (prior run ended with a client FIN mid-charlist) -> server-side (VTSTech/D2VP-D2GS): check d2cs log / restart d2cs and retry. MILESTONE CONFIRMED IN FIELD: Open Battle.net path FULLY WORKS - second connect: auth ok -> CharSelect mode=3 -> chat enter/channel (0x0b/0x0a 'Diablo II-1') -> news (0x46) -> enter-world qserver (gametype=6) -> D2GS connect 192.168.56.1:4000 -> create-game 0x67 -> SP create-game -> DRLG act generated -> character 'TestMage' loaded -> 0x5B playerjoin -> ~2.5 min of real gameplay (quest gossip kashya/akara, NPC menus, in-game pkts) -> clean exit to charselect. Open Battle.net is PLAYABLE end-to-end. [R200] bn-mcp-dispatch (3-arg handler contract + HandleIncomingPacket chain) + bn-mcp-marks. [R199] bn-threadconnect-revert + bn-options-restore. [R198] bn-sha1pump-rewrite + bn-sha1append-counter. [R196] bn-handlerloop-arith + bn-patchwait-bypass-skip. [R190] CheckRevision bypass. [R188] Issue #39 chain CLOSED.
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
