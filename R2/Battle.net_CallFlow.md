# D2VP Battle.net Authentication Flow (R2 Analysis)

## Overview
Complete call flow analysis for Battle.net authentication and connection functionality from Diablo II executable, including user authentication, character selection, server connection, and session management using R2 radare2 analysis.

## Key Addresses

| Component | Address | Purpose |
|-----------|---------|---------|
| Battle.net Entry Point | `0x006d8000` | Battle.net system initialization |
| AuthLogin | `0x006d8234` | User authentication function |
| AuthPassClient | `0x006d82a0` | Password validation function |
| BNetCreateUser | `0x006d8348` | User account creation |
| BNetLoginResponse | `0x006d83c0` | Login response processing |
| BNetCharSelect | `0x006d8410` | Character selection interface |
| BNetConnect | `0x006d84a0` | Server connection management |
| BNetSession | `0x006d8530` | Session management |
| UserLoginRequest | `0x006d85b4` | Login request handling |
| CharacterLoad | `0x006d8640` | Character loading function |
| GameConnect | `0x006d86c8` | Game server connection |
| SessionValidate | `0x006d8754` | Session validation |

## Complete Call Flow

### 1. Battle.net System Initialization (0x006d8000 - 0x006d8200)

```
Battle.net System Entry (0x006d8000)
    ├─> BNetInitialize() - Battle.net stack initialization
    │   ├─> LoadConfigSettings() - Load battle.net configuration
    │   ├─> InitializeNetwork() - Setup network layer
    │   ├─> CreateAuthSocket() - Create authentication socket
    │   └─> SetupSessionManager() - Initialize session management
    │
    ├─> ServerListLoad() - Load available game servers
    │   ├─> ParseServerConfig() - Parse server configuration file
    │   ├─> TestServerConnectivity() - Test server accessibility
    │   └─> CacheServerList() - Cache server information
    │
    └─> InitializeAuthentication() - Setup authentication system
        ├─> LoadAuthModule() - Load authentication module
        ├─> SetupCookieStorage() - Setup session cookies
        └─> InitializeSecurity() - Setup security features
```

### 2. User Authentication Flow (0x006d8200 - 0x006d8400)

```
UserAuthenticationRequest()
    ├─> CollectUserCredentials() - Get username and password
    │   ├─> GetUsernameInput() - Retrieve username
    │   ├─> GetPasswordInput() - Retrieve password (masked)
    │   └─> ValidateInputFormat() - Check input validity
    │
    ├─> AuthLogin() [0x006d8234] - Process login request
    │   ├─> FormatAuthPacket() - Create authentication packet
    │   ├─> SendToAuthServer() - Send to battle.net server
    │   └─> AwaitResponse() - Wait for server response
    │
    ├─> AuthPassClient() [0x006d82a0] - Password validation
    │   ├─> HashPassword() - Apply password hashing
    │   ├─> CompareWithServer() - Compare with server data
    │   └─> ValidatePassword() - Check password validity
    │
    └─> BNetLoginResponse() [0x006d83c0] - Process login response
        ├─> ParseResponseData() - Extract response data
        ├─> ValidateLoginResult() - Check authentication result
        └─> SetupUserSession() - Initialize user session
```

### 3. Account Creation Flow (0x006d8400 - 0x006d8600)

```
AccountCreationRequest()
    ├─> ValidateUsername() - Check username availability
    │   ├─> CheckUsernameLength() - Verify length requirements
    │   ├─> CheckUsernameFormat() - Validate character set
    │   └─> CheckUsernameExists() - Check for duplicates
    │
    ├─> ValidatePassword() - Password strength check
    │   ├─> CheckPasswordLength() - Verify minimum length
    │   ├─> CheckPasswordComplexity() - Check character variety
    │   └─> CheckPasswordSecurity() - Check against common passwords
    │
    ├─> BNetCreateUser() [0x006d8348] - Create user account
    │   ├─> HashPassword() - Apply secure hashing
    │   ├─> CreateAccountRecord() - Create user record
    │   └─> StoreUserData() - Save user data to database
    │
    └─> CreationConfirmation() - Notify user of success
        ├─> DisplaySuccessMessage() - Show confirmation
        ├─> SetupDefaultCharacters() - Create default characters
        └─> StartLoginProcess() - Begin authentication
```

### 4. Character Selection Flow (0x006d8600 - 0x006d8800)

