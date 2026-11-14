// vanilla-plus core gates: d2vp.ini plumbing shared by the opt-in features.
//
// Fork-owned TU (picked up by CMake's GLOB_RECURSE) - extracted from
// D2Client.cpp so the upstream merge surface stays minimal. Do not move this
// code back into upstream files; upstream files only ever call the entry
// points declared here (the fork-only-TU rule, handoff.md §8).
//
// Charter: every feature is opt-in via [features] keys in d2vp.ini next to
// d2.exe; missing file/section/key or a garbage value always reads as 0 =
// vanilla byte-for-byte. The ini is hot-re-read at most every 2 s
// (GetTickCount gate), and D2VP_ConfigReloaded busts those caches after a
// write so the options page takes effect on the next poll.

#pragma once

struct DC6;

// [resolution-mode] Route-B label primitives for the 1280x720 stop of the
// video-options Resolution row (attached by the row's fpInitCallback
// UI_SLIDER_LoadAndApplyResolution in Storm/_unnamespaced.cpp and drawn by
// its fpDrawCallback UI_SLIDER_DrawResolutionValue in
// D2Client/UI/EscMenu.cpp). The static esc-menu table keeps nSubImageCount
// = 2 on purpose: ESCMENU_LoadLocalizedImages loads one label DC6 per stop
// from the MPQs and no "1280x720" asset exists, so a third static entry
// would halt the menu load. Instead the init callback attaches the blank
// placeholder below into pDC6SubImages[2] at runtime (retail-shape 8x8
// transparent DC6; the menu DC6s are process-lifetime, same as every
// file-loaded label) and the visible label is font-drawn per frame. Same
// Route B the Vanilla+ row label was field-proven with (see
// VanillaPlusOptions.cpp for the Route A post-mortem).
// FALSE = allocation/fixup failure; caller keeps nSubImageCount = 2 and the
// init callback clamps the slider position back to 800x600.
BOOL __stdcall VPRES_BuildBlankPlaceholder(struct DC6** ppOut);
// Pick the in-tree menu font whose capital 'S' glyph frame height best
// matches the frame-0 pixel height of pTargetDC6 (the retail "800x600"
// value image of the same row). Probes Font24/Font30/Font42 ascending
// (same probe list as the Vanilla+ row label), ties resolve to the larger
// font, fallback Font42. Saves/restores the current font around the probes
// and caches the result per process for the given target pointer (the
// retail label DC6 lives from ESCMENU_Initialize to shutdown).
int __stdcall VPRES_PickFontMatchingDC6(struct DC6* pTargetDC6);
// [resolution site-5] TRUE when eResolutionMode renders the retail 800x600
// UI class (the "800*" art set + nScreenMode=1 D2Common layout tables +
// count-based stat/skill button input): mode 2 (retail 800x600), mode 3
// (dormant 1344x700, same class) and mode 4 (Vanilla Plus 1280x720). Every
// retail D2GFX_GetResolutionMode() consumer that tests == 2 / != 2 to pick
// the 800-vs-640 layout routes through here, so a future wide mode is
// added in exactly one place.
BOOL __stdcall VPRES_IsWideLayoutMode(DWORD eResolutionMode);
// [resolution site-5] Horizontal offset that centers the exactly-800-px-wide
// retail UI class on the current playfield: (ScreenSizeX - 800) / 2, clamped
// to 0 on narrower widths. 0 at modes 0/1/2 (retail math stays
// byte-identical), 240 at mode 4 (1280x720), 272 at mode 3 (1344x700).
// Add it to left-anchored X coordinates and subtract it from
// ScreenSizeX-relative right anchors. Screen-center-anchored widgets and
// everything hanging off gnPanelOffsetX/gnPanelOffsetY (set once per frame
// in DRAW_UI) needs no per-site change.
int __stdcall VPRES_GetWideUiOffsetX();
// [resolution site-6] Vertical companion to VPRES_GetWideUiOffsetX: the Y
// half of the "move the 800-class UI as one rigid unit" rule. The 800-class
// panel art is bottom-anchored ("gnPanelOffsetY - const + ScreenSizeY",
// bar pieces at ScreenSizeY) and follows the screen height on its own, but
// every FIXED Y coordinate - table rects from Belts.txt/inventory.txt and
// "const - gnPanelOffsetY" text/hit-test expressions - stays in 800x600
// space and lands exactly one height-delta too high. This offset is that
// delta: ScreenSizeY - 600, clamped to 0. 0 at modes 0/1/2 (retail math
// stays byte-identical), 100 at mode 3 (1344x700), 120 at mode 4 (1280x720),
// 168 at mode 5 (1366x768) [R112], 480 at mode 6 (1920x1080) [R115].
// Add it to fixed Y coordinates of the 800-class UI. DO NOT add it to
// bottom-anchored expressions (anything already containing "+ ScreenSizeY")
// or change gnPanelOffsetY (-60) - double-shifting is the failure mode.
int __stdcall VPRES_GetWideUiOffsetY();

