// D2VP-D2GS Compatibility Patch Implementation
// Modified packet sending functions with compatibility filtering

#include "Source/Bnclient/compatibility_patch.h"
#include "Source/Bnclient/net_sid.h"
#include "Source/Bnclient/BnSend.h"
#include "Source/Bnclient/Debug.h"
#include "../Redivivus/d2_boot.h"

// Hook implementation for NET_SID_CLIENT_Send
extern "C" int32_t __fastcall BnSend_NET_SID_CLIENT_Send_Hook(uint8_t dwPacketId, char* pData, size_t nSize) {
    if (!IsPacketAllowedInCompatibilityMode(dwPacketId)) {
        // Log blocked packet for debugging
        D2BOOT_MarkFmt("BLOCKED PACKET: 0x%08X (compatibility mode)", (DWORD)dwPacketId);
        
        // Return 0 to block packet sending
        return 0;
    }
    
    // Packet is allowed - proceed with normal sending
    // The actual NET_SID_CLIENT_Send function will be called from the hook in BnSend.cpp
    return 1; // Success (this will be replaced by actual call in BnSend.cpp)
}

// Check if packet is allowed in compatibility mode
bool IsPacketAllowedInCompatibilityMode(DWORD dwPacketId) {
    if (!g_bStandardServerCompatibilityMode) {
        // Compatibility mode disabled - allow all packets
        return true;
    }
    
    // Check if packet is in whitelist
    for (int i = 0; g_StandardD2GSPacketWhitelist[i] != 0xFFFFFFFF; i++) {
        if (g_StandardD2GSPacketWhitelist[i] == dwPacketId) {
            return true;
        }
    }
    
    // Packet not in whitelist - block it in compatibility mode
    return false;
}

// Modified authentication packet sender with compatibility filtering
int __stdcall NET_SID_CLIENT_Send_0x50_AuthInfo_Compatible() {
// Always allow 0x50 (SID_AUTH_INFO) as it's essential
	#if defined(D2_MODULE_D2Launch)
		return Bnclient::NET_SID_CLIENT_Send(0x50, NULL, 0);
	#else
		return Bnclient::BnSend::NET_SID_CLIENT_Send(0x50, NULL, 0);
	#endif
}

// Modified auth check packet sender with compatibility filtering  
int32_t __stdcall NET_SID_CLIENT_Send_0x51_AuthCheck_Compatible() {
// Always allow 0x51 (SID_AUTH_CHECK) as it's essential
	#if defined(D2_MODULE_D2Launch)
		return Bnclient::NET_SID_CLIENT_Send(0x51, NULL, 0);
	#else
		return Bnclient::BnSend::NET_SID_CLIENT_Send(0x51, NULL, 0);
	#endif
}

// Modified account logon packet sender with compatibility filtering
int __fastcall NET_SID_CLIENT_Send_0x53_AuthAccountLogon_Compatible(char * pUsername, char * pPassword) {
// Always allow 0x53 (SID_AUTH_ACCOUNTLOGON) as it's essential
	#if defined(D2_MODULE_D2Launch)
		return Bnclient::NET_SID_CLIENT_Send(0x53, NULL, 0);
	#else
		return Bnclient::BnSend::NET_SID_CLIENT_Send(0x53, NULL, 0);
	#endif
}

// Modified auth proof packet sender with compatibility filtering
int32_t __fastcall NET_SID_CLIENT_Send_0x54_AuthAccountLogonProof_Compatible(void * pSalt, void * pServerPublicKeyB) {
// Always allow 0x54 (SID_AUTH_ACCOUNTLOGONPROOF) as it's essential  
	#if defined(D2_MODULE_D2Launch)
		return Bnclient::NET_SID_CLIENT_Send(0x54, NULL, 0);
	#else
		return Bnclient::BnSend::NET_SID_CLIENT_Send(0x54, NULL, 0);
	#endif
}

// Potentially problematic packets that should be filtered in compatibility mode
// These are D2VP-specific packets that standard D2GS servers don't recognize

// Filter SID_AUTH_ACCOUNTCREATE (0x52) in compatibility mode
int __fastcall NET_SID_CLIENT_Send_0x52_AuthAccountCreate_Compatible(char * pUsername, char * pPassword) {
    if (!IsPacketAllowedInCompatibilityMode(0x52)) {
        D2BOOT_MarkFmt("BLOCKED: SID_AUTH_ACCOUNTCREATE (0x52) - standard D2GS doesn't support account creation");
        return 0; // Block packet
    }
    #if defined(D2_MODULE_D2Launch)
	return Bnclient::NET_SID_CLIENT_Send(0x52, NULL, 0);
#else
	return Bnclient::BnSend::NET_SID_CLIENT_Send(0x52, NULL, 0);
#endif
}

