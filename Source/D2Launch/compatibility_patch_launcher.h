// D2VP-D2GS Compatibility Patch - Launcher Header
// This header provides access to compatibility functions for the D2Launch module

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for Bnclient functions
void InitCompatibilitySystem(void);
void EnableStandardServerCompatibilityMode(void);
void DisableStandardServerCompatibilityMode(void);
bool DetectStandardServer(void);

#ifdef __cplusplus
}
#endif