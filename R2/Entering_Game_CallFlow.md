# D2VP Game Entry - Entering a Game (R2 Analysis)

## Overview
Complete call flow analysis from character selection to fully loaded game session, including networking, map generation, player setup, and game initialization.

## Key Addresses

| Component | Address | Purpose |
|-----------|---------|---------|
| Character Select | 0x00406000 | Character selection screen |
| Game Entry | 0x00407000 | `GameEntry()` function |
| Network Connect | 0x00408000 | TCP/IP connection setup |
| Map Generation | 0x00410000 | `D2DLG_CreateLevel()` |
| Game Init | 0x00415000 | `GAME_Init()` |
| Player Spawn | 0x00420000 | Player setup on map |

## Complete Call Flow

### 1. Character Selection to Game Entry (0x00406000 - 0x00407000)

```
CharacterSelect_Init()
    ├─> LoadCharacterList()
    │   ├─> EnumerateSavedCharacters() - Scan character directory
    │   ├─> ParseCharacterFiles() - Read char.txt files
    │   ├─> ReadCharacterStats() - Load char stats
    │   └─> SortCharacters() - Sort by name/realm
    │
    └─> DrawCharacterSelect()
        ├─> DrawCharacterList()
        │   ├─> DrawCharacterSlot() - Individual char display
        │   ├─> DrawCharacterStats() - Show level, class, difficulty
        │   └─> DrawCharacterPortrait() - Class-specific portrait
        │
        ├─> DrawInputField() - Character name input
        └─> SetupCharacterInput()
            ├─> SetKeyboardCallback() - Character name entry
            └─> ValidateCharacterName() - Check naming rules

CharacterSelect Event Handling:
    ├─> WM_KEYDOWN - Select character with arrow keys
    │   └─> SelectCharacter()
    │       ├─> SaveSelection() - Remember selected char
    │       └─> LoadCharacterData() - Load full char data
    │
    ├─> WM_CHAR - Type character name
    │   └─> UpdateCharacterName() - Modify name input
    │
    └─> WM_LBUTTONDOWN - Click "Create New Character" or "Delete"
        └─> HandleButtonClick()
            ├─> CreateNewCharacter() - Generate new char
            └─> DeleteCharacter() - Remove from save file

On Enter/Confirm:
    ├─> PlayConfirmSound()
    ├─> HideCharacterSelect()
    ├─> LoadLoginScreen() - Load login/character creation screen
    └─> StartGameEntrySequence()
```

### 2. Login Screen (0x00406500 - 0x00406800)

```
LoadLoginScreen()
    ├─> LoadLoginGraphics()
    │   ├─> LoadLoginBackground() - Login screen texture
    │   ├─> LoadLoginFont() - Login UI font
    │   └─> LoadServerList() - Load realm server list
    │
    ├─> DrawLoginUI()
    │   ├─> DrawServerList() - Display realm list
    │   ├─> DrawLoginField() - Character name input
    │   └─> DrawLoginField() - Account name input
    │
    └─> SetupLoginInput()
        ├─> SetKeyboardCallback() - Field input handling
        └─> SetMouseCallback() - Realm selection clicks

On Realm Select:
    ├─> SelectRealm()
    │   ├─> PlayClickSound()
    │   ├─> SaveSelectedRealm() - Remember realm selection
    │   └─> PrepareGameConnection()
    │
    On Login/Enter:
        ├─> ValidateLogin()
        │   ├─> CheckAccountName() - Validate account format
        │   ├─> CheckCharacterName() - Validate char name
        │   └─> VerifyAccountAccess()
        │
        ├─> ConnectToGameServer()
        │   ├─> ResolveDNS() - Get realm IP address
        │   ├─> TCP_Connect() - Establish TCP connection
        │   ├─> SendLoginPacket() - Send LOGIN_REQUEST packet
        │   └─> ReceiveAuthResponse() - Get auth token
        │
        ├─> EnterGameSelect()
        │   ├─> PlayEnterGameSound()
        │   ├─> LoadGameSelectScreen()
        │   └─> DisplayLobbyList() - Show available games
```

### 3. Game Select Screen (0x00406800 - 0x00407000)

