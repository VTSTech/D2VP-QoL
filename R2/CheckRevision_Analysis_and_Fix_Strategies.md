# CheckRevision Analysis and D2VP.exe CRC Fix Strategies

## Overview

This document analyzes the CheckRevision validation process that causes D2VP.exe connection failures with D2GS/PvPGN servers, and provides comprehensive strategies to bypass or fix the CRC validation issues.

## Problem Analysis

### Root Cause: CheckRevision Function

From the code analysis, the CheckRevision validation occurs in `BnDownload::BNDOWNLOAD_PerformCheckRevision()` in `Source/Bnclient/BnDownload.cpp`:

```cpp
// Lines 194-263 in BnDownload.cpp
int32_t __fastcall BNDOWNLOAD_PerformCheckRevision(uint32_t * pdwVersionOut, uint32_t * pdwChecksumOut, uint8_t * pbResultOut) {
    // ... initialization code ...
    
    // Load CheckRevision DLL
    HMODULE hCheckRevDll = LoadLibraryA(__frame0.szDllPath);
    if (hCheckRevDll) {
        pfnCheckRevision pCheckRevision = (pfnCheckRevision)GetProcAddress(hCheckRevDll, "CheckRevision");
        if (pCheckRevision) {
            // Get current game executable path
            hGameModule = GetModuleHandleA((LPCSTR)0x0);
            GetModuleFileNameA(hGameModule, szGameExePathPtr, dwStartTick);
            
            // Execute CheckRevision function
            __frame0.nCheckResult = (*pCheckRevision)(__frame0.szGameExePath, __frame0.szArchivePath, __frame0.szVersionString, gszBNetChatInputBuffer, (DWORD*)__frame0.pdwVersionOutSaved, (DWORD*)__frame0.pdwChecksumOutSaved, pbResultOut);
        }
    }
}
```

### Key Issues

1. **Dynamic CRC Calculation**: D2VP.exe's CRC changes with rapid development builds
2. **Strict Validation**: D2GS expects specific CRC values that match original Game.exe
3. **Critical Path**: CheckRevision must succeed for authentication to continue
4. **No Bypass**: Current implementation doesn't provide a way to bypass validation

## CheckRevision Function Details

### Function Signature
```cpp
typedef int (__stdcall *pfnCheckRevision)(LPCSTR szFile1, LPCSTR szFile2, LPCSTR szFile3, LPCSTR szValueString, DWORD * pdwVersion, DWORD * pdwChecksum, uint8_t * pbStatstring);
```

### Parameters
- `szFile1`: Game executable path (D2VP.exe)
- `szFile2`: Archive path (typically empty)
- `szFile3`: Version string path (typically empty)  
- `szValueString`: Value string from buffer
- `pdwVersion`: Output for version number
- `pdwChecksum`: Output for CRC checksum
- `pbStatstring`: Output for status string

### Execution Flow
1. Load CheckRevision.dll
2. Get CheckRevision function pointer
3. Get current D2VP.exe path
4. Call CheckRevision function with parameters
5. Return result (0 = success, non-zero = failure)

## CRC Fix Strategies

### Strategy 1: Direct Function Hooking (Recommended)

#### Implementation
Create a custom CheckRevision function that returns expected CRC values:

```cpp
// In Source/Bnclient/BnDownload.cpp - Replace CheckRevision function call

int32_t __fastcall BNDOWNLOAD_PerformCheckRevision(uint32_t * pdwVersionOut, uint32_t * pdwChecksumOut, uint8_t * pbResultOut) {
    // Hook implementation that bypasses original CheckRevision
    if (pdwVersionOut) {
        *pdwVersionOut = 0x00010064;  // Diablo II 1.14d version
    }
    if (pdwChecksumOut) {
        *pdwChecksumOut = 0x00000000;  // Fake CRC (or calculate from modified D2VP.exe)
    }
    if (pbResultOut) {
        *pbResultOut = 0;  // Success
    }
    
    // Log for debugging
    D2BOOT_MarkFmt("CheckRevision bypassed - using fake CRC values");
    
    return 0;  // Success
}
```

#### Implementation Steps

