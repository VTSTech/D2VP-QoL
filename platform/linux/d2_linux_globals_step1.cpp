// platform/linux/d2_linux_globals_step1.cpp — [d2linux M1, STEP-1 ONLY]
//
// Weak stand-ins for the handful of globals/globals.cpp definitions the M1
// foundation slice externs. globals.cpp (the 39K-line central-data TU) embeds
// the game's command dispatch tables, whose closure is the whole client — it
// joins at step 2 (build.cmake's slice ledger), and its STRONG definitions
// then take over from these weak ones automatically. Nothing here changes
// behavior that step 2 would redo:
//   - ExceptionList: the tree's global SEH list head (the PE links its
//     frames onto it and never dispatches).
//   - the s_* constants: the decomp's literal error texts (verbatim content
//     from globals.cpp).
//   - the bit-mask/zlib tables: the canonical table contents globals.cpp
//     also carries (1<<i, ~(1<<i), the standard zlib length-code bits).
//   - MISC_OperatorDeleteShim: routes to free; step 2's strong body routes
//     into the Fog pools instead — the weak link flips with no call-site
//     change.
//   - __GSHandlerCheck_EH_6c9f58 / the language probes: stored-not-called or
//     default-answer, the same never-dispatched contract support/
//     d2_crt_impl.cpp documents for the CRT_ExceptionFilter family.
//
// PE safety: compiles only in the linux build (build.cmake).

#include <cstdint>
#include <cstdlib>

#include "platform/linux/d2_linux_compat.h"
#include "Fog/Engine/Application.h"
#include "Fog/Fog.h"  // [d2linux M2 round 3] D2FogArchiveStrc (the MPQ slot weaks)

extern "C" {

// globals.cpp:38051 — the SEH list head the crash path stores onto.
__attribute__((weak)) void* ExceptionList = (void*)0x00000000;

// globals.cpp:38039 — the fault path's thread self-identity.
__attribute__((weak)) uint32_t dwCurrentThreadId = 0;

// globals.cpp:373/409 — the bit-mask tables (BitMask / BitMaskOr).
__attribute__((weak)) uint32_t gadwBitMasks[32] = {
    0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
    0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000,
    0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x400000, 0x800000,
    0x1000000, 0x2000000, 0x4000000, 0x8000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000
};
__attribute__((weak)) uint32_t gadwInvBitMasks[32] = {
    0xfffffffe, 0xfffffffd, 0xfffffffb, 0xfffffff7, 0xffffffef, 0xffffffdf, 0xffffffbf, 0xffffff7f,
    0xfffffeff, 0xfffffdff, 0xfffffbff, 0xfffff7ff, 0xffffefff, 0xffffdfff, 0xffffbfff, 0xffff7fff,
    0xfffeffff, 0xfffdffff, 0xfffbffff, 0xfff7ffff, 0xffefffff, 0xffdfffff, 0xffbfffff, 0xff7fffff,
    0xfeffffff, 0xfdffffff, 0xfbffffff, 0xf7ffffff, 0xefffffff, 0xdfffffff, 0xbfffffff, 0x7fffffff
};

// globals.cpp:482+ — zlib's length-code tables (the standard RFC 1951 shapes
// the vendored zlib cpp set references).
__attribute__((weak)) int gaZlibExtraBlBits[19] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 7
};

// globals.cpp:1975 — UTF-8 lead-byte marks.
__attribute__((weak)) int gaUTF8LeadByteMarks[8] = { 0x0, 0x0, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0x0 };

// zlib's message table (zutil.h's extern; the vendored cpp set references it
// and carries no definition — upstream zlib keeps it in zutil.c).
extern const char* const z_errmsg[10] __attribute__((weak)) = {
    "need dictionary", "stream end", "", "file error", "stream error",
    "data error", "insufficient memory", "buffer error", "incompatible version", "",
};

}  // extern "C"

