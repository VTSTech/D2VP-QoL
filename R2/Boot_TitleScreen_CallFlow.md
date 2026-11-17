# D2VP Game Boot - Title Screen Flow (R2 Analysis)

## Overview
Complete call flow analysis from Diablo II executable entry to title screen rendering, including all initialization, resource loading, and UI setup steps.

## Key Addresses

| Component | Address | Purpose |
|-----------|---------|---------|
| Entry Point | `0x00401000` | Game.exe initialization entry |
| Main Game Loop | `0x00401100` | `main()` function |
| Title Screen | `0x00401200` | Title screen rendering loop |
| D2Win Initialization | `0x00403000` | D2Win window creation |
| Sound System | `0x00404000` | DirectSound initialization |

## Complete Call Flow

### 1. Entry Point to Main (0x00401000 - 0x00401100)

```
Game.exe Entry (0x00401000)
    ├─> D2DllInit() - DLL initialization
    │   ├─> LoadModuleNames() - Load core module names
    │   └─> InitializeGlobals() - Setup global game state
    │
    └─> main() [0x00401100]
        ├─> ParseCommandLine() - Parse /w, /devmode, /direct3d flags
        ├─> InitGlobalData() - Initialize shared game data
        │   ├─> GAME_Init() - Game global initialization
        │   ├─> PLAYER_Init() - Player data initialization
        │   └─> D2LANG_Init() - Language system initialization
        └─> WinMain() - Windows application entry
```

### 2. Window Creation & D2Win Setup (0x00403000 - 0x00404000)

```
WinMain()
    ├─> D2Win_CreateMainWindow() - Create main window
    │   ├─> CreateWindowExW() - System window creation
    │   └─> SetWindowLongPtr() - Set window styles
    │
    └─> D2Win_Init()
        ├─> LoadLanguageFiles() - Load language text files
        │   ├─> LoadLanguageDLL() - Load enGB.dll or localization
        │   └─> ParseLangStrings() - Build string table
        │
        ├─> InitGraphicsSystem()
        │   ├─> DirectX_Init() - Initialize DirectDraw/Direct3D
        │   ├─> CreatePrimarySurface() - Create main display surface
        │   └─> SetGammaRamp() - Set screen gamma levels
        │
        └─> InitSoundSystem()
            ├─> DirectSoundCreate() - Initialize DirectSound
            ├─> SetCooperativeLevel() - Set sound permissions
            └─> CreatePrimaryBuffer() - Create sound buffer
```

### 3. Title Screen Initialization (0x00401200 - 0x00405000)

```
WinMain() loop
    └─> TitleScreen_Init()
        ├─> LoadTitleScreenGraphics()
        │   ├─> LoadSplashImages() - Load title logo, backgrounds
        │   ├─> LoadFontTextures() - Load title font bitmaps
        │   └─> LoadLogoAnimation() - Setup animated logo frames
        │
        ├─> LoadTitleScreenStrings()
        │   ├─> GetLocaleString(0x2001) - "DIABLO II"
        │   ├─> GetLocaleString(0x2002) - "Blizzard Entertainment"
        │   ├─> GetLocaleString(0x2003) - Version string
        │   └─> GetLocaleString(0x2004) - "Press any key to continue"
        │
        └─> SetupTitleScreenInput()
            ├─> SetKeyboardCallback() - Register key handlers
            ├─> SetMouseCallback() - Register mouse handlers
            └─> SetTimerCallback() - Setup animation timers
```

### 4. Title Screen Rendering Loop (0x00406000 - 0x00410000)

```
Main Game Loop (0x00401100)
    └─> WinMain() message loop
        └─> TitleScreen_Render()
            ├─> ClearScreen() - Fill with black
            │
            ├─> DrawTitleScreenBackground()
            │   ├─> DrawGrainyNoise() - Add texture noise
            │   ├─> DrawStylizedLogo() - Draw Diablo logo
            │   └─> DrawBloodSplatter() - Add atmospheric effects
            │
            ├─> DrawTitleScreenText()
            │   ├─> TextOutW(0x2001) - "DIABLO II"
            │   ├─> TextOutW(0x2002) - "Blizzard Entertainment"
            │   ├─> TextOutW(0x2003) - Version number
            │   └─> TextOutW(0x2004) - Prompt text
            │
            └─> UpdateTitleScreenAnimation()
                ├─> AnimateLogo() - Logo scaling/rotation
                ├─> DrawTimer() - Progress bar animation
                └─> UpdateCursor() - Flashing mouse cursor

WinMain() message handling:
    ├─> WM_KEYDOWN - Handle any key press
    │   └─> TransitionToCharacterSelect()
    │       ├─> PlayClickSound()
    │       ├─> HideTitleScreen()
    │       └─> LoadCharacterSelect()
    │
    └─> WM_CLOSE - Handle window close
        └─> ExitGame() - Clean shutdown
```