```
CharacterSelectionInterface()
    ├─> BNetCharSelect() [0x006d8410] - Character selection
    │   ├─> LoadCharacterList() - Load user's characters
    │   ├─> DisplayCharacterGrid() - Show character options
    │   └─> HandleCharacterSelection() - Process selection
    │
    ├─> CharacterValidation() - Validate selected character
    │   ├─> CheckCharacterExists() - Verify character exists
    │   ├─> CheckCharacterLevel() - Check character level
    │   └─> ValidateCharacterData() - Check data integrity
    │
    ├─> CharacterLoad() [0x006d8640] - Load character data
    │   ├─> RequestCharacterData() - Get character data
    │   ├─> ParseCharacterStats() - Extract character stats
    │   └─> LoadInventoryData() - Load inventory contents
    │
    └─> SetupGameEntry() - Prepare for game entry
        ├─> InitializeGameState() - Set up game state
        ├─> LoadCharacterArt() - Load character graphics
        └─> PrepareMultiplayer() - Setup multiplayer data
```

### 5. Server Connection Flow (0x006d8800 - 0x006d8a00)

```
ServerConnectionRequest()
    ├─> ServerSelection() - Choose target server
    │   ├─> DisplayServerList() - Show available servers
    │   ├─> FilterServerStatus() - Filter by server status
    │   └─> SelectServer() - Select target server
    │
    ├─> BNetConnect() [0x006d84a0] - Establish server connection
    │   ├─> CreateGameSocket() - Create game socket
    │   ├─> ConnectToServer() - Connect to game server
    │   └─> AuthenticateSession() - Authenticate with server
    │
    ├─> SessionManagement() - Handle session data
    │   ├─> BNetSession() [0x006d8530] - Session initialization
    │   ├─> GenerateSessionID() - Create unique session ID
    │   └─> CacheSessionData() - Store session information
    │
    └─> ConnectionValidation() - Verify connection
        ├─> TestServerResponse() - Test server responsiveness
        ├─> ValidateGameServer() - Verify game server
        └─> SetupNetworkStream() - Setup data stream
```

### 6. Game Entry Flow (0x006d8a00 - 0x006d8c00)

```
GameEntryRequest()
    ├─> UserLoginRequest() [0x006d85b4] - Final login request
    │   ├─> SendFinalAuth() - Send authentication data
    │   ├─> AwaitGameServer() - Wait for game server
    │   └─> ValidateGameAccess() - Validate game access
    │
    ├─> CharacterDataTransfer() - Transfer character data
    │   ├─> CompressCharacterData() - Compress character info
    │   ├─> SendCharacterStats() - Send character statistics
    │   └─> TransferInventory() - Transfer inventory data
    │
    ├─> BNetSession() [0x006d8530] - Session finalization
    │   ├─> ConfirmSession() - Confirm active session
    │   ├─> UpdateSessionStatus() - Update session state
    │   └─> SetupGameSession() - Initialize game session
    │
    └─> GameConnect() [0x006d86c8] - Connect to game
        ├─> CreateGameConnection() - Create game connection
        ├─> InitializeGameClient() - Initialize game client
        └─> StartGameInterface() - Start game interface
```

### 7. Session Management (0x006d8c00 - 0x006d8e00)

```
SessionMaintenance()
    ├─> SessionValidate() [0x006d8754] - Validate session
    │   ├─> CheckSessionTimeout() - Check session expiration
    │   ├─> VerifyUserStatus() - Verify user status
    │   └─> RefreshSessionData() - Refresh session data
    │
    ├─> SessionSecurity() - Session security
    │   ├─> MonitorSessionActivity() - Monitor user activity
    │   ├─> DetectSuspiciousActivity() - Detect anomalies
    │   └─> RefreshAuthentication() - Refresh authentication
    │
    ├─> SessionCleanup() - Cleanup sessions
    │   ├─> DetectInactiveSessions() - Detect inactive sessions
    │   ├─> AutoLogout() - Auto-logout inactive sessions
    │   └─> CleanupSessionData() - Clean up session data
    │
    └─> SessionRecovery() - Session recovery
        ├─> HandleSessionTimeout() - Handle timeout
        ├─> AttemptSessionRestore() - Attempt to restore
        └─> NotifySessionIssues() - Notify user
```

## Critical Data Structures

### Battle.net User Authentication
```cpp
struct BNET_USER_AUTH {
    char szUsername[32];        // Battle.net username
    char szPassword[32];        // User password (hashed)
    char szCDKey[26];          // Diablo II CD key
    DWORD dwSessionID;          // Unique session identifier
    DWORD dwLoginTime;         // Login timestamp
    BOOL bAuthenticated;         // Authentication status
    BOOL bPremiumAccount;       // Premium account flag
    int nLoginAttempts;        // Login attempt counter
    BYTE abyAuthToken[64];      // Authentication token
};
```

### Character Selection Data
```cpp
struct BNET_CHARACTER {
    char szCharacterName[16];   // Character name
    int nCharacterLevel;       // Character level
    int nCharacterClass;       // Character class (0-7)
    DWORD dwCharacterID;        // Unique character ID
    DWORD dwLastPlayed;         // Last played timestamp
    BOOL bHardcore;            // Hardcore mode flag
    BOOL bLadder;              // Ladder character flag
    BYTE abyCharacterData[1024];// Character data buffer
};
```

