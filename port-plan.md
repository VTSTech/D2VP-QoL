# Port Plan: D2VP-QoL → redivivus (upstream)

## Overview

Port the Vanilla+ quality-of-life features from D2VP-QoL fork into redivivus (upstream) to create a fresh upstream that includes these enhancements.

## Status

- **Fork**: `~/workspace/D2VP-QoL` (D2VP-QoL, VTSTech's maintained fork)
- **Upstream Base**: `origin/jaenster-main` branch (jaenster's upstream with latest Ghidra v2097)
- **Target**: Port D2VP-QoL changes into a fresh upstream using jaenster-main as base
- **Approach**: Cherry-pick/patch D2VP-QoL features into jaenster-main, then push to new upstream repo
- **Current Branch**: `port-vanillaplus-into-upstream` (merged with jaenster-main)

## Progress Summary

| Phase | Description | Status | Commit |
|-------|-------------|--------|--------|
| Phase 1 | Add core Vanilla+ files (VanillaPlusCore, VanillaPlusOptions, VanillaPlusStash, VanillaPlusCombat, VanillaPlusSound) | ✅ Complete | df415863 |
| Phase 2 | Apply VP-marked changes to upstream files (47 files) | ✅ Complete | 41f90c30 |
| Phase 3 | Client integration (included in Phase 2) | ✅ Complete | 41f90c30 |
| Phase 4 | Game system changes (included in Phase 2) | ✅ Complete | 41f90c30 |
| Phase 5 | Add documentation files (FEATURES.md, CHANGELOG.md, HISTORY.md, VANILLA-PLUS.md, ROUND-SUMMARY.md, SMOKE-CHECKLIST.md, handoff.md) | ✅ Complete | 91bc33fc |

## Files with VP-marked Code (53 files)

### Core Vanilla+ Files (✅ Added - Phase 1)

1. **Source/D2Client/UI/VanillaPlusCore.cpp** (132KB) ✅
   - Core vanilla+ functionality: FPS unlock, automap clock, transparent HUD, camera zoom
   - Feature gating system
   - Per-feature state management

2. **Source/D2Client/UI/VanillaPlusCore.h** (31KB) ✅
   - Header with function declarations and global state

3. **Source/D2Client/UI/VanillaPlusOptions.cpp** (40KB) ✅
   - In-game "Vanilla+ Options" menu page (FEATURES.md row 10)
   - Options for: unlock_fps, unlock_fps_interp, automap_clock, transparent_hud, camera_zoom

4. **Source/D2Client/UI/VanillaPlusOptions.h** (3.5KB) ✅
   - Options page header

### Game System Files (✅ Added - Phase 1)

5. **Source/D2Game/PLAYER/VanillaPlusStash.cpp** (New) ✅
   - Shared stash implementation (FEATURES.md row 9)
   - 100-page shared stash support

6. **Source/D2Game/PLAYER/VanillaPlusStash.h** (New) ✅
   - Shared stash header

7. **Source/D2Game/UNIT/VanillaPlusCombat.cpp** (New) ✅
   - Combat enhancements (mana burn fix, etc.)

8. **Source/D2Game/UNIT/VanillaPlusCombat.h** (New) ✅
   - Combat header

9. **Source/D2Sound/VanillaPlusSound.cpp** (New) ✅
   - Sound enhancements

10. **Source/D2Sound/VanillaPlusSound.h** (New) ✅
    - Sound header

### D2Client Modifications (✅ Applied - Phase 2)

11. **Source/D2Client/D2Client.cpp** (modifications) ✅
    - Vanilla+ integration points

12. **Source/D2Client/Draw/Weather.cpp** (modifications) ✅
    - Weather rendering enhancements

13. **Source/D2Client/Engine.cpp** (modifications) ✅
    - Engine loop modifications for FPS unlock

14. **Source/D2Client/Engine/Gfx.cpp** (modifications) ✅
    - Graphics enhancements

15. **Source/D2Client/Engine/GfxUtil.cpp** (modifications) ✅
    - Graphics utility enhancements

16. **Source/D2Client/GAME/Draw.cpp** (modifications) ✅
    - Game drawing enhancements

17. **Source/D2Client/GAME/Game/Game.cpp** (modifications) ✅
    - Game loop, FPS unlock integration

18. **Source/D2Client/GAME/SCmd.cpp** (modifications) ✅
    - Command handling

19. **Source/D2Client/GAME/Select.cpp** (modifications) ✅
    - Selection enhancements

20. **Source/D2Client/GAME/View.cpp** (modifications) ✅
    - View/viewport enhancements

21. **Source/D2Client/GAME/Wall2.cpp** (modifications) ✅
    - Wall rendering enhancements

22. **Source/D2Client/Message.cpp** (modifications) ✅
    - Message handling for vanilla+ features

23. **Source/D2Client/SKILLS/SkillsBar.cpp** (modifications) ✅
    - Skills bar enhancements

24. **Source/D2Client/Sound/Sound.cpp** (modifications) ✅
    - Sound system enhancements

25. **Source/D2Client/UI/Automap.cpp** (modifications) ✅
    - Automap clock implementation (FEATURES.md row 19)

26. **Source/D2Client/UI/Draw.cpp** (modifications)
    - Drawing enhancements

27. **Source/D2Client/UI/EscMenu.cpp** (modifications)
    - Esc menu enhancements

28. **Source/D2Client/UI/EscMenu.h** (modifications)
    - Esc menu header

29. **Source/D2Client/UI/Help.cpp** (modifications)
    - Help system enhancements

30. **Source/D2Client/UI/Inv.cpp** (modifications)
    - Inventory enhancements (shift compare, ctrl+click)

31. **Source/D2Client/UI/ListBox/ListBoxImplementation.cpp** (modifications)
    - List box enhancements

32. **Source/D2Client/UI/MiniPanel.cpp** (modifications)
    - Mini panel enhancements

33. **Source/D2Client/UI/Panel.cpp** (modifications)
    - Panel enhancements (transparent HUD, FEATURES.md row 4)

34. **Source/D2Client/UI/Party.cpp** (modifications)
    - Party system enhancements

35. **Source/D2Client/UI/Path.cpp** (modifications)
    - Pathfinding enhancements

36. **Source/D2Client/UI/QuestLog.cpp** (modifications)
    - Quest log enhancements

37. **Source/D2Client/UI/ShowItems.cpp** (modifications)
    - Item display enhancements (alt_toggle, FEATURES.md row 0)

38. **Source/D2Client/UI/SpellSel.cpp** (modifications)
    - Spell selection enhancements

39. **Source/D2Client/UI/Text.cpp** (modifications)
    - Text rendering enhancements

40. **Source/D2Client/UI/ui.cpp** (modifications)
    - UI core enhancements

41. **Source/D2Client/UI/ui.data.h** (modifications)
    - UI data enhancements

42. **Source/D2Client/UI/Waypoint.cpp** (modifications)
    - Waypoint system enhancements

43. **Source/D2Client/UNIT/CUnit.cpp** (modifications)
    - Unit rendering enhancements (camera zoom)

44. **Source/D2Client/UNIT/Item.cpp** (modifications)
    - Item handling enhancements

45. **Source/D2Client/UNIT/Player.cpp** (modifications)
    - Player enhancements

### D2Game Modifications

46. **Source/D2Game/ITEMS/Items/Drop.cpp** (modifications)
    - Item drop enhancements (countess chest, etc.)

47. **Source/D2Game/SKILLS/SkillNec.cpp** (modifications)
    - Necromancer skill enhancements

48. **Source/D2Game/UNIT/Monster.cpp** (modifications)
    - Monster enhancements

49. **Source/D2Game/UNIT/SUnitDmg.cpp** (modifications)
    - Unit damage (mana burn fix, FEATURES.md row 12)

### Renderer Modifications

50. **Source/D2DDraw/Renderer/DirectDraw.cpp** (modifications)
    - DirectDraw renderer enhancements

51. **Source/D2Direct3D/Renderer/Direct3D.cpp** (modifications)
    - D3D renderer enhancements (camera zoom, transparent HUD)

52. **Source/D2Gdi/Renderer/Windowed.cpp** (modifications)
    - GDI renderer enhancements

53. **Source/D2gfx/D2GFX.cpp** (modifications)
    - GFX system enhancements

54. **Source/D2gfx/D2GFX.h** (modifications)
    - GFX header enhancements

55. **Source/D2Glide/Renderer/Glide.cpp** (modifications)
    - Glide renderer enhancements (camera zoom)

56. **Source/D2Glide/Renderer/ShadowDiag.cpp** (modifications)
    - Shadow diagnostics

### Global and System Files

57. **Source/D2Headers/d2_enums.h** (modifications)
    - Enum enhancements (options page flags)

58. **Source/D2Launch/MainMenus.cpp** (modifications)
    - Main menu enhancements (build tag, nointro)

59. **Source/Game/Main.cpp** (modifications)
    - Main game loop

60. **Source/globals/D2Client.cpp** (modifications)
    - Global state enhancements

61. **Source/globals/globals.h** (modifications)
    - Global structures enhancements (dismiss mode, vanilla-plus flag)

## Documentation Files (to be added)

1. **FEATURES.md** (126KB)
   - Comprehensive feature documentation
   - Implementation status for each feature
   - Field verification notes

2. **README.md** (20KB)
   - Project description and status

3. **CHANGELOG.md** (35KB)
   - Change history

4. **HISTORY.md** (904KB)
   - Detailed history and development notes

5. **ROUND-SUMMARY.md** (5KB)
   - Round summaries

6. **SMOKE-CHECKLIST.md** (50KB)
   - Smoke test checklist

7. **VANILLA-PLUS.md** (9KB)
   - Vanilla+ overview

8. **handoff.md** (16KB)
   - Handoff documentation

## Key Features to Port

### Shipped Features (R156)

1. **Camera Zoom** (R155/R156) - FEATURES.md row 0
   - Z-key world zoom (1.0 → 1.25 → 1.5 → 1.0)
   - Anchored on character, UI stays 1:1

2. **Transparent HUD** (R154) - FEATURES.md row 4
   - Transparent bottom bar (console frame, run/menu buttons, hostile indicator)

3. **Automap Clock** (R147/R148) - FEATURES.md row 19
   - Session time (SESS: hh:mm:ss) + System time (SYS: 12:01PM)
   - Integrated into automap text overlay

4. **Mana Burn Fix** (R106/R108) - FEATURES.md row 12
   - Fixes melee drain 256x bug

5. **Shared Stash** (R66→R146) - FEATURES.md row 10
   - 100-page shared stash
   - Side-file implementation (.sss)

6. **FPS Unlock** (R142/R143/R144) - FEATURES.md row 18
   - Presentation FPS to 60 Hz
   - Sim stays 25 Hz
   - Interpolation support

7. **1344x700 Resolution** (R109) - FEATURES.md row 15
   - Retail dormant mode 3

8. **1920x1080 Resolution** (R115) - FEATURES.md row 16
   - 1080p support

### Additional Features

9. **Shift Equipment Compare** (R125/R126) - FEATURES.md row 1
10. **Ctrl+Click Stash Transfer** (R140/R152) - FEATURES.md row 2
11. **Persistent Globe Text** (0.0.5) - FEATURES.md row 3
12. **Item ilvl on Tooltips** (0.0.7) - FEATURES.md row 4
13. **Monster HP Bar** - FEATURES.md row 6
14. **Auto Gold Pickup** (0.0.4) - FEATURES.md row 7
15. **Automap Scale + Position** - FEATURES.md row 8
16. **Quick-Cast Keys** - FEATURES.md row 9
17. **Stackable Gems** (planned) - FEATURES.md row 13
18. **Stackable Runes** (planned) - FEATURES.md row 14
19. **Inventory Sort** (planned) - FEATURES.md row 17
20. **Alt Toggle** (0.0.2/0.0.3) - pre-slate features

## Implementation Strategy

### Step 1: Checkout upstream base
```bash
cd ~/workspace/D2VP-QoL
git checkout origin/jaenster-main
git checkout -b upstream-jaenster-main  # or move to new repo
git push upstream upstream-jaenster-main
```

### Phase 1: Core Files ✅ COMPLETE
1. Add VanillaPlusCore.cpp/.h ✅
2. Add VanillaPlusOptions.cpp/.h ✅
3. Add VanillaPlusStash.cpp/.h ✅
4. Add VanillaPlusCombat.cpp/.h ✅
5. Add VanillaPlusSound.cpp/.h ✅

### Phase 2: Renderer Support & VP-marked Changes ✅ COMPLETE
4. Update D2Direct3D/Renderer/Direct3D.cpp (camera zoom, transparent HUD) ✅
5. Update D2Glide/Renderer/Glide.cpp (camera zoom) ✅
6. Update D2DDraw/Renderer/DirectDraw.cpp ✅
7. Update D2Gdi/Renderer/Windowed.cpp ✅

### Phase 3: Client Integration ✅ COMPLETE (included in Phase 2)
8. Update D2Client/D2Client.cpp ✅
9. Update D2Client/Message.cpp ✅
10. Update D2Client/UI/*.cpp (UI enhancements) ✅
11. Update D2Client/GAME/*.cpp (game loop, FPS unlock) ✅

### Phase 4: Game System ✅ COMPLETE (included in Phase 2)
12. Update D2Game/UNIT/*.cpp (combat, stash) ✅
13. Update D2Game/ITEMS/Items/Drop.cpp ✅
14. Update Source/globals/globals.h ✅

### Phase 5: Documentation ✅ COMPLETE
15. Add FEATURES.md ✅
16. Add CHANGELOG.md ✅
17. Add HISTORY.md ✅
18. Add VANILLA-PLUS.md ✅
19. Add ROUND-SUMMARY.md ✅
20. Add SMOKE-CHECKLIST.md ✅
21. Add handoff.md ✅

### Next Steps
- [x] Phase 6: Build Verification & Compilation Fixes (IN PROGRESS)
- [ ] Build verification: Ensure all files compile without errors
- [ ] Feature testing: Test each vanilla+ feature individually
- [ ] Integration testing: Verify features work together
- [ ] Field verification: Test in-game as the owner would

## Testing Strategy

1. **Build verification**: Ensure all files compile without errors
2. **Feature testing**: Test each vanilla+ feature individually
3. **Integration testing**: Verify features work together
4. **Field verification**: Test in-game as the owner would

## Risk Assessment

- **High Risk**: Major game logic changes (shared stash, mana burn fix)
- **Medium Risk**: UI/UX changes (FPS unlock, camera zoom)
- **Low Risk**: Documentation additions

## Notes

- All changes use `// VP:` and `// VP-BEGIN [...]` markers for tracking
- Features are gated via `d2vp.ini` `[features]` section
- In-game options page added at FEATURES.md row 10
- Default feature state is disabled (vanilla behavior)

## Build Verification Status (Phase 6 - IN PROGRESS)

### Completed Fixes ✅
- [x] Configure CMake with mingw-i686 toolchain
- [x] Add missing Vanilla+ support files (`d2_boot.h`, `d2_hangwatch.h`)
- [x] Fix module gate errors (D2_GATE_FROM_ROOT handling in globals.h)
- [x] Add missing typedefs to globals.h:
  - `fpProfCoreInitCb`, `fpProfCoreRegisterSlotCb`
  - `gnGlideWindowHandle` (int vs HWND conflict)
  - `gbBloodFlag`, `gnFrameDrawCounter`
  - `s_unknown_006cd5fc` string literal
- [x] Remove conflicting typedefs from globals.h:
  - `ChatDlgCallback` (authoritative in D2Multi/ChatDlg.h)
  - `fnCUnitEventCallback` (authoritative in D2Client/UNIT/CUnitEvent.h)
- [x] Add struct definition guards in D2GFX.h:
  - `D2GFXWindowPlacementStrc`
  - `D2RendererFunctionsStrc`
- [x] Fix function signature mismatches:
  - `D2GFX_CreateWindow`: HWND → BOOL
  - `SkillNec` functions: BOOL → void/uint32_t
  - `SUnit::CreateUnit` pMonsterData: uint32_t → void*
- [x] Add missing function declarations:
  - `SOUND_GetDirectSoundInitializedFlag()` in D2Sound.h/D2SoundRoot.h
  - String literals for damage events in SUnitDmg.cpp
  - `D2TCStackEntryStrc` members (nMagic, nRare, nSet, nUnique, nFromTCOffset0x30, nFromTCOffset0x32)

### Current Build Status (as of commit 37c0e588)
- **Compiling**: ~400+ of 500+ translation units (80%+)
- **Blocking Errors**: ~20 remaining compilation errors (down from 50+)
- **Primary Categories**:
  1. **Missing function declarations** in headers (ITEM_IsQuestItem, DRAW_WORLD_SetWarpFadeTimer, CLIENT_SendIdlePlayerMessage, CUNIT_IterateHashTableWithCallback, FindUnit, InsertIntoHashTable, SKILL_SelectProc_*, DRAW_WORLD_SetWarpFadeTimer, OBJECT_CheckAutoLight, NET_D2GS_CLIENT_Incoming0x5E_QUESTPICKS, NET_D2GS_CLIENT_Incoming0x89_QUESTEVENTTRIGGER, UI_HandleNpcInteractionPacket, TRADE_HandleTradeComplete)
  2. **Namespace qualification issues** (D2Client::UNIT::Player vs D2Client, D2Win vs D2Win::Src::D2WinFont)
  3. **Minor signature mismatches** (ITEM_RecalcAllEquippedItems_EBX vs _ECX)

### Progress Summary
- **Fixed**: 40+ major compilation errors across 36 files
- **Remaining**: ~20 errors (mostly missing function declarations in headers)
- **Build Progress**: ~80% of translation units compiling
- **All major structural issues resolved**: module gates, struct redefinitions, signature mismatches, missing globals, missing function declarations, callback signatures, cross-module extern declarations

### Pending
- [ ] Add remaining missing function declarations to appropriate headers
- [ ] Fix namespace qualification issues
- [ ] Build all translation units successfully
- [ ] Link executable successfully
- [ ] Run smoke tests (SMOKE-CHECKLIST.md)