// The error-text constants the slice externs (globals.cpp's definitions,
// verbatim content). Weak DEFINITIONS, no extern keyword: "extern" + init
// at namespace scope is legal C++ but warns "initialized and declared
// 'extern'" once per string (owner build, round 3); a plain definition has
// the same external linkage, and weak keeps the real globals.cpp symbol
// winning once that TU joins the slice.
#define D2_WEAK_STR(name, value) \
    char name[] __attribute__((weak)) = value

D2_WEAK_STR(s_Unknown_006cc780, "Unknown");
D2_WEAK_STR(s_Do_you_wish_to_terminate_the_appl_006cf1f4, "Do you wish to terminate the application?");
D2_WEAK_STR(s_Press_OK_to_terminate_the_applica_006cf220, "Press OK to terminate the application.");
D2_WEAK_STR(s_ERROR___u__0x_08x__006cf2e0, "ERROR: %u (0x%08x)\n\n");
D2_WEAK_STR(s_Expr___s_006cf250, "Expr: %s");
D2_WEAK_STR(s_File___s_006cf1e8, "File: %s");
D2_WEAK_STR(s_File___s_Line___d__006cf284, "File: %s  Line: %d");
D2_WEAK_STR(s_Function___s__006cf274, "Function: %s");
D2_WEAK_STR(s_Handle___s__006cf25c, "Handle: %s");
D2_WEAK_STR(s_Object___s__006cf268, "Object: %s");
D2_WEAK_STR(s_Program___s__006cf298, "Program: %s");
D2_WEAK_STR(s_This_application_has_encountered__006cf2a8, "This application has encountered a critical error:\n");
D2_WEAK_STR(s__s_006cf248, "%s");
D2_WEAK_STR(s_client_006cc920, "client");
D2_WEAK_STR(s_expand_006cc8f8, "expand");
D2_WEAK_STR(s_launcher_006cc900, "launcher");
D2_WEAK_STR(s_modstate0_006cc928, "modstate0");
D2_WEAK_STR(s_multiplayer_006cc90c, "multiplayer");
D2_WEAK_STR(s_server_006cc918, "server");
D2_WEAK_STR(s_AMD_006cdeb8, "AuthenticAMD");
D2_WEAK_STR(s_Cyrix_006cdeb0, "CyrixInstead");
D2_WEAK_STR(s_Intel_006cdebc, "GenuineIntel");
D2_WEAK_STR(s_PKWARE_Data_Compression_Library_f_0074be60, "PKWARE Data Compression Library for Win32");
D2_WEAK_STR(s_Assertion_Failure_006ccb6c, "Assertion Failure");
D2_WEAK_STR(s_Deadlock_006ccb34, "Deadlock");
D2_WEAK_STR(s_English_006cc81c, "English");
D2_WEAK_STR(s_English_with_Korean_Chat_006cc788, "English (Korean Chat)");
D2_WEAK_STR(s_File_Read_Error_006ccb94, "File Read Error");
D2_WEAK_STR(s_File_Write_Error_006ccb80, "File Write Error");
D2_WEAK_STR(s_French_006cc804, "French");
D2_WEAK_STR(s_German_006cc80c, "German");
D2_WEAK_STR(s_Halt_006ccb64, "Halt");
D2_WEAK_STR(s_Italian_006cc7f0, "Italian");
D2_WEAK_STR(s_Japanese_006cc7e4, "Japanese");
D2_WEAK_STR(s_Korean_006cc7dc, "Korean");
D2_WEAK_STR(s_Missing_Dll_006ccb50, "Missing Dll");
D2_WEAK_STR(s_No_Error_006ccbc8, "No Error");
D2_WEAK_STR(s_Out_of_Memory_Error_006ccba4, "Out of Memory Error");
D2_WEAK_STR(s_Polish_006cc7ac, "Polish");
D2_WEAK_STR(s_Portuguese_006cc7f8, "Portuguese");
D2_WEAK_STR(s_Registry_Error_006ccb40, "Registry Error");
D2_WEAK_STR(s_Russian_006cc7a4, "Russian");
D2_WEAK_STR(s_Simplified_Chinese_006cc7c8, "Simplified Chinese");
D2_WEAK_STR(s_Spanish_006cc814, "Spanish");
D2_WEAK_STR(s_Traditional_Chinese_006cc7b4, "Traditional Chinese");
D2_WEAK_STR(s_Unknown_Error_006ccbb8, "Unknown Error");
D2_WEAK_STR(s_Warning_006ccb5c, "Warning");