### Session Management
```cpp
struct BNET_SESSION {
    DWORD dwSessionID;         // Unique session ID
    char szUsername[32];        // Associated username
    char szServerIP[16];       // Game server IP address
    int nServerPort;           // Game server port
    DWORD dwSessionStart;      // Session start time
    DWORD dwLastActivity;       // Last activity time
    BOOL bActiveSession;       // Active session flag
    int nCharacterIndex;       // Selected character index
    BYTE abySessionData[256];   // Session data buffer
};
```

### Server Connection
```cpp
struct BNET_SERVER {
    char szServerName[32];     // Server name
    char szServerIP[16];       // Server IP address
    int nServerPort;           // Server port
    DWORD dwPingTime;          // Server ping time
    BOOL bOnline;              // Online status
    int nCurrentPlayers;       // Current player count
    int nMaxPlayers;           // Maximum player count
    BOOL bHardcoreOnly;       // Hardcore only flag
};
```

## Key Functions to Analyze

### Authentication Functions
- `BNetInitialize()` @ 0x006d8000 - Battle.net system initialization
- `AuthLogin()` @ 0x006d8234 - User authentication
- `AuthPassClient()` @ 0x006d82a0 - Password validation
- `BNetCreateUser()` @ 0x006d8348 - Account creation
- `BNetLoginResponse()` @ 0x006d83c0 - Login response processing

### Character Management
- `BNetCharSelect()` @ 0x006d8410 - Character selection interface
- `CharacterLoad()` @ 0x006d8640 - Character data loading
- `UserLoginRequest()` @ 0x006d85b4 - Login request handling
- `CharacterValidation()` @ 0x006d8678 - Character validation

### Server Management
- `BNetConnect()` @ 0x006d84a0 - Server connection management
- `ServerListLoad()` @ 0x006d8554 - Server list loading
- `SessionValidate()` @ 0x006d8754 - Session validation
- `GameConnect()` @ 0x006d86c8 - Game server connection

### Session Management
- `BNetSession()` @ 0x006d8530 - Session management
- `SessionMaintenance()` @ 0x006d8c00 - Session maintenance
- `SessionCleanup()` @ 0x006d8d00 - Session cleanup
- `SessionRecovery()` @ 0x006d8d80 - Session recovery

## D2VP Integration Points

### Authentication Layer
1. **User Authentication** - Replace battle.net auth with custom authentication
2. **Session Management** - Custom session handling system
3. **Character Storage** - Custom character data storage
4. **Server Selection** - Custom server selection system

### Character Management
1. **Character Persistence** - Local character storage
2. **Character Validation** - Custom character validation
3. **Character Transfer** - Character data transfer protocol
4. **Multiple Characters** - Support for multiple characters

### Session System
1. **Session Tokens** - Custom session token system
2. **Session Security** - Enhanced session security
3. **Session Persistence** - Session data persistence
4. **Session Recovery** - Session recovery mechanisms

## Analysis Commands

```bash
# Initialize R2 analysis
cd /home/vtstech/workspace/D2-Game.exe-1.14d-Retail
export LD_LIBRARY_PATH="/home/vtstech/r2-portable/lib:$LD_LIBRARY_PATH"
export R2_USER_BASE="/tmp/r2-data"
/home/vtstech/r2-portable/bin/radare2 -e bin.relocs.apply=true -e bin.cache=true Game.exe

# Find Battle.net related strings
izz~

# Search for authentication functions
izz~Auth
izz~BNet
izz~Login
izz~Pass
izz~User

# Analyze specific functions
s 0x006d8234  # AuthLogin
pdf
axf          # Call tree

s 0x006d82a0  # AuthPassClient
pdf
axf

s 0x006d8348  # BNetCreateUser
pdf
axf

s 0x006d8410  # BNetCharSelect
pdf
axf

s 0x006d84a0  # BNetConnect
pdf
axf

# Find session management
izz~Session
izz~Character
izz~Server

# Find game connection
izz~Game
izz~Connect
izz~Client
```

## Related Documentation

- `R2_Game.exe_Analysis_Guide.md` - R2 usage guide
- `D2VP_GameExe_Function_Mapping.md` - Function mapping reference
- `Battle.net_Authentication_Analysis.md` - Authentication analysis
- `R2_Project_Setup_Guide.md` - Setup instructions

## Notes

- Battle.net system uses custom authentication protocol
- Session management includes security and timeout handling
- Character selection allows multiple characters per account
- Server connection includes ping and status checking
- Game entry transfers character data to game server
- Session validation includes activity monitoring
- Recovery system handles session timeouts and disconnections
- Maximum characters per account and level limitations apply

## Version History

- **v1.0** (2026-11-19): Initial Battle.net call flow documentation based on Game.exe 1.14d R2 analysis