// [vanilla-plus no-panel-shift] (issue #26 / R95) d2vp.ini next to d2.exe,
// [features] no_panel_shift=0|1 - retail slides the whole game world a
// quarter screen sideways when exactly one panel (character screen /
// inventory) is open, so the action re-centers in the uncovered strip
// (both panels open = no slide). That retail assumption is exact only when
// the open panel reaches the screen edge (640x480/800x600); at the wide
// stops (1280x720 / 1344x700 / 1366x768 [R112] / 1920x1080 [R115]) the 800-class UI floats
// inboard, the
// slide drags world out from under the visible outer margins, and every
// margin artifact of issue #26 lives on that panned path (the perspective
// projection anchors on the player and expands from the screen center, so
// the pan rides half a dozen independent channels: MouseYRelatedB, the
// pView walk rect, the four roof/floor light-grid half-screen adjusts, the
// weather clip switches, the automap border offset and the renderer
// quarter offsets). no_panel_shift=1 renders the world exactly like the
// field-proven both-panels-open case (no slide, full unpanned view rect)
// on the wide stops and lets the panel float over it; retail 640x480 /
// 800x600 always keeps the vanilla slide. Default/missing/garbage = 0 =
// vanilla slide (charter rule 1). Hot-reads at most every 2 s like every
// gate.
BOOL __stdcall PANELSHIFT_NoShiftWorld();
// Split-side neutralizer for the world-draw call sites: returns 0 (= border
// none: full-width walk rect, zero world pan, zero renderer offset - the
// renderers receive 0 and take their own border-0 full-screen path) for
// border 1/2 while PANELSHIFT_NoShiftWorld() is active, else eBorderType
// unchanged. Border 3 passes through - it already renders unpanned. World
// culling and clipping only: UI/interaction consumers (panel click rects,
// label culls, selection guards) keep reading the real border type.
// R100: the unit-sprite draw path routes its real-border read through this
// too - the Ghidra-1864+ retail bodies hand-pan sprites/shadows/selection
// rings/hover text +/-quarter in the perspective branch and must follow the
// border-0 camera whenever the world does (no_panel_shift=1).
uint32_t __stdcall PANELSHIFT_BorderForWorld(uint32_t eBorderType);

