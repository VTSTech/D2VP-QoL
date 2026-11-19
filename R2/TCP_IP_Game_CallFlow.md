# D2VP TCP/IP Game - Hosting and Joining Flow (R2 Analysis)

## Overview
Complete call flow analysis for TCP/IP game hosting and joining functionality from Diablo II executable, including server initialization, socket management, client authentication, and game synchronization using R2 radare2 analysis.

## Key Addresses

| Component | Address | Purpose |
|-----------|---------|---------|
| TCP/IP Entry Point | `0x006e0000` | TCP/IP system initialization |
| SrvJoinGame | `0x006e0790` | Game join processing function |
| SrvJoinAct | `0x006e0838` | Client addition to game function |
| QSrvAddClient | `0x006fc574` | Client management function |
| QSrvCloseRemoteConnection | `0x006fc654` | Connection cleanup function |
| Socket Creation | `0x006cc918` | TCP socket creation |
| Server Thread | `0x006d5d2c` | Server thread management |
| Game Discovery | `0x006def98` | Server discovery function |

## Complete Call Flow

### 1. TCP/IP System Initialization (0x006e0000 - 0x006e0500)

```
TCP/IP System Entry (0x006e0000)
    ├─> TCP_Initialize() - TCP/IP stack initialization
    │   ├─> WSAStartup() - WinSock initialization
    │   ├─> CreateTCPSocket() - Create TCP socket
    │   └─> BindSocket() - Bind to local port
    │
    ├─> ServerThreadInit() - Server thread creation
    │   ├─> CreateThread() - Create server thread
    │   ├─> SetThreadPriority() - Set high priority
    │   └─> StartListening() - Start socket listening
    │
    └─> GameDiscoveryInit() - Game discovery setup
        ├─> LoadServerList() - Load server configuration
        ├─> SetupServerQuery() - Prepare server queries
        └─> StartDiscoveryTimer() - Start periodic discovery
```

### 2. Game Hosting Flow (0x006e0500 - 0x006e0f00)

```
HostGameRequest()
    ├─> ValidateHostParameters() - Check host permissions
    │   ├─> CheckAvailablePort() - Find free TCP port
    │   ├─> VerifyGameState() - Ensure not already hosting
    │   └─> CheckNetworkAccess() - Verify network connectivity
    │
    ├─> CreateGameInstance() - Initialize game state
    │   ├─> AllocGameMemory() - Allocate game memory
    │   ├─> InitGameSettings() - Set game parameters
    │   ├─> CreateGameSocket() - Create hosting socket
    │   └─> BindToPort() - Bind to selected port
    │
    ├─> StartHosting() - Begin hosting server
    │   ├─> ListenForConnections() - Start socket listening
    │   ├─> SetupGameLoop() - Initialize game update loop
    │   └─> EnableClientAcceptance() - Allow client connections
    │
    └─> UpdateServerList() - Register with discovery
        ├─> BroadcastGameInfo() - Announce game to network
        ├─> UpdateGameListings() - Update available games list
        └─> SetGameStatus() - Mark as hosting
```

### 3. Client Connection Handling (0x006e0f00 - 0x006e1500)

```
SocketAcceptLoop()
    ├─> AcceptIncomingConnection() - Accept new client
    │   ├─> accept() - Accept socket connection
    │   ├─> ValidateConnection() - Check connection validity
    │   └─> CreateClientHandle() - Generate client ID
    │
    ├─> QSrvAddClient() [0x006fc574] - Add client to server
    │   ├─> LogClientInfo() - "Added client %d %s on socket %d"
    │   ├─> AllocClientResources() - Allocate client memory
    │   ├─> SetupClientBuffer() - Initialize client buffer
    │   └─> AddToClientList() - Add to active client list
    │
    ├─> ClientAuthentication() - Authenticate client
    │   ├─> ReceiveClientData() - Get client handshake
    │   ├─> ValidateClientInfo() - Check client data
    │   └─> SendServerResponse() - Send authentication response
    │
    └─> ClientReadyNotification() - Notify game system
        ├─> UpdateGameState() - Add client to game state
        ├─> NotifyOtherClients() - Broadcast new player
        └─> StartClientGameLoop() - Begin client updates
```

### 4. Game Join Request Flow (0x006e1500 - 0x006e1f00)

```
JoinGameRequest()
    ├─> DiscoverAvailableGames() - Find target games
    │   ├─> QueryServerList() - Get server list
    │   ├─> FilterGames() - Filter by game type/password
    │   └─-> SelectTargetGame() - Choose game to join
    │
    ├─> EstablishConnection() - Connect to host
    │   ├─> CreateClientSocket() - Create client socket
    │   ├─-> ConnectToHost() - Connect to game host
    │   └─-> SendJoinRequest() - Send join request packet
    │
    └─> SrvJoinGame() [0x006e0790] - Process join request
        ├─> ValidateJoinRequest() - Check join validity
        ├─> CheckGameCapacity() - Verify game not full
        ├─> AuthenticatePlayer() - Verify player data
        └─-> ProcessJoinDecision() - Accept or reject join
```

