// vanilla-plus d2boot bridge - see VanillaPlusSound.h. Fork-owned TU: compiled
// into the D2Sound module, where SOUND_GetEAXEnabledFlag's module-private
// declaration is visible; the boot verdict in Game/Main.cpp rides this wrapper.
// [eleventh sync] 7b70255 split one public header per module: D2Sound.h is now
// the slim public surface (3 cross-module functions); the private block this
// bridge exists to see moved to the internal D2SoundRoot.h (rename-detected
// from the old single header). Include the internal one - this TU compiles
// inside the D2Sound module, so its gate passes and the private block is
// visible; the public header no longer carries the declaration at all.
#include "Source/D2Sound/VanillaPlusSound.h"
#include "Source/D2Sound/D2SoundRoot.h"

DWORD __stdcall VPSOUND_GetEAXEnabledFlag() {
    return D2Sound::SOUND_GetEAXEnabledFlag();
}
