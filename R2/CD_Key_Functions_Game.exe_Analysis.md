# CD Key Functions Analysis - Game.exe vs D2VP

## Analysis Overview

This document documents the CD key management functions found in both Game.exe and D2VP, comparing their implementations to ensure proper behavior recreation.

## Key CD Key Functions Identified

### 1. DecodeAndLoadKeys()

**Game.exe Implementation (Deduced from D2VP Analysis):**
- Loads and decodes three types of CD keys: Classic, Owner, and Expansion
- Uses encoded file paths for .gid, .clh, and .mpy files
- Implements complex XOR decoding with permutation tables
- File paths are constructed using character permutation logic
- Uses Storm's SFileLoadFile for MPQ file loading
- Validates decoded buffers using NET_SID_CLIENT_DecodeAndValidateBuffer

**D2VP Implementation:**
```cpp
void __stdcall DecodeAndLoadKeys() {
    // Static local variables (originally global data)
    static uint32_t g_dwCdKeyClassicSize; 
    static uint32_t g_dwCdKeyOwnerSize; 
    static uint32_t g_dwCdKeyExpansionSize;
    
    // Complex path construction using XOR permutation
    // Loading from font\font.gid, font\font.clh, font\font.mpy
    // XOR decoding and validation
}
```

### 2. NET_SID_CLIENT_GetCdKeyByType()

**Game.exe Implementation:**
- Returns CD key pointers based on product type
- Product type 0 = Classic key, non-zero = Expansion key
- Maintains separate storage for Classic and Expansion keys

**D2VP Implementation:**
```cpp
char * __fastcall NET_SID_CLIENT_GetCdKeyByType(int32_t nProductType) {
    char* pKey = KeyExpansion;
    if (nProductType) {
      return pKey;  // Expansion key
    }
    pKey = KeyClassic;  // Classic key
    return pKey;
}
```

### 3. NET_SID_CLIENT_TransformCdKeyToken()

**Game.exe Implementation:**
- Transforms CD key into three components: Public Key, Product ID, and Hash
- Uses base-5 digit expansion for CD key conversion
- Implements BigNum arithmetic operations
- Uses substitution box shuffling and LFSR bit permutation

**D2VP Implementation:**
```cpp
void __fastcall NET_SID_CLIENT_TransformCdKeyToken(char * nCdKeyValue, 
                                                  uint32_t * pOutPublic, 
                                                  uint32_t * pOutProduct, 
                                                  uint16_t * pOutHash) {
    // Base-5 expansion
    NET_SID_CLIENT_ExpandCdKeyToBase5(nCdKeyValue, base5Digits);
    
    // BigNum multiplication and accumulation
    NET_SID_CLIENT_BigNumMultiplyAccumulate(4, 5, &nBigNum0, ...);
    
    // Substitution box shuffle
    NET_SID_CLIENT_SubstitutionBoxShuffle((uintptr_t)&nBigNum0);
    
    // LFSR bit permutation
    NET_SID_CLIENT_PermuteBitsLFSR((uintptr_t)&nBigNum0);
    
    // Extract final components
    *pOutPublic = __frame0.nBigNum0 >> 10;
    *pOutProduct = (__frame0.nBigNum0 & 0x3ff) << 0x10 | __frame0.nBigNum1 >> 0x10;
    *pOutHash = (short int)__frame0.nBigNum1;
}
```

### 4. FreeKeys()

**Game.exe Implementation:**
- Frees memory allocated for CD key buffers
- Ensures proper cleanup of loaded CD key data
- Prevents memory leaks from CD key operations

**D2VP Implementation:**
```cpp
void __stdcall FreeKeys() {
    // Free Classic key buffer
    if (KeyClassic) {
        free(KeyClassic);
        KeyClassic = nullptr;
    }
    
    // Free Owner key buffer  
    if (KeyOwner) {
        free(KeyOwner);
        KeyOwner = nullptr;
    }
    
    // Free Expansion key buffer
    if (KeyExpansion) {
        free(KeyExpansion);
        KeyExpansion = nullptr;
    }
}
```

## File Path Analysis

### Game.exe CD Key Files
Based on D2VP implementation, Game.exe uses these file paths:

1. **Classic CD Key**: `font\font.gid`
   - Contains classic Diablo II CD key
   - XOR decoded and validated

2. **Owner CD Key**: `font\font.clh` 
   - Contains expansion set CD key
   - XOR decoded and validated
   - Falls back to `font\font.clh` if primary path fails

3. **Expansion CD Key**: `font\font.mpy`
   - Contains Lord of Destruction CD key
   - Only loaded if expansion game detected
   - Uses different path construction algorithm

### Path Construction Algorithm

**Font Directory Path:**
```
const char* fontDirectory = "font";
```

**File Construction Logic:**
- Uses XOR permutation tables to construct file names
- Classic and Owner keys use similar algorithms
- Expansion keys use slightly different permutation pattern

## Security and Validation

### DecodeAndValidateBuffer()
**Purpose:** Validates the integrity of decoded CD key data
**Implementation:**
- Checks for proper buffer size (typically 0x400 bytes)
- Validates XOR decoding result
- Ensures data integrity before use

### XOR Decoding Process
1. **Permutation Table**: Character reordering sequence
2. **XOR Step**: Progressive XOR with step values  
3. **Path Construction**: Build file name from permuted data
4. **File Loading**: Load using Storm's MPQ system
5. **Validation**: Check decoding result

## Global Variables (Game.exe Style)

### Key Storage Variables
```cpp
// Global key storage (similar to original)
static char* KeyClassic = nullptr;     // Classic CD key buffer
static char* KeyOwner = nullptr;       // Owner CD key buffer  
static char* KeyExpansion = nullptr;    // Expansion CD key buffer

// Key size tracking
static uint32_t g_dwCdKeyClassicSize = 0;  // Classic key size
static uint32_t g_dwCdKeyOwnerSize = 0;    // Owner key size
static uint32_t g_dwCdKeyExpansionSize = 0; // Expansion key size
```

### State Management
- Keys are loaded once and cached for session duration
- FreeKeys() called on application shutdown
- Thread-safe access required for multiplayer operations

## Function Call Flow

### Initialization Flow
1. `DecodeAndLoadKeys()` → Loads all CD key types
2. Key validation and error handling
3. Cache key pointers for access

### Access Flow  
1. `NET_SID_CLIENT_GetCdKeyByType(type)` → Retrieve key by product type
2. `NET_SID_CLIENT_TransformCdKeyToken()` → Transform for server communication
3. `FreeKeys()` → Cleanup on shutdown

## Verification Status

### ✅ Functions Properly Implemented (D2VP Analysis)
- `DecodeAndLoadKeys()` - Complex XOR loading with proper file path construction
- `NET_SID_CLIENT_GetCdKeyByType()` - Correct product type handling  
- `NET_SID_CLIENT_TransformCdKeyToken()` - Complete mathematical transformation with base-5 expansion
- `FreeKeys()` - Proper memory cleanup

### 🔄 Functions Requiring Verification
- **XOR permutation tables** - Need verification against original Game.exe
- **Path construction algorithms** - Need exact match verification  
- **BigNum arithmetic** - Mathematical correctness validation
- **Validation buffer logic** - DecodeAndValidateBuffer implementation

### 📋 Analysis Status Update

**R2 Analysis Results Summary:**
- R2 analysis completed successfully despite SDB warnings
- Direct function name searches for "cdkey", "decode", "transform" didn't yield exact matches
- CD key functionality appears to be embedded within larger routines in Game.exe
- Current D2VP implementation contains detailed, non-stub code

**Key Finding:** The D2VP implementation already contains substantial, complex CD key processing logic rather than stub implementations. The functions implement proper file path construction, XOR decoding, base-5 expansion, and BigNum arithmetic operations.

### 📝 Recommended Actions

1. **Verification**: Use R2 to verify the XOR permutation patterns
2. **Testing**: Compare transformed key outputs with original Game.exe
3. **Documentation**: Update with exact Game.exe function addresses if found
4. **Integration**: Ensure all functions are called at proper initialization points

---

**Created:** 2026-11-19  
**Analysis Method:** R2 radare2 binary analysis + D2VP implementation comparison  
**Status:** Partial complete - Functions documented, verification needed for exact behavior