// vanilla-plus: "Vanilla+ Options" in-game settings page (FEATURES.md row 10).
// Design contract: DRAFT_vanilla_plus_options.md (Route B, above Previous Menu).
// Charter: VANILLA-PLUS.md - additive UI only; every toggle stays opt-in with
// 0-defaults; no D2Game/D2Common edits; no loader edits; no shipped assets.
//
// ===== Route B: the row label (field-QA revision, 2026-10-29) ===============
// Route A (synthesizing a text DC6 from font glyph frames) was retired after
// the first field build returned "label invisible, row clickable". The
// synthesis had fallen back to its blank image, and it carried three latent
// defects that would have broken it even on success:
//   1. DC6 RLE streams are bottom-up - FASTCMP_DecompressRLEWithAlpha walks
//      the destination with -nDstPitch - while the Route A encoder emitted
//      rows top-down, so the label would have rendered vertically flipped;
//   2. font glyph pixels are meant to be drawn through D2GFX_DrawShiftedImage
//      with the font's palette-shift index; a plain DrawImage of copied glyph
//      indices renders them unshifted;
//   3. the 1240-byte encode scratch buffer could not hold the label's RLE
//      stream (~2 KB for a 0x100-wide raster - worst case far more), a
//      stack-smashing bug hiding behind the fallback path.
// The retail esc-menu loop already invokes a per-row fpDrawCallback every
// frame (ESCMENU_DrawMenu, before the row's image draw), so the label is
// rendered there with DRAW_text (font resolved at runtime by measuring the
// retail neighbor label image - see the tunables block and
// VPLABEL_DecideMetrics). The row's
// fpInitCallback still seeds pDC6Main, but only with the field-proven 8x8
// blank placeholder:
// type-0 rows draw pDC6Main unconditionally (a null pointer would halt the
// draw), and the blank frame is exactly the image the first field build drew
// invisibly-but-safely. Same allocator + CELCMP_FixupPointersAndPrepare
// shape as a file-loaded image, so the retail free path (DC6_FreeAndDelete
// -> DeleteCellFile + FreeClientMemory) still tears it down with zero
// special-casing.
//
// ===== cursor handling (field-QA revision, 2026-10-29) ======================
// The first field build registered a WM_MOUSEMOVE (0x200) handler that ended
// in SMsgBreakHandlerChain - it ran first in the message chain and starved
// ui::UI_HandleMousePositionUpdate (the always-on gUIFlagGeneralInit batch),
// which is what updates MouseX/MouseY and refreshes the OS-cursor visibility
// on every move. Result: the on-screen cursor froze ("locked") while the page
// was open. Neither ESCMENU nor ControlConfig registers a 0x200 handler, so
// neither does this page: hover state is derived per frame in
// DRAW_VanillaPlusOptions from ui::GetMouseX/GetMouseY, and the mouse down/up
// handlers keep reading coordinates from the message itself.

#include "Source/D2Client/UI/VanillaPlusOptions.h"

#include "Source/D2Client/Cursor.h" // VP: upstream moved GetMouseX/GetMouseY from ui:: to Cursor::

#include "Source/D2CMP/SRC/CelCmp.h"
#include "Source/D2Client/D2Client.h"
#include "Source/D2Client/UI/VanillaPlusCore.h"
#include "Source/D2Client/D2GfxDataStrc.h"
#include "Source/D2Client/Engine/Gfx.h"
#include "Source/D2Client/Message.h"
#include "Source/D2Client/Sound/Sound.h"
#include "Source/D2Client/UI/Draw.h"
#include "Source/D2Client/UI/EscMenu.h"
#include "Source/D2Client/UI/ui.h"
#include "Source/D2Sound/Src/D2SoundFast.h"
#include "Source/D2Win/DC6.h"
#include "Source/D2Win/DC6Block.h"
#include "Source/D2Win/Src/D2WinFont.h"
#include "Source/D2gfx/D2GFX.h"
#include "Source/Fog/Fog.h"
#include "Source/Fog/Memory.h"
#include "3rdparty/Storm/Storm.h"
#include "Source/globals/globals.h"
#include "Source/D2Client/D2Client.globals.h" // VP: [tenth sync] the restructure moved the UI globals' externs (ScreenSizeX et al) into the per-module globals header; globals.h no longer carries them

#define DEFAULT_SECURITY_COOKIE 0xBB40E64E

#ifndef D2_ALLOC
#define D2_ALLOC(size) Fog::Memory::AllocClientMemory((size), __FILE__, __LINE__, 0)
#endif

#ifndef D2_FREE
#define D2_FREE(p) Fog::Memory::FreeClientMemory((p), __FILE__, __LINE__, 0)
#endif