// [vanilla-plus alt-toggle] Config-gated Alt-key ground-item-label toggle
// (D2R "Show Items: Toggle" parity). d2vp.ini next to d2.exe:
//   [features]
//   alt_toggle=1
// Default/missing file/missing section/missing key/garbage value = 0 =
// vanilla hold-Alt behavior byte-for-byte (Principle 1: opt-in, config-gated).
// The ini is re-checked at most every 2000 ms via GetTickCount, mirroring the
// wrapper's ~2 s conf hot-reload cadence. A missing d2vp.ini is created with
// a documented all-defaults template: once per process at client boot
// (ApplicationMain) and lazily before the first config read, so the opt-in
// keys are discoverable without reading any docs. Auto-repeat never reaches
// the handlers (CmdTbl.cpp filters lParam bit 30 upstream), so the keydown
// handler fires exactly once per physical press.
BOOL __stdcall ALTTOGGLE_Active();   // config: toggle mode on?
BOOL __stdcall ALTTOGGLE_Latched();  // current latch value
void __stdcall ALTTOGGLE_Flip();     // keydown: latch = !latch
void __stdcall ALTTOGGLE_Reset();    // InitializeUI: latch = FALSE
void __stdcall ALTTOGGLE_EnsureIni(); // create d2vp.ini template if missing
// [vanilla-plus alt-toggle] True while labels are shown by the toggle LATCH
// (never true in vanilla hold mode). While latched, the ground-label renderer
// must not own the click selection (vanilla DRAW_AltItems clears the selected
// unit every frame the cursor is not over a label, which turns every click
// into a walk), and ground items stay hit-testable so a label click picks the
// item up. D2R "Show Items: Toggle" parity: labels stay up, clicks still
// attack/interact.
BOOL __stdcall ALTTOGGLE_LabelClickThrough();
// Resolve "<exe dir>\\d2vp.ini". GetModuleFileNameA always yields a fully
// qualified path; refuse the (unreachable) dir-less case rather than fall
// back to a CWD-relative file. Shared by the [features] gates and the
// options page's [ui] label_font reader. FALSE = unresolvable.
BOOL __stdcall ALTTOGGLE_BuildIniPath(char* szIniPath);
// [vanilla-plus auto-gold] Config-gated walk-over gold pickup (D2R "Gold
// Pickup" parity). d2vp.ini next to d2.exe, [features] auto_gold=0|1 -
// default/missing file/missing key/garbage = 0 = vanilla byte-for-byte.
// While on, gold piles inside the server's own instant-pick radius are
// picked up through the literal click path (client 0x16 -> unmodified
// D2Game pick code); suppressed in combat modes; a fire can never start
// the retail walk-to-item mode (strict radius + collision parity).
BOOL __stdcall AUTO_GOLD_Active();   // config: auto_gold on?
void __stdcall AUTO_GOLD_Tick();     // per-frame trigger, called from DRAW_UI
// [globe-persistent] always-on orb text (FEATURES.md row 1, repurposed
// post-0.0.4). The 0.0.4 orb-face readout was removed after the first field
// round - retail already prints the same cur/max above each orb on hover,
// so ours was a duplicate display. With [features] globe_persistent=1,
// Panel.cpp's two orb-text draw conditions also fire without hover, so
// retail's above-orb Life/Mana text shows persistently; 0 = vanilla
// hover-only. Display-only, no state, no input changes.
BOOL __stdcall VANILLAPLUS_GlobeTextPersistent(); // per-frame, Panel.cpp draw conditions
// [vanilla-plus item-ilvl] Optional " (N)" suffix on the tooltip's item
// name (D2R "Advanced Tooltips" parity, the ilvl readout half). d2vp.ini next
// to d2.exe:
//   [features]
//   item_ilvl=1
// Default/missing file/section/key/garbage value = 0 = vanilla byte-for-byte:
// the formatter returns without touching the name buffer, so the upstream
// builders (Inv.cpp LoadItemDesc identified + unidentified paths and
// ITEM_BuildSetItemTooltip) proceed exactly as retail. While on, the hovered
// item's name line reads "Long Sword (12)" - the suffix is appended to
// the GetItemName buffer BEFORE the caller's AppendAsWideChar color wrap, so
// it renders in the item's own name color (gold on uniques, green on sets).
// One line, no extra tooltip row, same coverage as the tooltip line it
// replaces. Data source: the client's own item unit, D2Common's parsed item
// data (same struct that feeds the retail req-level line) - ilvl travels in
// the item bitstream on every path that reaches this tooltip (dropped,
// bought, gambled, traded, socketed, mercenary). The suffix is a hardcoded
// ASCII wide string: the vanilla string table has no such entry and the fork
// never edits MPQ data. Name buffers have big slack (real names cap near 40
// chars vs 0x80-0x400 buffers; GetItemName truncates at nSize), and the
// suffix adds <= 7 wchars. Display-only, no state, no input changes; the
// tooltip is rebuilt every frame the cursor dwells, so the 2 s hot-read gate
// below applies the ini flip without any rebuild trigger.
BOOL __stdcall ITEMILVL_Active(); // config: item_ilvl on?
// Appends " (N)" to the caller's item-name wide buffer, or returns
// untouched when the gate is off / the name is empty / the item carries no
// level. Called from Inv.cpp's three GetItemName sites right after the name
// is built and before the color wrap; the suffix inherits the name's color
// from the caller's own AppendAsWideChar. Gate on: the name's trailing
// spaces/tabs are trimmed off first (R59) - the GetItemName composition
// glues string-table entries and some (magic affix words, unique names) end
// with their own space, which read as a double gap before (N).
void __stdcall ITEMILVL_AppendNameSuffix(struct D2UnitStrc * pItem, uint16_t * wszName);
// [vanilla-plus shift-compare] Hold-Shift equipped-item compare tooltip
// (FEATURES.md row 0b, R125). D2R hover-compare parity: with
// [features] shift_compare=1, holding the key bound to retail cmd 0x24
// (CMD_SetClickModifier, default VK_SHIFT - a keybind remap is honored)
// while an equippable item is hovered in a panel draws a SECOND tooltip
// beside the hovered item's: the equipped item for the matching body slot.
// Side-by-side viewing only - no stat-delta lines, that is D2R's own
// compare scope (diablo2.io launch-era statement, SHIFT-COMPARE-PLAN.md
// Sources). Placement: left of the primary with a 4 px gap, falling back
// to the right when that leaves the screen; the same X/bottom clamp
// idioms Draw_UnitNameUnderCursor applies.
//
// Coverage (owner decision 2026-11-06, "shop stock included"): every
// context that publishes through Inv.cpp's single LoadItemDesc call site
// - player inventory, stash, cube, vendor shop (BOTH your own items and
// the vendor's stock: the stock hover takes the shop-narrow name+price
// builder branch, and the compare capture runs right after either branch
// publishes), trade screen. Hovering the paperdoll never compares (those
// items are already equipped); hovering a non-equippable (potion, charm,
// jewel) resolves no body slot and draws no box; rings compare against
// the first occupied of {6,7} (retail's first-fit order); with both arm
// cells occupied the same-kind slot wins (R126: a shield/quiver hover
// compares the off-hand bodyloc 5, a weapon hover the primary bodyloc 4 -
// weapons and shields share the ItemTypes body pair rarm/larm, so kind
// not bl order picks the box, retail's own auto-equip convention);
// weapons are 2H-aware (a 2H occupying both arms describes from slot 4, the
// same paired-cell convention INV_UpdateBodySlotUnderCursor uses). Equipped
// set items compose through LoadItemDesc's own quality-5 route into
// ITEM_BuildSetItemTooltip, so the compare box carries the set-bonus
// lines. The compare is one composed tooltip per frame: the capture
// saves pItemToDescribe + g_nCurrentInventoryPage (+ the primary's
// published DisplayedTooltip), swaps in the equipped item, calls
// LoadItemDesc(pPlayer, 1), copies the result into the second channel
// (D2WinFont.cpp gVpCompareTooltip, drawn by DRAW_CompareTooltipUnderCursor
// right after the primary draw call), then restores everything - zero
// footprint on the upstream builders themselves. Default/missing/garbage
// ini = 0 = retail byte-for-byte (charter rule 1); 2 s hot-read like
// every gate; while a panel is open held-Shift is inert in world space,
// and the click-modifier semantics of Shift are untouched.
BOOL __stdcall SHIFTCOMPARE_Active();      // config: shift_compare on?
BOOL __stdcall VPCTRLXFER_Active();        // config: ctrl_click_xfer on?
void __stdcall VPCTRLXFER_ParkPlace(int nGuid, int nX, int nY, int nPageByte); // R141: park the 0x18 half of the pair (Inv.cpp click branch)
void __stdcall VPCTRLXFER_TickPendingPlace(); // R141: release the parked 0x18 one frame later (Inv.cpp stash draw site)
BOOL __stdcall VPCTRLXFER_PairInFlight(); // R149 #37, R150 round-trip window: TRUE from the park stamp until 500 ms - the click branch consumes the click while a pair's pick+place+echoes settle
BOOL __stdcall VPCTRLXFER_EchoSuppressHand(int nItemGuid); // R150 #37: TRUE while nItemGuid is the pair's picked item inside the round-trip window - Item.cpp's 0x19 echo skips the client-hand set (the item stays in its source cell; the hand never fills; see PairInFlight)
void __stdcall SHIFTCOMPARE_CompareForHover(); // per-frame trigger, Inv.cpp after the LoadItemDesc publish
// [vanilla-plus options page] d2vp.ini feature-toggle bridge (row 10). The
// page reads/flips the same [features] keys the gates consume; writes use
// WritePrivateProfileStringA and flush every gate's 2 s hot-read cache.
int __stdcall D2VP_GetFeatureToggle(const char* szKey);   // 0 = off/missing/garbage
int __stdcall D2VP_GetFeatureToggleDefault(const char* szKey, int nDefault); // R144: per-row read default (unlock_fps_interp's gate reads a missing key as 1)
void __stdcall D2VP_SetFeatureToggle(const char* szKey, int nValue); // write + flush
void __stdcall D2VP_ConfigReloaded();                     // bust all 2 s gate caches
// Boot-time consistency audit of the Vanilla+ injection points (count bumps,
// table rows). Halts loudly with a d2boot.log line when an edit was lost -
// see handoff.md §8. Cheap; called from ALTTOGGLE_EnsureIni.
void __stdcall VANILLAPLUS_SelfCheck();
// [vanilla-plus shared-stash] Client side of the shared stash (track N
// round 1 = one page; two shared pages + an outlined pager label since R75,
// TEN shared pages since R82, A HUNDRED since R146; server half lives in
// D2Game/PLAYER/VanillaPlusStash.cpp).
// d2vp.ini next to d2.exe, [features] shared_stash=0|1 - default/missing
// file/missing key/garbage = 0 = vanilla byte-for-byte (no tab pixels, no
// clicks, no state). While on, the stash panel gains an OUTLINED "Shared"
// text tab (top-right, no hotkey by owner decision): clicking the word
// flips the visible stash page by sending the C->S 0x4F Click-Button packet
// with the out-of-retail-range button id 0x7D and the ABSOLUTE desired page
// in the param word (0 personal, 1..100 shared pages 1..100); the <<
// and >> arrows beside the word step one page at a time. The server
// materializes the requested shared page into the retail stash grid
// (page 4) - every retail stash flow keeps working - and borrows the
// goldbank stat for the shared gold pool while a shared page is active, so
// the retail deposit/withdraw gold buttons move shared gold untouched. The
// save flush (PlayerSave::SaveGameAllGameTypes VP site) parks all hundred
// shared pages into _d2vp_shared.sss (v2 format; R82-R145 ten-page, pre-R82
// two-page and pre-R75 one-page files read back - the pages the file
// doesn't carry start empty)
// and restores the personal page + personal gold before any .d2s is built
// (I1: character saves stay retail-pure).
BOOL __stdcall VPSHAREDUI_FeatureActive();   // config: shared_stash on?
int __stdcall VPSHAREDUI_IsSharedPage();     // client mirror: 0 personal, 1..100 = shared page on screen
void __stdcall VPSHAREDUI_ResetForNewGame(); // entering a game starts personal
// Hit-test + pager for the tab composite (word + << >> arrows). TRUE = the
// click hit an element whose target differs from the current page: the
// mirror is updated, the 0x4F/0x7D toggle is sent with the desired page,
// and the caller must swallow the message (VP site in
// INV_HandleStashLeftClick). FALSE = miss or no-op - retail proceeds.
BOOL __stdcall VPSHAREDUI_HandleStashClick(int nMouseX, int nMouseY);
// Per-frame tab draw (VP site in Inv.cpp's stash draw branch, right after
// the gold-button draw). Draws the outlined word + pager arrows; draws
// nothing when the gate is off.
void __stdcall VPSHAREDUI_DrawTab();

