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
#define D2VP_ROUND "200" // [R200] R199 FIELD: auth STILL perfect; realm/MCP socket now CONNECTS (R199 revert confirmed live) and the R199 pcap proves the MCP handshake works end-to-end: client dialed 192.168.0.14:6113 (0x3E parse correct: ip c0a8000e port 17e1), d2cs accepted, MCP_STARTUP (72 B) sent, d2cs replied result 0 (accepted!), client sent CHARLIST2 request (0x19), d2cs DELIVERED THE CHARLIST containing the "Test" character - and the client dropped it and FIN'd 1.6 s later. TWO stacked D2VP defects closed by bn-mcp-dispatch: (1) ProcessReceivedData passed pHandlerContext (nullptr) where the packet pointer belongs - MCPCONNECT_SocketCallback_Dispatch received garbage args and routed nothing (fixed: 3-arg handler contract pfnPacketHandler(pMessage, wMsgSize, pHandlerContext), typedef updated in D2QSSocketStrc.h); (2) HandleIncomingPacket - the full INCOMING table (0x02..0x19: charlist->CopyCharListToGlobal, charlogon, gamelist...) - had ZERO callers (dead code since reconstruction; only the sparse gaMcpSocketCallbackHandlers[0x01] join-signal ran). Fixed by chaining HandleIncomingPacket inside Dispatch for unhandled packets; handled packets return 1 so ProcessReceivedData skips enqueueing (no double-processing of the 0x11 gamelist append offset via the UI pumps' ReadAndParsePacket). Plus bn-mcp-marks: ReceiveLoopCallback gate-break/recv/close marks + MCP pkt dispatch marks. MILESTONE: 'Other Multiplayer -> Open Battle.net' connects all the way and CREATES A GAME (user field report) - the R196/R198/R199 socket-layer fixes unblocked the whole direct-play path. [R199] bn-threadconnect-revert + bn-options-restore. [R198] bn-sha1pump-rewrite + bn-sha1append-counter. [R196] bn-handlerloop-arith + bn-patchwait-bypass-skip. [R190] CheckRevision bypass. [R188] Issue #39 chain CLOSED.
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