### 5. Join Result Processing (0x006e1f00 - 0x006e2500)

```
SrvJoinGame() Result Processing
    ├─> JoinSuccessPath()
    │   ├─> LogSuccess() - "JOIN 5 %d clientid, SrvJoinAct: Added to game."
    │   ├─> SrvJoinAct() [0x006e0838] - Complete client addition
    │   │   ├─> FindClientSlot() - Find client position
    │   │   ├─> LoadCharacterData() - Load player character
    │   │   ├─-> SyncGameState() - Synchronize game state
    │   │   └─> NotifyClientReady() - Notify client ready
    │   │
    │   └─> StartGameSync() - Begin real-time sync
    │       ├─-> SendInitialGameState() - Send full game state
    │       ├─-> SetupUpdateInterval() - Set update timer
    │       └─-> EnableGameInput() - Enable client controls
    │
    └─> JoinFailurePath()
        ├─> LogJoinError() - Error logging
        │   ├─> "SrvJoinGame: ERROR adding Client to game - Game(%d), ClientID(%d), Player(%s)"
        │   └─> "SrvJoinGame: ERROR cannot loc game - Game(%d), ClientID(%d), Player(%s)"
        │
        ├─-> SrvJoinAct() [0x006e086c] - Handle failure
        │   ├─> "SrvJoinAct: No game found for client(%d)"
        │   ├─> SendRejection() - Send join rejection
        │   └─-> CleanupClientData() - Remove partial client data
        │
        └─> NotifyJoinFailure() - Notify UI of failure
            ├─-> UpdateJoinUI() - Update join screen
            ├─-> ShowErrorDialog() - Display error message
            └─-> EnableJoinRetry() - Allow retry attempt
```

### 6. Real-time Game Synchronization (0x006e2500 - 0x006e2f00)

```
GameSyncLoop()
    ├─> ReceiveClientUpdates() - Process client input
    │   ├─> QSrvProcessMsg() - Process incoming messages
    │   ├─-> ValidateMessage() - Check message validity
    │   ├─-> ApplyGameStateChanges() - Update game state
    │   └─-> BroadcastToClients() - Send updates to all clients
    │
    ├─> UpdateGameLogic() - Game state processing
    │   ├─-> ProcessPlayerActions() - Handle player inputs
    │   ├─-> UpdateMonsterAI() - Update monster positions
    │   ├─-> CalculateCollisions() - Handle collision detection
    │   └─-> ApplyDamageEffects() - Process damage and effects
    │
    └─-> SendGameStateUpdates() - Broadcast game state
        ├─-> CompressGameState() - Compress state data
        ├─-> SendToAllClients() - Send to all connected clients
        └─-> HandleDeliveryErrors() - Handle packet loss
```

### 7. Connection Management (0x006e2f00 - 0x006e3500)

```
ConnectionManagement()
    ├─> MonitorConnectionHealth() - Check connection status
    │   ├─-> CheckSocketTimeout() - Monitor WSAETIMEDOUT
    │   ├─-> DetectPacketLoss() - Monitor packet loss
    │   └─-> MeasureLatency() - Measure network latency
    │
    ├─> HandleConnectionIssues() - Address problems
    │   ├─-> QSrvCloseRemoteConnection() [0x006fc654] - Close connection
    │   │   ├─-> "*** closesocket() err = %d ***"
    │   │   ├─-> CleanupClientResources() - Free client memory
    │   │   └─-> UpdateClientList() - Remove from active list
    │   │
    │   └─-> AttemptReconnection() - Try to restore connection
    │       ├─-> ReconnectSocket() - Recreate socket
    │       ├─-> ResyncGameState() - Resync game state
    │       └─-> NotifyRecovery() - Notify game system
    │
    └─-> SecurityMonitoring() - Anti-cheating measures
        ├─-> PACKET_INVALID detection - "User at %s banned due to PACKET_INVALID in QSrvProcessMsg"
        ├─-> sQSSpamCheck detection - "User at %s banned due to sQSSpamCheck in QSrvProcessMsg"
        └─-> HACKLIST logging - Ban system logging
```

## Critical Data Structures

### TCP/IP Game State
```cpp
struct TCP_GAME_STATE {
    int nGameID;                    // Unique game identifier
    int nHostPlayerID;              // Host player ID
    int nMaxPlayers;                // Maximum players (8)
    int nCurrentPlayers;            // Current player count
    char szGameName[32];            // Game name/description
    char szGamePassword[16];        // Game password (optional)
    BOOL bGameInProgress;           // Game active flag
    DWORD dwGameStartTime;          // Game creation timestamp
    SOCKET hGameSocket;             // Main game socket
    SOCKET ClientSockets[8];        // Player socket array
};
```