// [vanilla-plus nointro] R86: the -nointro command-line switch. Opt-out by
// launcher argument (not d2vp.ini - it only means something when the user
// passes it), parsed ONCE per process from GetCommandLineA(): a
// whitespace-separated token exactly "-nointro" (case-insensitive; tokens
// of 32+ bytes can never match) sets the flag, anything else leaves the
// vanilla boot untouched. First query writes one [d2boot] mark so the field
// log pins the switch. Consumers: the two boot/title cutscene entries in
// D2Launch/MainMenus.cpp (PlaysBlizzardNorthCutScene and
// UIMENU_PlayIntroAndReturnToMainScreen) read this and, when it is set,
// never enter BeginCutSceneAtResolutionMode0 - the retail begin-failed
// fallthrough each site already has (no cutscene, straight to the next
// screen) becomes the boot path, which skips the Blizzard + Blizzard North
// logo .biks, the first-run D2/LoD intro .bik and the title-screen
// (click/timer) d2intro .bik. In-game quest cutscenes use a different call
// path and are NOT affected. Returns 0 without the switch: vanilla
// byte-for-byte.
BOOL __stdcall VPNOINTRO_IsRequested();

// [vanilla-plus unlock-fps] R142: presentation-FPS unlock (FEATURES.md row 17,
// plans/FPS-UNLOCK-PLAN.md). Owner v1 scope (2026-11-09): bool gate, fixed
// 60 Hz target, the decoupled present only, vsync untouched, option named
// "Unlock FPS". Retail shape: the simulation paces itself (SrvProcessAllGames
// keeps its own 1000/25 cadence and runs every pump iteration) and the 25 FPS
// *presentation* exists only because CLIENT_GameLoopFrame gates the draw
// behind the tick (gnFrameSkipCounter). Under [features] unlock_fps=1 the draw
// moves onto a 60 Hz present pacer - skip-never-queue: at the deadline draw
// once and advance a full period; >= 4 periods late re-anchors, never queues -
// so server ticks and client world updates keep the retail 25 Hz cadence
// byte-exact while cursor/tooltip/highlight redraw rides 60. The pump idle
// Sleep(10) paces to the deadline (Message.cpp), and D2GFX_EndScene's 5 ms
// anti-spin budget drops to 1 ms under the gate (its own 2 s-poll reader - no
// D2Client imports across the renderer boundary). Vsync: bVSYNC semantics
// untouched - on a 60 Hz panel vsync hands back exactly the 60 the pacer asks
// for; NOVSYNC lets the pacer rule. Sub-tick interpolation shipped R143 (plan
// §4 Phase 2) and took two field-driven revisions: R143.1 replaced the
// velocity prediction ("jittery, looks better at 0" - a prediction snaps by
// its own error every tick) with interpolation between two real samples, and
// R143.2 fixed the delivery (still "jittery at 1"): the snapshot now runs
// BEFORE the draw (the draw reads the live post-tick position - a pair caught
// up after the draw made renderPos sawtooth a full tick every boundary), the
// sprite world offset gets the delta unconditionally (R143.1 added it after
// DRAW_UpdateMouseWorldOffsets' shake early-return, so shadows/state
// overlays/culling stayed tick-locked while the camera glided), and the
// player-sprite cancel flips sign in perspective (MouseOffsets already
// carries the delta into projected coords). R149 (#38, perspective still
// jittery) re-derived the perspective half: the perspective pipeline anchors
// EVERY layer (floor/wall tiles, sprites, shadows) on the warp origin
// D2GFX_SetPerspectiveScale - the player's live precision position - so the
// screen-unit delta never reaches those layers (the sprite path's WorldOffset
// pre-add/internal-subtract neutralizes it) and the R143.2 perspective cancel
// was the only moving thing on screen. R149 glides the ORIGIN instead
// (UNLOCKFPS_GetPlayerRenderOffsetOrigin, the same two samples in 16.16
// precision units, added at ViewDraw2's SetPerspectiveScale) and keeps the
// screen-unit delta top-down-only (camera/MouseOffsets/Wall2-cancel all
// !perspective-gated; the cancel is a plain ADD in both modes).
// UNLOCKFPS_GetPlayerRenderOffset feeds the top-down camera, the sprite world
// offset and the player's sprite/shadow/state draws one frame-shared delta
// interpolated between two real position samples taken at each client world
// update (unlock_fps_interp, default on; ini-only through R143, a Vanilla+
// Options row since R144).
BOOL __stdcall UNLOCKFPS_Active();         // config: unlock_fps on? (2 s hot-read)
BOOL __stdcall UNLOCKFPS_ShouldPresent();  // pacer: draw now? (FALSE when gate off)
void __stdcall UNLOCKFPS_NotePresent();    // telemetry: a present actually happened
void __stdcall UNLOCKFPS_NoteClientTick(); // snapshot prev/cur pos pair (R143.2: called BEFORE the draw)
int __stdcall UNLOCKFPS_IdleSleepMs();     // pump idle: ms to sleep (<= 0 = retail Sleep(10))
// R143 sub-tick interpolation, R143.1 two-sample + R143.2 delivery revisions
// (plan §4 Phase 2): NoteClientTick - called before the draw block, every
// iteration a client world update ran - snapshots the player's draw-space
// position (GetTargetX/Y, the same fields every consumer reads live);
// UNLOCKFPS_GetPlayerRenderOffset then returns the frame-shared delta that
// carries the camera, the sprite world offset and the player's sprite/shadow/
// state draws from the previous sample toward the current one -
// offset = -(cur - prev) x (1 - alpha), alpha = ms since the last client world
// update / 40 clamped to one tick. Interpolation between two real samples:
// continuous at every tick boundary by construction (a prediction would snap
// by its own error every tick - the R143 field jitter). Write-nowhere: every
// consumer adds it to its draw position only; nothing here touches
// D2DynamicPathStrc. Fills 0 and returns FALSE whenever it should not apply
// (gate/interp off, no snapshot yet, null out) so every site reduces to
// retail; standing still returns 0 offsets.
BOOL __stdcall UNLOCKFPS_GetPlayerRenderOffset(int32_t* pnOffsetX, int32_t* pnOffsetY);
// R149 #38: the same two-sample delta in the path's 16.16 precision units,
// for the PERSPECTIVE warp origin - ViewDraw2 adds it to the player precision
// position it hands D2GFX_SetPerspectiveScale, so the perspective camera
// (the warp anchor every perspective layer - floor/wall tiles, sprites,
// shadows - projects around) glides between the same two samples the top-down
// consumers interpolate. Per-present cached since R151 - the warp and every
// player rider (body, shadow, states, selection shape, WW sprite) share one
// delta per frame; same gating and zero-fills
// as the screen-unit accessor (gate/interp off = byte-for-byte retail).
BOOL __stdcall UNLOCKFPS_GetPlayerRenderOffsetOrigin(int32_t* pnOffsetX, int32_t* pnOffsetY);