// Filter various 0x55 response packets in compatibility mode
bool HandleAuthResponsePacket_0x55_Compatible(DWORD dwResponseCode) {
    if (!IsPacketAllowedInCompatibilityMode(0x55)) {
        D2BOOT_MarkFmt("BLOCKED: 0x55 response packet - standard D2GS doesn't support extended auth responses");
        return false; // Block packet
    }
    
    // Original handling logic here
    return true;
}

// Main authentication flow wrapper with compatibility detection
void HandleAuthenticationFlowWithCompatibility() {
    // Detect if we're connecting to a standard D2GS server
    if (DetectStandardServer()) {
        EnableStandardServerCompatibilityMode();
    } else {
        DisableStandardServerCompatibilityMode();
    }
    
    // Proceed with authentication using filtered packet senders
    NET_SID_CLIENT_Send_0x50_AuthInfo_Compatible();
    NET_SID_CLIENT_Send_0x51_AuthCheck_Compatible();
    
    // Continue with normal authentication flow...
}

// Post-authentication packet handling with compatibility filtering
void HandlePostAuthenticationPackets_Compatible() {
    if (g_bStandardServerCompatibilityMode) {
        D2BOOT_MarkFmt("Using filtered post-auth packet flow for standard D2GS");
        
        // Send only essential packets that standard servers understand
        // Character list request
        // Game info request  
        // Player info request
        
        // Avoid D2VP-specific initialization packets
    } else {
        D2BOOT_MarkFmt("Using full D2VP packet flow");
        
        // Send full D2VP packet set including QoL features
        // Full post-auth initialization with all D2VP features
    }
}

// Initialize compatibility system
void InitCompatibilitySystem() {
    // Start with compatibility mode disabled by default
    DisableStandardServerCompatibilityMode();
    
    // TODO: Add configuration file reading
    // Could read from registry or config file:
    // StandardServerCompatibility=1
    // LogBlockedPackets=1
    
    D2BOOT_MarkFmt("D2VP-D2GS compatibility system initialized");
}

// Initialize compatibility patch during startup
void InitCompatibilityPatch() {
    InitCompatibilitySystem();
    
    // Force enable compatibility mode for D2GS connections
    EnableStandardServerCompatibilityMode();
    
    // TODO: Hook into packet sending functions
    // This would involve replacing function pointers or
    // using preprocessor directives to redirect calls
    
    D2BOOT_MarkFmt("D2VP-D2GS compatibility patch initialized - FORCED MODE ENABLED");
}

// Cleanup compatibility patch
void CleanupCompatibilityPatch() {
    DisableStandardServerCompatibilityMode();
    
    // TODO: Restore original function pointers if hooked
    
    D2BOOT_MarkFmt("D2VP-D2GS compatibility patch cleaned up");
}

// Implementation of the compatibility check function
namespace Bnclient {
namespace BnSend {

bool IsPacketAllowedInCompatibilityMode(DWORD dwPacketId) {
    if (!g_bStandardServerCompatibilityMode) {
        // Compatibility mode disabled - allow all packets
        return true;
    }
    
    // Check if packet is in whitelist
    for (int i = 0; g_StandardD2GSPacketWhitelist[i] != 0xFFFFFFFF; i++) {
        if (g_StandardD2GSPacketWhitelist[i] == dwPacketId) {
            return true;
        }
    }
    
    // Packet not in whitelist - block it in compatibility mode
    return false;
}

} // namespace BnSend
} // namespace Bnclient

// Enable standard server compatibility mode
void EnableStandardServerCompatibilityMode() {
    g_bStandardServerCompatibilityMode = true;
    D2BOOT_MarkFmt("Standard D2GS compatibility mode ENABLED");
}

// Disable standard server compatibility mode  
void DisableStandardServerCompatibilityMode() {
    g_bStandardServerCompatibilityMode = false;
    D2BOOT_MarkFmt("Standard D2GS compatibility mode DISABLED");
}

// Detect if server is standard D2GS (to be called after auth check)
bool DetectStandardServer() {
    // For now, we'll use a configuration-based approach
    // In a real implementation, this could check server version or response patterns
    
    // TODO: Implement server detection logic
    // Could check:
    // - Server version response
    // - Response to specific packets
    // - Connection characteristics
    
    return false; // Default to compatibility mode until detection is implemented
}