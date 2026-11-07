// vanilla-plus d2boot bridge - the tenth sync's per-module gates made
// SOUND_GetEAXEnabledFlag module-private to D2Sound; Game's boot verdict mark
// is the one cross-module caller. This TU is claimed by the D2Sound module
// (Source/D2Sound/), so the private declaration is visible here and only
// here; the wrapper below is the fork's public surface for it.
// R137: Main.cpp declares the wrapper locally instead of including this
// header - the module gates charge a non-public D2Sound header to the
// including module and Game is strict (the R136 build was the first to
// reach the module-gates edge).
#pragma once
DWORD __stdcall VPSOUND_GetEAXEnabledFlag();