// [vanilla-plus automap-clock R147/R148] Session/System clock on the automap
// overlay (owner ask R147: "Let's add an in game clock to the Automap. Session
// Time and System Time"; R148: "change the Automap clock display to 1 line
// instead of 2. SESS: hh:mm:ss SYS: 12:01PM"). Display-only client feature -
// no server half, no packets, no files. One line that rides the retail
// automap text-overlay stack (AUTOMAP_Draw's font-1 family - right-aligned at
// WindowSizeX - width - 0x10, one 0x10 row, color 4) directly under the
// build-tag line, so the stack reads version, clock, difficulty. Gate
// [features] automap_clock (default/missing = 0 = vanilla byte-for-byte, no
// pixels), the shared 2 s hot-read shape, busted by the Options-page write
// bridge (D2VP_ConfigReloaded) so a flip lands on the next automap frame;
// the thirteenth Vanilla+ Options row (Automap Clock) since R147 - the page
// went to two columns of SEVEN (box 0x151 -> 0x171, back row 0x124 ->
// 0x144, still whole at 640x480).
//   SESS - real time elapsed since the current game was entered,
//      hh:mm:ss (zero-padded). Wall-clock elapsed, not tick-counted play
//      time: pause menus and cutscenes count - the honest stopwatch
//      reading. GetTickCount wraps at 49.7 days; unsigned subtraction is
//      wrap-correct for any session shorter than that.
//   SYS - the PC's local wall clock (Win32 GetLocalTime) in 12-hour form
//      with a trailing AM/PM and no space (12:01PM, 1:05AM), read fresh
//      every frame the automap draws.
// Text building: pure-ASCII char buffers through SStrPrintf widened 1:1
// into the uint16_t buffers D2WinFont draws.
void __stdcall VPCLOCK_ResetSession(); // game entry: anchor the session clock
// Draw the one line (the R147 pair merged). pnTextHeightOffset =
// AUTOMAP_TextHeightOffset (read + advanced one 0x10 row); nScreenX =
// SpellSel::WindowSizeX (the family's right-edge base). Draws nothing with
// the gate off.
void __stdcall VPCLOCK_Draw(int* pnTextHeightOffset, int nScreenX);