### Client Connection Structure
```cpp
struct CLIENT_CONNECTION {
    int nClientID;                 // Unique client ID
    char szPlayerName[32];         // Player character name
    SOCKET hClientSocket;          // Client socket handle
    BOOL bAuthenticated;           // Authentication status
    BOOL bReadyForGame;            // Ready to play flag
    DWORD dwLastActivity;           // Last activity timestamp
    DWORD dwJoinTime;              // Connection join time
    int nPingMs;                  // Network latency
    BYTE abyGameState[2048];       // Client game state buffer
};
```

### Server Thread Structure
```cpp
struct SERVER_THREAD {
    HANDLE hThreadHandle;          // Thread handle
    DWORD dwThreadID;             // Thread ID
    BOOL bThreadActive;            // Thread active flag
    SOCKET hListenSocket;          // Listening socket
    int nListenPort;              // Listening port number
    DWORD dwLastActivity;          // Last server activity
    int nActiveClients;            // Connected client count
    CLIENT_CONNECTION Clients[8];  // Client connections array
};
```

## Key Functions to Analyze

### Core TCP Functions
- `TCP_Initialize()` @ 0x006e0000 - TCP/IP system initialization
- `CreateGameSocket()` @ 0x006cc918 - Socket creation
- `ServerThreadInit()` @ 0x006d5d2c - Server thread management
- `GameDiscoveryInit()` @ 0x006def98 - Server discovery

### Server Management Functions
- `SrvJoinGame()` @ 0x006e0790 - Game join processing
- `SrvJoinAct()` @ 0x006e0838 - Client addition to game
- `QSrvAddClient()` @ 0x006fc574 - Client management
- `QSrvCloseRemoteConnection()` @ 0x006fc654 - Connection cleanup

### Game Management Functions
- `HostGameRequest()` @ 0x006e0500 - Host game initialization
- `JoinGameRequest()` @ 0x006e1500 - Join game request
- `GameSyncLoop()` @ 0x006e2500 - Real-time game synchronization
- `ConnectionManagement()` @ 0x006e2f00 - Connection health monitoring

## D2VP Integration Points

### What D2VP Needs to Override
1. **Socket Management** - Replace WinSock with custom networking
2. **Game Discovery** - Implement custom game listing system
3. **State Synchronization** - Replace TCP sync with custom protocol
4. **Connection Handling** - Custom connection management logic

### Potential D2VP Extensions
1. **Custom Protocol** - Use UDP instead of TCP for better performance
2. **P2P Architecture** - Direct peer-to-peer connections without server
3. **Anti-Cheating** - Enhanced security beyond basic PACKET_INVALID checks
4. **Lag Compensation** - Advanced lag handling for smoother gameplay

## Analysis Commands

```bash
# Initialize R2 analysis
cd /home/vtstech/workspace/D2-Game.exe-1.14d-Retail
export LD_LIBRARY_PATH="/home/vtstech/r2-portable/lib:$LD_LIBRARY_PATH"
export R2_USER_BASE="/tmp/r2-data"
/home/vtstech/r2-portable/bin/radare2 -e bin.relocs.apply=true -e bin.cache=true Game.exe

# Find TCP/IP related strings
izz~

# Search for server functions
izz~Srv
izz~QSrv
izz~Join

# Analyze specific functions
s 0x006e0790  # SrvJoinGame
pdf
axf          # Call tree

s 0x006e0838  # SrvJoinAct
pdf
axf

s 0x006fc574  # QSrvAddClient
pdf
axf

# Find socket operations
izz~socket
izz~WSA
izz~bind

# Find game discovery
izz~GetServersList
izz~discover
```

## Related Documentation

- `R2_Game.exe_Analysis_Guide.md` - R2 usage guide
- `D2VP_GameExe_Function_Mapping.md` - Function mapping reference
- `TCP_IP_Game_Hosting_and_Joining_Analysis.md` - High-level analysis
- `R2_Project_Setup_Guide.md` - Setup instructions

## Notes

- TCP/IP system uses traditional WinSock API - similar to D2VP networking
- Server architecture is client-server with host managing game state
- Error handling is comprehensive with detailed logging
- Anti-cheating system monitors for invalid packets and spam
- Connection management includes timeout detection and automatic cleanup
- Game discovery system queries server lists for available games
- Real-time synchronization requires TCP for reliable packet delivery
- Maximum 8 players per game with individual socket connections

## Version History

- **v1.0** (2026-11-19): Initial TCP/IP call flow documentation based on Game.exe 1.14d R2 analysis