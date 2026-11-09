// vanilla-plus core gates - implementation. Fork-owned TU; see VanillaPlusCore.h
// for the charter.
// Include set mirrors D2Client.cpp's (the code moved verbatim from there).

#include "Source/D2Client/D2Client.h"
#include "Source/D2Client/UI/VanillaPlusCore.h"
#include "Source/D2Client/UI/VanillaPlusOptions.h" // self-check pins the row wiring
#include "Source/D2CMP/SRC/CelCmp.h"
#include "Source/D2CMP/SRC/SpriteCache.h"
#include "Source/D2Client/ClientModeInGame.h"
#include "Source/D2Client/Core/Archive.h"
#include "Source/D2Client/D2ClientStrc.h"
#include "Source/D2Client/D2DialogStrc.h"
#include "Source/D2Client/D2GfxDataStrc.h"
#include "Source/D2Client/D2IniConfigStrc.h"
#include "Source/D2Client/D2LightMapStrc.h"
#include "Source/D2Client/Draw/LightMap.h"
#include "Source/D2Client/Engine.h"
#include "Source/D2Client/Engine/Gfx.h"
#include "Source/D2Client/Engine/GfxUtil.h"
#include "Source/D2Client/GAME/Game/Game.h"
#include "Source/D2Client/GAME/Game/SGAMEDATA.h"
#include "Source/D2Client/GAME/Msg.h"
#include "Source/D2Client/GAME/SCmd.h"
#include "Source/D2Client/GAME/Select.h"
#include "Source/D2Client/GAME/Sound.h"
#include "Source/D2Client/GAME/Wall2.h"
#include "Source/D2Client/Message.h"
#include "Source/D2Client/SKILLS/SkillsPal.h"
#include "Source/D2Client/Sound/Sound.h"
#include "Source/D2Client/Sound/SoundHdr.h"
#include "Source/D2Client/Source/ClientEngine.h"
#include "Source/D2Client/UI/Automap.h"
#include "Source/D2Client/UI/CmdTbl.h"
#include "Source/D2Client/UI/Dialog.h"
#include "Source/D2Client/UI/Draw.h"
#include "Source/D2Client/UI/EscMenu.h"
#include "Source/D2Client/UI/ui.data.h" // VP: gnUIFlagCount/gaUIFlagDescs/gUIFlagDismissModeTable decls (TU-private since 962f88d5)
#include "Source/D2Client/UI/Inv.h"
#include "Source/D2Client/UI/Panel.h"
#include "Source/D2Client/UI/Path.h"
#include "Source/D2Client/UI/QuestLog.h"
#include "Source/D2Client/UI/SkillDesc.h"
#include "Source/D2Client/UI/SpellSel.h"
#include "Source/D2Client/UI/Text.h"
#include "Source/D2Client/UI/UseItem.h"
#include "Source/D2Client/UI/npcmenu.h"
#include "Source/D2Client/UI/npcmenu/Merc.h"
#include "Source/D2Client/UI/ui.h"
#include "Source/D2Client/UNIT.h"
#include "Source/D2Client/UNIT/CUnit.h"
#include "Source/D2Client/UNIT/CUnitEvent.h"
#include "Source/D2Client/UNIT/Item.h"
#include "Source/D2Client/UNIT/MonUnique.h"
#include "Source/D2Client/UNIT/Monster.h"
#include "Source/D2Client/UNIT/Player.h"
#include "Source/D2Client/UNIT/UnitMode.h"
#include "Source/D2Client/Warden/WardenClient.h"
#include "Source/D2Common/D2BitBufferStrc.h"
#include "Source/D2Common/D2InventoryStrc.h"
#include "Source/D2Common/D2MissilesTxt.h"
#include "Source/D2Common/D2MonStatsTxt.h"
#include "Source/D2Common/D2RoomExStrc.h"
#include "Source/D2Common/D2RoomStrc.h"
#include "Source/D2Common/D2SeedStrc.h"
#include "Source/D2Common/D2SkillsTxt.h"
#include "Source/D2Common/D2StatListExStrc.h"
#include "Source/D2Common/D2WaypointStrc.h"
#include "Source/D2Common/DATATBLS/LvlTbls.h"
#include "Source/D2Common/DATATBLS/OverlayTbls.h"
#include "Source/D2Common/INVENTORY/Inventory.h" // [shift-compare] INVENTORY_GetItemByGridOffset / GetWeaponHandedness
#include "Source/D2Common/DUNGEON/Dungeon.h"
#include "Source/D2Common/Drlg/D2DrlgActStrc.h"
#include "Source/D2Common/Drlg/D2DrlgDeleteStrc.h"
#include "Source/D2Common/Drlg/D2DrlgStrc.h"
#include "Source/D2Common/Drlg/Path.h"
#include "Source/D2Common/Drlg/Preset.h"
#include "Source/D2Common/Environment.h"
#include "Source/D2Common/ITEMS/Items.h"
// [tenth sync] D2Common/Skills.h retired by upstream's restructure; this TU uses no
// symbol from it that D2SkillsTxt.h (above) and D2UnitStrc.h don't already provide.
#include "Source/D2Common/Stats/StatsEx.h"
#include "Source/D2Common/D2UnitStrc.h" // R123: upstream refiled the header D2Common/Unit/ -> D2Common/
#include "Source/D2Common/Unit.h"
#include "Source/D2Common/Units.h"
#include "Source/D2DDraw/Renderer/DirectDraw.h"
#include "Source/D2Game/D2ParsedTreasureClassStrc.h"
#include "Source/D2Game/GAME/Event.h"
#include "Source/D2Game/GAME/Targets.h"
#include "Source/D2Game/Objects/Objects.h"
#include "Source/D2Game/PLAYER/Player.h"
#include "Source/D2Game/PLAYER/PlayerTrade.h"
#include "Source/D2Game/UNIT/PlayerList.h"
#include "Source/D2Game/UNIT/SUnit.h"
#include "Source/D2Game/UNIT/SUnitDmg.h"
#include "Source/D2Game/UNIT/SUnitInactive.h"
#include "Source/D2Game/UNIT/SUnitMsg.h"
#include "Source/D2Lang/StrTable/strtable.h"
#include "Source/D2Lang/Unicode/UNISYS.h"
#include "Source/D2Launch/MainMenus.h"
#include "Source/D2Sound/Src/D2SoundSmp.h"
#include "Source/D2Win/D2GFX.h"
#include "Source/D2Win/DC6.h"
#include "Source/D2Win/DC6Block.h"
#include "Source/D2Win/Src/D2WinArchive.h"
#include "Source/D2Win/Src/D2WinFont.h"
#include "Source/D2Win/Src/D2WinPalette.h"
#include "Source/D2gfx/D2GFX.h"
#include "Source/Fog/D2PoolManagerStrc.h"
#include "Source/Fog/Debug.h"
#include "Source/Fog/Fog.h"
#include "Source/Fog/Memory.h"
#include "Source/Fog/SMem.h"
#include "Source/Fog/Source/SFile.h"
#include "Source/Fog/Src/AsyncData.h"
#include "Source/Fog/Src/ErrorManager.h"
#include "Source/Fog/Src/LogManager.h"
#include "3rdparty/Storm/D2ArchiveStrc.h"
#include "3rdparty/Storm/D2MPQFileStrc.h"
#include "3rdparty/Storm/Source/SFile.h"
#include "3rdparty/Storm/Source/SSTR.h" // [item-ilvl] SStrPrintf for the level digits
#include "3rdparty/Storm/Source/SSystem.h"
#include "3rdparty/Storm/Storm.h"
#include "Source/globals/globals.h"
#include "Source/D2Client/D2Client.globals.h" // VP: [tenth sync] the restructure moved the UI globals' externs (ScreenSizeX et al) into the per-module globals header; globals.h no longer carries them
#include "Source/D2Client/UI/Chat.h"
#include "Source/D2Client/UI/MiniPanel.h"
#include "Source/D2Client/UNIT/UnitSnd.h"
#include "Source/D2Common/DATATBLS/ItemStats.h"
#include "Source/D2Common/Drlg/Drlg.h"
#include "Source/D2Common/ITEMS/ItemMods.h"
#include "Source/D2Game/GAME/Arena.h"
#include "Source/D2Game/Monster/AI/AI.h"
#include "Source/D2Launch/CharSel.h"
#include "Source/D2Common/Collision.h" // [vanilla-plus auto-gold] GetDistanceBetweenUnits
#include "Redivivus/d2_boot.h" // [vanilla-plus] self-check failures
#include <stdlib.h> // exit (self-check halt)
#include <cstddef> // offsetof (flag-store tripwire)
#include <mmsystem.h> // [unlock-fps R142] timeBeginPeriod - 16 ms pacing needs 1 ms Sleep granularity (winmm is linked)

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
static int gbAltToggleMode = 0;
static int gbAltLatched = 0;
static DWORD dwAltToggleLastCheck = 0;
static int gbAltIniAttempted = 0;

// Resolve "<exe dir>\d2vp.ini". GetModuleFileNameA always yields a fully
// qualified path; refuse the (unreachable) dir-less case rather than fall
// back to a CWD-relative file.
// non-static: shared with VanillaPlusOptions.cpp's [ui] label_font reader
BOOL __stdcall ALTTOGGLE_BuildIniPath(char* szIniPath) {
    szIniPath[0] = '\0';
    DWORD dwLen = GetModuleFileNameA(nullptr, szIniPath, 240);
    if (!dwLen || dwLen >= 240) {
        return FALSE;
    }
    char* pSlash = strrchr(szIniPath, '\\');
    if (!pSlash) {
        return FALSE;
    }
    pSlash[1] = '\0';
    lstrcatA(szIniPath, "d2vp.ini");
    return TRUE;
}

