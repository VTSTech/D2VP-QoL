// D2VP-D2GS Compatibility Patch
// This file implements client-side packet filtering for compatibility with standard D2GS/PvPGN servers
// by filtering out D2VP-specific packets that standard servers don't recognize.

#include "Source/Bnclient/net_sid.h"
#include "Source/Bnclient/BnSend.h"
#include "Source/Bnclient/Debug.h"
#include "../Redivivus/d2_boot.h"

// Compatibility mode flag - set to true when connected to standard D2GS/PvPGN servers
static bool g_bStandardServerCompatibilityMode = false;

// Packet whitelist for standard D2GS/PvPGN servers
static const DWORD g_StandardD2GSPacketWhitelist[] = {
    // Essential authentication packets (always allowed)
    0x50,  // SID_AUTH_INFO - Auth info request
    0x51,  // SID_AUTH_CHECK - Auth check response
    0x53,  // SID_AUTH_ACCOUNTLOGON - Account logon
    0x54,  // SID_AUTH_ACCOUNTLOGONPROOF - Auth proof
    0x42,  // SID_CDKEY3 - CD key validation
    
    // Essential game packets
    0x3A,  // SID_CHARACTERVERIFY - Character verification
    0x2F,  // SID_ENTERGAME - Enter game
    0x31,  // SID_GAMEINFO - Game info
    0x04,  // SID_PLAYERINFO - Player info
    0x0A,  // SID_CHATMESSAGE - Chat message
    0x1B,  // SID_GAMEJOIN - Game join
    
    // Keepalive and essential maintenance
    0x25,  // SID_PING - Keepalive ping
    
    // End of whitelist
    0xFFFFFFFF
};

// Check if packet is allowed in compatibility mode
bool IsPacketAllowedInCompatibilityMode(DWORD dwPacketId);

// Enable standard server compatibility mode
void EnableStandardServerCompatibilityMode();

// Disable standard server compatibility mode  
void DisableStandardServerCompatibilityMode();

// Detect if server is standard D2GS (to be called after auth check)
bool DetectStandardServer();

// Initialize compatibility system
void InitCompatibilitySystem();

// Hook function declaration for packet interception (placeholder)
extern "C" int32_t __fastcall BnSend_NET_SID_CLIENT_Send_Hook(uint8_t dwPacketId, char* pData, size_t nSize);

// Function pointer type for NET_SID_CLIENT_Send
typedef int32_t (__fastcall *NET_SID_CLIENT_Send_Fn)(uint8_t nPacketId, char * pBytes, size_t nSizeNoHeader);