// [transparent-hud R154] fully-transparent bottom bar (FEATURES.md row 4,
// owner asks 2026-11-11, plans/TRANSPARENT-HUD-PLAN.md). With
// [features] transparent_hud=1 the bottom bar's background frame art
// (PANEL_DrawCharacterInfo's CtrlPnl7 / 800CtrlPnl7 DRAW_SOLID pieces -
// the console stone behind the globes and belt, including the chat area
// background and the orb sockets / button wells baked into the frame DC6s)
// plus the run button, the menu button and the hostile indicator are NOT
// drawn - the world shows through where they were. Everything functional
// keeps its vanilla draw and stays interactive: the globes (+ orb text and
// gold amount), the belt (slots, pop-belt art, item sprites, hotkey
// labels), the skill buttons, the XP bar, the stamina bar - and every
// panel, tooltip, the automap and chat text are never touched. The hidden
// run/menu buttons keep their hit areas and tooltips (input paths
// untouched: invisible is not intangible). Display-only, no renderer code:
// the skip is at the four D2Client UI call sites, upstream of every
// renderer. Gate = the charter: missing file/section/key or garbage = 0 =
// vanilla byte-for-byte; the shared 2 s hot-read cache, busted by the
// Options-page write bridge (D2VP_ConfigReloaded) so a flip lands on the
// next frame that draws the bar; one d2boot.log mark on first activation
// (the unlock-fps one-way latch shape). The fourteenth Vanilla+ Options
// row (Transparent HUD).
//   Call sites (the only consumers):
//     Panel.cpp  DRAW_UI_BordersAndOrbsAndFillers - the PANEL_DrawCharacter
//                Info / PANEL_DrawRunButton / PANEL_DrawMenuButton calls
//     Draw.cpp   DRAW_UI - the PANEL_DrawHostileIndicator call
BOOL __stdcall VANILLAPLUS_HudBarTransparent(); // per-frame, 2 s hot-read