namespace D2Client::UI::VanillaPlus {

// ===== tunables (single place, field-QA) ====================================

// Esc-menu row band. The retail draw loop bottom-anchors every row image at
// nItemBottomY = nRowTop + nRowHeight; DC6 draws (font glyph frames included)
// extend upward from the y passed to the renderer, i.e. the y passed to
// DRAW_text anchors the glyphs' bottoms (the baseline for non-descending
// frames). The label baseline is inset from the band bottom so the text sits
// visually centered in the 0x27 band (tunable).
//
// Field-QA timeline for the row label FONT (rev3 -> revE):
//   rev3: font 0xd (FontInGameChat, chat cell)  -> "label shows, ~10% size"
//   rev4: font 2   (Font30)                     -> "closer, ~50% size"
// The five retail rows (SoundOptions / VideoOptions / AutoMapOptions /
// CfgOptions / Previous) are DC6 label IMAGES (DATA\LOCAL\{LANG}\UI\*.dc6),
// so no font id is authoritative for them - only their pixel height is, and
// that data lives in the MPQ, not in this tree. revE therefore stops guessing:
// on the row's first draw it measures the retail neighbor label image's frame
// height and picks the in-tree font whose capital-letter glyph frame height
// matches it (candidates below, probed ascending; ties resolve to the larger
// font). The pick is cached for the process lifetime; later draws just use
// it. If measurement or glyph probes fail wholesale, the fallback is
// Font42 - the largest menu font, which is where the "go bigger" answer the
// 50% datapoint points at lives.
//
// Manual override (escape hatch, no rebuild needed): add to d2vp.ini (the
// same file the [features] gates live in):
//   [ui]
//   label_font = <aFontNames index>
// 1 = Font16, 2 = Font30, 3 = Font42, 7 = Font24; anything outside 0..13
// falls back to the measured pick. Read once, when the metrics are decided.
static const int gaVpLabelFontProbe[3] = { 7, 2, 3 };  // Font24, Font30, Font42 (ascending)
static const int VPLABEL_FONT_FALLBACK = 3;            // Font42 - largest menu font

// Resolved once per process by VPLABEL_DecideMetrics (first row draw).
// -2 = undecided; afterwards a valid aFontNames index 0..13.
static int gnVpLabelFontId = -2;

// Baseline inset above the band bottom. Kept at the rev4 field-accepted
// value: glyph frames are bottom-anchored, so a taller font extends upward
// from the same anchor and the accepted placement carries over to whatever
// font the measurement picks ('p' descenders may reach a few px into the
// inter-row gap - the retail images bottom-anchor at the band bottom and
// behave the same way).
static const int VPLABEL_BASELINE_INSET = 4;

// The Vanilla+ page itself is a ControlConfig mirror and keeps ControlConfig's
// own font (FontInGameChat) - its 0x1a row bands are sized for it. Deliberately
// separate from the row label's resolved font so the esc-menu row and the
// page can't drag each other's metrics around.
static const uint16_t VPPAGE_FONT_ID = 0xd;        // FontInGameChat - ControlConfig's font

// Page layout (mirrors the ControlConfig panel box for a familiar look).
// R144: the page is TWO COLUMNS of VPP_ROWS_PER_COL rows each. The one-column
// page had grown to 0x1f1 (497 px) over eleven rows - past the 640x480 height
// (the box clipped its bottom 9 px there) and past one glance; the twelfth
// row (unlock_fps_interp, this round) would have pushed the box to 0x211
// (529 px). Six per column pulls it back to 0x151 (337 px) - the box fits
// every stop whole again, 640x480 included ((480-337)/2 = 71 top margin).
static const int VPPAGE_BOX_WIDTH = 0x26c;
static const int VPPAGE_COL_WIDTH = 0x136;         // R144: half the box width - one column's span
static const int VPPAGE_BOX_HEIGHT = 0x171;        // R147: was 0x151 - two columns of seven rows (see the layout check)
static const int VPPAGE_ROW_PITCH = 0x20;
static const int VPPAGE_ROW_HEIGHT = 0x1a;
static const int VPPAGE_FIRST_ROW_Y = 0x50;        // from box top
static const int VPPAGE_BACK_ROW_Y = 0x144;        // from box top (R147: was 0x124 - seven rows per column push it down one pitch)
static const int VPPAGE_TEXT_BASELINE_INSET = 4;   // baseline above row bottom

// [features] keys rendered by the page. Adding a slate feature = +1 entry
// here and +1 template block in ALTTOGGLE_WriteIniTemplate - nothing else.
// (shared_stash's and no_panel_shift's template blocks shipped with their
// features themselves - R63/Rev 82 and the issue-#26 gate - so for the R101
// row the +1 entry really was all it took beyond the back-row move below.)
struct VPPFeatureRow {
    const char* szKey;        // d2vp.ini [features] key
    const wchar_t* wszLabel;  // row label
    int nReadDefault;         // R144: read default while the key is missing - must match what the key's own gate assumes (0 everywhere; unlock_fps_interp's gate reads missing as 1)
};
static const VPPFeatureRow gaVanillaPlusRows[14] = {
    { "alt_toggle", L"Show Items Toggle", 0 },
    { "auto_gold", L"Auto Gold Pickup", 0 },
    { "portable_save", L"Portable Saves", 0 },
    { "globe_persistent", L"Persistent Globe Text", 0 },
    { "item_ilvl", L"Item Level Tooltips", 0 },
    { "shared_stash", L"Shared Stash", 0 },  // VP: [shared-stash] track N round 1 (R67: in-game Options row - the tab is no longer ini-only)
    // VP: [issue #26 / R101] no-panel-shift - the toggle is no longer ini-only.
    // The row rides the same D2VP_* bridge as the other six; the write bridge
    // already busts dwNoPanelShiftLastCheck (D2VP_ConfigReloaded), so flipping
    // it here re-reads the key on the very next PANELSHIFT_NoShiftWorld poll.
    { "no_panel_shift", L"No Panel World Shift", 0 },
    // VP: [manaburn-fix / R108] mana_burn_fix - the toggle is no longer
    // ini-only. The row rides the same D2VP_* bridge as the other seven
    // (the ini template block shipped with R106, so a fresh d2vp.ini already
    // carries the key). The server gate MANABURN_FixActive
    // (D2Game/UNIT/VanillaPlusCombat.cpp) keeps its 2 s poll by design - the
    // shared-stash server-gate shape, D2Game cannot import D2Client helpers -
    // so a flip here lands on the next drain roll within 2 s, the same
    // latency as an external ini edit, while the panel draw below re-reads
    // the key fresh every frame and shows the flip immediately.
    { "mana_burn_fix", L"Mana Burn Fix", 0 },
    // VP: [shift-compare / R125] shift_compare - the toggle rides the same
    // D2VP_* bridge as the other eight (label = the owner's wording, 2026-11-06).
    // The ini template block ships in the same round, so both a fresh
    // d2vp.ini and an existing one carry the key. The gate SHIFTCOMPARE_Active
    // (VanillaPlusCore.cpp) polls at tooltip-draw time per frame, and the
    // write bridge busts dwShiftCompareLastCheck (D2VP_ConfigReloaded), so a
    // flip here shows on the very next hover frame; the compare capture also
    // re-reads the key fresh every frame it fires, so the round trip is
    // immediate in both directions.
    { "shift_compare", L"Shift Item Compare", 0 },
    // VP: [ctrl-click stash / R140] ctrl_click_xfer - the toggle rides the
    // same D2VP_* bridge as the other nine. The ini template block ships in
    // the same round, so both a fresh d2vp.ini and an existing one can carry
    // the key (an ini from before R140 reads 0 until the row is flipped
    // once). The gate VPCTRLXFER_Active (VanillaPlusCore.cpp) is read per
    // Ctrl+Click and the write bridge busts its cache tick, so a flip here
    // lands on the very next Ctrl+Click - no restart, no panel reopen.
    { "ctrl_click_xfer", L"Ctrl Click Transfer", 0 },
    // VP: [unlock-fps / R142] unlock_fps - the presentation-FPS unlock
    // (FEATURES.md row 17, plans/FPS-UNLOCK-PLAN.md). Label = the owner's
    // wording ("for v1 call the option Unlock FPS"). Bool gate, fixed 60 Hz
    // present target; the sim (server ticks + client world updates) stays 25 Hz
    // byte-exact, vsync untouched. The gate UNLOCKFPS_Active
    // (VanillaPlusCore.cpp) is 2 s hot-read and the write bridge busts its
    // cache tick (D2VP_ConfigReloaded), so a flip here re-reads the key on the
    // very next draw poll; the pacer unanchors itself on gate-off and the
    // retail draw gate takes over from the same frame.
    { "unlock_fps", L"Unlock FPS", 0 },
    // VP: [unlock-fps-interp R144] unlock_fps_interp - the R143 sub-tick
    // interpolation sub-gate leaves ini-only and takes the twelfth row,
    // directly under its owner. It rides the same D2VP_* bridge; nReadDefault
    // is 1 because the gate itself reads a missing key as 1 (UNLOCKFPS_Active:
    // "missing key = 1, the intended look") - the generic 0-default read
    // would display Off while the interp actually runs on a pre-R143 ini
    // (ALTTOGGLE_EnsureIni writes the template only for a missing FILE, it
    // never backfills keys). The write path is key-generic already: a flip
    // writes an explicit 0/1 and busts dwUnlockFpsLastCheck
    // (D2VP_ConfigReloaded), so the next UNLOCKFPS_Active poll re-reads
    // within the same 2 s window an external ini edit gets. With the gate off
    // the row is inert (the interp consumers all poll UNLOCKFPS_Active first)
    // - it stays a plain toggle like every other row, no graying.
    { "unlock_fps_interp", L"FPS Interpolation", 1 },
    // VP: [automap-clock R147] automap_clock - the thirteenth row. The ini
    // template block ships in the same round, so both a fresh d2vp.ini and
    // an existing one can carry the key (an ini from before R147 reads 0
    // until the row is flipped once). The gate VPCLOCK_Draw's 2 s cache is
    // busted by the write bridge (dwAutomapClockLastCheck in
    // D2VP_ConfigReloaded), so a flip here lands on the next automap frame
    // - no restart, no map toggle needed. Display-only: two overlay lines
    // on the automap (Session H:MM:SS since game entry + System HH:MM),
    // FEATURE row 18.
    { "automap_clock", L"Automap Clock", 0 },
    // VP: [transparent-hud R154] transparent_hud - the fourteenth row
    // (FEATURES.md row 4, plans/TRANSPARENT-HUD-PLAN.md). Label = the
    // owner's wording ("Transparent HUD"). The gate VANILLAPLUS_HudBar
    // Transparent (VanillaPlusCore.cpp) is 2 s hot-read and the write bridge
    // busts dwHudBarTranslucentLastCheck (D2VP_ConfigReloaded), so a flip
    // here lands on the next frame that draws the bottom bar - no restart,
    // no panel reopen. Display-only: the bar's console frame art + the
    // run/menu buttons + the hostile indicator are skipped when the gate is
    // on; globes, belt, skill buttons, XP and stamina bars draw retail and
    // the hidden buttons stay clickable. No renderer code ships - the skip
    // is upstream of every renderer, so there is no Glide/D3D axis.
    { "transparent_hud", L"Transparent HUD", 0 },
};
static const int VPP_ROW_COUNT = 14;
static const int VPP_ROW_BACK = 14; // hit-test index of the Previous Menu row
static const int VPP_ROWS_PER_COL = 7; // R147: was 6 - two columns of seven (fourteen rows: col 0 bands 0-6, col 1 bands 0-6)
// Layout check (R101, re-run for R108 and R125): the seventh feature row spans
// boxTop+0x110..0x12a, so R101 moved the Previous Menu row down from
// boxTop+0x124 (which would overlap it by 6 px) to boxTop+0x144 - the same
// one-row 0x1a gap the R67 layout kept above it (row 6 ends 0x12a, back row
// starts 0x12a+0x1a). R108: the eighth row (mana_burn_fix) spans
// boxTop+0x130..0x14a, so the back row moved down one pitch, to
// boxTop+0x164 (0x14a+0x1a); its bottom 0x17e grew the box from 0x171 to
// 0x191 (same 0x13 bottom margin as R101). R125: the ninth row
// (shift_compare) spans boxTop+0x150..0x16a, so the back row moves down one
// pitch again, to boxTop+0x184 (0x16a+0x1a); its bottom 0x19e grows the box
// from 0x191 to 0x1b1 (same 0x13 bottom margin as R101/R108). R140: the
// tenth row (ctrl_click_xfer) spans boxTop+0x170..0x18a, so the back row
// moves down one pitch again, to boxTop+0x1a4 (0x18a+0x1a); its bottom
// 0x1be grows the box from 0x1b1 to 0x1d1 (same 0x13 bottom margin). R142:
// the eleventh row (unlock_fps) spans boxTop+0x190..0x1aa, so the back row
// moves down one pitch again, to boxTop+0x1c4 (0x1aa+0x1a); its bottom
// 0x1de grows the box from 0x1d1 to 0x1f1 (same 0x13 bottom margin).
// 0x1f1 = 497 exceeded the 640x480 height (the bottom 9 px of empty box
// clipped there). R144: the twelfth row (unlock_fps_interp) would have
// pushed the back row to boxTop+0x1e4 and the box to 0x211 (529 px, 50 px
// past 480p) - instead the page splits into TWO COLUMNS of six rows:
// the last row of each column spans boxTop+0xf0..0x10a, the back row
// returns to boxTop+0x124 (0x10a+0x1a, the same one-row 0x1a gap), its
// bottom 0x13e sets the box to 0x151 (337 px, same 0x13 bottom margin) -
// every stop fits whole again, 640x480 included ((480-337)/2 = 71).
// Horizontal (R144): each column's band is its half of the box inset 0x0e
// per side (width 0x11a); the label sits 0x10 into the band (column 0 keeps
// the 0x1e label x this page has always used) and the On/Off value
// right-aligns 0x10 inside the band's right edge - measured per draw, so a
// fixed left x sized for the full box can't crowd the longest label
// ("Persistent Globe Text") in either column.
// R147: the thirteenth row (automap_clock) needs seven bands per column -
// VPP_ROWS_PER_COL 6 -> 7. Column 0's last band (index 6) spans
// boxTop+0x110..0x12a (the exact band R101's seventh row once occupied),
// so the back row moves down one pitch to boxTop+0x144 (0x12a+0x1a, the
// same one-row 0x1a gap), its bottom 0x15e grows the box from 0x151 to
// 0x171 (369 px, the same 0x13 bottom margin) - every stop still fits
// whole, 640x480 included ((480-369)/2 = 55 top margin). Column 1 carries
// six rows (bands 0-5, boxTop+0x50..0x10a); the empty band 6 of column 1
// stays empty - the hit-test only accepts bands rows actually occupy.

// Input handlers (definitions below; the batch array references them).
// Deliberately NO WM_MOUSEMOVE (0x200) entry - see the header comment block:
// the always-on gUIFlagGeneralInit batch owns 0x200 and must keep running.
void __stdcall INPUT_VP_MouseDown(D2UIMouseMsgStrc * pMsg);
void __stdcall INPUT_VP_MouseUp(D2UIMouseMsgStrc * pMsg);
void __stdcall INPUT_VP_EscapeKey(D2UIMouseMsgStrc * pMsg);

// Input batch registered while the page is open (cube/panel style: dedicated
// handlers, self-contained registration). eEventType: 0=window message,
// 3=key down (ui.h D2UIFlagInitStrc contract).
static D2UIFlagInitStrc gaVanillaPlusInit[3] = {
    { 0x0, 0x201, &INPUT_VP_MouseDown },
    { 0x3, 0x01b, &INPUT_VP_EscapeKey },
    { 0x0, 0x202, &INPUT_VP_MouseUp },
};

// ===== page geometry helpers (kept in one place for draw and hit tests) =====

static void VPPAGE_GetBoxOrigin(int* pnX, int* pnY) {
    *pnX = (D2Client::UI::ui::ScreenSizeX - VPPAGE_BOX_WIDTH) / 2;
    *pnY = (D2Client::UI::ui::WindowSizeY - VPPAGE_BOX_HEIGHT) / 2;
}

static int VPPAGE_RowTopY(int nRowIndex) {
    int nBoxX = 0;
    int nBoxY = 0;
    VPPAGE_GetBoxOrigin(&nBoxX, &nBoxY);
    if (nRowIndex == VPP_ROW_BACK) {
        return nBoxY + VPPAGE_BACK_ROW_Y;
    }
    // R144: two columns - the vertical band follows the row's position WITHIN
    // its column (i % VPP_ROWS_PER_COL), not the global index.
    return nBoxY + VPPAGE_FIRST_ROW_Y
        + (nRowIndex % VPP_ROWS_PER_COL) * VPPAGE_ROW_PITCH;
}

// R144: the on-screen band (left x + width) of one feature row - the strip
// the hover highlight fills and the mouse hit-test accepts: the row's
// column-half of the box inset 0x0e per side. The back row keeps the
// full-width strip (draw and hit-test handle it separately, as before).
static void VPPAGE_RowBand(int nRowIndex, int* pnLeft, int* pnWidth) {
    int nBoxX = 0;
    int nBoxY = 0;
    VPPAGE_GetBoxOrigin(&nBoxX, &nBoxY);
    int nCol = nRowIndex / VPP_ROWS_PER_COL;
    *pnLeft = nBoxX + nCol * VPPAGE_COL_WIDTH + 0x0e;
    *pnWidth = VPPAGE_COL_WIDTH - 0x1c;
}

static int VPPAGE_HitTest(int nMouseX, int nMouseY) {
    // Feature rows (R144): a hit needs the row's vertical band AND its own
    // column's horizontal band - the same y is two rows now, one per column.
    for (int i = 0; i < VPP_ROW_COUNT; i++) {
        int nTop = VPPAGE_RowTopY(i);
        if (nMouseY < nTop || nMouseY >= nTop + VPPAGE_ROW_HEIGHT) {
            continue;
        }
        int nBandLeft = 0;
        int nBandWidth = 0;
        VPPAGE_RowBand(i, &nBandLeft, &nBandWidth);
        if (nMouseX >= nBandLeft && nMouseX < nBandLeft + nBandWidth) {
            return i;
        }
    }
    // Previous Menu row: full-width strip, the contract it always had.
    int nBoxX = 0;
    int nBoxY = 0;
    VPPAGE_GetBoxOrigin(&nBoxX, &nBoxY);
    int nRowLeft = nBoxX + 0x12;
    int nRowRight = nBoxX + VPPAGE_BOX_WIDTH - 0x12;
    int nBackTop = VPPAGE_RowTopY(VPP_ROW_BACK);
    if (nMouseX >= nRowLeft && nMouseX < nRowRight
        && nMouseY >= nBackTop && nMouseY < nBackTop + VPPAGE_ROW_HEIGHT) {
        return VPP_ROW_BACK;
    }
    return -1;
}

// ===== row placeholder (Route B) =============================================

// The esc-menu draw path for type-0 rows calls IMAGE_DrawDC6ImageOnScreen
// unconditionally (ESCMENU_DrawMenu case 0), so pDC6Main must be a renderable
// DC6 or the draw halts. This placeholder is the exact image the first field
// build ended up drawing (Route A's blank fallback): an 8x8 fully transparent
// frame. Its RLE stream is the deterministic encoding of an all-zero 8x8
// raster - per row one 0x80|8 transparent-run token plus the 0x80 end-of-row
// token - so it is hard-coded instead of runtime-encoded (the Route A encoder
// and its undersized scratch buffer are gone from the shipped code path).
//
// Byte shape (file-loaded DC6 layout):
//   0x00  24-byte DC6 header   (version 6, flags 1, format 0, 1 dir, 1 frame)
//   0x18  frame-offset slot    (0x1C, file-absolute like every loaded image)
//   0x1C  0x20-byte frame header (nFlip=1 retail terminator, 8x8, nLength=16)
//   0x3C  RLE frame data       (16 bytes)
// CELCMP_FixupPointersAndPrepare converts the offset slot to an absolute
// pointer and allocates the per-frame 0x2C bookkeeping row, exactly as it
// does for file-loaded images. FALSE = out of memory (caller leaves pDC6Main
// null; the type-0 draw would halt, but allocation failure is failing anyway).
static BOOL VPLABEL_BuildBlankPlaceholder(DC6** ppOut) {
    *ppOut = nullptr;
    static const uint8_t gaBlankRle[16] = {
        0x88, 0x80, 0x88, 0x80, 0x88, 0x80, 0x88, 0x80,
        0x88, 0x80, 0x88, 0x80, 0x88, 0x80, 0x88, 0x80,
    };
    const int nTotalSize = 0x3C + (int)sizeof(gaBlankRle);
    DC6* pRaw = (DC6*)D2_ALLOC(nTotalSize);
    if (!pRaw) {
        return FALSE;
    }
    memset(pRaw, 0, nTotalSize);
    pRaw->nVersion = 6;       // required by the fixup (nExpectedFormat == -1)
    pRaw->nFlags = 1;         // bit0 required, bit1 clear (pre-fixup state)
    pRaw->nFormat = 0;
    /* nTerm stays zeroed */
    pRaw->nDirections = 1;
    pRaw->nFrames = 1;
    *(uint32_t*)((char*)pRaw + 0x18) = 0x1C; // file-absolute frame offset

    DC6Block* pFrame = (DC6Block*)((char*)pRaw + 0x1C);
    pFrame->nFlip = 1;        // retail frame termination value
    pFrame->nWidth = 8;
    pFrame->nHeight = 8;
    pFrame->nOffsetX = 0;
    pFrame->nOffsetY = 0;
    pFrame->nAllocSize = 8 * 8;
    pFrame->nNextBlock = 0;   // fixup patch slot: overwritten with the row ptr
    pFrame->nLength = (int)sizeof(gaBlankRle);
    memcpy((char*)pRaw + 0x3C, gaBlankRle, sizeof(gaBlankRle));

    // Retail fixup: converts the file-relative offset to an absolute pointer,
    // allocates the 1x1 block row, patches the frame's +0x18 slot. Idempotent
    // (nFlags bit1), exactly what every file-loaded DC6 goes through.
    D2CMP::SRC::CelCmp::CELCMP_FixupPointersAndPrepare(pRaw, ppOut, ".\\UI\\VANILLAPLUS.CPP", 0x0, -1, "vanilla-plus options placeholder");
    if (!*ppOut) {
        D2_FREE(pRaw);
        return FALSE;
    }
    return TRUE;
}

// ===== Esc-menu row callbacks ================================================

void __fastcall ESCMENU_InitVanillaPlusLabel(D2EscMenuItemStrc * pItem, D2UIMouseMsgStrc * pMsg) {
    (void)pMsg;
    if (!pItem || pItem->pDC6Main) {
        return; // idempotent: ESCMENU_Initialize runs once, but stay safe
    }
    // Route B: the visible label is drawn by VANILLAPLUS_DrawRowLabel (the
    // row's fpDrawCallback); this pointer only satisfies the unconditional
    // type-0 image draw with a fully transparent frame.
    VPLABEL_BuildBlankPlaceholder(&pItem->pDC6Main);
}

// ===== row label metrics (revE: measure the retail neighbor) ================

// Height of one glyph's frame in font nFontId (0 = not measurable). Forces
// the standard lazy load through SetAndReturnLastFont (the exact path every
// text draw takes when a font is first used), then reads the glyph's frame
// from the font DC6 through the retail accessors - the same glyph-entry ->
// frame-number lookup D2WINFONT_DrawWideString performs for every drawn
// character. Leaves the current font switched; callers save/restore around
// probe batches.
static int VPLABEL_GlyphFrameHeight(int nFontId, uint16_t wCharCode) {
    if (nFontId < 0 || nFontId >= 14) {
        return 0;
    }
    D2Win::Src::D2WinFont::SetAndReturnLastFont(nFontId);
    // A failed load leaves the cache entry null (SetAndReturnLastFont assigns
    // the loader result unconditionally); FONT_GetCharWidthCallback would
    // then index pGlyphs + ch against a null/garbage table - never
    // dereference that.
    const D2FontCacheEntryStrc * pFontEntry = D2Win::Src::D2WinFont::VP_GetFontCacheEntry(nFontId);
    if (!pFontEntry->pFontDC6 || !pFontEntry->pGlyphs) {
        return 0;
    }
    D2FontGlyphStrc* pGlyph = (*D2Win::Src::D2WinFont::VP_GetFontCharWidthFn())(wCharCode);
    if (!pGlyph) {
        return 0;
    }
    DC6* pFontDc6 = (DC6*)pFontEntry->pFontDC6;
    D2GfxDataStrc sGfxData;
    memset(&sGfxData, 0, sizeof(sGfxData));
    sGfxData.pDC6 = pFontDc6;
    sGfxData.nDirection = 0;
    sGfxData.nFrameNumber = (int)*(const uint16_t*)(pGlyph->abUnverified + 6);
    DC6Block* pBlock = D2CMP::SRC::CelCmp::CELCMP_GetDC6BlockFromGfxData(&sGfxData);
    if (!pBlock) {
        return 0;
    }
    int nHeight = (int)D2CMP::SRC::CelCmp::GetDC6Height(pBlock);
    return nHeight > 0 ? nHeight : 0;
}

// Frame-0 height of a retail row's label image (the measurement target: what
// the user visually compares our label against). Type-0 rows bottom-anchor
// this exact image into the band, so its height IS the retail label height.
static int VPLABEL_MeasureRowImageHeight(D2EscMenuItemStrc* pItem) {
    if (!pItem || !pItem->pDC6Main) {
        return 0;
    }
    D2GfxDataStrc sGfxData;
    memset(&sGfxData, 0, sizeof(sGfxData));
    sGfxData.pDC6 = pItem->pDC6Main;
    sGfxData.nDirection = 0;
    sGfxData.nFrameNumber = 0;
    DC6Block* pBlock = D2CMP::SRC::CelCmp::CELCMP_GetDC6BlockFromGfxData(&sGfxData);
    if (!pBlock) {
        return 0;
    }
    int nHeight = (int)D2CMP::SRC::CelCmp::GetDC6Height(pBlock);
    return nHeight > 0 ? nHeight : 0;
}

// Resolve the row label font, once per process, on the row's first draw.
//   1. [ui] label_font override from d2vp.ini (validated 0..13).
//   2. Measure the retail neighbor label (row [5] "Previous" - the direct
//      visual neighbor below us, always visible, no expansion/visibility
//      gates; fallback row [0] "SoundOptions") and pick the probed font whose
//      'S' frame height best matches the image height. Capital height is the
//      comparison metric: "Previous" has no descenders, so its baked image
//      height IS its capital extent (+1-2 px of bake padding - the same bias
//      for every candidate, so the argmin stays stable).
//   3. Fallbacks: no measurable image or no probeable glyph -> Font42.
static void VPLABEL_DecideMetrics() {
    if (gnVpLabelFontId != -2) {
        return; // already decided this process
    }

    // Provisional fallback; the paths below keep or replace it.
    gnVpLabelFontId = VPLABEL_FONT_FALLBACK;

    char szIniPath[260];
    int nOverride = -1;
    if (ALTTOGGLE_BuildIniPath(szIniPath)) {
        nOverride = (int)GetPrivateProfileIntA("ui", "label_font", -1, szIniPath);
    }
    if (nOverride >= 0 && nOverride < 14) {
        gnVpLabelFontId = nOverride;
        return;
    }

    int nVpOptionsCount = 0;
    D2EscMenuItemStrc * pVpOptionsTable =
        D2Client::UI::EscMenu::VP_GetOptionsTable(&nVpOptionsCount);
    int nTargetHeight = nVpOptionsCount > 5
        ? VPLABEL_MeasureRowImageHeight(&pVpOptionsTable[5]) : 0;
    if (nTargetHeight <= 0 && nVpOptionsCount > 0) {
        nTargetHeight = VPLABEL_MeasureRowImageHeight(&pVpOptionsTable[0]);
    }
    if (nTargetHeight <= 0) {
        return; // keep Font42, nothing to measure against
    }

    int32_t nSavedFont = D2Win::Src::D2WinFont::SetAndReturnLastFont(gaVpLabelFontProbe[0]);
    int nBestFont = -1;
    int nBestDist = 0;
    for (int i = 0; i < 3; i++) {
        int nFontId = gaVpLabelFontProbe[i];
        int nCapsHeight = VPLABEL_GlyphFrameHeight(nFontId, (uint16_t)L'S');
        if (nCapsHeight <= 0) {
            continue;
        }
        int nDist = nCapsHeight > nTargetHeight ? nCapsHeight - nTargetHeight : nTargetHeight - nCapsHeight;
        // "<=" on purpose: the probe order is ascending, so a distance tie
        // resolves to the larger font (the retail labels are the big menu
        // family - when in doubt, go bigger).
        if (nBestFont == -1 || nDist <= nBestDist) {
            nBestFont = nFontId;
            nBestDist = nDist;
        }
    }
    D2Win::Src::D2WinFont::SetAndReturnLastFont(nSavedFont);

    if (nBestFont != -1) {
        gnVpLabelFontId = nBestFont;
    }
}

// Row draw callback (fpDrawCallback), invoked by DRAW_EscMenu every frame
// before the row's image draw. Renders the label through the retail text
// path (DRAW_text, font resolved by VPLABEL_DecideMetrics) instead of a
// synthesized DC6. Geometry mirrors the DRAW_EscMenu per-item formula so the
// text lands in the same band the retail images bottom-anchor into;
// DRAW_text's y anchors the glyph bottoms (DC6 frames draw upward from y),
// hence the small inset above the band bottom.
void __fastcall VANILLAPLUS_DrawRowLabel(D2EscMenuItemStrc * pItem, D2UIMouseMsgStrc * pMsg) {
    (void)pMsg;
    if (!pItem) {
        return;
    }

    // Resolve the label font once (measures the retail neighbor label; no-op
    // after the first call). Must run before the SetAndReturnLastFont below.
    VPLABEL_DecideMetrics();

    // Row band: replicate ESCMENU_DrawMenu's per-item Y formula for the
    // Options table (this row only ever draws while that table is active).
    // nVisibleBefore counts visible rows above ours (expansion gate + per-row
    // fpVisibilityCheck), matching the retail loop's visibility accounting.
    int nVisibleBefore = 0;
    bool bFound = false;
    int nVpOptionsCount = 0;
    D2EscMenuItemStrc * pVpOptionsTable =
        D2Client::UI::EscMenu::VP_GetOptionsTable(&nVpOptionsCount);
    const D2EscMenuHeaderStrc * pVpHeader =
        D2Client::UI::EscMenu::VP_GetHeaderOptions();
    for (int i = 0; i < pVpHeader->nItemCount; i++) {
        if (&pVpOptionsTable[i] == pItem) {
            bFound = true;
            break;
        }
        if (pVpOptionsTable[i].nExpansionOnly && !Fog::D2CheckExpansionGame()) {
            continue;
        }
        pfnEscMenuItemVisible pfnVisible = pVpOptionsTable[i].fpVisibilityCheck;
        if (pfnVisible && !pfnVisible()) {
            continue;
        }
        nVisibleBefore++;
    }
    if (!bFound) {
        return; // foreign row: never ours, draw nothing
    }

    const int nMenuCenterY = D2Client::UI::Inv::ScreenSizeY - 0x50;
    const int nRowTop = pVpHeader->nVerticalSpacing * nVisibleBefore
        + (nMenuCenterY / 2 - pVpHeader->nVerticalSpacing * pVpHeader->nItemCount / 2);
    const int nBaselineY = nRowTop + pVpHeader->nRowHeight - VPLABEL_BASELINE_INSET;

    static const wchar_t gwszLabel[] = L"Vanilla+ Options";
    int32_t nPrevFont = D2Win::Src::D2WinFont::SetAndReturnLastFont(gnVpLabelFontId);
    int nTextWidth = D2Win::Src::D2WinFont::DRAW_text0((uint16_t*)gwszLabel);
    D2Win::Src::D2WinFont::DRAW_text((uint16_t*)gwszLabel, D2Client::UI::ui::ScreenSizeX / 2 - nTextWidth / 2, nBaselineY, 0, 0);
    D2Win::Src::D2WinFont::SetAndReturnLastFont(nPrevFont);
}

void __fastcall ESCMENU_ShowVanillaPlusOptions(D2EscMenuItemStrc * pItem, D2UIMouseMsgStrc * pMsg) {
    (void)pItem;
    (void)pMsg;
    // ESCMENU_ShowControlConfig's body, with our flag in UI_CONFIG's place.
    if (ui::GetUIFlag(UI_ESCMENU)) {
        ui::SetUIFlag(UI_ESCMENU, UI_RESET, 0);
    }
    ui::SetUIFlag(UI_VANILLAPLUS, UI_SET, 0);
}

// ===== page: back + toggles ==================================================

static void VANILLAPLUS_BackToOptionsMenu() {
    // INPUT_ConfigCancel's shape (minus the key-binding revert): close the
    // page, re-open the Esc menu on the Options table.
    ui::SetUIFlag(UI_VANILLAPLUS, UI_RESET, 0);
    EscMenu::ESCMENU_ShowMenu(0, 1);
    Sound::SOUND_PlaySound(SOUND_CURSOR_PASS, nullptr, 0, 0, 0);
}

static void VANILLAPLUS_ToggleRow(int nRowIndex) {
    if (nRowIndex < 0 || nRowIndex >= VPP_ROW_COUNT) {
        return;
    }
    const char* szKey = gaVanillaPlusRows[nRowIndex].szKey;
    // R144: read through the row's own default so the flip inverts what the
    // display shows (unlock_fps_interp reads a missing key as 1 - a generic
    // 0-default read would make the first click a silent no-op write of 1).
    int nValue = D2VP_GetFeatureToggleDefault(szKey, gaVanillaPlusRows[nRowIndex].nReadDefault);
    // WritePrivateProfileStringA through D2Client's bridge: writes d2vp.ini
    // and flushes every gate's 2 s hot-read cache (same-frame effect for the
    // draw, <= 1 frame for auto_gold, next save-path resolution for
    // portable_save - matching its ini template wording).
    D2VP_SetFeatureToggle(szKey, nValue ? 0 : 1);
    Sound::SOUND_PlaySound(SOUND_CURSOR_SELECT, nullptr, 0, 0, 0);
}

// ===== page: input handlers ==================================================
// Coordinates come from the message, not from ui::GetMouseX/GetMouseY: the
// chain position of these handlers relative to UI_HandleMousePositionUpdate
// is not something this page controls, and the message payload is always the
// event's own position.

void __stdcall INPUT_VP_MouseDown(D2UIMouseMsgStrc * pMsg) {
    if (GAME::Game::GAMEDATA_GetExitFlag3()) {
        pMsg->bHandled = 0;
        return;
    }
    int nHit = VPPAGE_HitTest((uint32_t)pMsg->uLParam.sMousePos.nMouseX, (uint32_t)pMsg->uLParam.sMousePos.nMouseY);
    if (nHit >= 0 && nHit < VPP_ROW_COUNT) {
        VANILLAPLUS_ToggleRow(nHit);
    } else if (nHit == VPP_ROW_BACK) {
        VANILLAPLUS_BackToOptionsMenu();
    }
    pMsg->bHandled = 1;
    pMsg->dwResult = 0;
    Storm::SMsgBreakHandlerChain(pMsg);
}

void __stdcall INPUT_VP_MouseUp(D2UIMouseMsgStrc * pMsg) {
    // Swallow the matching button-up so no click leaks to the game layer.
    pMsg->bHandled = 1;
    pMsg->dwResult = 0;
    Storm::SMsgBreakHandlerChain(pMsg);
}

void __stdcall INPUT_VP_EscapeKey(D2UIMouseMsgStrc * pMsg) {
    if (GAME::Game::GAMEDATA_GetExitFlag3()) {
        pMsg->bHandled = 0;
        return;
    }
    VANILLAPLUS_BackToOptionsMenu();
    pMsg->bHandled = 1;
    pMsg->dwResult = 0;
    Storm::SMsgBreakHandlerChain(pMsg);
}

// ===== page: lifecycle =======================================================

void __stdcall VANILLAPLUS_OnToggleUI(int32_t nParam) {
    HWND hWnd = D2gfx::D2GFX::GetHwnd();
    if (nParam) {
        _Message::MESSAGE_RegisterEventHandlerBatch(hWnd, gaVanillaPlusInit, 3);
    } else {
        _Message::MESSAGE_UnregisterEventHandlerBatch(hWnd, gaVanillaPlusInit, 3);
    }
}

// ===== page: draw ============================================================
// Text Y convention: DRAW_text's y is the glyph baseline (DC6 frames draw
// upward from y), so every text call anchors its baseline relative to the
// BOTTOM of its row band - the same convention ControlConfig's row renderer
// uses (baseline at the highlight bottom).

static void VPPAGE_DrawCenteredText(const wchar_t* wszText, int nBaselineY, int nColor) {
    int nWidth = D2Win::Src::D2WinFont::DRAW_text0((uint16_t*)wszText);
    D2Win::Src::D2WinFont::DRAW_text((uint16_t*)wszText, D2Client::UI::ui::ScreenSizeX / 2 - nWidth / 2, nBaselineY, nColor, 0);
}

static void VPPAGE_DrawValueText(int nValueRightX, int nValue, int nBaselineY) {
    // On = green (2), Off = red (1) - classic D2 locale color codes.
    // R144: right-aligned to the row band (the x argument is the band's
    // right text edge, 0x10 in from the band's right): two columns halve the
    // row width, and a fixed left x sized for the full box would crowd the
    // longest labels. DRAW_text0 measures without drawing - the same
    // measure-then-place pattern the centered-text helper uses.
    const wchar_t* wszValue = nValue ? L"On" : L"Off";
    int nWidth = D2Win::Src::D2WinFont::DRAW_text0((uint16_t*)wszValue);
    D2Win::Src::D2WinFont::DRAW_text((uint16_t*)wszValue, nValueRightX - nWidth, nBaselineY, nValue ? 2 : 1, 0);
}

void __stdcall DRAW_VanillaPlusOptions() {
    int nBoxX = 0;
    int nBoxY = 0;
    VPPAGE_GetBoxOrigin(&nBoxX, &nBoxY);

    int32_t nPrevFont = D2Win::Src::D2WinFont::SetAndReturnLastFont(VPPAGE_FONT_ID);

    // Panel: the ControlConfig box geometry, so the page reads as a sibling.
    Engine::Gfx::D2GFX_DrawSolidRectExBySize(nBoxX, nBoxY, VPPAGE_BOX_WIDTH, VPPAGE_BOX_HEIGHT, 0, DRAW_TRANSPARENT);
    RECT sFrame;
    sFrame.left = nBoxX;
    sFrame.right = nBoxX + VPPAGE_BOX_WIDTH - 3;
    sFrame.top = nBoxY + 3;
    sFrame.bottom = nBoxY + VPPAGE_BOX_HEIGHT;
    ui::DRAW_Direct_RectFrame(&sFrame);

    // Title: baseline at boxTop + 0x1e, ControlConfig's exact title anchor.
    VPPAGE_DrawCenteredText(L"Vanilla+ Options", nBoxY + 0x1e, 4);

    // Feature rows: label left, value right, hover highlight. Hover is
    // derived live from the shared mouse position (kept fresh by the
    // always-on gUIFlagGeneralInit 0x200 handler - this page registers none).
    int nMouseX = Cursor::GetMouseX();
    int nMouseY = Cursor::GetMouseY();
    int nHover = VPPAGE_HitTest(nMouseX, nMouseY);
    for (int i = 0; i < VPP_ROW_COUNT; i++) {
        int nRowY = VPPAGE_RowTopY(i);
        int nBandLeft = 0;
        int nBandWidth = 0;
        VPPAGE_RowBand(i, &nBandLeft, &nBandWidth);
        if (nHover == i) {
            Engine::Gfx::D2GFX_DrawSolidRectExBySize(nBandLeft, nRowY, nBandWidth, VPPAGE_ROW_HEIGHT, 0, DRAW_TRANSPARENT);
        }
        int nBaselineY = nRowY + VPPAGE_ROW_HEIGHT - VPPAGE_TEXT_BASELINE_INSET;
        D2Win::Src::D2WinFont::DRAW_text((uint16_t*)gaVanillaPlusRows[i].wszLabel, nBandLeft + 0x10, nBaselineY, 0, 0);
        VPPAGE_DrawValueText(nBandLeft + nBandWidth - 0x10,
            D2VP_GetFeatureToggleDefault(gaVanillaPlusRows[i].szKey, gaVanillaPlusRows[i].nReadDefault), nBaselineY);
    }

    // Previous Menu row (same highlight/click contract as the toggles).
    int nBackY = VPPAGE_RowTopY(VPP_ROW_BACK);
    if (nHover == VPP_ROW_BACK) {
        Engine::Gfx::D2GFX_DrawSolidRectExBySize(nBoxX + 0x12, nBackY, VPPAGE_BOX_WIDTH - 0x24, VPPAGE_ROW_HEIGHT, 0, DRAW_TRANSPARENT);
    }
    VPPAGE_DrawCenteredText(L"Previous Menu", nBackY + VPPAGE_ROW_HEIGHT - VPPAGE_TEXT_BASELINE_INSET, 0);

    D2Win::Src::D2WinFont::SetAndReturnLastFont(nPrevFont);
}

} // namespace D2Client::UI::VanillaPlus
