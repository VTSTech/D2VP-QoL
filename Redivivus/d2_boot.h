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
#define D2VP_ROUND "160" // [R155] camera_zoom: Z steps the world zoom 1.0/1.25/1.5 behind [features] camera_zoom (opt-in, 2 s hot-read, Options row 15); world-phase bracket + cursor inverse keep the UI 1:1 and clicks on the zoomed world; [R156] Glide renderer ported: D3D + Glide scaled, DDraw/Gdi clamp; [R157] ctrl_click_xfer covers the Horadric Cube: cube panel open = inventory/stash items into the cube, cube items out to the inventory (same 0x19+parked-0x18 pair, buffer byte 3 - the server routes bufferId 3 to place_item unconditionally); [R158] extended_stats: the character screen's second view - Magic Find, Gold Find and ten more hidden stats read live off the statlist (display-only, Options row 16, plans/EXTENDED-STATS-PLAN.md); [R159] audit quick wins: the DEV control channel binds loopback only, its mon/watch replies guard the remote-supplied pointers, three unbalanced VP markers closed; [R160] shared-stash data safety: a transient .sss read failure defers the load (toggle + save refused until a read succeeds - ROB-01) and a partial page materialization keeps the stored block authoritative, toggling away no longer re-serializes the subset (ROB-02)
#define D2VP_BUILD_TAG "D2VP-R" D2VP_ROUND

extern "C" {
void D2BOOT_Mark(const char* pszStage);
void D2BOOT_MarkFmt(const char* pszFormat, ...);
void D2BOOT_Fail(const char* pszStage);
}