namespace Fog { namespace Engine { namespace Application {

// The allocation shim's step-1 body: the plain CRT heap. Step 2's strong
// Application.cpp body routes into the Fog pools instead — the weak link
// flips without any call-site change.
__attribute__((weak)) void MISC_OperatorDeleteShim(void* pMemory) { free(pMemory); }

}}}  // namespace Fog::Engine::Application

namespace D2Lang { namespace StrTable { namespace strtable {

// The language probes SErr/SLog's error reporting calls. Step 1 has no
// language table (strtable.cpp's closure is the Unicode/kamap/MPQ layer —
// step 2's slice); the honest weak bodies answer "default language", which
// is what the PE build answers with an empty language registry value.
__attribute__((weak)) uint32_t MISC_GetLanguageCode() { return 0; }
__attribute__((weak)) uint32_t STRTABLE_GetRegistryLanguage() { return 0; }

}}}  // namespace D2Lang::StrTable::strtable

namespace D2Client { namespace WindowHandle {

// The /GS EH-check thunk SBig.cpp stores by address (globals.h's handler
// table). Never dispatched, like the CRT_ExceptionFilter family — an empty
// body is its whole behavior (support/d2_crt_impl.cpp's documented contract).
__attribute__((weak)) void __GSHandlerCheck_EH_6c9f58(EHExceptionRecord*, EHRegistrationNode*, CONTEXT*, void*) {}

}}  // namespace D2Client::WindowHandle

// ---------------------------------------------------------------------------
// [d2linux M2 round 3] the archive-rung weaks: the globals the D2Hell +
// D2Lang TUs extern that live in globals/globals.cpp (still a step-2 TU) or
// in TUs whose closure has not joined yet. Same rules as above: verbatim
// content, weak linkage so the strong TU wins when it joins.
// ---------------------------------------------------------------------------

D2_WEAK_STR(s_Blizzard_Entertainment__Inc__006deb80, "Blizzard Entertainment, Inc.");
D2_WEAK_STR(s_Diablo_2_EXE_006cc838, "Diablo 2.EXE");
D2_WEAK_STR(s__not_xlated_call_ken_w_00730520, " -not xlated call ken w");
D2_WEAK_STR(s_SIN_006e0264, "SIN");
D2_WEAK_STR(s_HAN_006e0268, "HAN");
D2_WEAK_STR(s_ITA_006e0274, "ITA");
D2_WEAK_STR(s_BRA_006e0278, "BRA");
D2_WEAK_STR(s_POR_006e027c, "POR");
D2_WEAK_STR(s_FRA_006e0280, "FRA");
D2_WEAK_STR(s_DEU_006e0284, "DEU");
// globals.cpp:1987 — the crash-dump register-name header the strtable layer
// reads; the PE renders "ESP" first, same answer here.
D2_WEAK_STR(gszCrashDumpRegisterFormatString, "ESP");

namespace Fog { namespace Engine { namespace Application {

// The allocation pair's step-1 NEW half (globals.cpp / Application.cpp's
// strong body routes into the Fog pools at step 2). This weak half routes
// into the plain CRT heap — the SAME domain the weak MISC_OperatorDeleteShim
// above frees from, so the pair stays consistent until the strong link
// replaces both. NEVER mix: weak new + strong delete (or the reverse) would
// cross free() with SMemAlloc.
__attribute__((weak)) void* MISC_OperatorNewShim(uint32_t nSize) {
    return malloc(nSize);
}

}}}  // namespace Fog::Engine::Application

