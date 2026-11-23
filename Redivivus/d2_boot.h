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
#define D2VP_ROUND "203" // [R203] CLOSED-REALM CHARLIST FIXED (last gap - R202 field + D2GS agent): LOGINREQ now ACCEPTED server-side (d2cs: account test accepted 02:12:39/02:13:04/02:13:40, no more 'bad account name'); the remaining stall = D2VP never sends MCP CHARLISTREQ (0x19) after LOGINREPLY - d2cs has NO auto-push charlist (pvpgn handle_d2cs.cpp: charlists only for 0x17/0x19 requests) and requires conn_state_authed (set by loginreply) first, so d2cs correctly goes quiet and charselect sat at 'Connecting'. USER'S CHARCREATE DETOUR PROVED EVERYTHING ELSE WORKS: charcreate test -> 0x14 already-exists (correct!), testt created, CHARLOGINREQ 0x07 authed, CREATEGAMEREQ 0x03 (game 'Test' created on D2GS), JOINGAMEREQ 0x04 (GS IP 192.168.0.14), joined + played + exited clean, char saved to d2dbs. R203 bn-mcp-charlistreq: (1) ReadAndParsePacket fires NET_MCP_CLIENT_Send_0x19_CharList2() (already present, never called - wire: h(size2+type1)+u16 maxchar=8+u16 u1 = 7 bytes, matches agent's CHARLISTREQ_110) on LOGINREPLY (queue frame [0]==0x01) - once per MCP session (s_bMCPCharListRequested, reset in MCPCONNECT_CreateThread); (2) McpConnectionLoop's wait loop pumps the queue via ReadAndParsePacket and breaks once the charlistreq is out (the MCP thread then exits -> PollMCPThreadStatus proceeds -> realm charselect pumps consume the CHARLISTREPLY -> CopyCharListToGlobal); (3) post-loop safety send for the event path; (4) MAINMENU_PollMCPThreadStatusPump calls ReadAndParsePacket (dequeue during 'Connecting'). Wire formats per D2GS agent (big-endian d2cs): LOGINREQ 67B fixed + account+NUL (len 70 + 2 size = 72 exact - DO NOT TOUCH); CHARLISTREPLY_110 = h + u16 maxchar + u16 currchar + u16 u1 + u16 currchar2 (11B) then per char: NUL name + NUL portrait block; CREATECHARREPLY results 0x00 ok / 0x14 exists / 0x15 reject. [R202] bn-mcp-loginreq (contiguous LOGINREQ buffer; d2cs-confirmed accepted). [R201] bn-wsaref-quiet. [R200] bn-mcp-dispatch + bn-mcp-marks. [R199] bn-threadconnect-revert. [R198] bn-sha1pump-rewrite. [R196] bn-handlerloop-arith. [R190] CheckRevision bypass. MILESTONE: Open Battle.net PLAYABLE + closed realm works past charlist via charcreate path.
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