1. **Modify BnDownload.cpp**:
```cpp
// Around line 263, replace the CheckRevision call with:
if (pCheckRevision) {
    // Use our custom CheckRevision bypass instead of real one
    __frame0.nCheckResult = BNDOWNLOAD_PerformCheckRevision_Custom(__frame0.szGameExePath, __frame0.szArchivePath, __frame0.szVersionString, gszBNetChatInputBuffer, (DWORD*)__frame0.pdwVersionOutSaved, (DWORD*)__frame0.pdwChecksumOutSaved, pbResultOut);
} else {
    // Fallback to fake values if CheckRevision function not found
    __frame0.nCheckResult = 0;  // Success
    *pdwVersionOutSaved = 0x00010064;
    *pdwChecksumOutSaved = 0x00000000;
    if (pbResultOut) *pbResultOut = 0;
}
```

2. **Create Custom Function**:
```cpp
int32_t __fastcall BNDOWNLOAD_PerformCheckRevision_Custom(LPCSTR szFile1, LPCSTR szFile2, LPCSTR szFile3, LPCSTR szValueString, DWORD * pdwVersion, DWORD * pdwChecksum, uint8_t * pbStatstring) {
    if (pdwVersion) *pdwVersion = 0x00010064;  // 1.14d version
    if (pdwChecksum) *pdwChecksum = 0x00000000;  // Fake CRC
    if (pbStatstring) *pbStatstring = 0;  // Success
    
    D2BOOT_MarkFmt("CheckRevision bypassed - using standard Game.exe CRC values");
    
    return 0;  // Success
}
```

### Strategy 2: DLL Injection and Hooking

#### Implementation
Create a custom CheckRevision.dll that intercepts the function call:

```cpp
// CustomCheckRevision.cpp
#include <windows.h>
#include <stdio.h>

// Hook function that replaces original CheckRevision
int __stdcall CheckRevision_Hook(LPCSTR szFile1, LPCSTR szFile2, LPCSTR szFile3, LPCSTR szValueString, DWORD * pdwVersion, DWORD * pdwChecksum, uint8_t * pbStatstring) {
    // Log the call
    FILE *log = fopen("CheckRevision.log", "a");
    if (log) {
        fprintf(log, "CheckRevision called with: %s\n", szFile1);
        fclose(log);
    }
    
    // Return fake values
    if (pdwVersion) *pdwVersion = 0x00010064;
    if (pdwChecksum) *pdwChecksum = 0x00000000;
    if (pbStatstring) *pbStatstring = 0;
    
    return 0;  // Success
}

// Export the hooked function
extern "C" __declspec(dllexport) int __stdcall CheckRevision(LPCSTR szFile1, LPCSTR szFile2, LPCSTR szFile3, LPCSTR szValueString, DWORD * pdwVersion, DWORD * pdwChecksum, uint8_t * pbStatstring) {
    return CheckRevision_Hook(szFile1, szFile2, szFile3, szValueString, pdwVersion, pdwChecksum, pbStatstring);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
```

#### Build and Deploy
1. Compile the custom DLL: `g++ -shared -o CustomCheckRevision.dll CustomCheckRevision.cpp`
2. Place in D2VP.exe directory or system path
3. Modify BnDownload.cpp to load the custom DLL

### Strategy 3: Memory Patching

#### Implementation
Patch the CheckRevision function call at runtime:

```cpp
// In BnDownload.cpp - Add memory patching
void PatchCheckRevisionCall() {
    // Find the CheckRevision function call in BnDownload.cpp
    // This is a more advanced technique that requires understanding the assembly
    
    // Example of patching the call instruction
    DWORD oldProtect;
    VirtualProtect((void*)0x00401234, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    
    // JMP to our custom function
    *(BYTE*)0x00401234 = 0xE9;  // JMP opcode
    *(DWORD*)((BYTE*)0x00401234 + 1) = (DWORD)BNDOWNLOAD_PerformCheckRevision_Custom - ((DWORD)0x00401234 + 5);
    
    VirtualProtect((void*)0x00401234, 5, oldProtect, &oldProtect);
}
```

### Strategy 4: CheckRevision.dll Creation

#### Implementation
Create a minimal CheckRevision.dll that satisfies the D2VP requirements:

```cpp
// MiniCheckRevision.cpp
#include <windows.h>

int __stdcall CheckRevision(LPCSTR szFile1, LPCSTR szFile2, LPCSTR szFile3, LPCSTR szValueString, DWORD * pdwVersion, DWORD * pdwChecksum, uint8_t * pbStatstring) {
    // Calculate real CRC if needed, or use fixed values
    if (pdwVersion) *pdwVersion = 0x00010064;  // 1.14d version
    if (pdwChecksum) *pdwChecksum = 0x00000000;  // Will be calculated from original Game.exe
    if (pbStatstring) *pbStatstring = 0;
    
    return 0;  // Success
}

extern "C" __declspec(dllexport) int __stdcall DllMain(HINSTANCE hInstance, DWORD reason, LPVOID reserved) {
    return 1;
}
```

## Implementation Recommendations

### Phase 1: Quick Fix (Strategy 1)
1. **Priority**: High
2. **Effort**: Low
3. **Risk**: Low
4. **Implementation**: Direct function hooking in BnDownload.cpp
5. **Testing**: Build and test with D2GS connection

### Phase 2: Robust Solution (Strategy 2)
1. **Priority**: Medium
2. **Effort**: Medium  
3. **Risk**: Medium
4. **Implementation**: Custom CheckRevision.dll
5. **Testing**: Comprehensive testing with different D2GS versions

### Phase 3: Advanced Solution (Strategy 3)
1. **Priority**: Low
2. **Effort**: High
3. **Risk**: High
4. **Implementation**: Runtime memory patching
5. **Testing**: Extensive testing required

## Testing Procedure

### Test Environment Setup
1. Use existing D2VP-QoL build
2. Connect to D2GS server
3. Monitor authentication process

### Test Cases
1. **Basic Connection**: Verify D2VP.exe can connect to D2GS
2. **Authentication**: Verify authentication completes successfully
3. **Character List**: Verify character list loads
4. **Game Creation**: Verify game creation works
5. **Multiplayer**: Verify multiplayer functionality

### Debug Output
Monitor for:
- "CheckRevision bypassed" messages
- Authentication success/failure
- Any CRC-related errors
- Connection stability

## Configuration Options

### Registry Settings
```ini
[CheckRevision Bypass]
EnableBypass=1
UseFakeCRC=1
LogCheckRevision=1
CRCValue=0x00000000
VersionValue=0x00010064
```

### Environment Variables
```
D2VP_CRC_BYPASS=1
D2VP_FAKE_CRC=1
D2VP_LOG_CHECKREVISION=1
```

## Error Scenarios and Solutions

### Scenario 1: CheckRevision Function Not Found
- **Solution**: Implement fallback with fake values
- **Implementation**: Add try-catch around CheckRevision loading

### Scenario 2: CRC Validation Still Fails
- **Solution**: Enhance bypass to include more sophisticated CRC calculation
- **Implementation**: Calculate CRC from original Game.exe file

### Scenario 3: Authentication Fails After Bypass
- **Solution**: Check for other validation points that may need similar treatment
- **Implementation**: Add debugging to identify additional validation steps

## Performance Considerations

### Memory Usage
- Custom DLL approach: Additional ~50KB memory
- Function hooking: Minimal memory impact
- Memory patching: Minimal memory impact

### CPU Overhead
- Custom DLL: Minimal CPU overhead
- Function hooking: Negligible CPU overhead  
- Memory patching: Negligible CPU overhead

## Maintenance Considerations

### Future Updates
- Monitor D2VP CheckRevision changes
- Update bypass implementation as needed
- Keep CRC values consistent with official releases

### Documentation
- Maintain detailed change logs
- Document CRC values used
- Update testing procedures

## Conclusion

The CheckRevision validation is the primary barrier preventing D2VP.exe from connecting to D2GS/PvPGN servers. By implementing one of the bypass strategies outlined above, particularly the direct function hooking approach (Strategy 1), you can successfully resolve the CRC validation issue and enable D2VP.exe to work with standard D2GS servers.

The recommended approach is to start with Strategy 1 for a quick fix, then move to Strategy 2 for a more robust solution. This ensures immediate resolution while providing a foundation for future enhancements.

---
**Created**: 2026-11-19  
**Analysis Method**: Code review and reverse engineering  
**Priority**: High - Critical for D2VP-D2GS compatibility  
**Next Steps**: Implement Strategy 1 (Direct Function Hooking)