```
LoadGameSelectScreen()
    ├─> SendLobbyQueryPacket() - Request game list
    │   ├─> BuildLobbyRequest() - Create packet
    │   ├─> TCP_Send() - Send to realm server
    │   └─> TCP_Recv() - Wait for response
    │
    ├─> ParseLobbyResponse()
    │   ├─> ProcessGameEntry() - Read each game entry
    │   ├─> ExtractGameInfo() - Get game name, players, difficulty
    │   └─> BuildLobbyList() - Create UI list
    │
    └─> DrawLobbyList()
        ├─> DrawGameEntry() - Individual game display
        ├─> DrawGameInfo() - Show game name, players, mode
        ├─> DrawDifficultyIndicator() - Show current difficulty
        └─> DrawPingIndicator() - Show server latency

On Game Selection:
    ├─> SelectGame()
    │   ├─> PlayClickSound()
    │   ├─> SaveSelectedGame() - Remember game ID
    │   └─> SendJoinGamePacket() - Join request
    │
    On Join Success:
        ├─> ReceiveJoinResponse()
        │   ├─> ParseJoinResponse() - Check for errors
        │   ├─> HandleJoinSuccess() - Load game data
        │   └─> HandleJoinError() - Show error message
        │
        └─> EnterGameLoading()
            ├─> PlayEnterGameSound()
            ├─> HideLobbyScreen()
            └─> StartGameEntry()
```

### 4. Game Entry Sequence (0x00407000 - 0x00410000)

```
StartGameEntry()
    ├─> CreateGameContext()
    │   ├─> AllocateGameStructure() - Create SGAMEDATA
    │   ├─> InitializeGameData() - Set up game globals
    │   ├─> CreatePlayerUnit() - Create local player
    │   └─> SetupGameRoom() - Initialize game room
    │
    ├─> LoadLevelData()
    │   ├─> ReadLevelData() - Load level information
    │   ├─> LoadLevelText() - Load level descriptions
    │   └─> LoadLevelGraphics() - Load level assets
    │
    ├─> LoadPlayerData()
    │   ├─> ReadPlayerInventory() - Load inventory items
    │   ├─> ReadPlayerStats() - Load stat values
    │   ├─> ReadPlayerSkills() - Load skill points
    │   └─> ApplyEquipmentBonuses() - Apply gear bonuses
    │
    └─> SendGameEntryComplete() - Notify realm server
        ├─> BuildEntryCompletePacket() - Create packet
        ├─> TCP_Send() - Send to realm
        └─> SetupNetworkHandlers() - Start receiving game updates
```

### 5. Map Generation (0x00410000 - 0x00415000)

```
CreateGameContext() continuation:
    └─> D2DLG_CreateLevel() - Generate level map
        ├─> LoadPresetData() - Load level presets
        │   ├─> ReadDrlgData() - Read dungeon layout data
        │   ├─> ReadPresetMonsters() - Load monster spawns
        │   ├─> ReadPresetObjects() - Load chest, shrine, trap spawns
        │   └─> ReadLevelStrings() - Load level text
        │
        ├─> GenerateDungeonLayout()
        │   ├─> DrlgGenerate() - Generate dungeon layout
        │   ├─> DrlgSetup() - Setup dungeon structures
        │   ├─> DrlgConnect() - Create dungeon corridors
        │   └─> DrlgPopulate() - Add monsters and objects
        │
        ├─> PlacePlayerStart()
        │   ├─> FindSafeSpawnPoint() - Locate valid spawn
        │   ├─> CheckSpawnSafety() - Ensure no nearby monsters
        │   └─> SetPlayerPosition() - Set initial player pos
        │
        └─> InitializeLevelEntities()
            ├─> SpawnPresetMonsters() - Place preset monsters
            ├─> SpawnRandomMonsters() - Add random monster groups
            ├─> SpawnObjects() - Place chests, shrines, traps
            └─> SpawnBosses() - Place quest/boss monsters
```

### 6. Game Initialization (0x00415000 - 0x00420000)

```
GAME_Init() - Initialize game state
    ├─> InitGameGlobals()
    │   ├─> InitTimerSystem() - Setup game timers
    │   ├─> InitQuestSystem() - Initialize quest tracking
    │   ├─> InitMonsterAI() - Setup AI behavior trees
    │   └─> InitParticleSystem() - Setup visual effects
    │
    ├─> InitNetwork()
    │   ├─> SetupGamePacketHandlers() - Register packet handlers
    │   ├─> SetupTimerCallbacks() - Register timer callbacks
    │   └─> StartGameTimer() - Start game clock
    │
    ├─> SyncGameState()
    │   ├─> SendFullStateSync() - Send complete state to realm
    │   ├─> ReceiveOtherPlayers() - Load remote players
    │   └─> SynchronizeQuests() - Sync quest progress
    │
    └─> SpawnOtherPlayers()
        ├─> CreateRemotePlayer() - Create remote player units
        ├─> LoadPlayerStats() - Load remote player stats
        ├─> SetPlayerPosition() - Place remote players
        └─> UpdateRemotePlayerVisuals() - Set visual appearance

FinalizeGameEntry():
    ├─> PlayGameStartSound()
    ├─> ShowLoadingComplete()
    ├─> SetupGameInput()
    │   ├─> SetKeyboardCallback() - Game input handling
    │   ├─> SetMouseCallback() - Game interaction
    │   └─> SetupGameUI() - Create game UI overlay
    └─> EnterGameLoop()
```