namespace D2Launch { namespace MainMenus {

// MainMenus.cpp:526 verbatim — UNISYS's STRING_Compare references it; the
// function zero-fills one wchar slot. Its closure (the whole D2Launch UI
// layer) is a later rung, so the body rides here verbatim instead.
__attribute__((weak)) void __fastcall STRING_ZeroOneWCHAR(void* pBuffer) {
    *(uint16_t*)pBuffer = 0;
}

}}  // namespace D2Launch::MainMenus

namespace D2Client { namespace UI { namespace ui {

// globals.cpp:38499-38756 verbatim — the case-fold table UNISYS's
// STRING_Compare maps through (strtable's closure needs it; ui.cpp joins at
// the D2Client rung). Verbatim bytes, weak linkage.
__attribute__((weak)) uint16_t CharacterLookupTable[256] = {
    0x0,
    0x1,
    0x2,
    0x3,
    0x4,
    0x5,
    0x6,
    0x7,
    0x8,
    0x9,
    0xa,
    0xb,
    0xc,
    0xd,
    0xe,
    0xf,
    0x10,
    0x11,
    0x12,
    0x13,
    0x14,
    0x15,
    0x16,
    0x17,
    0x18,
    0x19,
    0x1a,
    0x1b,
    0x1c,
    0x1d,
    0x1e,
    0x1f,
    0x20,
    0x21,
    0x22,
    0x23,
    0x24,
    0x25,
    0x26,
    0x27,
    0x28,
    0x29,
    0x2a,
    0x2b,
    0x2c,
    0x2d,
    0x2e,
    0x2f,
    0x30,
    0x31,
    0x32,
    0x33,
    0x34,
    0x35,
    0x36,
    0x37,
    0x38,
    0x39,
    0x3a,
    0x3b,
    0x3c,
    0x3d,
    0x3e,
    0x3f,
    0x40,
    0x41,
    0x42,
    0x43,
    0x44,
    0x45,
    0x46,
    0x47,
    0x48,
    0x49,
    0x4a,
    0x4b,
    0x4c,
    0x4d,
    0x4e,
    0x4f,
    0x50,
    0x51,
    0x52,
    0x53,
    0x54,
    0x55,
    0x56,
    0x57,
    0x58,
    0x59,
    0x5a,
    0x5b,
    0x5c,
    0x5d,
    0x5e,
    0x5f,
    0x60,
    0x41,
    0x42,
    0x43,
    0x44,
    0x45,
    0x46,
    0x47,
    0x48,
    0x49,
    0x4a,
    0x4b,
    0x4c,
    0x4d,
    0x4e,
    0x4f,
    0x50,
    0x51,
    0x52,
    0x53,
    0x54,
    0x55,
    0x56,
    0x57,
    0x58,
    0x59,
    0x5a,
    0x7b,
    0x7c,
    0x7d,
    0x7e,
    0x7f,
    0x80,
    0x81,
    0x82,
    0x83,
    0x84,
    0x85,
    0x86,
    0x87,
    0x88,
    0x89,
    0x8a,
    0x8b,
    0x8c,
    0x8d,
    0x8e,
    0x8f,
    0x90,
    0x91,
    0x92,
    0x93,
    0x94,
    0x95,
    0x96,
    0x97,
    0x98,
    0x99,
    0x8a,
    0x9b,
    0x8c,
    0x9d,
    0x9e,
    0x9f,
    0xa0,
    0xa1,
    0xa2,
    0xa3,
    0xa4,
    0xa5,
    0xa6,
    0xa7,
    0xa8,
    0xa9,
    0xaa,
    0xab,
    0xac,
    0xad,
    0xae,
    0xaf,
    0xb0,
    0xb1,
    0xb2,
    0xb3,
    0xb4,
    0xb5,
    0xb6,
    0xb7,
    0xb8,
    0xb9,
    0xba,
    0xbb,
    0xbc,
    0xbd,
    0xbe,
    0xbf,
    0xc0,
    0xc1,
    0xc2,
    0xc3,
    0xc4,
    0xc5,
    0xc6,
    0xc7,
    0xc8,
    0xc9,
    0xca,
    0xcb,
    0xcc,
    0xcd,
    0xce,
    0xcf,
    0xd0,
    0xd1,
    0xd2,
    0xd3,
    0xd4,
    0xd5,
    0xd6,
    0xd7,
    0xd8,
    0xd9,
    0xda,
    0xdb,
    0xdc,
    0xdd,
    0xde,
    0xdf,
    0xc0,
    0xc1,
    0xc2,
    0xc3,
    0xc4,
    0xc5,
    0xc6,
    0xc7,
    0xc8,
    0xc9,
    0xca,
    0xcb,
    0xcc,
    0xcd,
    0xce,
    0xcf,
    0xd0,
    0xd1,
    0xd2,
    0xd3,
    0xd4,
    0xd5,
    0xd6,
    0xf7,
    0xd8,
    0xd9,
    0xda,
    0xdb,
    0xdc,
    0xdd,
    0xde,
    0x9f
};

}}}  // namespace D2Client::UI::ui

