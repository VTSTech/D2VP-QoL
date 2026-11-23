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
#define D2VP_ROUND "202" // [R202] CLOSED-REALM LOGINREQ FIXED (D2GS agent's d2cs log cracked it: on_client_loginreq 'got bad account name' -> d2cs_conn_destroy - client DID send data; my 'EOF before client data' was a misread of recv=0 = rejected-without-reply). pvpgn ground truth (src/d2cs/d2cs_protocol.h + handle_d2cs.cpp): t_client_d2cs_loginreq = header(size u16 + type u8) seqno@3 u1@7 bncs_addr1@11 sessionnum@15 sessionkey@19 cdkey_id@23 u5@27 clienttag@31 bnversion@35 bncs_addr2@39 u6@43 secret_hash[5]@47, account-name STRING @67 (packet_get_str_const(packet, 67, MAX_CHARNAME_LEN)). Our LOGINREQ shape matched pvpgn exactly (R199 pcap: name 'test' at @67, d2cs ACCEPTED it - LOGINREPLY 0 + charlist). THE DEFECT: NET_MCP_CLIENT_Send_0x01_StartUp sent pStrLen+0x41 bytes from &mcp0x01.nPacketId - the 65-byte struct PLUS stack adjacency luck: the name was only in the packet when payloadBuffer (holding the name) sat right behind mcp0x01 on the stack. R199-era build: lucky placement -> accepted; R200/R201 builds: garbage behind the struct -> d2cs read junk as the account name -> reject + close. bn-mcp-loginreq R202: copy struct (0x41 bytes) + name string into ONE contiguous buffer, send exactly its length; added 'MCP: 0x01 LOGINREQ sending len=%u account=%s' mark. [R201] bn-wsaref-quiet (cleanup marks suppressed; first-negative-crossing only). [R200] bn-mcp-dispatch + bn-mcp-marks. [R199] bn-threadconnect-revert + bn-options-restore. [R198] bn-sha1pump-rewrite + bn-sha1append-counter. [R196] bn-handlerloop-arith + bn-patchwait-bypass-skip. [R190] CheckRevision bypass. MILESTONE: Open Battle.net PLAYABLE end-to-end (R200 field: create-game, TestMage, quests, NPC menus).
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