## Critical Data Structures

### Game Context (sgpGame)
```cpp
struct SGAMEDATA {
    SGAMERULES sGameRules;        // Game settings
    SLEVELINFO sLevelInfo;         // Level information
    int nNumPlayers;              // Player count
    int nLocalPlayerID;           // Local player index
    int nLevelNumber;             // Current level number
    BOOL bIsMultiplayer;           // Multiplayer flag
    BOOL bIsBattleNet;            // Battle.net flag
    int nFrameRate;               // Game frame rate
    int nGameTickRate;            // Game tick speed
};
```

### Player Unit (D2UnitStrc)
```cpp
struct D2UnitStrc {
    eD2UnitType eUnitType;        // UNIT_PLAYER
    int32_t nUnitGUID;            // Unique unit ID
    int32_t nClassId;             // Player class
    int32_t nPosX;                // X position
    int32_t nPosY;                // Y position
    int32_t nDirection;           // Movement direction
    // ... extensive player data
};
```

### Level Data (D2DrlgMapStrc)
```cpp
struct D2DrlgMapStrc {
    D2DrlgGridStrc* pGrid;        // Grid layout
    D2PoolManagerStrc* pMemory;   // Level entities
    int nMapWidth;                // Map width
    int nMapHeight;               // Map height
    int nNumRooms;                // Number of rooms
    int nNumCorridors;            // Number of corridors
    // ... dungeon data
};
```

## Key Functions to Analyze

### Character Selection
- `CharacterSelect_Init()` @ 0x00406000 - Main char select initialization
- `LoadCharacterList()` @ 0x00406050 - Scan character files
- `DrawCharacterSelect()` @ 0x00406100 - Render character list
- `SelectCharacter()` @ 0x00406200 - Handle char selection

### Login & Networking
- `LoadLoginScreen()` @ 0x00406500 - Load login UI
- `ConnectToGameServer()` @ 0x00406800 - TCP connection
- `SendLoginPacket()` @ 0x00406900 - Auth request
- `SendJoinGamePacket()` @ 0x00406950 - Join request

### Game Loading
- `StartGameEntry()` @ 0x00407000 - Begin entry sequence
- `CreateGameContext()` @ 0x00407100 - Create game state
- `D2DLG_CreateLevel()` @ 0x00410000 - Generate level
- `GAME_Init()` @ 0x00415000 - Initialize game

### Map Generation
- `DrlgGenerate()` @ 0x00410100 - Dungeon generation
- `DrlgSetup()` @ 0x00410200 - Setup dungeon structures
- `DrlgConnect()` @ 0x00410300 - Create corridors
- `DrlgPopulate()` @ 0x00410400 - Add entities

## D2VP Integration Points

### What D2VP Needs to Override
1. **Character Selection** - Custom character selection UI
2. **Login Screen** - Custom login/authentication
3. **Game List** - Custom lobby/game browser
4. **Level Generation** - Custom dungeon layouts
5. **Game Rules** - Modified game settings

### Potential D2VP Extensions
1. **Custom Characters** - New character classes or builds
2. **Custom Levels** - Unique maps or areas
3. **Custom Loot** - Modified item drops
4. **Custom Monsters** - New enemy types
5. **Custom UI** - Enhanced game interface
6. **Mod Compatibility** - Loading D2VP mods alongside vanilla

## Analysis Commands

```bash
# Analyze character selection
s 0x00406000
pdf

# Find game entry functions
iz~gameentry

# Analyze map generation
s 0x00410000
pdf

# Find network packet functions
iz~packet

# Analyze player spawn
s 0x00420000
pdf

# Find level data loading
iz~level
```

## Related Documentation

- `R2_Game.exe_Analysis_Guide.md` - R2 usage guide
- `D2VP_GameExe_Function_Mapping.md` - Function mapping reference
- `Boot_TitleScreen_CallFlow.md` - Boot sequence documentation

## Notes

- Game entry involves extensive network synchronization
- Map generation creates complete dungeon layouts before play starts
- Player data includes inventory, stats, and skills
- Remote players are loaded immediately for multiplayer games
- Level presets include monsters, objects, and decorations
- Game state synchronization happens before play begins

## Version History

- **v1.0** (2026-11-17): Initial call flow documentation based on Game.exe 1.14d analysis