// Best-effort template write. CREATE_NEW so a file that appears between the
// existence check and the create is never clobbered; any failure (read-only
// dir, AV lock, invalid path) stays silent - GetPrivateProfileIntA then keeps
// returning the vanilla defaults, exactly like a missing file. Even a partial
// write is safe: whatever GetPrivateProfileIntA can parse is a default.
static void ALTTOGGLE_WriteIniTemplate(const char* pszPath) {
    static const char szTemplate[] =
        "; d2vp.ini - vanilla-plus client options (auto-created by d2.exe)\r\n"
        ";\r\n"
        "; Missing file, section or key, or a garbage value, always falls back\r\n"
        "; to vanilla behavior. The file is re-read at most every 2 seconds\r\n"
        "; while the game runs. Delete this file and a fresh copy is written\r\n"
        "; on the next game start.\r\n"
        "\r\n"
        "[features]\r\n"
        "; Ground-item labels on the Alt key:\r\n"
        ";   0 = hold Alt to show labels         (vanilla behavior)\r\n"
        ";   1 = press Alt once to toggle labels (D2R \"Show Items: Toggle\")\r\n"
        "; While labels are toggled on, clicking still attacks, moves and\r\n"
        "; interacts normally; clicking an item's label picks the item up.\r\n"
        "alt_toggle=0\r\n"
        "\r\n"
        "; Save file location (single-player):\r\n"
        ";   0 = retail resolution: the registry save path if one is set,\r\n"
        ";       else the system Saved Games location\r\n"
        ";   1 = keep saves in the game folder, .\\save next to this file\r\n"
        ";       (the folder is created on demand, the registry is not\r\n"
        ";       touched, and existing saves are not migrated - copy your\r\n"
        ";       .d2s/.d2x/.key/.map files over to keep them)\r\n"
        "portable_save=0\r\n"
        "\r\n"
        "; Auto gold pickup (single-player focused):\r\n"
        ";   0 = off                                 (vanilla behavior)\r\n"
        ";   1 = walk over gold piles to pick them up. Uses the exact\r\n"
        ";       same server path as clicking the pile; never moves\r\n"
        ";       your character; disabled while attacking/casting;\r\n"
        ";       gold never sits on the cursor\r\n"
        "auto_gold=0\r\n"
        "\r\n"
        "; Persistent globe text:\r\n"
        ";   0 = the Life/Mana text above the orbs shows on hover only (vanilla)\r\n"
        ";   1 = keep the Life/Mana text above the orbs always visible\r\n"
        "globe_persistent=0\r\n"
        "\r\n"
        "; Item level tooltips:\r\n"
        ";   0 = item tooltips exactly as retail               (vanilla behavior)\r\n"
        ";   1 = append \" (N)\" to the item name on every item\r\n"
        ";       tooltip, e.g. Long Sword (12)\r\n"
        "item_ilvl=0\r\n"
        "\r\n"
        "; Shared stash (single-player):\r\n"
        ";   0 = off                                        (vanilla behavior)\r\n"
        ";   1 = add a \"Shared\" tab to the stash panel. The shared stash is\r\n"
        ";       one hundred extra stash pages (since R146; ten since R82) every\r\n"
        ";       character on this PC\r\n"
        ";       shares: items and gold deposited there survive the\r\n"
        ";       character. Stored in its own file (_d2vp_shared.sss) next to\r\n"
        ";       the saves - character .d2s files stay retail-compatible, with\r\n"
        ";       or without the key.\r\n"
        "shared_stash=0\r\n"
        "\r\n"
        "; Static world with one panel open (wide layouts only):\r\n"
        ";   0 = opening one panel slides the game world aside so the action\r\n"
        ";       re-centers in the uncovered strip    (vanilla behavior)\r\n"
        ";   1 = the world stays exactly where it is when the character\r\n"
        ";       screen or the inventory is open; the panel just floats\r\n"
        ";       over it. Applies at the wide stops (1280x720 / 1344x700 /\r\n"
        ";       1366x768 / 1920x1080) only - at 640x480 / 800x600 the vanilla slide\r\n"
        ";       is kept.\r\n"
        "no_panel_shift=0\r\n"
        "\r\n"
        "; Mana Burn fix (D2R 2.4 parity):\r\n"
        ";   0 = retail behavior: Mana Burn unique monsters drain 256x the\r\n"
        ";       intended mana with melee hits (512x in Hell) - a bug that\r\n"
        ";       ships in retail 1.14d and empties a mana pool in one hit\r\n"
        ";   1 = a Mana Burn melee hit drains the intended amount instead\r\n"
        ";       (66-100% of the monster's level damage; Nightmare and Hell\r\n"
        ";       still drain more, exactly like D2R after its 2.4 fix)\r\n"
        "mana_burn_fix=0\r\n"
        "\r\n"
        "; Shift item compare (D2R hover-compare parity):\r\n"
        ";   0 = off: item tooltips exactly as retail         (vanilla behavior)\r\n"
        ";   1 = hold Shift (or whatever the Click/Hold Move key is bound\r\n"
        ";       to) while hovering an item in a panel to draw a second\r\n"
        ";       tooltip beside it: the item you have equipped for that\r\n" 
        ";       slot. Works in the inventory, stash, cube, trade and at\r\n"
        ";       the vendor (your items and the shop stock). Side-by-side\r\n"
        ";       viewing only, like D2R's compare - no stat-delta lines.\r\n"
        "shift_compare=0\r\n"
        "\r\n"
        "; Ctrl+Click stash transfer (D2R parity):\r\n"
        ";   0 = off: Ctrl+Click does exactly retail           (vanilla behavior)\r\n"
        ";   1 = with the stash panel open, Ctrl+Click moves an item in one\r\n"
        ";       click: an inventory item goes to the stash, a stash item to\r\n"
        ";       the inventory; the Shared tab behaves the same. A full\r\n"
        ";       target plays the \"won't fit\" sound and nothing moves.\r\n"
        ";       Ctrl+Click in an open shop still sells.\r\n"
        "ctrl_click_xfer=0\r\n"
        "\r\n"
        "; Presentation FPS unlock (the Vanilla+ Options row is called\r\n"
        "; \"Unlock FPS\"):\r\n"
        ";   0 = the game presents at the classic 25 FPS    (vanilla behavior)\r\n"
        ";   1 = the picture (cursor, tooltips, target highlights, alt labels)\r\n"
        ";       refreshes at ~60 FPS while the game itself keeps the exact\r\n"
        ";       retail speed: ticks, movement, animations and timers are\r\n"
        ";       unchanged. Vsync behavior is untouched. d2boot.log carries a\r\n"
        ";       presents/ticks-per-second pair once per 5 s while this is on,\r\n"
        ";       as the proof the game speed is unchanged.\r\n"
        "unlock_fps=0\r\n"
        "\r\n"
        "; Sub-tick interpolation (R143) - rides Unlock FPS; the Vanilla+\r\n"
        "; Options row (R144) is called \"FPS Interpolation\":\r\n"
        ";   1 = while Unlock FPS is on, the world scroll and everything in it\r\n"
        ";       glide between game ticks at the present rate (the game speed is\r\n"
        ";       still exactly retail - this is render smoothing only)\r\n"
        ";   0 = the R142 look: the picture still refreshes at ~60 FPS but the\r\n"
        ";       world and characters step at the classic 25 Hz cadence\r\n"
        "unlock_fps_interp=1\r\n"
        "\r\n"
        "; Automap clock (R147):\r\n"
        ";   0 = off: the automap overlay lines exactly as retail (vanilla behavior)\r\n"
        ";   1 = the automap gains two lines under the build tag, right-aligned\r\n"
        ";       with the other overlay text: Session (real time since you\r\n"
        ";       entered the current game, H:MM:SS - pause menus and cutscenes\r\n"
        ";       count) and System (your PC's local clock, HH:MM 24-hour).\r\n"
        "automap_clock=0\r\n";
    HANDLE hFile = CreateFileA(pszPath, GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD dwWritten = 0;
    WriteFile(hFile, szTemplate, (DWORD)(sizeof(szTemplate) - 1), &dwWritten, nullptr);
    CloseHandle(hFile);
}

// Create d2vp.ini with documented defaults when none exists. At most one
// attempt per process: a file deleted while the game runs is recreated on
// the next game start, and a failed create (read-only dir) is not retried
// every 2 s.
void __stdcall ALTTOGGLE_EnsureIni() {
    VANILLAPLUS_SelfCheck();
    if (gbAltIniAttempted) {
        return;
    }
    gbAltIniAttempted = 1;
    char szIniPath[260];
    if (!ALTTOGGLE_BuildIniPath(szIniPath)) {
        return;
    }
    if (GetFileAttributesA(szIniPath) != INVALID_FILE_ATTRIBUTES) {
        return; // exists (or exists-but-inaccessible: never overwrite)
    }
    ALTTOGGLE_WriteIniTemplate(szIniPath);
}

BOOL __stdcall ALTTOGGLE_Active() {
    DWORD dwNow = GetTickCount();
    if (!dwAltToggleLastCheck || dwNow - dwAltToggleLastCheck >= 2000) {
        char szIniPath[260];
        dwAltToggleLastCheck = dwNow;
        if (ALTTOGGLE_BuildIniPath(szIniPath)) {
            // The first config read can precede the boot hook in odd flows
            // (e.g. non-interactive modes): create the template here too,
            // before the read, so the read always sees a parseable file.
            ALTTOGGLE_EnsureIni();
            // GetPrivateProfileIntA: missing file/section/key or a non-numeric
            // value all return the 0 default = vanilla hold mode.
            gbAltToggleMode = GetPrivateProfileIntA("features", "alt_toggle", 0, szIniPath) ? 1 : 0;
        } else {
            gbAltToggleMode = 0;
        }
    }
    return gbAltToggleMode;
}

BOOL __stdcall ALTTOGGLE_Latched() {
    return gbAltLatched;
}

void __stdcall ALTTOGGLE_Flip() {
    if (ALTTOGGLE_Active()) {
        gbAltLatched = !gbAltLatched;
    }
}

void __stdcall ALTTOGGLE_Reset() {
    gbAltLatched = 0;
}

// [vanilla-plus alt-toggle] True while labels are shown by the toggle latch
// (never true in hold mode). Vanilla's ground-label renderer (DRAW_AltItems)
// clears the selected unit every frame the cursor is not over a label, which
// turns every click into a walk while labels are on screen - survivable for a
// momentary Alt hold, fatal for a latched toggle. Toggle-latch mode hands
// selection back to the normal hover machinery instead (D2R "Show Items:
// Toggle" parity: labels stay up, clicks still attack/interact, and clicking
// an item's label or sprite picks the item up).
BOOL __stdcall ALTTOGGLE_LabelClickThrough() {
    return ALTTOGGLE_Active() && ALTTOGGLE_Latched();
}

// ===== [vanilla-plus auto-gold] =============================================
// D2R "Gold Pickup" parity. While [features] auto_gold=1 in d2vp.ini, gold
// piles near the player are picked up through the exact click path -
// INTERACT_AnyObject(UNIT_ITEM) sends the same 0x16 packet a click sends, and
// unmodified D2Game code answers with its own immediate SERVER_PickToInventory
// (gold-to-stat with retail cap semantics). Strict radius + collision parity:
// a pile is fired only when GetDistanceBetweenUnits < 5 AND
// TestCollisionBetweenInteractingUnits(0x804) is clear - the exact condition
// under which the server ITEM branch (PlayerMsg.cpp) picks instantly, so a
// fire can NEVER start the retail walk-to-item mode (the character is never
// moved by this feature). Combat guard: nothing fires outside
// Neutral/TownNeutral/Walk/Run, so attack/cast/gethit/block/sequence/
// knockback/dead frames never touch it. Missing/garbage/0 ini = 0 sends =
// vanilla byte-for-byte (charter rule 1). Single-player focused; the 0x16
// flow itself is untouched retail.

static int gbAutoGoldMode = 0;
static DWORD dwAutoGoldLastCheck = 0;   // 2 s ini re-read gate (row-28 pattern)
static DWORD dwAutoGoldLastScan = 0;    // 200 ms scan floor (UnitMode throttle style)

// Per-pile fire dedupe: 16-entry ring of (GUID, last fired tick). Keeps a
// pile that cannot be removed (gold cap full, removal in flight) from being
// re-fired on every 200 ms scan while the player stands inside the radius.
// Client-side only - no retail unit field is ever written.
static DWORD gdwAutoGoldRingGuid[16];
static DWORD gdwAutoGoldRingTick[16];
static int gnAutoGoldRingNext = 0;

BOOL __stdcall AUTO_GOLD_Active() {
    DWORD dwNow = GetTickCount();
    if (!dwAutoGoldLastCheck || dwNow - dwAutoGoldLastCheck >= 2000) {
        char szIniPath[260];
        dwAutoGoldLastCheck = dwNow;
        if (ALTTOGGLE_BuildIniPath(szIniPath)) {
            ALTTOGGLE_EnsureIni();
            // Missing file/section/key or a non-numeric value = 0 = vanilla.
            gbAutoGoldMode = GetPrivateProfileIntA("features", "auto_gold", 0, szIniPath) ? 1 : 0;
        } else {
            gbAutoGoldMode = 0;
        }
    }
    return gbAutoGoldMode;
}

// Per-frame trigger, called from DRAW_UI (UI/Draw.cpp). Guards in order; any
// fail = return with zero sends.
void __stdcall AUTO_GOLD_Tick() {
    D2RoomStrc** ppRooms;
    D2RoomStrc* pScanRoom;
    D2RoomStrc* pPlayerRoom;
    D2UnitStrc* pItem;
    D2UnitStrc* pPlayer;
    DWORD dwNow;
    DWORD dwGUID;
    int nRoomCount;
    int nRing;
    int i;
    BOOL bFired;

    if (!AUTO_GOLD_Active()) {
        return;
    }
    pPlayer = D2Client::UNIT::Player::GetPlayerUnit_UI(); // fully qualified: this TU is outside namespace D2Client (D2Client.cpp was inside)
    if (!pPlayer) {
        return;
    }
    // Combat guard: fire only from idle/walk modes.
    switch (pPlayer->eAnimMode.ePlayerMode) {
      case eD2PlayerAnimMode_ns::Neutral:
      case eD2PlayerAnimMode_ns::TownNeutral:
      case eD2PlayerAnimMode_ns::Walk:
      case eD2PlayerAnimMode_ns::Run:
        break;
      default:
        return;
    }
    dwNow = GetTickCount();
    if (dwAutoGoldLastScan && dwNow - dwAutoGoldLastScan < 200) {
        return;   // scan throttle
    }
    dwAutoGoldLastScan = dwNow;
    pPlayerRoom = D2Common::Units::UNITS_GetRoom(pPlayer);
    if (!pPlayerRoom) {
        return;
    }
    // Ground-item scan model of the label renderer (ShowItems): the player's
    // room plus its adjacent rooms, each room's pUnitList chain.
    ppRooms = nullptr;
    nRoomCount = 0;
    D2Common::DUNGEON::Dungeon::DRLGROOM_GetAdjacentRoomsList(pPlayerRoom, &ppRooms, &nRoomCount);
    if (!ppRooms || !nRoomCount) {
        return;
    }
    for (i = 0; i < nRoomCount; ++i) {
      pScanRoom = ppRooms[i];
      if (!pScanRoom) {
        continue;
      }
      for (pItem = pScanRoom->pUnitList; pItem; pItem = pItem->pRoomNext) {
        if (pItem->eUnitType != UNIT_ITEM) {
          continue;
        }
        if (!(pItem->eUnitFlagsEx >> 7 & 1)) {
          continue;   // not lying on the ground
        }
        if (pItem->eAnimMode.ePlayerMode != eD2PlayerAnimMode_ns::Run) {
          continue;   // ground-mode check (CUnit.cpp precedent)
        }
        if (D2Common::ITEMS::Items::ITEM_GetItemsTxt_szCode(pItem) != 0x20646c67) {
          continue;   // items.txt 4cc 'gld ' - gold piles only
        }
        // Strict radius: 5 is the server's own instant-pick distance.
        if (D2Common::Collision::GetDistanceBetweenUnits(pPlayer, pItem) >= 5) {
          continue;
        }
        // The server's own collision test in the same branch: with < 5 and a
        // clear 0x804 test the server always answers with an immediate pick,
        // so firing can never start its walk-to-item mode.
        if (D2Common::Units::TestCollisionBetweenInteractingUnits(pPlayer, pItem, 0x804)) {
          continue;
        }
        dwGUID = (DWORD)pItem->nUnitGUID;
        bFired = FALSE;
        for (nRing = 0; nRing < 16; ++nRing) {
          if (gdwAutoGoldRingGuid[nRing] == dwGUID && dwNow - gdwAutoGoldRingTick[nRing] < 1000) {
            bFired = TRUE;
            break;
          }
        }
        if (bFired) {
          continue;
        }
        gdwAutoGoldRingGuid[gnAutoGoldRingNext] = dwGUID;
        gdwAutoGoldRingTick[gnAutoGoldRingNext] = dwNow;
        gnAutoGoldRingNext = (gnAutoGoldRingNext + 1) & 15;
        D2Client::UNIT::UnitMode::INTERACT_AnyObject(UNIT_ITEM, dwGUID);   // the click path (0x16)
      }
    }
}

// ===== [globe-persistent] always-on orb text (FEATURES.md row 1) ============
// History: the 0.0.4 orb-face readout (patches 0013+0014) was removed after
// the first field round - retail's own PANEL_DrawPanelLayout already prints
// the same cur/max ABOVE each orb on hover (locale 0x1045/0x1046, baseline
// ScreenSizeY-0x5f, fed by its own "Show HP/MP Text" registry toggle), so
// the orb-face copy was a duplicate display. The slate row is repurposed:
// with globe_persistent=1, Panel.cpp's two orb-text draw conditions also
// fire without hover, pinning retail's above-orb Life/Mana text on (the
// consumer reads this gate directly; there is no draw code in this TU and
// nothing reads the retired 0.0.4 key a stale d2vp.ini may still carry).
static int gbGlobeTextPersistentMode = 0;
static DWORD dwGlobeTextPersistentLastCheck = 0;

BOOL __stdcall VANILLAPLUS_GlobeTextPersistent() {
    // 2 s hot-read cache (the shared gate pattern; this runs per frame).
    DWORD dwNow = GetTickCount();
    if (dwNow - dwGlobeTextPersistentLastCheck >= 2000) {
        dwGlobeTextPersistentLastCheck = dwNow;
        gbGlobeTextPersistentMode = D2VP_GetFeatureToggle("globe_persistent");
    }
    return gbGlobeTextPersistentMode;
}

// ===== [vanilla-plus item-ilvl] optional " (N)" item-name suffix =====
// D2R "Advanced Tooltips" parity (the ilvl readout half). See VanillaPlusCore.h
// for the feature contract. Gate = the shared 2 s hot-read cache pattern
// (globe_persistent shape): tooltip rebuilds happen every frame the cursor
// dwells, so no rebuild trigger is needed - the next hover after the flip
// carries the new state.
static int gbItemIlvlMode = 0;
static DWORD dwItemIlvlLastCheck = 0;

BOOL __stdcall ITEMILVL_Active() {
    DWORD dwNow = GetTickCount();
    if (!dwItemIlvlLastCheck || dwNow - dwItemIlvlLastCheck >= 2000) {
        dwItemIlvlLastCheck = dwNow;
        gbItemIlvlMode = D2VP_GetFeatureToggle("item_ilvl");
    }
    return gbItemIlvlMode;
}

// Appends " (N)" to the caller's item-name buffer (Inv.cpp's three
// GetItemName sites). No-op when the gate is off / the name is empty / the
// item carries no parsed level: the upstream builders then proceed
// byte-identical to retail. Gate ON: the name's trailing spaces/tabs are
// trimmed off first (R59) - GetItemName glues string-table entries (magic
// affix words, unique/set names, the separator) and some end with their own
// space, which used to read as a double gap before (N). The suffix joins the
// name buffer BEFORE the
// caller's AppendAsWideChar color wrap, so it renders in the item's own name
// color. Hardcoded ASCII: the vanilla string table has no such entry and the
// fork never edits MPQ data. Digit idiom = the FormatLine round's (SStrPrintf
// + char->wchar copy); AppendToBuffer is the tooltip builder's own concat.
void __stdcall ITEMILVL_AppendNameSuffix(D2UnitStrc * pItem, uint16_t * wszName) {
    uint16_t wszSuffix[24];
    char szSuffix[20];
    if (!wszName || *wszName == L'\0') {
        return;
    }
    if (!ITEMILVL_Active() || !pItem) {
        return;
    }
    int nItemLevel = D2Common::ITEMS::Items::ITEM_GetItemData_nItemLevel(pItem);
    if (nItemLevel < 1) {
        return;   // not an item unit / no parsed level (accessor returns 0)
    }
    // VP: [item-ilvl] R59 trailing-space trim - GetItemName glues string-table
    // entries together (base names, magic affix words, unique/set names, the
    // 0xf9e separator) and some of those entries end with their own space, so
    // a plain append read "Name  (12)". Strip the name's trailing spaces/tabs
    // first; the gap before (N) is then always the one space the format owns.
    // Sits below every early-out, so OFF still never writes the buffer.
    int nNameLen = 0;
    while (wszName[nNameLen] != L'\0') {
        nNameLen++;
    }
    while (nNameLen > 0 && (wszName[nNameLen - 1] == L' ' || wszName[nNameLen - 1] == L'\t')) {
        nNameLen--;
        wszName[nNameLen] = L'\0';
    }
    if (nNameLen == 0) {
        return;   // paranoia: name was all whitespace
    }
    memset(wszSuffix, 0, sizeof(wszSuffix));
    Storm::Source::SSTR::SStrPrintf(szSuffix, sizeof(szSuffix), " (%i)", nItemLevel);
    D2Lang::Unicode::UNISYS::STRING_CopyCharToWCharWithSetMaxSize(wszSuffix, szSuffix, sizeof(wszSuffix) / sizeof(wszSuffix[0]) - 1);
    D2Lang::Unicode::UNISYS::AppendToBuffer(wszName, wszSuffix);
}

// ===== [vanilla-plus shift-compare] hold-Shift equipped-item compare ========
// D2R hover-compare parity (FEATURES.md row 0b; the full behavior spec is
// SHIFT-COMPARE-PLAN.md + VanillaPlusCore.h). One code round (R125), owner
// decisions 2026-11-06: side-by-side second box, hold-Shift trigger, row
// label "Shift Item Compare", SHOP STOCK INCLUDED (hovering vendor stock
// compares too - the plan's draft v1 skipped it).
//
// Mechanism: Inv.cpp's DRAW_UI_Inventory publishes the hovered item's
// tooltip through exactly one LoadItemDesc call site (both the full builder
// and the shop-narrow name+price branch publish through
// TEXT_TooltipSetAttributes inside it). The VP block right after that call
// invokes SHIFTCOMPARE_CompareForHover(), which:
//   1. bails unless the gate is on, the cmd-0x24 key (default VK_SHIFT, a
//      keybind remap is honored - the exact per-frame read retail's
//      UpdateMouseModifierKeyState performs, Game.cpp:1193-1221) is down,
//      the hover is NOT a paperdoll body slot (gbInvBodySlotUnderCursorExists
//      - those items are already equipped), and pItemToDescribe names the
//      item LoadItemDesc just described this frame;
//   2. resolves the hovered item's ItemTypes bl1/bl2 to the equipped item
//      to compare against (rings first-occupied of {6,7}; arm items pick the
//      same-kind slot when both arms are occupied - R126: shields/quivers the
//      off-hand bodyloc 5, weapons the primary bodyloc 4, retail's own
//      choose_body_loc convention - with the 2H paired-cell convention,
//      handedness 2 describes from slot 4; everything else straight bl1;
//      both empty = no box, which also covers potions/charms/jewels);
//   3. composes the equipped item's tooltip by the save/swap/capture/
//      restore round trip below - zero footprint on the upstream builder
//      (the item_ilvl lesson: hook, don't refactor). The swap set is
//      exactly { pItemToDescribe, g_nCurrentInventoryPage }: the :8514
//      hover gate reads gpInvItemUnderCursor (a hover FLAG, still set - the
//      hover is real) and pItemToDescribe; the :8532 shop-narrow selector
//      additionally reads gnItemDescTitleMaxWidth (TU-private) + shop mode
//      + g_nCurrentInventoryPage - forcing the page to 0 makes the
//      "page - 1U > 8" term TRUE, which takes the FULL builder and also
//      suppresses the vendor price line the full path appends on shop
//      pages (:8783-8787), so the compare box is the plain equipped-item
//      tooltip everywhere, shop included. pUnit=pPlayer with bUsePlayer=1
//      (owner = the player: requirement colors check against the player).
//      Equipped set items ride LoadItemDesc's own quality-5 route into
//      ITEM_BuildSetItemTooltip, so the set-bonus lines come through.
//   4. publishes the composed text into the second channel
//      (D2WinFont.cpp gVpCompareTooltip via TEXT_CompareTooltipSetAttributes)
//      together with the primary's published rect snapshot, then restores
//      the globals AND the primary's DisplayedTooltip so the hovered
//      item's tooltip draws unchanged this frame. The snapshot makes the
//      side-by-side placement immune to the Alt-labels path wiping the
//      primary channel between capture and draw (ShowItems.cpp wipes run
//      in DRAW_AltItems, after the panel draws and before Draw.cpp:739).
// The channel self-clears on draw, so no state can outlive the frame.
static int gbShiftCompareMode = 0;
static DWORD dwShiftCompareLastCheck = 0;   // 2 s ini re-read gate (row-28 pattern)

BOOL __stdcall SHIFTCOMPARE_Active() {
    DWORD dwNow = GetTickCount();
    if (!dwShiftCompareLastCheck || dwNow - dwShiftCompareLastCheck >= 2000) {
        dwShiftCompareLastCheck = dwNow;
        gbShiftCompareMode = D2VP_GetFeatureToggle("shift_compare");
    }
    return gbShiftCompareMode;
}

// VP: [ctrl-click stash / R140] ctrl_click_xfer - the Ctrl+Click stash
// transfer's gate, same 2 s hot-read shape as the gate above. Read per
// Ctrl+Click rather than per frame, so the cache only saves ini opens; the
// write bridge busts dwCtrlClickXferLastCheck (D2VP_ConfigReloaded), so a
// flip in the Options page lands on the very next Ctrl+Click.
static int gbCtrlClickXferMode = 0;
static DWORD dwCtrlClickXferLastCheck = 0;   // 2 s ini re-read gate (row-28 pattern)

BOOL __stdcall VPCTRLXFER_Active() {
    DWORD dwNow = GetTickCount();
    if (!dwCtrlClickXferLastCheck || dwNow - dwCtrlClickXferLastCheck >= 2000) {
        dwCtrlClickXferLastCheck = dwNow;
        gbCtrlClickXferMode = D2VP_GetFeatureToggle("ctrl_click_xfer");
    }
    return gbCtrlClickXferMode;
}

// VP: [ctrl-click stash / R141] the deferred half of the transfer pair.
// R141 field catch (owner, first field pass on the R140 build): "The items
// move, but the slot they once occupied becomes unusable." Root cause: R140
// sent 0x19 (pick) + 0x18 (place) back to back in one click, so the client
// processed the pick echo and the place echo in the SAME packet drain - a
// sequence no retail flow produces (a manual drag settles the pick on the
// hand - item lifted out of the source grid - many frames before any place).
// The target container is fine (D2Common PutItemInInventory unlinks-before-
// relink, Inventory.cpp :755), but the source container's client-side grid
// entry went stale: the slot the item left refused every later placement.
// Fix, retail-shaped: the click sends 0x19 only and PARKS the place here;
// the stash-panel draw (Inv.cpp, every frame the stash view is open) calls
// VPCTRLXFER_TickPendingPlace, which releases the 0x18 on the second tick -
// one full frame (one packet drain) after the pick, exactly a fast manual
// drag. Drop conditions, both safe by construction:
//  - panel closed inside the 1-frame window: the tick stops running, the
//    0x18 never sends, and the item simply stays on the hand/cursor - the
//    interrupted-drag state, legal and player-recoverable (place it
//    manually, or drop it).
//  - a new game joins with a stale stage: impossible in practice (the
//    window is one frame), and if it ever fired the stale-GUID 0x18 is
//    rejected server-side (the hand is empty) - the same harmless reject
//    the R140 first build's lone 0x18 produced.
static int gnVpXferPlacePending = 0;   // 0 idle; 1 parked this frame; 2 release on the next tick
static int gnVpXferPlaceGuid = 0;
static int gnVpXferPlaceX = 0;
static int gnVpXferPlaceY = 0;
static int gnVpXferPlacePageByte = 0;

void __stdcall VPCTRLXFER_ParkPlace(int nGuid, int nX, int nY, int nPageByte) {
    gnVpXferPlaceGuid = nGuid;
    gnVpXferPlaceX = nX;
    gnVpXferPlaceY = nY;
    gnVpXferPlacePageByte = nPageByte;
    gnVpXferPlacePending = 1;
}

void __stdcall VPCTRLXFER_TickPendingPlace() {
    if (gnVpXferPlacePending == 1) {
        gnVpXferPlacePending = 2;   // hold one full frame after the park
        return;
    }
    if (gnVpXferPlacePending != 2) {
        return;                     // idle
    }
    gnVpXferPlacePending = 0;
    // Same two-packet pair a manual drag sends - now separated by one frame
    // the way every retail item flow separates them.
    D2Client::GAME::Msg::NET_D2GS_CLIENT_Send_INT_INT_INT_INT(0x18, gnVpXferPlaceGuid, gnVpXferPlaceX, gnVpXferPlaceY, gnVpXferPlacePageByte);
}


// The trigger read, retail's own idiom (UpdateMouseModifierKeyState,
// Game.cpp:1193-1221): the key bound to cmd 0x24 in either binding slot,
// GetAsyncKeyState(vk) < 0 = down. No CmdTbl entry, no latch, no keyup
// hook - a per-frame poll, deliberately NOT the alt_toggle latch shape.
static int SHIFTCOMPARE_ClickModifierDown() {
    uint16_t wVirtualKey;
    if (D2Client::UI::CmdTbl::CMD_IsCommandBound(0x24, 1)) {
        wVirtualKey = D2Client::UI::CmdTbl::CMD_GetKeyForCommand(0x24, 1);
        if (wVirtualKey != 0xffff && GetAsyncKeyState((uint32_t)wVirtualKey) < 0) {
            return 1;
        }
    }
    if (D2Client::UI::CmdTbl::CMD_IsCommandBound(0x24, 0)) {
        wVirtualKey = D2Client::UI::CmdTbl::CMD_GetKeyForCommand(0x24, 0);
        if (wVirtualKey != 0xffff && GetAsyncKeyState((uint32_t)wVirtualKey) < 0) {
            return 1;
        }
    }
    return 0;
}

// Hovered item -> equipped item to compare against. The hovered item's
// intended slot comes from its ItemTypes row (bl1/bl2), NEVER from the item
// instance (GetBodyLocation is only set once equipped). Slot map per
// SHIFT-COMPARE-PLAN.md 2: rings first-occupied of {6,7} (deterministic,
// retail's first-fit auto-equip order); arm items (weapons AND shields -
// vanilla ItemTypes.txt gives both classes body1=rarm/body2=larm) pick the
// same-kind slot when both arms are occupied (R126: shield/quiver hovers
// compare the off-hand bodyloc 5, weapon hovers the primary bodyloc 4 -
// retail's own auto-equip resolves the same ambiguity by kind, ItemMode.cpp
// choose_body_loc) and stay 2H-aware (a 2H occupying both arms describes
// from slot 4 - the same "2H describes from the paired cell" convention the
// equipped-slot hover path uses, INV_UpdateBodySlotUnderCursor); armor 3,
// helm 1, boots 9, gloves 10, belts 8, amulet 2 are straight bl1 lookups;
// both bodylocs empty (potion, charm, jewel, ...) = no compare box.
// Returns the equipped unit, or null when there is nothing to compare.
static D2UnitStrc * SHIFTCOMPARE_ResolveSlot(D2UnitStrc * pPlayer, D2UnitStrc * pItem) {
    eD2BodyLoc nBodyLoc1 = BODYLOC_NONE;
    eD2BodyLoc nBodyLoc2 = BODYLOC_NONE;
    D2InventoryStrc* pInventory;
    D2UnitStrc* pEquipped;

    D2Common::ITEMS::Items::ITEM_GetTxtItemTypes_BodyLoc1andBodyLoc2(pItem, &nBodyLoc1, &nBodyLoc2);
    pInventory = pPlayer->pInventory;
    if (!pInventory) {
        return nullptr;
    }

    if (nBodyLoc1 == BODYLOC_NONE && nBodyLoc2 == BODYLOC_NONE) {
        return nullptr;   // not equippable: no compare box
    }

    if (nBodyLoc1 == BODYLOC_R_RING) {
        // Rings (bl1=6, bl2=7): compare against the first occupied of {6, 7}.
        pEquipped = D2Common::INVENTORY::Inventory::INVENTORY_GetItemByGridOffset(pInventory, BODYLOC_R_RING);
        if (!pEquipped) {
            pEquipped = D2Common::INVENTORY::Inventory::INVENTORY_GetItemByGridOffset(pInventory, BODYLOC_L_RING);
        }
        return pEquipped;
    }

    if (nBodyLoc1 == BODYLOC_R_ARM || nBodyLoc1 == BODYLOC_L_ARM) {
        // Arm items - weapons AND shields share the SAME ItemTypes body pair
        // (vanilla ItemTypes.txt: every weapon class and the Shield row read
        // body1=rarm/body2=larm), so the bl order cannot tell the primary
        // weapon box (bodyloc 4, screen-left) from the off-hand box (5,
        // screen-right). R125 tried bl1 first, so a shield hover compared the
        // primary weapon (owner field report on the R125 build). Retail hits
        // the same ambiguity in its own auto-equip and resolves it by item
        // kind (ItemMode.cpp choose_body_loc's type-conflict check): when
        // both arm cells are occupied the same-kind slot wins - shield/quiver
        // hovers compare the off-hand, weapon hovers the primary. One arm
        // occupied = that item (the only possible comparison); both empty =
        // no box (returned above).
        int nFirst = (int)BODYLOC_R_ARM;    // primary weapon box (screen-left)
        int nSecond = (int)BODYLOC_L_ARM;   // off-hand box (shields/quivers)
        if (D2Common::ITEMS::Items::ITEMS_IsA(pItem, ITEMTYPE_AnyShield)
            || D2Common::ITEMS::Items::ITEMTYPE_IsQuiver((int)D2Common::ITEMS::Items::ITEM_GetItemData_wType(pItem))) {
            nFirst = (int)BODYLOC_L_ARM;
            nSecond = (int)BODYLOC_R_ARM;
        }
        pEquipped = D2Common::INVENTORY::Inventory::INVENTORY_GetItemByGridOffset(pInventory, nFirst);
        if (!pEquipped) {
            pEquipped = D2Common::INVENTORY::Inventory::INVENTORY_GetItemByGridOffset(pInventory, nSecond);
        }
        if (!pEquipped) {
            return nullptr;
        }
        if (D2Common::INVENTORY::Inventory::INVENTORY_GetWeaponHandedness(pPlayer, pInventory) == 2) {
            // A 2H occupies both arms: describe slot 4's item - the same
            // paired-cell convention INV_UpdateBodySlotUnderCursor uses.
            pEquipped = D2Common::INVENTORY::Inventory::INVENTORY_GetItemByGridOffset(pInventory, BODYLOC_R_ARM);
            if (!pEquipped) {
                pEquipped = D2Common::INVENTORY::Inventory::INVENTORY_GetItemByGridOffset(pInventory, BODYLOC_L_ARM);
            }
            if (!pEquipped) {
                return nullptr;
            }
        }
        return pEquipped;
    }

    // Everything else: straight bl1; if that bodyloc is empty, bl2 (both
    // empty already returned above - one occupied bodyloc is guaranteed).
    pEquipped = D2Common::INVENTORY::Inventory::INVENTORY_GetItemByGridOffset(pInventory, nBodyLoc1);
    if (!pEquipped) {
        pEquipped = D2Common::INVENTORY::Inventory::INVENTORY_GetItemByGridOffset(pInventory, nBodyLoc2);
    }
    return pEquipped;
}

// Per-frame trigger, called from the VP block in Inv.cpp's DRAW_UI_Inventory
// immediately after the LoadItemDesc call site - the one point where the
// primary tooltip (hovered item) is freshly published and the frame's
// context globals are live. Gate off / Shift up / paperdoll hover / no item
// under cursor / nothing to compare = returns with zero side effects:
// exactly today's single tooltip.
void __stdcall SHIFTCOMPARE_CompareForHover() {
    D2TooltipTextStrc sPrimary;
    D2UnitStrc* pHovered;
    D2UnitStrc* pPlayer;
    D2UnitStrc* pEquipped;
    D2UnitStrc* pSavedDescribe;
    int nSavedPage;

    if (!SHIFTCOMPARE_Active()) {
        return;
    }
    if (D2Client::UI::Inv::gbInvBodySlotUnderCursorExists) {
        return;   // paperdoll hover: the item is already equipped, no compare
    }
    pHovered = D2Client::UI::Inv::pItemToDescribe;
    if (!pHovered) {
        return;   // nothing hovered this frame (also covers cursor-held items)
    }
    if (!SHIFTCOMPARE_ClickModifierDown()) {
        return;   // Shift up: exactly today's single tooltip
    }
    pPlayer = D2Client::UNIT::Player::GetPlayerUnit_UI();
    if (!pPlayer || !pPlayer->pInventory) {
        return;
    }
    pEquipped = SHIFTCOMPARE_ResolveSlot(pPlayer, pHovered);
    if (!pEquipped || pEquipped == pHovered) {
        return;   // empty target slot, or the hover IS the equipped item
    }

    // The compose round trip. The primary tooltip is saved wholesale: it is
    // restored below so the hovered item's box draws unchanged this frame,
    // and its published rect rides into the compare channel for the
    // side-by-side placement (snapshot - immune to the Alt-labels wipes).
    memcpy(&sPrimary, &D2Win::Src::D2WinFont::DisplayedTooltip, sizeof(sPrimary));
    pSavedDescribe = D2Client::UI::Inv::pItemToDescribe;
    nSavedPage = D2Client::UI::Inv::g_nCurrentInventoryPage;

    D2Client::UI::Inv::pItemToDescribe = pEquipped;
    D2Client::UI::Inv::g_nCurrentInventoryPage = 0;   // forces the FULL builder (:8532) and no price line (:8783)
    D2Win::Src::D2WinFont::TEXT_TooltipSetAttributes(nullptr, 0, 0, 0, 0); // clear: a compose early-out must be detectable
    D2Client::UI::Inv::LoadItemDesc(pPlayer, 1);   // bUsePlayer=1: owner = the player
    if (D2Win::Src::D2WinFont::DisplayedTooltip.wszString[0]) {
        D2Win::Src::D2WinFont::TEXT_CompareTooltipSetAttributes(
            D2Win::Src::D2WinFont::DisplayedTooltip.wszString,
            sPrimary.nX, sPrimary.nY, sPrimary.nColor, sPrimary.nCenterFlag,
            D2Win::Src::D2WinFont::DisplayedTooltip.nColor);
    }

    D2Client::UI::Inv::pItemToDescribe = pSavedDescribe;
    D2Client::UI::Inv::g_nCurrentInventoryPage = nSavedPage;
    memcpy(&D2Win::Src::D2WinFont::DisplayedTooltip, &sPrimary, sizeof(sPrimary));   // primary draws unchanged
}

// ===== [vanilla-plus no-panel-shift] ========================================
// (issue #26 / R95) Retail slides the whole game world a quarter screen
// sideways when exactly one panel is open (Path.cpp TypeOfUiOverlayOverWorld:
// MouseYRelatedB = -/+ScreenSizeX/4) so the action re-centers in the strip
// the panel leaves uncovered - exact only when that panel reaches the screen
// edge (640x480/800x600). At the wide stops the 800-class UI floats 240 px
// inboard, so the slide drags world out from under the visible outer margins
// and the perspective projection (player-anchored, expanding from the screen
// center) makes every panned channel a separate artifact source - the whole
// R92-R94 tail of issue #26 lived on that panned path. While [features]
// no_panel_shift=1, the world renders exactly like the both-panels-open case
// (border 3: no slide, full unpanned view rect) on the wide stops and the
// open panel overdraws its own rect. Implementation: Path.cpp keeps the
// border-0 camera for border 1/2, and every world-draw call site reads its
// border type through PANELSHIFT_BorderForWorld, so the renderers receive 0
// and take their own field-proven full-screen no-pan path - zero renderer
// edits, and the R92-R94 renderer guards stay live for the vanilla slide
// (no_panel_shift=0). UI/interaction consumers (panel click rects, label
// culls, selection guards, CUnit R92 click rejection) keep reading the real
// border type. Retail 640x480/800x600: the gate is false, every call site
// sees its retail value, byte-identical. Missing/garbage/0 ini = 0 = vanilla
// slide (charter rule 1); 2 s hot-read like every gate.

static int gbNoPanelShiftMode = 0;
static DWORD dwNoPanelShiftLastCheck = 0;   // 2 s ini re-read gate (row-28 pattern)

BOOL __stdcall PANELSHIFT_NoShiftWorld() {
    DWORD dwNow = GetTickCount();
    if (!dwNoPanelShiftLastCheck || dwNow - dwNoPanelShiftLastCheck >= 2000) {
        char szIniPath[260];
        dwNoPanelShiftLastCheck = dwNow;
        if (ALTTOGGLE_BuildIniPath(szIniPath)) {
            ALTTOGGLE_EnsureIni();
            // Missing file/section/key or a non-numeric value = 0 = vanilla.
            gbNoPanelShiftMode = GetPrivateProfileIntA("features", "no_panel_shift", 0, szIniPath) ? 1 : 0;
        } else {
            gbNoPanelShiftMode = 0;
        }
    }
    // Wide stops only - the retail slide is exact because the open panel
    // reaches the screen edge there. Same backbuffer-dims test as the
    // R92-R94 renderer guards (no D2Client imports across that boundary;
    // here the globals are direct).
    return gbNoPanelShiftMode
        && ((D2Client::UI::ui::ScreenSizeX == 1280 && D2Client::UI::Inv::ScreenSizeY == 720)
            || (D2Client::UI::ui::ScreenSizeX == 1344 && D2Client::UI::Inv::ScreenSizeY == 700)
            || (D2Client::UI::ui::ScreenSizeX == 1366 && D2Client::UI::Inv::ScreenSizeY == 768) // VP [R112]: 1366x768 joins the wide class
            || (D2Client::UI::ui::ScreenSizeX == 1920 && D2Client::UI::Inv::ScreenSizeY == 1080)); // VP [R115]: 1920x1080 joins the wide class
}

uint32_t __stdcall PANELSHIFT_BorderForWorld(uint32_t eBorderType) {
    if ((eBorderType == 1 || eBorderType == 2) && PANELSHIFT_NoShiftWorld()) {
        return 0;
    }
    return eBorderType;
}

// ===== [vanilla-plus unlock-fps] R142 - presentation-FPS unlock =============
// FEATURES.md row 17 / plans/FPS-UNLOCK-PLAN.md. See VanillaPlusCore.h for the
// mechanism summary. This TU owns the pacer state and the gate; the three
// upstream consumers are one-line sites: the draw dispatch
// (Game.cpp CLIENT_GameLoopFrame), the pump idle sleep (Message.cpp
// MessageGameLoop) and the EndScene budget (D2GFX.cpp, which keeps its own
// reader - renderer boundary). Design points the code below must keep true:
//   - Skip-never-queue (the frame-skip lesson, moved to the present side): at
//     the deadline draw once and advance the deadline a full period; when the
//     pump was away (message storm, loading, the SP pause path - none of them
//     poll ShouldPresent) the anchor is >= 4 periods stale, so re-anchor
//     instead of looping the backlog off. Small lateness (pump wake jitter,
//     a few ms) stays on the absolute grid - the += keeps the average at
//     exactly 16 ms/present instead of compounding wake jitter upward.
//   - The pacer only decides when to DRAW, never when to poll: input rides
//     the message pump exactly as retail.
//   - Unanchor on gate-off and recency-out (menus/loading/pause stop polling
//     ShouldPresent; after 100 ms the pump falls back to the retail Sleep(10)
//     by itself, so those loops keep their vanilla pacing untouched).
//   - Telemetry (pillar 1 of the plan - sim identity proven by counting, not
//     eyeballing): while the gate is on, one d2boot mark per 5 s window pairs
//     the present rate with the client-tick rate. Zero marks when off.

static const DWORD UNLOCKFPS_PERIOD_MS = 16; // fixed 60 Hz v1 target (owner: bool for v1, numeric later)
static const DWORD UNLOCKFPS_SIM_TICK_MS = 40; // [unlock-fps-interp R143] retail client world-update period (25 Hz) - the alpha clock base

static int gbUnlockFpsMode = 0;
static int gbUnlockFpsInterp = 0;           // [unlock-fps-interp R143] unlock_fps_interp sub-gate (default 1; ini-only through R143, the FPS Interpolation Options row since R144)
static DWORD dwUnlockFpsLastCheck = 0;      // 2 s ini re-read gate (row-28 pattern)
static DWORD dwUnlockFpsNextPresent = 0;    // pacer deadline (GetTickCount); 0 = unanchored
static DWORD dwUnlockFpsLastPoll = 0;       // recency of the last ShouldPresent poll
static DWORD dwUnlockFpsLastClientTick = 0; // [unlock-fps-interp R143] alpha clock anchor: GetTickCount at the last client world update
// [unlock-fps-interp R143.1] two-sample interpolation state: the player's
// draw-space position - the same D2Common::Units::GetTargetX/Y every consumer
// site reads - snapshotted at each client world update. The render offset
// glides the world from the previous sample to the current one (interpolation
// between two real samples: continuous at every tick boundary by construction,
// no velocity prediction to be wrong - R143's extrapolation snapped by its own
// prediction error every tick, the field jitter the owner reported).
static const int UNLOCKFPS_INTERP_SNAP_PX = 128; // per-tick displacement beyond this = teleport/level change: snap, don't glide
static int gnUnlockFpsPrevPosX = 0;
static int gnUnlockFpsPrevPosY = 0;
static int gnUnlockFpsCurPosX = 0;
static int gnUnlockFpsCurPosY = 0;
static int gbUnlockFpsHaveSnap = 0;                 // prev/cur pair valid (player seen at least once)
static int gnUnlockFpsOffsetX = 0;                  // per-present cache: one offset per frame,
static int gnUnlockFpsOffsetY = 0;                  // shared by the camera and every sprite draw
static DWORD dwUnlockFpsFrameId = 1;                // [R143.1] increments per present - the interp cache epoch (starts 1 so the 0 cache key never matches)
static DWORD dwUnlockFpsOffsetFrame = 0;            // [R143.1] present epoch the cached offset was computed for
static int sbUnlockFpsTimerRaised = 0;      // timeBeginPeriod(1) one-way latch
static DWORD dwUnlockFpsStatsWindowTick = 0;
static DWORD dwUnlockFpsPresentCount = 0;
static DWORD dwUnlockFpsTickCount = 0;

BOOL __stdcall UNLOCKFPS_Active() {
    DWORD dwNow = GetTickCount();
    if (!dwUnlockFpsLastCheck || dwNow - dwUnlockFpsLastCheck >= 2000) {
        char szIniPath[260];
        dwUnlockFpsLastCheck = dwNow;
        if (ALTTOGGLE_BuildIniPath(szIniPath)) {
            ALTTOGGLE_EnsureIni();
            // Missing file/section/key or a non-numeric value = 0 = vanilla.
            gbUnlockFpsMode = GetPrivateProfileIntA("features", "unlock_fps", 0, szIniPath) ? 1 : 0;
            // [unlock-fps-interp R143] interpolation rides the gate; missing key = 1
            // (the intended look). 0 restores the R142 step-cadence presentation.
            gbUnlockFpsInterp = GetPrivateProfileIntA("features", "unlock_fps_interp", 1, szIniPath) ? 1 : 0;
        } else {
            gbUnlockFpsMode = 0;
            gbUnlockFpsInterp = 0;
        }
    }
    return gbUnlockFpsMode;
}

BOOL __stdcall UNLOCKFPS_ShouldPresent() {
    if (!UNLOCKFPS_Active()) {
        dwUnlockFpsNextPresent = 0; // gate off (or just flipped off): unanchor
        return FALSE;
    }
    DWORD dwNow = GetTickCount();
    dwUnlockFpsLastPoll = dwNow;
    if (!sbUnlockFpsTimerRaised) {
        sbUnlockFpsTimerRaised = 1;
        timeBeginPeriod(1); // 16 ms pacing needs 1 ms Sleep granularity; one-way latch, never lowered
        D2BOOT_MarkFmt("unlock-fps: [features] unlock_fps=1 - present pacer 60 Hz, sim stays 25 Hz (timeBeginPeriod(1))%s",
                       gbUnlockFpsInterp ? " + sub-tick interp (two-sample)" : "");
    }
    if (!dwUnlockFpsNextPresent) {
        // First activation this process (or the first poll after a re-anchor
        // reset): present immediately, pace from here.
        dwUnlockFpsNextPresent = dwNow + UNLOCKFPS_PERIOD_MS;
        return TRUE;
    }
    if ((int32_t)(dwNow - dwUnlockFpsNextPresent) < 0) {
        return FALSE; // before the deadline
    }
    if ((int32_t)(dwNow - dwUnlockFpsNextPresent) >= (int32_t)(UNLOCKFPS_PERIOD_MS * 4)) {
        // The pump was away (menus / loading / SP pause path don't poll, or a
        // stall longer than four periods): re-anchor - skip, never queue.
        dwUnlockFpsNextPresent = dwNow + UNLOCKFPS_PERIOD_MS;
        return TRUE;
    }
    // At/after the deadline, on the grid: consume the slot(s).
    do {
        dwUnlockFpsNextPresent += UNLOCKFPS_PERIOD_MS;
    } while ((int32_t)(dwNow - dwUnlockFpsNextPresent) >= 0);
    return TRUE;
}

static void UNLOCKFPS_StatsIfDue() {
    if (!gbUnlockFpsMode) {
        // Gate off: keep the counters zeroed so a later flip-on starts a clean
        // window; no marks - the vanilla log stays vanilla.
        dwUnlockFpsPresentCount = 0;
        dwUnlockFpsTickCount = 0;
        dwUnlockFpsStatsWindowTick = 0;
        return;
    }
    DWORD dwNow = GetTickCount();
    if (!dwUnlockFpsStatsWindowTick) {
        dwUnlockFpsStatsWindowTick = dwNow;
        return;
    }
    DWORD dwWindowMs = dwNow - dwUnlockFpsStatsWindowTick;
    if (dwWindowMs < 5000) {
        return;
    }
    DWORD dwPresents = dwUnlockFpsPresentCount;
    DWORD dwTicks = dwUnlockFpsTickCount;
    dwUnlockFpsStatsWindowTick = dwNow;
    dwUnlockFpsPresentCount = 0;
    dwUnlockFpsTickCount = 0;
    DWORD dwPresentsPerSec = dwPresents * 1000 / dwWindowMs;
    DWORD dwTicksPerSec = dwTicks * 1000 / dwWindowMs;
    D2BOOT_MarkFmt("unlock-fps: presents %u in %u ms (~%u/s), client ticks %u (~%u/s) - sim stays 25 Hz",
                   (unsigned)dwPresents, (unsigned)dwWindowMs, (unsigned)dwPresentsPerSec,
                   (unsigned)dwTicks, (unsigned)dwTicksPerSec);
}

void __stdcall UNLOCKFPS_NotePresent() {
    dwUnlockFpsFrameId++; // [unlock-fps-interp R143.1] new present epoch: the next draw recomputes the shared offset
    dwUnlockFpsPresentCount++;
    UNLOCKFPS_StatsIfDue();
}

void __stdcall UNLOCKFPS_NoteClientTick() {
    // [unlock-fps-interp R143.1] two-sample snapshot: capture the player's
    // draw-space position right after a client world update moved it, shifting
    // the previous sample aside. [R143.2] the CALL SITE moved before the draw
    // block (Game.cpp) - on a tick+present iteration the draw reads this
    // position live, so the pair must already describe it or the render
    // sawtooths a full tick at every boundary (the residual field jitter).
    // Kept fresh even with the gate off (two int reads) so a mid-session
    // flip-on starts from a valid pair.
    D2UnitStrc* pPlayer = D2Client::UNIT::Player::GetPlayerUnit_UI();
    if (pPlayer) {
        int nPosX = D2Common::Units::GetTargetX(pPlayer);
        int nPosY = D2Common::Units::GetTargetY(pPlayer);
        if (!gbUnlockFpsHaveSnap) {
            gnUnlockFpsPrevPosX = nPosX; // first sample: no previous tick, no glide
            gnUnlockFpsPrevPosY = nPosY;
            gbUnlockFpsHaveSnap = 1;
        } else if (nPosX - gnUnlockFpsCurPosX > UNLOCKFPS_INTERP_SNAP_PX
                || gnUnlockFpsCurPosX - nPosX > UNLOCKFPS_INTERP_SNAP_PX
                || nPosY - gnUnlockFpsCurPosY > UNLOCKFPS_INTERP_SNAP_PX
                || gnUnlockFpsCurPosY - nPosY > UNLOCKFPS_INTERP_SNAP_PX) {
            gnUnlockFpsPrevPosX = nPosX; // teleport / level change / resurrect:
            gnUnlockFpsPrevPosY = nPosY; // discontinuity - snap, never glide it
        } else {
            gnUnlockFpsPrevPosX = gnUnlockFpsCurPosX;
            gnUnlockFpsPrevPosY = gnUnlockFpsCurPosY;
        }
        gnUnlockFpsCurPosX = nPosX;
        gnUnlockFpsCurPosY = nPosY;
    } else {
        gbUnlockFpsHaveSnap = 0; // out of game / loading: drop the stale pair
    }
    dwUnlockFpsLastClientTick = GetTickCount(); // alpha clock anchor (cheap; harmless when the gate is off)
    dwUnlockFpsTickCount++;
    UNLOCKFPS_StatsIfDue();
}

int __stdcall UNLOCKFPS_IdleSleepMs() {
    if (!UNLOCKFPS_Active()) {
        dwUnlockFpsNextPresent = 0;
        return 0; // caller keeps the retail Sleep(10)
    }
    DWORD dwNow = GetTickCount();
    if (!dwUnlockFpsNextPresent || dwNow - dwUnlockFpsLastPoll > 100) {
        // No live pacer (gate on, but the draw site is not polling: menus,
        // loading, the SP pause path): retail pacing - those loops own their
        // own cadence.
        return 0;
    }
    int32_t nUntilDeadline = (int32_t)(dwUnlockFpsNextPresent - dwNow) - 1;
    if (nUntilDeadline < 1) {
        return 1; // due: wake now, the next callback iteration draws
    }
    if (nUntilDeadline > (int32_t)UNLOCKFPS_PERIOD_MS) {
        nUntilDeadline = (int32_t)UNLOCKFPS_PERIOD_MS; // stay responsive to a gate flip
    }
    return nUntilDeadline;
}

// [vanilla-plus unlock-fps-interp] R143.1 two-sample, R143.2 delivery fix -
// sub-tick render delta (plan §4 Phase 2). R143 extrapolated the player's
// position with PATH_GetScreenVelocity x alpha; a prediction is never exact
// (quantized path velocity vs the real per-tick displacement, start/stop,
// turns), so every tick boundary snapped by the prediction error - the field
// jitter the owner reported ("jittery, looks better at 0"). R143.1
// interpolates between two real samples instead: at each client world update
// NoteClientTick snapshots the player's draw-space position (the same
// GetTargetX/Y every consumer reads); the render offset glides the world from
// the previous sample to the current one, renderPos = prev + (cur - prev) x
// alpha with alpha = ms since that update / 40 clamped to one tick.
// R143.2 (still "jittery at 1" in the field) fixed the delivery around that
// math, which is only as continuous as its wiring:
//   1. The snapshot must run BEFORE the draw. Consumers read GetTargetX/Y
//      LIVE - on a tick+present iteration the draw sees the post-tick
//      position while a pair caught up after the draw still described the
//      previous segment, so renderPos leapt a full tick forward (offset ~0
//      on the new base) and was pulled back the next present: a sawtooth at
//      the tick rate. Game.cpp now calls NoteClientTick before the draw
//      block, so the pair always matches the base the draws read.
//   2. The sprite world offset (MouseXOffset/YOffset - shadows, state
//      overlays, perspective sprite anchor, GFX_IsUnitOnScreen culling, the
//      mouse world-pick) must get the delta unconditionally: R143.1 added it
//      after DRAW_UpdateMouseWorldOffsets' shake early-return, dead in
//      normal play, leaving that whole layer tick-locked against a gliding
//      camera. Path.cpp now applies it before the shake check.
//   3. The player-sprite cancel in Wall2 flips sign in perspective mode
//      (MouseOffsets already carries the delta into projected coords).
// With that wiring: just before a tick the offset reaches 0 (render = cur),
// just after it is -(new cur - old cur) (render = the same point) -
// continuous at every tick boundary by construction, whatever the step
// shape. Standing still: delta 0, offset 0. A displacement over
// UNLOCKFPS_INTERP_SNAP_PX px in one tick (teleport, level change,
// resurrect) snaps instead of gliding across the map. The offset is
// computed once per present (cache keyed on a dedicated present epoch, not
// the stats counter - StatsIfDue resets that every 5 s window) and shared by
// every consumer, so the camera and the player sprite ride the exact same
// delta within a frame (per-site GetTickCount calls could straddle a 1 ms
// boundary and break the glue). Write-nowhere by contract: consumers add this
// to their draw position only; D2DynamicPathStrc is never touched. Fills 0
// and returns FALSE whenever it should not apply (gate/interp off, no
// snapshot yet, null out) so every site reduces to retail.
BOOL __stdcall UNLOCKFPS_GetPlayerRenderOffset(int32_t* pnOffsetX, int32_t* pnOffsetY) {
    if (pnOffsetX) {
        *pnOffsetX = 0;
    }
    if (pnOffsetY) {
        *pnOffsetY = 0;
    }
    if (!pnOffsetX || !pnOffsetY) {
        return FALSE;
    }
    if (!gbUnlockFpsMode || !gbUnlockFpsInterp) {
        return FALSE;
    }
    if (!gbUnlockFpsHaveSnap || !dwUnlockFpsLastClientTick) {
        return FALSE; // no client world update seen yet this process
    }
    if (dwUnlockFpsOffsetFrame != dwUnlockFpsFrameId) {
        // First consumer this present: compute the frame's offset once so the
        // camera and every sprite draw share it.
        dwUnlockFpsOffsetFrame = dwUnlockFpsFrameId;
        DWORD dwSinceTick = GetTickCount() - dwUnlockFpsLastClientTick;
        if (dwSinceTick >= UNLOCKFPS_SIM_TICK_MS) {
            dwSinceTick = UNLOCKFPS_SIM_TICK_MS; // clamp at the sample: offset 0, render = cur
        }
        int nDeltaX = gnUnlockFpsCurPosX - gnUnlockFpsPrevPosX;
        int nDeltaY = gnUnlockFpsCurPosY - gnUnlockFpsPrevPosY;
        gnUnlockFpsOffsetX = nDeltaX * ((int)dwSinceTick - (int)UNLOCKFPS_SIM_TICK_MS) / (int)UNLOCKFPS_SIM_TICK_MS;
        gnUnlockFpsOffsetY = nDeltaY * ((int)dwSinceTick - (int)UNLOCKFPS_SIM_TICK_MS) / (int)UNLOCKFPS_SIM_TICK_MS;
    }
    *pnOffsetX = gnUnlockFpsOffsetX;
    *pnOffsetY = gnUnlockFpsOffsetY;
    return TRUE;
}

// ===== [vanilla-plus options page] d2vp.ini feature toggle bridge ==========
// The "Vanilla+ Options" page (D2Client/UI/VanillaPlusOptions.cpp) reads and
// flips the same [features] keys the gates consume. Reads go through the
// gate's own accessor path (path builder + template ensure + 0 default);
// writes go through WritePrivateProfileStringA - the first ini writer in the
// tree - and end with D2VP_ConfigReloaded() so every 2 s hot-read gate sees
// the change on its very next poll (same frame for the draw, <= 1 frame for
// auto_gold, next save-path resolution for portable_save). A failed write is
// silently ignored: the gates keep reading whatever the file still says,
// exactly like an external editor that lost a race.

int __stdcall D2VP_GetFeatureToggle(const char* szKey) {
    return D2VP_GetFeatureToggleDefault(szKey, 0);
}

// R144: the options page's per-row read default. Every feature key is
// 0-defaulted (missing/garbage = vanilla) except unlock_fps_interp - its
// gate reads a missing key as 1 (UNLOCKFPS_Active: interp is the intended
// look), and the row must display what the gate acts on: ALTTOGGLE_EnsureIni
// writes the template only for a MISSING FILE and never backfills keys, so
// a pre-R143 ini carries no unlock_fps_interp and a 0-default read would
// show Off while the interp runs. The panel flips stay exact too: the page
// toggle inverts the displayed default, so the first click on a default-on
// row writes 0 (with a 0-default read it would write 1 - a silent no-op).
int __stdcall D2VP_GetFeatureToggleDefault(const char* szKey, int nDefault) {
    char szIniPath[260];
    ALTTOGGLE_EnsureIni();
    if (!szKey || !ALTTOGGLE_BuildIniPath(szIniPath)) {
        return nDefault ? 1 : 0;
    }
    // Missing file/section/key or a non-numeric value = nDefault.
    return GetPrivateProfileIntA("features", szKey, nDefault ? 1 : 0, szIniPath) ? 1 : 0;
}

void __stdcall D2VP_SetFeatureToggle(const char* szKey, int nValue) {
    char szIniPath[260];
    char szValue[2];
    if (!szKey || !ALTTOGGLE_BuildIniPath(szIniPath)) {
        return;
    }
    ALTTOGGLE_EnsureIni();
    szValue[0] = nValue ? '1' : '0';
    szValue[1] = '\0';
    WritePrivateProfileStringA("features", szKey, szValue, szIniPath);
    D2VP_ConfigReloaded();
}

// Bust every gate's 2 s hot-read cache so the next poll re-reads d2vp.ini.
// Called by D2VP_SetFeatureToggle after each write; exported for future
// tooling (console commands, external reload hooks).
static void VPSHAREDUI_FlushGateCache(); // defined in the shared-stash block below
// [automap-clock R147] the clock's gate cache + session anchor live here so
// the reload bust below sees them; the functions are in the clock block at
// the end of the TU.
static int gbAutomapClockMode = 0;
static DWORD dwAutomapClockLastCheck = 0;   // 2 s ini re-read gate (row-28 pattern)
static DWORD dwVpClockSessionStartTick = 0; // GetTickCount() captured at game entry
void __stdcall D2VP_ConfigReloaded() {
    dwAltToggleLastCheck = 0;     // next ALTTOGGLE_Active() re-reads alt_toggle
    dwAutoGoldLastCheck = 0;      // next AUTO_GOLD_Active() re-reads auto_gold
    dwGlobeTextPersistentLastCheck = 0; // next VANILLAPLUS_GlobeTextPersistent() re-reads globe_persistent
    dwItemIlvlLastCheck = 0;      // next ITEMILVL_Active() re-reads item_ilvl
    dwNoPanelShiftLastCheck = 0;  // next PANELSHIFT_NoShiftWorld() re-reads no_panel_shift
    dwShiftCompareLastCheck = 0;  // next SHIFTCOMPARE_Active() re-reads shift_compare
    dwCtrlClickXferLastCheck = 0; // next VPCTRLXFER_Active() re-reads ctrl_click_xfer
    dwUnlockFpsLastCheck = 0;     // next UNLOCKFPS_Active() re-reads unlock_fps
    dwAutomapClockLastCheck = 0;  // next VPCLOCK_Draw() re-reads automap_clock (R147)
    VPSHAREDUI_FlushGateCache();  // next VPSHAREDUI_FeatureActive() re-reads shared_stash
    Fog::Source::SFile::PORTABLESAVE_FlushConfig(); // next save-path poll re-reads portable_save
}

// ===== [resolution-mode] Route-B label primitives ===========================
// Shared with the video-options Resolution row's 1280x720 stop (see
// VanillaPlusCore.h for the wiring and the why). The builder is the same
// retail-shape 8x8 transparent DC6 the Vanilla+ row label shipped
// (VPLABEL_BuildBlankPlaceholder in VanillaPlusOptions.cpp); deliberately
// duplicated here instead of shared so the field-proven options page never
// churns when this feature evolves. Both copies freeze the SAME image:
// 24-byte DC6 header (version 6, flags 1, 1 direction, 1 frame), file-
// absolute frame-offset slot 0x1C, 0x20-byte frame header (nFlip=1, 8x8,
// nAllocSize 64, nLength 16), 16 RLE bytes encoding an all-zero 8x8 raster
// (per row one 0x80|8 transparent-run token plus the 0x80 end-of-row token).
// Keep the copies in sync byte-for-byte.

#ifndef D2_ALLOC
#define D2_ALLOC(size) Fog::Memory::AllocClientMemory((size), __FILE__, __LINE__, 0)
#endif

#ifndef D2_FREE
#define D2_FREE(p) Fog::Memory::FreeClientMemory((p), __FILE__, __LINE__, 0)
#endif

BOOL __stdcall VPRES_BuildBlankPlaceholder(struct DC6** ppOut) {
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
    D2CMP::SRC::CelCmp::CELCMP_FixupPointersAndPrepare(pRaw, ppOut, ".\\UI\\VANILLAPLUSCORE.CPP", 0x0, -1, "resolution stop placeholder");
    if (!*ppOut) {
        D2_FREE(pRaw);
        return FALSE;
    }
    return TRUE;
}

// Frame-0 pixel height of pDC6 (0 = not measurable). Same accessors as
// VPLABEL_MeasureRowImageHeight (VanillaPlusOptions.cpp): fixup-prepared
// DC6, GfxData for frame 0, retail block lookup, GetDC6Height.
static int VPRES_MeasureDC6FrameHeight(struct DC6* pDC6) {
    if (!pDC6) {
        return 0;
    }
    D2GfxDataStrc sGfxData;
    memset(&sGfxData, 0, sizeof(sGfxData));
    sGfxData.pDC6 = pDC6;
    sGfxData.nDirection = 0;
    sGfxData.nFrameNumber = 0;
    DC6Block* pBlock = D2CMP::SRC::CelCmp::CELCMP_GetDC6BlockFromGfxData(&sGfxData);
    if (!pBlock) {
        return 0;
    }
    int nHeight = (int)D2CMP::SRC::CelCmp::GetDC6Height(pBlock);
    return nHeight > 0 ? nHeight : 0;
}

// Capital-glyph frame height in font nFontId (0 = not measurable). Mirrors
// VPLABEL_GlyphFrameHeight: forces the standard lazy font load through
// SetAndReturnLastFont (the exact path every text draw takes when a font is
// first used), then reads the glyph's frame through the retail accessors -
// the same glyph-entry -> frame-number lookup every drawn character takes.
// Leaves the current font switched; callers save/restore around batches.
static int VPRES_GlyphFrameHeight(int nFontId, uint16_t wCharCode) {
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

int __stdcall VPRES_PickFontMatchingDC6(struct DC6* pTargetDC6) {
    // Same probe list + policy as the Vanilla+ row label (revE): ascending
    // probe, distance ties resolve to the larger font, Font42 fallback.
    static const int gaProbe[3] = { 7, 2, 3 };  // Font24, Font30, Font42
    static const int VPRES_FONT_FALLBACK = 3;   // Font42 - largest menu font

    // One decision per process per target: the retail label DC6 lives from
    // ESCMENU_Initialize to shutdown, so pointer identity is stable. A null
    // target (should not happen - slot 1 is retail-loaded) is never cached.
    static const DC6* pCachedTarget = nullptr;
    static int nCachedFontId = VPRES_FONT_FALLBACK;
    if (pTargetDC6 && pTargetDC6 == pCachedTarget) {
        return nCachedFontId;
    }

    int nResult = VPRES_FONT_FALLBACK;
    int nTargetHeight = VPRES_MeasureDC6FrameHeight(pTargetDC6);
    if (nTargetHeight > 0) {
        int32_t nSavedFont = D2Win::Src::D2WinFont::SetAndReturnLastFont(gaProbe[0]);
        int nBestFont = -1;
        int nBestDist = 0;
        for (int i = 0; i < 3; i++) {
            int nCapsHeight = VPRES_GlyphFrameHeight(gaProbe[i], (uint16_t)L'S');
            if (nCapsHeight <= 0) {
                continue;
            }
            int nDist = nCapsHeight > nTargetHeight ? nCapsHeight - nTargetHeight : nTargetHeight - nCapsHeight;
            // "<=" on purpose: the probe order is ascending, so a distance
            // tie resolves to the larger font (the retail value images are
            // the big menu family - when in doubt, go bigger).
            if (nBestFont == -1 || nDist <= nBestDist) {
                nBestFont = gaProbe[i];
                nBestDist = nDist;
            }
        }
        D2Win::Src::D2WinFont::SetAndReturnLastFont(nSavedFont);
        if (nBestFont != -1) {
            nResult = nBestFont;
        }
    }

    if (pTargetDC6) {
        pCachedTarget = pTargetDC6;
        nCachedFontId = nResult;
    }
    return nResult;
}

// ===== [vanilla-plus] merge tripwires ======================================
// The fork extends upstream-sized things in several places (UI flag table,
// esc-menu options row, dismiss-mode alias). A future upstream merge that
// drops or reverts one of those edits previously failed only at runtime, far
// from the cause. These checks fail LOUDLY instead:
//   - static_assert pins what the compiler can see (enum values, array bounds
//     of the extern declarations in globals/globals.h);
//   - VANILLAPLUS_SelfCheck pins what only the runtime can see (values and
//     wired callbacks), once per process from ALTTOGGLE_EnsureIni (boot).
// Any failure means a fork edit was lost or the tree changed shape - audit
// the VP markers before changing these expectations.
static_assert(UI_VANILLAPLUS == 38,
    "vanilla-plus: UI_VANILLAPLUS moved - upstream added/changed a UI var; "
    "re-audit the flag-38 count bumps (handoff.md §8)");
static_assert(UI_RECIPESCROLL == 37,
    "vanilla-plus: upstream inserted a UI var before UI_VANILLAPLUS; "
    "re-audit the flag-38 count bumps (handoff.md §8)");
// VP: the gaEscMenuOptionsTable sizeof assert lived here until the row-10
// table went TU-private (962f88d5); the self-check now runtime-checks the
// count through EscMenu::VP_GetOptionsTable.
static_assert(sizeof(gUIFlagDismissModeTable) / sizeof(gUIFlagDismissModeTable[0]) == 39,
    "vanilla-plus: gUIFlagDismissModeTable bounds drifted from the extern "
    "declaration - the 38 -> 39 edit or globals.h extern was lost in a merge");
static_assert(sizeof(D2UIFlagDescStrc) == 39 * sizeof(uint32_t),
    "vanilla-plus: D2UIFlagDescStrc::anDismissMode no longer 39 entries - "
    "the globals.h struct growth was lost or upstream changed the struct");
static_assert(offsetof(D2UIFlagStrc, VanillaPlus) == 0x98,
    "vanilla-plus: D2UIFlagStrc::VanillaPlus moved off 0x98 - the flag-store "
    "member addition was lost, or upstream added a flag before it");

void __stdcall VANILLAPLUS_SelfCheck() {
    static int nDone = 0;
    if (nDone) {
        return;
    }
    nDone = 1;
    if (gnUIFlagCount != 0x27) {
        D2BOOT_Fail("vanilla-plus self-check: gnUIFlagCount != 0x27 - a merge "
                    "likely dropped the count bumps (see handoff.md §8)");
        exit(-1);
    }
    if (D2Client::UI::EscMenu::VP_GetHeaderOptions()->nItemCount != 0x6) {
        D2BOOT_Fail("vanilla-plus self-check: gaEscMenuHeaderOptions.nItemCount "
                    "!= 6 - the row-10 esc-menu edit was lost (see handoff.md §8)");
        exit(-1);
    }
    // The row-10 entry is INSERTED above "Previous Menu" (index 4 of 6, owner
    // decision 2026-10-29), not appended - and a future upstream merge may
    // reshuffle rows again. Scan for it instead of hardcoding an index: fail
    // only if the wired row is missing or duplicated.
    {
        int kRow10Count = 0;
        const D2EscMenuItemStrc * pOptionsTable =
            D2Client::UI::EscMenu::VP_GetOptionsTable(&kRow10Count);
        if (kRow10Count != 6) {
            D2BOOT_Fail("vanilla-plus self-check: gaEscMenuOptionsTable count "
                        "!= 6 - the row-10 table edit or its home moved in a "
                        "merge (see handoff.md §8)");
            exit(-1);
        }
        int nWired = 0;
        for (int i = 0; i < kRow10Count; ++i) {
            if (pOptionsTable[i].fpClickCallback ==
                &D2Client::UI::VanillaPlus::ESCMENU_ShowVanillaPlusOptions) {
                ++nWired;
            }
        }
        if (nWired != 1) {
            D2BOOT_Fail("vanilla-plus self-check: gaEscMenuOptionsTable row-10 "
                        "click callback not wired (or duplicated) - the row-10 "
                        "table edit was lost or doubled in a merge "
                        "(see handoff.md §8)");
            exit(-1);
        }
    }
    // [resolution-mode] the video-options Resolution row (expansion table)
    // must carry the VP value-label draw callback; its fpInitCallback (the
    // stop-2 attach point) identifies the row. Scan instead of hardcoding an
    // index - a merge may reshuffle rows. Runtime state (nSubImageCount bump,
    // blank placeholder) is applied by that init callback; this pins only the
    // static globals.cpp wiring a merge could drop.
    {
        int kVidRowCount = 0;
        const D2EscMenuItemStrc * pDeleteCharTable =
            D2Client::UI::EscMenu::VP_GetDeleteCharTable(&kVidRowCount);
        int nWired = 0;
        for (int i = 0; i < kVidRowCount; ++i) {
            if (pDeleteCharTable[i].fpInitCallback !=
                &D2Client::UI::EscMenu::UI_SLIDER_LoadAndApplyResolution) {
                continue;
            }
            ++nWired;
            if (pDeleteCharTable[i].fpDrawCallback !=
                &D2Client::UI::EscMenu::UI_SLIDER_DrawResolutionValue) {
                D2BOOT_Fail("vanilla-plus self-check: Resolution row draw "
                            "callback not wired - the resolution-mode "
                            "globals.cpp edit was lost (see handoff.md §8)");
                exit(-1);
            }
        }
        if (nWired != 1) {
            D2BOOT_Fail("vanilla-plus self-check: Resolution row init callback "
                        "not found (or duplicated) - the resolution-mode table "
                        "wiring was lost or doubled in a merge "
                        "(see handoff.md §8)");
            exit(-1);
        }
    }
    if (gUIFlagDismissModeTable[38] !=
        (D2UIFlagDescStrc*)&gaUIFlagDescs[9].anDismissMode[0]) {
        D2BOOT_Fail("vanilla-plus self-check: gUIFlagDismissModeTable[38] alias "
                    "missing - the dismiss-mode edit was lost (see handoff.md §8)");
        exit(-1);
    }
}
// ---------------------------------------------------------------------------
// [resolution site-5] Wide-layout classification + 800-UI centering offset.
//
// Retail D2 UI code classifies layouts by D2GFX_GetResolutionMode(): the
// 640x480 class (mode 0) vs the 800x600 class (mode 2). Every gate that
// picks art ("Panel\\800CtrlPnl7" vs "Panel\\CtrlPnl7"), input handling
// (count-based vs geometric stat/skill button tests) or layout tables
// (nScreenMode 0/1 variants from D2Common) tests == 2 / != 2. The Vanilla
// Plus wide modes reuse the 800x600 art and tables (mode 4 sets
// nScreenMode = 1 in CLIENT_SetScreenResolution), so they must join the
// 800 class everywhere; this classifier is the single place that defines
// "renders like retail 800x600".
//
// The 800-class UI is exactly 800 px wide (interface bar pieces span
// 0..800, side panels and border frames tile 0..800), so on wider
// playfields it is re-centered as one rigid unit: every X anchor that is
// not already screen-center- or ScreenSizeX-relative gets
// VPRES_GetWideUiOffsetX() (= (ScreenSizeX - 800) / 2) added. The offset
// is 0 at 800x600 and clamped to 0 at 640x480, so retail modes keep
// byte-identical behavior; only modes 3 (1344x700), 4 (1280x720),
// 5 (1366x768) and 6 (1920x1080 [R115]) see a non-zero shift (240 px at
// 1280x720, 283 at 1366x768, 560 at 1920x1080).
// ---------------------------------------------------------------------------
BOOL __stdcall VPRES_IsWideLayoutMode(DWORD eResolutionMode) {
    // 2 = retail 800x600; 3 = dormant 1344x700 (same 800-class UI, already
    // in the D2GFX mode table); 4 = Vanilla Plus 1280x720; 5 = Vanilla Plus
    // 1366x768 [R112]; 6 = Vanilla Plus 1920x1080 [R115].
    return eResolutionMode == 2 || eResolutionMode == 3 || eResolutionMode == 4 || eResolutionMode == 5 || eResolutionMode == 6;
}

int __stdcall VPRES_GetWideUiOffsetX() {
    int nOffsetX = (D2Client::UI::ui::ScreenSizeX - 800) / 2;
    return 0 < nOffsetX ? nOffsetX : 0;
}

// [resolution site-6] see VanillaPlusCore.h - the Y half of the rigid-unit
// rule: every fixed (top-anchored) 800-space Y coordinate rides the
// bottom-anchored panel art by the same height delta the art gains
// automatically ("gnPanelOffsetY - const + ScreenSizeY" forms). Clamped to
// 0 at 640x480/800x600 so retail modes stay byte-identical; 100 at mode 3
// (1344x700), 120 at mode 4 (1280x720), 168 at mode 5 (1366x768) [R112],
// 480 at mode 6 (1920x1080) [R115].
// gnPanelOffsetY itself stays -60 -
// bottom-anchored expressions already contain ScreenSizeY and would
// double-shift.
int __stdcall VPRES_GetWideUiOffsetY() {
    int nOffsetY = D2Client::UI::Inv::ScreenSizeY - 600;
    return 0 < nOffsetY ? nOffsetY : 0;
}

// ===== [vanilla-plus shared-stash] ==========================================
// Client half of the 1-page shared stash (track N round 1; the server half
// lives in D2Game/PLAYER/VanillaPlusStash.cpp - materialization, the .sss
// file, the goldbank borrow, the save flush). This side is three small
// pieces:
//
//   1. the config gate below (same charter as every feature: missing
//      file/section/key or garbage = 0 = vanilla byte-for-byte, hot-re-read
//      at most every 2 s);
//   2. the "Shared" text tab on the stash panel (top-right, same text row
//      as the stash gold label) - hit-tested in INV_HandleStashLeftClick via
//      VPSHAREDUI_HandleStashClick, drawn by VPSHAREDUI_DrawTab right after
//      the gold-button draw. Since R75 the label is an OUTLINED composite:
//      a 1-px black halo under the glyphs (owner: "give the current stash
//      tab label an outline") plus a pager - << and >> arrows step one page at
//      a time between the personal page and the shared pages (two since R75
//      - owner: "add a 2nd page and buttons/arrows to navigate pages" - ten
//      since R82 - owner: "increase the shared stash pages to 10" - a
//      hundred since R146 - owner: "Good job, works. Let's increase the
//      Shared Stash pages to 100"), the
//      label reads Shared 1 .. Shared 100 on the shared pages. No hotkey:
//      the tab is the only affordance (owner decision 2026-11-01);
//   3. a per-game reset so entering a game always starts on the personal
//      page (the server starts every game on the personal page too).
//
// Clicking the tab sends the C->S 0x4F Click-Button packet with the
// out-of-retail-range button id 0x7D and the ABSOLUTE desired page in the
// param word (0 personal / 1..100 shared-N). Everything else - the
// grid, the item moves, the gold buttons - is untouched retail; while a
// shared page is active the server serves page 4 from that page's block and
// the goldbank stat carries the shared gold, so the retail deposit/withdraw
// buttons and the gold display just work.
//
// Client/server agreement: in SP every click is delivered, so the client
// flag and the server page cannot drift (the server toggle is idempotent
// even if a packet were retried). The feature is inert outside SP on the
// server side (the toggle is refused), so a TCP/BNet session with the key
// on shows a tab that toggles only its own label - documented v1 scope.

static int gbVpSharedStashMode = 0;   // 2 s ini gate cache
static DWORD dwVpSharedStashGateTick = 0;
static int gnVpSharedPage = 0;        // client mirror of the page toggle:
                                      // 0 personal, 1..100 = shared pages 1..100

// Options-page hook (D2VP_ConfigReloaded, R67): zero the gate tick so the
// next VPSHAREDUI_FeatureActive() re-reads shared_stash - the "Shared" tab
// follows the in-game toggle the same frame. The server half (D2Game/PLAYER/
// VanillaPlusStash.cpp) keeps its own 2 s poll: D2Game cannot import D2Client
// helpers (module boundary), so its gate follows within <= 2 s - same as an
// external ini edit, still far faster than walking to a stash.
static void VPSHAREDUI_FlushGateCache() {
    dwVpSharedStashGateTick = 0;
}

BOOL __stdcall VPSHAREDUI_FeatureActive() {
    DWORD dwNow = GetTickCount();
    if (!dwVpSharedStashGateTick || dwNow - dwVpSharedStashGateTick >= 2000) {
        char szIniPath[260];
        dwVpSharedStashGateTick = dwNow;
        if (ALTTOGGLE_BuildIniPath(szIniPath)) {
            ALTTOGGLE_EnsureIni();
            // Missing file/section/key or a non-numeric value = 0 = vanilla.
            gbVpSharedStashMode = GetPrivateProfileIntA("features", "shared_stash", 0, szIniPath) ? 1 : 0;
        } else {
            gbVpSharedStashMode = 0;
        }
    }
    return gbVpSharedStashMode;
}

int __stdcall VPSHAREDUI_IsSharedPage() {
    return gnVpSharedPage; // 0 = personal; 1..100 = the shared page on screen
}

void __stdcall VPSHAREDUI_ResetForNewGame() {
    gnVpSharedPage = 0;
}

// Tab geometry: in the stash panel's top-right corner (R77: the empty band
// between the panel's top edge and the retail gold display row), pulled LEFT
// of the right-panel art's rectangle. Field archaeology across four builds,
// each step field-caught:
//   R64  right edge +0x1f8 - misread the 2x256-px stash tiling as a
//        "512-px panel"; the word landed deep in the right panel.
//   R65  right edge +0x180 - "16 px inside the +0x190 art edge"; STILL
//        invisible: the RIGHT panel's DC6 (InvChar frames 4-7) draws at
//        ScreenSizeX - gnPanelOffsetX - 0x140, so its rectangle starts at
//        panel-relative +0x140 (abs x=400 at 800x600) and is blitted ~30
//        lines AFTER the tab's draw site - its opaque pixels repaint the
//        word. Field proof on the R65 build: the word stayed invisible
//        while clicks in the same rect toggled the page (the hit test
//        lives in the click handler, immune to draw order), and the whole
//        shared-page round trip worked (owner: gold + an item deposited,
//        character switched, both came back).
//   R66  right edge +0x130 - 16 px left of the +0x140 rect. Nothing after
//        the tab's draw site paints panel-relative x < 0x140 on that row
//        (the right-panel art is at >= 0x140; DRAW_UI's tail is bottom
//        HUD, orbs, corners and body slots), so the word survives at
//        800x600, 640x480 and the wide modes alike - the same expressions
//        put the word at gnPanelOffsetX + 0x130 while the art rect stays
//        at ScreenSizeX - gnPanelOffsetX - 0x140 (1280x720: word 624,
//        rect 640).
//   R76  right edge +0x13c - the R75 composite is wider than the R66 word
//        and still grew LEFTWARD from the R66 anchor, so its leftmost
//        element (the '<' tip) landed on the tail of the retail gold
//        amount text drawn on this same row at +0x4e (owner: "the page
//        arrows are displaying on top of the gold amount slightly").
//        Nudged +0x0c right: the '>' halo tip ends at +0x13d, 3 px inside
//        the +0x140 art-seam rule.
//   R77  the ROW moves - up 0x2d (45 px) into the empty band between the
//        panel's top edge and the retail gold display row. Owner on the R76
//        build: "Looks good horizontally, but still drawing on top of the
//        last gold character a bit with the arrow. can we move the stash
//        label up, closer to the top right corner ?" - the '<' halo and the
//        gold text's last glyph still kissed on the shared row, so vertical
//        separation is the end of it. New baseline (VPSHAREDUI_TabBaselineY):
//        expansion 0x19f -> 0x1cc, classic 0xdb -> 0x108. The band's right
//        side is EMPTY retail space: the coin + amount display (draw-Y
//        -0x1b8/-0xf4) keeps to the row's left end (amount tail ~ +0xd5 at
//        the 6-digit cap, a full 20 px below the new baseline), the gold-max
//        text row is 0x2d below, and the top-band hit rects (the coin rect
//        X +0x49..+0x5d, the gold-area button rect X +0x49..+0x96 -
//        INV_IsMouseOverStashCloseBtn) are all far left of the composite's
//        X span. Panel-top headroom is field-bounded (the R69 float word at
//        -0x1ef rode the world strip; the NPC-store tab strip has text at
//        -0x1e0), so the halo top (-0x1d9) stays >= 7 px inside the edge and
//        the click band [-0x1dc..-0x1c6] never probes the playfield - the
//        R69 down-walk class cannot re-open.
// R75 composite (R82: ten pages, same shape): the label is one row-built
// strip, right-aligned at the R66 anchor as R76 nudged it (the +0x140
// art-seam rule is untouched) - R145 (owner: "Can we make the navigation
// label '<<' and '>>' instead of just < >, i find just one is a little
// small to click on") doubles each arrow glyph, so on the personal page the
// composite reads  "Shared >>"  (the >> invites a step into shared page 1),
// on the shared pages  "<< Shared N >>"
// (green, the active color; N = 1..100, and the last page drops the '>>').
// The arrows step ONE page left/right; the word itself toggles
// personal<->shared-1 - the R63-R74 click-for-the-tab muscle memory, and
// always a way home from any shared page. The composite stays
// right-anchored, so the wider glyphs cost only leftward room on the R77
// band - the row's left side is empty retail space out to the gold-area
// hit rects (X +0x49..+0x96; the gold DISPLAY moved 0x2d BELOW the row at R77),
// and each arrow's click band widens with its measured width: the owner's
// actual complaint (the single glyph is a small click target) is answered
// by the band doubling with the pixels - one layout, draw and hit test
// alike (VPSHAREDUI_TabLayout). Both element rects come from the
// same per-frame layout the draw uses (widths measured in font 1), so the
// click bands can never drift from the pixels. The bands keep 2 px of
// margin around each element and keep the old band's Y extent (nY-16..nY+6,
// the R66 field-proven hug). On the R77 row the band shares its Y span with
// the retail gold display's own top-band hits (the coin rect and the
// gold-area button rect) - but only in Y: both live at X +0x49..+0x96, left
// of the composite's leftmost halo pixel, so no band can ever touch them.
// The bottom-anchored hits (close X at
// [ScreenSizeX-gnPanelOffsetX-0x12e..-0x10e] x [gnPanelOffsetY-0x60..-0x40],
// gold buttons at INV_GetGoldButtonX/Y) stay far below, and the grid rows
// start below the gold-max row, one pitch under this one.
constexpr int VPSHAREDUI_TAB_RIGHT_EDGE = 0x130; // R66 0x130; R76 +0x0c off the gold amount text
// Shared pages the pager steps through (the personal page is not one of
// them): two since R75, TEN since R82, A HUNDRED since R146 - the owner's
// "If that works we'll do 10 pages. Then 100." roadmap, completed. The
// composite stays right-anchored at VPSHAREDUI_TAB_RIGHT_EDGE and grows
// leftward, so a wider word list costs only leftward room on the R77 band
// (the gold display sits a full 0x2d below the row; see the R76/R77 notes
// above) - "Shared 100" is one glyph wider than the R82 "Shared 10", still
// far right of the +0x96 gold-area hit rects.
constexpr int VPSHAREDUI_PAGE_COUNT = 100;
constexpr int VPSHAREDUI_TAB_GAP = 5;     // word <-> arrow gap

// The tab row: R66-R76 shared the retail "Gold Max" text row (-0x19f/-0xdb);
// R77 lifts it 0x2d (45 px) into the empty band between the panel's top
// edge and the retail gold display row (DRAW_UI_INVENTORY_Gold's stash row,
// draw-Y -0x1b8/-0xf4) - the owner's "move the stash label up, closer to
// the top right corner", which also ends the '<'-vs-gold-text kiss for
// good (the gold-max text is now a full 0x2d below the baseline).
static int VPSHAREDUI_TabBaselineY() {
    BOOL bIsExpansion = D2Client::GAME::Game::CLIENT_CheckExpansion();
    return D2Client::UI::Inv::gnPanelOffsetY - (bIsExpansion ? 0x1cc : 0x108) + D2Client::UI::Inv::ScreenSizeY;
}

// The word for the current client page (0 personal, 1..100 shared). A
// literal table - no runtime formatting - so the D2Win text measure/draw
// sees plain static wide strings like every other call site; the bounds
// guard only exists to keep a bad mirror from indexing past the table.
// R146: the table grew to the hundred-page width (script-generated rows,
// same shape R82 shipped at ten).
static const uint16_t * VPSHAREDUI_PageWord() {
    static const uint16_t * const apWords[VPSHAREDUI_PAGE_COUNT + 1] = {
        (const uint16_t *)L"Shared", (const uint16_t *)L"Shared 1", (const uint16_t *)L"Shared 2",
        (const uint16_t *)L"Shared 3", (const uint16_t *)L"Shared 4", (const uint16_t *)L"Shared 5",
        (const uint16_t *)L"Shared 6", (const uint16_t *)L"Shared 7", (const uint16_t *)L"Shared 8",
        (const uint16_t *)L"Shared 9", (const uint16_t *)L"Shared 10", (const uint16_t *)L"Shared 11",
        (const uint16_t *)L"Shared 12", (const uint16_t *)L"Shared 13", (const uint16_t *)L"Shared 14",
        (const uint16_t *)L"Shared 15", (const uint16_t *)L"Shared 16", (const uint16_t *)L"Shared 17",
        (const uint16_t *)L"Shared 18", (const uint16_t *)L"Shared 19", (const uint16_t *)L"Shared 20",
        (const uint16_t *)L"Shared 21", (const uint16_t *)L"Shared 22", (const uint16_t *)L"Shared 23",
        (const uint16_t *)L"Shared 24", (const uint16_t *)L"Shared 25", (const uint16_t *)L"Shared 26",
        (const uint16_t *)L"Shared 27", (const uint16_t *)L"Shared 28", (const uint16_t *)L"Shared 29",
        (const uint16_t *)L"Shared 30", (const uint16_t *)L"Shared 31", (const uint16_t *)L"Shared 32",
        (const uint16_t *)L"Shared 33", (const uint16_t *)L"Shared 34", (const uint16_t *)L"Shared 35",
        (const uint16_t *)L"Shared 36", (const uint16_t *)L"Shared 37", (const uint16_t *)L"Shared 38",
        (const uint16_t *)L"Shared 39", (const uint16_t *)L"Shared 40", (const uint16_t *)L"Shared 41",
        (const uint16_t *)L"Shared 42", (const uint16_t *)L"Shared 43", (const uint16_t *)L"Shared 44",
        (const uint16_t *)L"Shared 45", (const uint16_t *)L"Shared 46", (const uint16_t *)L"Shared 47",
        (const uint16_t *)L"Shared 48", (const uint16_t *)L"Shared 49", (const uint16_t *)L"Shared 50",
        (const uint16_t *)L"Shared 51", (const uint16_t *)L"Shared 52", (const uint16_t *)L"Shared 53",
        (const uint16_t *)L"Shared 54", (const uint16_t *)L"Shared 55", (const uint16_t *)L"Shared 56",
        (const uint16_t *)L"Shared 57", (const uint16_t *)L"Shared 58", (const uint16_t *)L"Shared 59",
        (const uint16_t *)L"Shared 60", (const uint16_t *)L"Shared 61", (const uint16_t *)L"Shared 62",
        (const uint16_t *)L"Shared 63", (const uint16_t *)L"Shared 64", (const uint16_t *)L"Shared 65",
        (const uint16_t *)L"Shared 66", (const uint16_t *)L"Shared 67", (const uint16_t *)L"Shared 68",
        (const uint16_t *)L"Shared 69", (const uint16_t *)L"Shared 70", (const uint16_t *)L"Shared 71",
        (const uint16_t *)L"Shared 72", (const uint16_t *)L"Shared 73", (const uint16_t *)L"Shared 74",
        (const uint16_t *)L"Shared 75", (const uint16_t *)L"Shared 76", (const uint16_t *)L"Shared 77",
        (const uint16_t *)L"Shared 78", (const uint16_t *)L"Shared 79", (const uint16_t *)L"Shared 80",
        (const uint16_t *)L"Shared 81", (const uint16_t *)L"Shared 82", (const uint16_t *)L"Shared 83",
        (const uint16_t *)L"Shared 84", (const uint16_t *)L"Shared 85", (const uint16_t *)L"Shared 86",
        (const uint16_t *)L"Shared 87", (const uint16_t *)L"Shared 88", (const uint16_t *)L"Shared 89",
        (const uint16_t *)L"Shared 90", (const uint16_t *)L"Shared 91", (const uint16_t *)L"Shared 92",
        (const uint16_t *)L"Shared 93", (const uint16_t *)L"Shared 94", (const uint16_t *)L"Shared 95",
        (const uint16_t *)L"Shared 96", (const uint16_t *)L"Shared 97", (const uint16_t *)L"Shared 98",
        (const uint16_t *)L"Shared 99", (const uint16_t *)L"Shared 100"
    };
    int nPage = gnVpSharedPage;
    if (nPage < 0) {
        nPage = 0;
    } else if (nPage > VPSHAREDUI_PAGE_COUNT) {
        nPage = VPSHAREDUI_PAGE_COUNT;
    }
    return apWords[nPage];
}

// Layout of the composite on the tab row. Callers must have font 1 set (the
// widths are per-font). Fills the word's draw x + width, the left/right
// arrow draw x (valid only when the matching has-flag is set) and the
// measured arrow width. Right edge of the composite = gnPanelOffsetX +
// VPSHAREDUI_TAB_RIGHT_EDGE - the R66 word anchor as R76 nudged it.
static void VPSHAREDUI_TabLayout(int * pnWordX, int * pnWordW,
                                 int * pnLeftX, int * pbHasLeft,
                                 int * pnRightX, int * pbHasRight, int * pnArrowW) {
    int nWordW = D2Win::Src::D2WinFont::DRAW_text0((uint16_t *)VPSHAREDUI_PageWord());
    int nArrowW = D2Win::Src::D2WinFont::DRAW_text0((uint16_t *)L">>"); // R145: '<<'/'>>' - one measure feeds draw + click bands
    int nRightEdge = D2Client::UI::Inv::gnPanelOffsetX + VPSHAREDUI_TAB_RIGHT_EDGE;
    *pbHasRight = gnVpSharedPage < VPSHAREDUI_PAGE_COUNT; // '>>' on personal + pages 1..99
    *pbHasLeft = gnVpSharedPage > 0;                      // '<<' on shared pages
    int nWordR = nRightEdge - (*pbHasRight ? nArrowW + VPSHAREDUI_TAB_GAP : 0);
    *pnWordX = nWordR - nWordW;
    *pnWordW = nWordW;
    *pnLeftX = *pnWordX - VPSHAREDUI_TAB_GAP - nArrowW;   // drawn when *pbHasLeft
    *pnRightX = nWordR + VPSHAREDUI_TAB_GAP;              // drawn when *pbHasRight
    *pnArrowW = nArrowW;
}

// One outlined text element: a 1-px black halo under the color pass. The
// retail shadow recipe (D2Comp title rows draw the same string once in the
// shadow palette, then in the real color) extended to all four sides so the
// glyphs read on any terrain. Palette 6 = light black, the tone retail's
// own text shadows use.
static void VPSHAREDUI_DrawOutlinedText(const uint16_t * pwszText, int nX, int nY, int nColor) {
    static const int aOffs[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
    for (int i = 0; i < 4; ++i) {
        D2Win::Src::D2WinFont::DRAW_text((uint16_t *)pwszText, nX + aOffs[i][0], nY + aOffs[i][1], 6, 0);
    }
    D2Win::Src::D2WinFont::DRAW_text((uint16_t *)pwszText, nX, nY, nColor, 0);
}

BOOL __stdcall VPSHAREDUI_HandleStashClick(int nMouseX, int nMouseY) {
    if (!VPSHAREDUI_FeatureActive()) {
        return FALSE;
    }

    // The exact layout the draw uses (font 1 set/restored around the
    // measures) - the click bands hug the composite at every event, with
    // no stored state to go stale between frames.
    int32_t nPrevFont = D2Win::Src::D2WinFont::SetAndReturnLastFont(1);
    int nWordX, nWordW, nLeftX, nRightX, nArrowW;
    int bHasLeft, bHasRight;
    VPSHAREDUI_TabLayout(&nWordX, &nWordW, &nLeftX, &bHasLeft, &nRightX, &bHasRight, &nArrowW);
    D2Win::Src::D2WinFont::SetAndReturnLastFont(nPrevFont);

    int nY = VPSHAREDUI_TabBaselineY();
    if (nMouseY < nY - 16 || nMouseY > nY + 6) {
        return FALSE;
    }

    int nNewPage = -1;
    if (nMouseX >= nWordX - 2 && nMouseX <= nWordX + nWordW + 2) {
        // The word: personal <-> shared (enter shared page 1 from the
        // personal page; return home from any shared page).
        nNewPage = gnVpSharedPage ? 0 : 1;
    } else if (bHasLeft && nMouseX >= nLeftX - 2 && nMouseX <= nLeftX + nArrowW + 2) {
        nNewPage = gnVpSharedPage - 1; // '<<': one page left (page 1 -> personal)
    } else if (bHasRight && nMouseX >= nRightX - 2 && nMouseX <= nRightX + nArrowW + 2) {
        nNewPage = gnVpSharedPage + 1; // '>>': one page right
    }

    if (nNewPage < 0 || nNewPage > VPSHAREDUI_PAGE_COUNT || nNewPage == gnVpSharedPage) {
        return FALSE; // Miss (or a no-op arrow) - let retail handle the click.
    }

    gnVpSharedPage = nNewPage;
    // C->S 0x4F Click Button, VP toggle id 0x7D, the ABSOLUTE desired page
    // in the param word (0 personal / 1..100 shared-N). The send
    // helper packs the param word as (arg2 << 16) | (uint16)arg3, so the
    // page rides arg3 and the server activates exactly that page
    // (VanillaPlusStash.cpp; idempotent when the page is already active).
    D2Client::GAME::Msg::NET_D2GS_CLIENT_Send_SHORT_SHORT_SHORT(0x4f, 0x7d, 0, (int16_t)gnVpSharedPage);
    return TRUE;
}

void __stdcall VPSHAREDUI_DrawTab() {
    if (!VPSHAREDUI_FeatureActive()) {
        return;
    }

    // The composite, each element outlined: the word in white on the
    // personal page / the green active color on shared pages (same color
    // convention the options page uses for Off/On), the arrows in white.
    // Font 1 = the stash panel's own text font; saved/restored around the
    // draw like every retail site.
    int32_t nPrevFont = D2Win::Src::D2WinFont::SetAndReturnLastFont(1);
    int nWordX, nWordW, nLeftX, nRightX, nArrowW;
    int bHasLeft, bHasRight;
    VPSHAREDUI_TabLayout(&nWordX, &nWordW, &nLeftX, &bHasLeft, &nRightX, &bHasRight, &nArrowW);
    int nY = VPSHAREDUI_TabBaselineY();
    if (bHasLeft) {
        VPSHAREDUI_DrawOutlinedText((const uint16_t *)L"<<", nLeftX, nY, 0); // R145: was '<'
    }
    VPSHAREDUI_DrawOutlinedText(VPSHAREDUI_PageWord(), nWordX, nY, gnVpSharedPage ? 2 : 0);
    if (bHasRight) {
        VPSHAREDUI_DrawOutlinedText((const uint16_t *)L">>", nRightX, nY, 0); // R145: was '>' - same measured width the click band uses
    }
    D2Win::Src::D2WinFont::SetAndReturnLastFont(nPrevFont);
}

// ---- [vanilla-plus nointro] R86 --------------------------------------------
// The -nointro switch: parse GetCommandLineA() once, cache, mark d2boot.log
// on the first (detected) query. See VanillaPlusCore.h for the contract and
// the two consumer sites in D2Launch/MainMenus.cpp.
BOOL __stdcall VPNOINTRO_IsRequested() {
    static int nParsed = -1; // -1 = not parsed yet, else the flag (0/1)
    if (nParsed >= 0) {
        return nParsed;
    }
    nParsed = 0;
    const char* szCmdLine = GetCommandLineA();
    if (szCmdLine) {
        char szToken[32];
        const char* pScan = szCmdLine;
        while (*pScan) {
            while (*pScan == ' ' || *pScan == '\t') {
                pScan++;
            }
            if (!*pScan) {
                break;
            }
            const char* pTokenStart = pScan;
            while (*pScan && *pScan != ' ' && *pScan != '\t') {
                pScan++;
            }
            size_t nTokenLen = (size_t)(pScan - pTokenStart);
            if (nTokenLen < sizeof(szToken)) {
                memcpy(szToken, pTokenStart, nTokenLen);
                szToken[nTokenLen] = '\0';
                // Windows launchers may quote an argument; compare inside a
                // wrapping quote pair so "-nointro" matches too.
                char* szCompare = szToken;
                if (nTokenLen >= 2 && szCompare[0] == '"' && szCompare[nTokenLen - 1] == '"') {
                    szCompare[nTokenLen - 1] = '\0';
                    szCompare++;
                }
                if (!_stricmp(szCompare, "-nointro")) {
                    nParsed = 1;
                    break;
                }
            }
        }
    }
    if (nParsed) {
        D2BOOT_Mark("[vp] cmdline -nointro: boot intro videos disabled"); // [d2boot]
    }
    return nParsed;
}

// ===== [vanilla-plus automap-clock R147] ====================================
// Session/System clock on the automap overlay (owner ask 2026-11-10: "Let's
// add an in game clock to the Automap. Session Time and System Time"). A
// display-only client feature - no server half, no packets, no files. The
// lines ride the retail automap text-overlay stack (AUTOMAP_Draw's font-1
// family: right-aligned at WindowSizeX - width - 0x10, one 0x10 row per
// line, color 4) exactly like the level name, the build tag, the difficulty
// and the server IP; the VP site sits right after the build-tag line, so the
// stack order reads version, clock, difficulty. Gate = the charter every
// feature keeps: [features] automap_clock, missing file/section/key or
// garbage = 0 = vanilla byte-for-byte (no pixels), the same 2 s hot-read
// cache every gate uses, busted by the Options-page write bridge
// (D2VP_ConfigReloaded) so a flip lands on the next automap frame.
//
//   Session - real time elapsed since the current game was entered. The
//      anchor is taken in InitializeUI's VP site (the same wholesale UI
//      reset that starts the stash page mirror on the personal page) via
//      VPCLOCK_ResetSession: one GetTickCount() capture per game. Wall-clock
//      elapsed, not tick-counted play time: pause menus and cutscenes count,
//      exactly like a stopwatch on the desk - the honest "how long have I
//      been in this game" reading. Format H:MM:SS (hours unpadded).
//   System - the PC's local wall clock (Win32 GetLocalTime), HH:MM 24-hour,
//      read fresh every frame the automap draws, so it is always current.
//
// Text building: pure-ASCII char buffers through SStrPrintf (the item-ilvl
// digit idiom) widened 1:1 into the uint16_t buffers D2WinFont draws - no
// locale, no wide-format pitfalls. GetTickCount wraps at 49.7 days;
// unsigned subtraction is wrap-correct, so any session shorter than that
// reads true.

static BOOL VPCLOCK_GateActive() {
    DWORD dwNow = GetTickCount();
    if (!dwAutomapClockLastCheck || dwNow - dwAutomapClockLastCheck >= 2000) {
        dwAutomapClockLastCheck = dwNow;
        gbAutomapClockMode = D2VP_GetFeatureToggle("automap_clock");
    }
    return gbAutomapClockMode;
}

// Anchor the session clock at game entry (InitializeUI's wholesale UI reset
// - the same site that starts the stash page mirror on the personal page).
void __stdcall VPCLOCK_ResetSession() {
    dwVpClockSessionStartTick = GetTickCount();
}

// The two overlay lines. Called from AUTOMAP_Draw inside its font-1 window,
// right after the build-tag line: reads and advances *pnTextHeightOffset by
// one 0x10 row per line drawn, right-aligns at nScreenX - width - 0x10 (the
// family's right edge), color 4. Gate off = nothing drawn, offset untouched.
void __stdcall VPCLOCK_Draw(int* pnTextHeightOffset, int nScreenX) {
    if (!pnTextHeightOffset || !VPCLOCK_GateActive()) {
        return;
    }

    char szLine[40];
    uint16_t wLine[40];
    for (int i = 0; i < 40; i++) {
        wLine[i] = L'\0';
    }

    // Session: real elapsed since game entry, H:MM:SS.
    DWORD dwSeconds = (GetTickCount() - dwVpClockSessionStartTick) / 1000;
    Storm::Source::SSTR::SStrPrintf(szLine, sizeof(szLine), "Session %u:%02u:%02u",
                                    (unsigned)(dwSeconds / 3600),
                                    (unsigned)((dwSeconds / 60) % 60),
                                    (unsigned)(dwSeconds % 60));
    for (int i = 0; i < 40 && szLine[i]; i++) {
        wLine[i] = (uint16_t)(uint8_t)szLine[i]; // pure ASCII, 1:1 widen
    }
    int nWidth = D2Win::Src::D2WinFont::DRAW_text0(wLine);
    D2Win::Src::D2WinFont::DRAW_text(wLine, nScreenX - nWidth - 0x10, *pnTextHeightOffset, 4, 0);
    *pnTextHeightOffset += 0x10;

    // System: the PC's local wall clock, HH:MM 24-hour.
    SYSTEMTIME sTime;
    GetLocalTime(&sTime);
    Storm::Source::SSTR::SStrPrintf(szLine, sizeof(szLine), "System %02u:%02u",
                                    (unsigned)sTime.wHour, (unsigned)sTime.wMinute);
    for (int i = 0; i < 40; i++) {
        wLine[i] = L'\0';
    }
    for (int i = 0; i < 40 && szLine[i]; i++) {
        wLine[i] = (uint16_t)(uint8_t)szLine[i];
    }
    nWidth = D2Win::Src::D2WinFont::DRAW_text0(wLine);
    D2Win::Src::D2WinFont::DRAW_text(wLine, nScreenX - nWidth - 0x10, *pnTextHeightOffset, 4, 0);
    *pnTextHeightOffset += 0x10;
}