## Critical Data Structures

### Global Game State (sgpGlobal)
```cpp
struct SGLOBAL {
    HWND hWnd;                    // Main window handle
    HINSTANCE hInstance;          // Application instance
    int nScreenWidth;             // Window width
    int nScreenHeight;            // Window height
    int nScreenDepth;             // Color depth (16/32 bit)
    int nWindowStyle;             // Window styles (WS_OVERLAPPED)
    char szTitle[256];            // Window title
    char szCommandLine[512];      // Command line arguments
    int nFrameRate;               // Target frame rate
    BOOL bFullscreen;             // Fullscreen mode flag
    BOOL bDirect3D;               // Use Direct3D flag
};
```

### Title Screen Resources
```cpp
struct TITLESCREEN_RESOURCES {
    HBITMAP hbmTitleLogo;         // Main logo bitmap
    HBITMAP hbmBackground;        // Background texture
    HFONT hftTitleFont;           // Title font
    HFONT hftSubFont;             // Subtitle font
    LPWSTR wszTitleStrings[4];    // Loaded string pointers
    int nLogoAnimFrame;           // Current logo animation frame
    DWORD dwLogoAnimTimer;        // Logo animation timer
};
```

## Key Functions to Analyze

### Initialization Functions
- `main()` @ 0x00401100 - Entry point analysis
- `WinMain()` @ 0x00401200 - Main message loop
- `D2DllInit()` @ 0x00401050 - DLL initialization
- `InitGlobalData()` @ 0x00401250 - Global data setup

### Graphics Functions
- `DirectX_Init()` @ 0x00403050 - DirectX initialization
- `CreatePrimarySurface()` @ 0x00403100 - Main display surface
- `TitleScreen_Render()` @ 0x00405050 - Rendering loop

### Sound Functions
- `InitSoundSystem()` @ 0x00404050 - Sound initialization
- `DirectSoundCreate()` @ 0x00404100 - DirectSound creation
- `PlayClickSound()` @ 0x00404200 - Sound playback

## D2VP Integration Points

### What D2VP Needs to Override
1. **Title Screen Rendering** - D2VP may replace title screen with custom UI
2. **Language Loading** - Localization handling for different regions
3. **Version String** - Custom version display for D2VP releases
4. **Splash Screen** - Custom startup animations

### Potential D2VP Extensions
1. **Custom Background** - Replace background texture with D2VP branding
2. **Custom Logo** - Use D2VP logo instead of Diablo II
3. **Custom Strings** - Add D2VP-specific text
4. **Startup Sound** - Custom startup sound instead of default

## Analysis Commands

```bash
# Analyze entry point
r2 -w Game.exe
s 0x00401000
pdf

# Find main function
s entry
pdc

# Analyze title screen rendering
s 0x00405000
pdf

# Find all string references to title
iz~

# Find graphics loading functions
iz~graphics

# Find sound initialization
iz~sound
```

## Related Documentation

- `R2_Game.exe_Analysis_Guide.md` - R2 usage guide
- `D2VP_GameExe_Function_Mapping.md` - Function mapping reference
- `R2_Project_Setup_Guide.md` - Setup instructions

## Notes

- Entry point analysis reveals extensive pre-main initialization
- Title screen uses separate resource set from game screens
- Sound system initialized early for UI feedback
- Language system loads before rendering to support localization
- Title screen loop is simple compared to game loop - low CPU usage
- Graphics system uses DirectDraw/Direct3D - similar to D2VP implementation

## Version History

- **v1.0** (2026-11-17): Initial call flow documentation based on Game.exe 1.14d analysis