// [camera-zoom R155] Z-key camera zoom (plans/CAMERA-ZOOM-PLAN.md; FEATURES
// row 15). With [features] camera_zoom=1, Z steps the world zoom
// 1.0 -> 1.25 -> 1.5 -> 1.0 while the world is on screen; the UI, panels,
// tooltips and the cursor stay 1:1 and clicks land where the zoomed world
// shows them. D3D renderer only this round - other renderers clamp to 1.0
// with a one-way d2boot.log mark. The zoom level resets to 1.0 each join
// (VPCLOCK_ResetSession slot). State publishes through D2GFX
// (SetCameraZoomFactor + SetCameraZoomWorldPass - the world-phase bracket
// opens around the ViewDraw2 call, so the renderer scales only world-phase
// draws); the cursor pixel is inverse-zoomed before the retail projection
// (SELECT_HandleMouseInput / ClickMap), so hit rects, walk targets and the
// SCmd packets stay byte-retail. Gate = the charter: missing
// file/section/key or garbage = 0 = vanilla byte-for-byte; 2 s hot-read,
// busted by the Options-page write bridge (D2VP_ConfigReloaded); one
// d2boot.log mark on first activation + one per level change. The fifteenth
// Vanilla+ Options row (Camera Zoom).
//   Call sites (the only consumers):
//     Game.cpp    InGameDraw          - VANILLAPLUS_CameraZoomFrame + the
//                                       world bracket around ViewDraw2
//     Select.cpp  SELECT_HandleMouseInput - the cursor inverse (hover/click)
//     Player.cpp  ClickMap            - the cursor inverse (walk/cast)
BOOL __stdcall VANILLAPLUS_CameraZoomActive();  // 2 s hot-read gate
void __stdcall VANILLAPLUS_CameraZoomFrame();   // per-frame poll + publish
void __stdcall VANILLAPLUS_CameraZoomUnproject(int* pnX, int* pnY); // cursor inverse
void __stdcall VPCAMERAZOOM_ResetForJoin();     // level 0 each join (internal)