// ---------------------------------------------------------------------------
// [d2linux M2 round 3, second pass] the link loop's remainder: the strtable
// layer's font-name strings and language-code answers (globals.cpp's
// verbatim content), and the archive-slot pointers (globals/D2Win.cpp's
// verbatim zeros — that TU's UI handler tables are the round-4 closure, so
// its DATA rides here until then).
// ---------------------------------------------------------------------------

D2_WEAK_STR(s_FontExocet8_006dc8f4, "FontExocet8");
D2_WEAK_STR(s_FontInGameChat_006dc8d0, "FontInGameChat");
D2_WEAK_STR(s_FontRidiculous_006dc900, "FontRidiculous");
D2_WEAK_STR(s_Font16_006dc970, "Font16");
D2_WEAK_STR(s_Font24_006dc930, "Font24");
D2_WEAK_STR(s_Font30_006dc968, "Font30");
D2_WEAK_STR(s_Font42_006dc960, "Font42");
D2_WEAK_STR(s_Font6_006dc938, "Font6");
D2_WEAK_STR(s_Font8_006dc978, "Font8");
D2_WEAK_STR(s_FontExocet10_006dc910, "FontExocet10");
D2_WEAK_STR(s_FontFormal10_006dc950, "FontFormal10");
D2_WEAK_STR(s_FontFormal11_006dc920, "FontFormal11");
D2_WEAK_STR(s_FontFormal12_006dc940, "FontFormal12");
D2_WEAK_STR(s_ReallyTheLastSucker_006dc8e0, "ReallyTheLastSucker");

// globals.cpp:517-521 verbatim — the language-code answers strtable's
// MISC_GetLanguageCode reads (the strong bodies above yield to strtable.cpp
// when it joins; these are its own data).
char gaLanguageCodeLookup[4] __attribute__((weak)) = { 'E', 'N', 'U', '\x00' };
char gaLanguageCodeFallback[4] __attribute__((weak)) = { 'C', 'H', 'I', '\x00' };
char gaLanguageCodeAbbrevTable[4] __attribute__((weak)) = { 'E', 'N', 'G', '\x00' };

// globals/D2Win.cpp:1830-1837 verbatim (zero-initialized archive slots;
// file-scope globals there — the strong TU joins with the D2Win rung).
__attribute__((weak)) D2FogArchiveStrc* MPQ_d2data = 0;
__attribute__((weak)) D2FogArchiveStrc* MPQ_patch_d2 = 0;
__attribute__((weak)) D2FogArchiveStrc* MPQ_d2exp = 0;
