// platform/linux/d2_linux_compat.h — [d2linux M1] the Linux build's forced
// prelude (platform/linux/build.cmake includes it BEFORE d2_platform.h).
//
// The decomp was recovered against the 32-bit MSVC x86 ABI: game TUs spell
// MSVC calling conventions, MSVC CRT shapes and a Win32 SDK surface that
// mingw's windows.h supplies on the PE line. This file (plus tools/
// vp_host_api.h, force-included after d2_platform.h) supplies both. The
// fork's tools/vp_host_shim.h established this exact shape for the host
// syntax check (tools/syntax-check.sh) — the three glibc holes and the Win32
// SDK struct set are absorbed from there; its content is no-op under _WIN32,
// and fidelity is upgraded here from "declaration enough" to "ABI enough for
// a running build" where a struct is indexed, not just pointer-passed.
//
// PE safety: the PE build never sees this file (Linux-only forced include),
// so the _WIN32 path is byte-identical by construction — track rule 1
// (docs/linux-port.md) with zero decomp-TU edits for this surface.
#ifndef D2_LINUX_COMPAT_H
#define D2_LINUX_COMPAT_H

#if !defined(_WIN32)

// Runs before d2_platform.h, so the fixed-width types come from the CRT
// directly (the tree's DWORD/BYTE/UINT aliases do not exist yet here).
// <ctype.h>: game TUs call tolower/toupper and friends relying on the
// windows.h CRT chain to have pulled the C types in (e.g. Fog/SMem.cpp's
// name-hash loop) — on the PE line mingw's headers supply them.
#include <stdint.h>
#include <cstddef>
#include <ctype.h>

// ---------------------------------------------------------------------------
// 1. FILE: d2_platform.h claims the MSVC CRT layout (struct _iobuf, guarded
//    by _FILE_DEFINED — the MSVC/mingw guard). glibc uses its own guards
//    (__FILE_defined / __io_FILE_defined) and would typedef FILE to
//    _IO_FILE — a conflicting redeclaration. Pre-defining glibc's guards
//    skips both its struct and its typedef; d2_platform.h's typedef stands
//    and every later FILE* stays pointer-consistent. (vp_host_shim.h hole 1.)
// ---------------------------------------------------------------------------
#ifndef __FILE_defined
#define __FILE_defined 1
#endif
#ifndef __io_FILE_defined
#define __io_FILE_defined 1
#endif

// ---------------------------------------------------------------------------
// 2. MSVC calling conventions. i386: the register-passing forms the decomp
//    names, spelled as GCC attributes — declaration and definition always see
//    the same macro, so the build is self-consistent, and the i386 encodings
//    match the MSVC x86 ABI the decomp was recovered against (ECX/EDX for
//    fastcall, ECX this for thiscall, callee-cleans for stdcall). x86-64:
//    every convention collapses to the default ABI — the skeleton-only
//    configuration (PLAN.md R1: game code ships i386). Variadic stdcall
//    falls back to caller-cleans exactly as it does on mingw.
//    (vp_host_shim.h hole 2, upgraded from empty macros to real attributes.)
// ---------------------------------------------------------------------------
#if defined(__i386__)
#  ifndef __fastcall
#    define __fastcall __attribute__((fastcall))
#  endif
#  ifndef __stdcall
#    define __stdcall __attribute__((stdcall))
#  endif
#  ifndef __thiscall
#    define __thiscall __attribute__((thiscall))
#  endif
#else
#  ifndef __fastcall
#    define __fastcall
#  endif
#  ifndef __stdcall
#    define __stdcall
#  endif
#  ifndef __thiscall
#    define __thiscall
#  endif
#endif
#ifndef __cdecl
#define __cdecl
#endif


// ---------------------------------------------------------------------------
// 3. FILETIME: winnt.h supplies the 64-bit struct (two DWORDs), which the
//    decompiled code references by name. mingw provides it, but some builds
//    need the struct body to be visible here so the layout is locked.
//    (vp_host_shim.h hole 3, upgraded from forward declaration.)
// ---------------------------------------------------------------------------
#ifndef _WIN32
typedef struct _FILETIME {
    uint32_t dwLowDateTime;
    uint32_t dwHighDateTime;
} FILETIME, *LPFILETIME;
#endif // !defined(_WIN32)


// ---------------------------------------------------------------------------
// 3. Minimal Windows types needed BEFORE d2_platform.h
//    (d2_platform.h defines most Windows types, but we need these early)
// ---------------------------------------------------------------------------
#ifndef _WIN32
typedef uint32_t DWORD;
typedef int32_t BOOL;
typedef void* HANDLE;
typedef void* HWND;
typedef const char* LPCSTR;
typedef uint32_t WPARAM;
typedef int32_t LPARAM;
typedef uint32_t LRESULT;
typedef void* HMODULE;
typedef int32_t INT;
typedef uint32_t UINT;
typedef int32_t LONG;
#endif // !defined(_WIN32)

#endif // !defined(_WIN32)

#endif // D2_LINUX_COMPAT_H