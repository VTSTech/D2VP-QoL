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
#define D2VP_ROUND "241" // [R241] GAMBLE REFRESH - ACTS 4/5 + ini GATE. Jamella (0x195) and Larzuk (0x200/0x202) complete INV_IsGambleRefreshNpc, so all six classes UseMenu's gamble-open accepts are wired. The feature is now gated on [features] gamble_refresh (default 0 = retail): the client polls it in GAMBLEUI_FeatureActive (VanillaPlusCore.cpp, 2 s, flushed by D2VP_ConfigReloaded) and only builds the button when set; the server re-polls the same file in SUNITNPC_GambleRefreshEnabled (D2Game cannot import D2Client) and refuses the re-roll when unset, so a stock ini can never reach it even if a packet arrives. ini-only - deliberately no Options row, the button is a vendor affordance not a display preference. [R240] ACT 4/5 START CRASH FIX. A character created with -act4/-act5 faulted on the first rendered frame in D2Client::GAME::Draw::DrawFloorTile (AV read of 0x14, EAX=0; stack DrawFloorTile <- DRAW_DrawEdgeFloorTile <- DrawFloorByViewAndFloor). Root cause: D2DrlgStrc::sTileCache is left all-zero for acts 3 and 4 - InitCache (Drlg.cpp:971) only fills acts 0-2 and DRLGACTMISC_LoadTileProjects (Drlg.cpp:371) loads no town DT1 for them - so the cached pTileLibraryEntry stays NULL and pTileOther->nOrientation reads [0+0x14]. Retail never reads that cache; its only consumer is the fork's R117/R121 edge-void fill (DRAW_DrawEdgeFloorTile), which runs only at the wide stops, so the NULL was harmless until the band fill reached it. DrawFloorTile now skips a tile with no library entry, exactly as the retail 640x480 path does by never running the fill. [R239] GAMBLE REFRESH - ACT 3. Alkor (0xfe) joins Gheed (0x93, R237) and Elzix (0xc7, R238) behind the single predicate INV_IsGambleRefreshNpc; nothing else changed. Field-confirmed 2026-11-26. [R237/R238] GAMBLE REFRESH BUTTON. The gamble panel's blank 3rd slot (index 2) now carries a "Refresh" button for the gambling vendors - Gheed (Act 1, 0x93) in R237, Elzix (Act 2, 0xc7) in R238 - that re-rolls the whole inventory. Retail only ever re-rolls the single slot a purchase frees (SUNITNPC_DeductGoldForPurchase -> SUNITNPC_RefreshNpcItem), and a second gamble-open is a no-op because SUNITNPC_ValidateNpcInteraction returns early while the player still owns a proxy-inventory node, so the button sends C->S 0x38 action 2 with a magic params dword (D2VP_GAMBLE_REFRESH_PARAM 0x5246 in Redivivus/d2_hand.h - retail always sends 0 there) and SUNITNPC_GambleRefresh drops the node, regenerates and pushes the deltas. Three field-run bugs, all fixed: (1) keying the click branch on the shop-mode global fell through to the retail Repair All path, a silent no-op for a gambling vendor - now keyed on the NPC class via INV_IsGambleRefreshNpc; (2) SUNITPROXY_SendProxyItemsToClient's parameter names are shifted by one (retail passes (pGame, pNpc, pPlayer, pClient) from Item.cpp:592) and the reversed pair makes Pre0x9DAnd0x9CPacket's `pNpc != pUnit` check drop every packet silently; (3) GAMBLE_GenerateGambleInventory places items with SERVER_ItemToInventory(bSendUpdate = 0), so the adds never enter the inventory update list - the refresh queues them explicitly (UNITFLAGEX_ISVENDORITEM = the '8' add trigger, mirroring the removals' UNITFLAGEX_ITEMINIT = '9') and drains the queue afterwards. Field-confirmed 2026-11-26.

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
