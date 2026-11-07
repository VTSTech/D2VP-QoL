# INVENTORY.md — the M1 Win32-surface inventory and shim ledger (d2linux)

> Status: v2, 2026-11-08 — updated through the M2 round 3 (Storm/MPQ file
> routing — the archive rung). Original: the M1 inventory pass (PLAN.md §6
> M1, first bullet).
> Method: empirical. Every candidate translation unit was compiled with the
> linux forced-include chain (`d2_linux_compat.h` → platform-shadow
> `d2_platform.h` → `tools/vp_host_api.h` → `d2_linux_api.h`) under the i386
> charter flags, and the shim surface grew only at the named call sites the
> compile/link loop surfaced — the inventory-first rule (PLAN.md §7 R2:
> "shims grow only at named call sites; each step keeps CI green").

## 1. What the pass found

The decomp's Windows surface arrives through four doors:

1. **d2_platform.h's non-Windows branch** (the generator's own): scalar/handle
   typedefs and ~50 `extern "C"` Win32 declarations with no implementations —
   the M1 shim's job. It had never been compiled anywhere
   (tools/syntax-check.sh's comment: "one in-built inconsistency that has
   never compiled anywhere" — there is more than one; see §3).
2. **The fork's own host preludes** (`tools/vp_host_shim.h` +
   `tools/vp_host_api.h`, the syntax-check surface): the three glibc holes
   (FILE guards, calling conventions, `errno_t`) and the fork-TU Win32 fill-in
   (file/lstring/version/toolhelp/SEH/critical-section shapes). Both absorbed
   into the linux build by reference (vp_host_api.h) and by upgraded copy
   (vp_host_shim.h → d2_linux_compat.h, fidelity raised from
   declaration-enough to ABI-enough where the slice indexes the struct).
3. **The game TUs' own `#ifndef _WIN32` fallback blocks** (the generator's
   "provided by the Win32 SDK / CRT on Windows" markers): they expect the port
   to supply SDK types (`_devicemodeA`, `tagWNDCLASSA`, `WSAData`,
   `THREADENTRY32`, the wincrypt blob family, `FLOATING_SAVE_AREA`...) —
   supply list extracted by grepping every such block.
4. **MSVC CRT internals** the tree calls by name (`_time32`, `_itoa` family,
   `_strlwr/_strupr`, `strncpy_s`, `_localtime64_s`, `_wfopen`, the `CRT_*`
   x87 intrinsics, `_eh_vector_*_iterator_`, `_except_handler4`) — supplied
   by the PE line's own portable support files (d2_crt_impl.cpp /
   d2_crt_support.cpp, brought in with two guarded edits) plus the shim's CRT
   file.

## 2. The seam as built

```
game TUs (M1 slice, 44 TUs)
    |  -include d2_linux_compat.h      (glibc holes, conventions, SDK structs)
    |  -include d2_platform.h          (the platform shadow — see §3)
    |  -include tools/vp_host_api.h    (fork-curated Win32 fill-in)
    |  -include d2_linux_api.h         (the M1 surface: registry, winsock,
    |                                   threads, toolhelp, conversion, errors)
    v
platform/linux shims
    - d2_linux_shim_win32.cpp    (heap/locks/time/events/threads/toolhelp/
                                  module+memory queries over /proc, text
                                  conversion, errors, winsock over BSD, the
                                  fail-soft families)
    - d2_linux_shim_registry.cpp (the file-backed registry, ~/.config/d2linux)
    - d2_linux_shim_crt.cpp      (MSVC CRT bits, boot/trace twins, the VEH
                                  chain + unhandled filter + translator slots)
    - d2_linux_crash.cpp         (the crash twin: /proc module tables, EBP
                                  walk, dladdr symbolization, signal->CONTEXT,
                                  XDG known-folder)
    - d2_linux_globals_step1.cpp (STEP-1 ONLY: weak globals that yield to
                                  globals.cpp at step 2 — see §5)
    v
SDL3 (static, vendored release-3.4.18) + glibc (i386)
```

The slice (build.cmake's `D2LINUX_GAME_SOURCES`, 49 TUs after round 3 — the
ledger grows 5-10 per step): PKWARE; Fog foundation (SMem, Memory, BitBuffer,
File, Fog, Save, Debug, DataArrays, ErrorManager, LogManager, FogString,
Safesock, AsyncData, Source/{SFile, SSComp}, 3rdparty/crashy + crashy/Report);
Storm foundation (SReg, SErr, SComp, SBig, SMem, SSTR, SSystem, SEVT, SLog,
SFile, WindowHandle, 3rdparty/zlib 9 TUs); both SSignature files; D2Hell's two
CRT support files; globals/Fog.cpp + globals/Storm.cpp.

Round 3 added the ARCHIVE RUNG (5): D2Hell/D2Hell.cpp (the archive layer:
ARCHIVE_LoadMPQ / D2FILE_* / FaultError, whose ARCHIVE_LoadMPQ is exactly
what D2WinArchive::MPQ_LoadAllDataMpqFiles runs at boot) + the D2Lang string
layer (strtable, D2Lang, UNISYS) + their per-TU globals files
(globals/D2Lang.cpp, globals/D2Hell.cpp). globals/D2Win.cpp stays OUT — its
closure is the D2Win UI handler tables (D2WIN_Handle*/INPUT_Form*), the
round-4 rung; its three zero-initialized MPQ slot pointers ride the weak
file instead. support/d2_boot.cpp stays PE-only: d2_linux_shim_crt.cpp's
boot twins already serve D2Hell's D2BOOT_MarkFmt marks.

## 3. The platform shadow (mk-platform-shadow.sh, 12 patches)

tools/syntax-check.sh's temp-shadow pattern, scaled to a real build and
regenerated at configure time so upstream d2_platform.h regens (the R-syncs)
re-apply it. The tree file is never edited; every patch sits inside
`#ifndef _WIN32`, so the PE path is untouched by construction:

1. `typedef void* PRTL_CRITICAL_SECTION_DEBUG;` dropped — the struct-typed
   SDK form stands (vp_host_api.h).
2. `struct CONTEXT { DWORD ContextFlags; };` stub dropped — the compat layer's
   `CONTEXT = _CONTEXT` alias + Fog/Debug.h's real body stand.
3-5. The IMAGE_DOS_HEADER/IMAGE_NT_HEADERS/IMAGE_SECTION_HEADER stubs dropped —
   d2_crash's PE walk needs the REAL x86 layouts (vp_host_api.h).
6. `_LARGE_INTEGER` completed with the `.u` member (the decomp reads
   `.u.LowPart`; the stub only spelled `.s`).
7. `struct CRITICAL_SECTION` retagged onto `_RTL_CRITICAL_SECTION` (the
   globals declare their locks with the SDK's tag; two identical types for
   one lock broke the pointer types).
8. `typedef uint16_t WCHAR` → `typedef wchar_t WCHAR` (the build spells
   -fshort-wchar; MSVC's 16-bit wchar_t is part of the ABI the decomp models,
   and under GCC's flag the types are still distinct).
9. `typedef void* HMODULE/HINSTANCE` → the DECLARE_HANDLE form (Fog/Debug.cpp
   dereferences HMODULE for the `MZ` check, exactly like the SDK shape).
10. `DWORD_PTR/ULONG_PTR` per-arch: `unsigned long` on i386 (MSVC's own 32-bit
    spelling — same TYPE as DWORD, which the decomp's `DWORD*` → `ULONG_PTR*`
    arguments rely on), `uintptr_t` on the x86-64 skeleton.
11. The two stray non-Windows SDK declaration groups (CreateEventW /
    WaitForMultipleObjects / SendMessageA and GetFileSize..timeGetTime) and
    12. the IsBad*/CreateEventA/window block — wrapped in `extern "C"` (the
    game TUs' references and the shim's definitions must agree on one
    linkage; the SDK's own is C).

## 4. The guarded decomp edits (rule 1: #ifdef-guarded, PE-identical)

| File | Edit | Why |
|---|---|---|
| Bnclient/BnSend.h | `struct __time32_t;` forward decl stood down under `D2_LINUX_TIME32_SUPPLIED` | the elaborated decl never parsed anywhere (gcc: "using typedef-name after struct"); the compat prelude supplies the real typedef |
| support/d2_crt_impl.cpp | `<windows.h>` guarded; the `_MSC_VER` CRT-filter block also compiles under `D2LINUX_GAME` | the filters are stored, never dispatched (its own comment) — the same empty body is their whole linux behavior |
| support/d2_crt_support.cpp | the `d2_CRT_Floor` asm label spells `CRT_Floor` under `D2LINUX_GAME` | ELF carries no user-label underscore |
| D2Net/in_addr.h | the simple regen body stood down under `D2_LINUX_SDK_IN_ADDR` | Safesock.cpp's regenerated bodies reference the SDK-faithful union members (`_union_1226`); the compat layer supplies that shape |
| Fog/Src/Safesock.h | the winsock fallback types stood down under `D2_LINUX_WINSOCK_SUPPLIED` | they clash with the glibc types already in scope (timeval via bits/types, fd_set via sys/select); the api header supplies the winsock shapes |
| D2Win/Src/D2WinMain.h | the tagMONITORINFO/tagTRACKMOUSEEVENT fallbacks stood down under `D2_LINUX_SDK_WINDOW_SUPPLIED` | they double-define the real winuser.h layouts the compat layer has carried since M2 (round 2 added them for D2WinMain's shapes); the guard is nested inside the existing `#ifndef _WIN32`, so the PE path is untouched |

## 5. The step-1 weak globals (d2_linux_globals_step1.cpp)

globals/globals.cpp (the 39K-line central-data TU) embeds the game's command
dispatch tables — its closure is the whole client, so it joins at step 2.
The foundation slice externs a few of its definitions; they are weak-defined
here (verbatim content) and yield automatically to the strong ones when step 2
brings globals.cpp in: ExceptionList, the s_* error texts, the bit-mask and
zlib tables, z_errmsg, MISC_OperatorDeleteShim (free → Fog pools at step 2),
the D2Client GS-handler thunk (stored, never dispatched), and the two language
probes (default-language answers until the strtable layer joins).

Round 3 grew the file (verbatim content, weak linkage, same yield rule):
- the strtable layer's needs: the 13 font-name strings + the 4 language-code
  answers + the s_* language-label strings + gszCrashDumpRegisterFormatString
  (all globals.cpp verbatim), D2Client::UI::ui::CharacterLookupTable[256]
  (globals.cpp:38499, the case-fold table UNISYS's STRING_Compare maps
  through), D2Launch::MainMenus::STRING_ZeroOneWCHAR (MainMenus.cpp:526
  verbatim — one zero-filled wchar slot), and
  Fog::Engine::Application::MISC_OperatorNewShim — the CRT-heap twin of the
  existing weak DeleteShim, so the weak PAIR stays in one domain (weak new +
  strong delete would cross free() with SMemAlloc; the comment in the file
  says never mix).
- the archive slots: MPQ_d2data/MPQ_patch_d2/MPQ_d2exp — globals/D2Win.cpp's
  zero-initialized pointers at file scope; that TU's UI handler tables are
  the round-4 closure, so its data rides here until then.

## 6. Shim honesty table (every family has named call sites)

| Family | Body | Named sites |
|---|---|---|
| Heap (HeapAlloc/HeapFree/GetProcessHeap) | glibc malloc | Fog pool reservations |
| Critical sections | pthread_mutex in the RTL storage (24 = 24 on i386, static_assert) | Fog/Storm locks |
| Time (GetTickCount/QPC/QPF/Sleep/GetLocalTime/timeGetTime/SystemTimeToFileTime) | CLOCK_MONOTONIC, civil-date math | SBig, LogManager, SMem |
| Registry (RegOpenKeyExA family, 10 entry points) | file-backed store, ~/.config/d2linux/registry.ini, flush-through | SReg.cpp, Application boot |
| Events (CreateEvent*/Set/Reset/WaitForSingleObject) | pthread cond vars, in-process (cross-process names = M5) | crash-dump handshake |
| Threads (CreateThread/Open/Suspend/Resume/Priority) | pthread + tids; SIGSTOP/SIGCONT for suspend | Application crash path, LogManager |
| Toolhelp (snapshot + Thread/Process/Module walks) | /proc/self/task, /proc, dl_iterate_phdr | Fog/File, Fog/Debug, crashy |
| Memory status + VirtualQuery + GetSystemInfo | sysinfo, /proc/self/maps, sysconf | Fog/Memory OOM path, stack walker |
| Module info (GetModuleHandle/FileName/Information) | /proc/self/exe + maps; **round 3 fix: GetModuleFileNameA answers the Win32 face** — slashes flipped to `\`, because every decompiled caller munges the module path with 0x5C arithmetic (FindMpqFromDiskOrCd's strrchr + SStrPack module-dir fallback, FindMPQOnCd's SStrChr walk); d2_posix_path translates back at the open/stat boundary | Fog/Debug symbolizer, **the MPQ module-dir fallback (round 3)** |
| Text conversion (MultiByteToWideChar/WideCharToMultiByte) | hand-rolled UTF-8/UTF-16 codec, BMP-complete | Fog/File, Debug, Application |
| Errors (FormatMessageA, Get/SetLastError, WSAGetLastError) | code-family tables + TSD | Fog/File, SErr |
| Winsock (socket/connect/send/recv/closesocket/ioctlsocket/select + fd_set) | BSD fds; the winsock fd_set shape macro-mapped; select translates to poll(2) | Safesock client path |
| CreateFileA/ReadFile/WriteFile/SetFilePointer/GetFileSize/FlushFileBuffers/CreateFileMappingA/MapViewOfFile | open(2) + fd-honest HANDLEs (HANDLE = fd) + mmap; **round 3: Win32→POSIX path translation at the boundary** (`d2_posix_path`: backslash→slash, separator bytes only) — the archive open path composes `dir\name` shapes (FindMpqFromDiskOrCd, GetArchiveHandle) that open(2) would otherwise refuse | SFile save paths, **Storm MPQ opens (round 3)** |
| CloseHandle | **round 3: fd-aware** — fd-range handles close(fd); events still delete; pseudo-handles no-op. Storm closes every MPQ OS handle through here (SFileCloseArchive, SFileCloseFile, the failed-open cleanup); the M1 event-only body would have deleted an fd | **Storm MPQ archive/file closes (round 3)**, crash-dump handshake |
| Local heap (LocalAlloc/LocalFree) | **round 3**: glibc malloc, LMEM_ZEROINIT honored | D2Hell.cpp's Authenticode msg-info scratch |
| lstring (lstrcpyA/lstrcatA/lstrlenA) | **round 3**: straight CRT copies | d2_boot's log paths (the shim-crt boot twins), strtable paths |
| Fail-soft (service control, CreateProcess, window fns, LoadLibrary, FindResource) | the same failures the PE handles without the feature | Application boot |
| Authenticode (WinVerifyTrust, CryptQueryObject, CryptMsg*, Cert*) | **round 3**: the PE's "signature cannot be verified" outcome — TRUST_E_NOSIGNATURE, FALSE/NULL down the chain; D2FILE_GetFileSignature proceeds unsigned, exactly as a PE without crypt32 | D2Hell.cpp's SFile signature probe |

## 6b. The Storm/MPQ routing round found (round 3)

The vendored Storm's OS layer is plain `CreateFileA/ReadFile/SetFilePointer/
GetFileSize/CloseHandle` — the M1 file shim IS the MPQ routing layer. Four
gaps closed at the shim, no Storm edits:
1. backslash paths (above); 2. fd-aware CloseHandle (above); 3. the archive
open path itself needed nothing — SFileOpenArchive → SFILE_s_OpenArchive
parses the header at 0x200 steps, decrypts the hash/block tables with
SFILE_Hash(3, ...) + SFILE_Decrypt (the decomp's own crypt — the fixture
generator transcribes it), and reads stored single-unit files contiguously
(`dwFlags & 0xff00` gates the sector table; EXISTS|SINGLE_UNIT reads raw).
4. [round 3 fix, CI] GetModuleFileNameA's Win32 face (row above): the
module-dir fallback truncated the module path at strrchr(0x5C), and a
POSIX-slash answer made it miss — the MPQ name glued onto the full exe path
("/build/d2linuxd2vp_smoke.mpq"), the CI round's mpq self-test failure.
The smoke fixture (platform/linux/tests/d2vp_smoke.mpq, 1136 bytes, generated
by tools/gen-mpq-fixture.py and validated against the independent mpyq
reader) exercises all of it in CI.
5. [round 3 fix, owner field] the case-fold boundary: NTFS, the PE's home
   filesystem, matches file names case-insensitively — ext4 does not. The
   PE's baked lowercase "patch_d2.mpq" (MPQ_LoadAllDataMpqFiles,
   D2Win/Src/D2WinArchive.cpp:133) opens a real install's mixed-case
   "Patch_D2.mpq" on Windows and got ENOENT here (the owner renamed the file
   lowercase for the first --mpq-check pass). The shim resolves it
   Wine-unixfs style: d2_casefold_path walks each path component against its
   parent directory on the ENOENT path only, at BOTH faces the MPQ ladder
   meets (GetFileAttributesA's SFILE_FileExistsOnDisk probe and CreateFileA's
   open; CREATE_NEW excluded — an other-case existing file must not satisfy a
   create-new, exactly as Win32 wouldn't). A case-only match has the same
   length, every folded component is stat-verified on the way in, and a true
   absent still answers ENOENT. Pinned in --smoke by RunMpqCaseFoldSelfTest
   (the fixture rides the module dir as D2VP_CASE.MPQ, opened through the
   lowercase "d2vp_case.mpq" request — the real-install shape with a
   collision-proof name), and — while the sandbox i386 RUN stays
   seccomp-refused — by the verbatim-copy harness
   (scripts/mpq_fallback_harness.cpp): root fold, nested fold (BOTH
   components), exact spelling untouched, absent stays absent, phantom
   parent no fold. Also in this fix: --mpq-check's probe stops requiring
   patch_d2 (a never-patched install boots unpatched — the mirrored
   boot verdict only needs d2data+d2sfx+d2speech, +d2exp for expansion; the
   probe table now says what the code does).

## 7. Verification (this round)

- sandbox: the full slice (49 game TUs) compiles i386 and links — `d2linux`,
  ELF 32-bit PIE, via a user-space multilib sysroot (scripts/i386-gcc /
  i386-g++ wrappers + sysroot-i386/; no root in the container; the i386
  RUN is still seccomp-refused — the smoke run of the charter binary is
  CI's ring).
- sandbox: the `native` build (skeleton + shim layer, host arch) configures,
  builds and passes `d2linux --smoke` headless (the MPQ self-test prints
  SKIPPED there — no game TUs in the 64-bit binary, charter R1).
- sandbox: the smoke fixture self-checks against the decomp's own crypt and
  passes the independent mpyq reader (open + read + byte-verify).
- sandbox [round 3 fix]: the native gate (check-build-linux.sh, no multilib)
  is green again at the fix commit — d2_linux_static_init.cpp compiles empty
  under native (its StaticInit_* callees are i386-only; the gate is
  D2LINUX_HAS_GAME_SLICE, the same define the MPQ probes use), instead of
dangling 18 undefined references at link.
- sandbox [round 3 fix]: the module-dir fallback proven both ways by the
  standalone harness (scripts/mpq_fallback_harness.cpp, verbatim port of
  FindMpqFromDiskOrCd's path shape + d2_posix_path): Win32 face opens
  "\dir\d2vp_smoke.mpq" → "/dir/d2vp_smoke.mpq"; the pre-fix POSIX face
  produces "/dir/binaryd2vp_smoke.mpq" and fails — the exact CI shape.
  The i386 RUN itself is still CI's ring (no multilib here).
- sandbox: PE gates **380/380** on the shared-TU diff (tools/check-vanilla-plus.sh).
- sandbox [round 3 fix]: the case-fold boundary proven by the standalone
  harness (verbatim copy of d2_casefold_path + the open/stat boundary):
  "\tmp\patch_d2.mpq" resolves the on-disk "Patch_D2.mpq" (the owner's exact
  scenario), "\data\patch_d2.mpq" folds BOTH components onto
  "Data/PATCH_D2.MPQ", the exact spelling is untouched, and absents/phantom
  parents keep their ENOENT. The i386 --smoke (which runs the same logic
  through the real ARCHIVE_LoadMPQ ladder via RunMpqCaseFoldSelfTest) is
  CI's ring; both arch builds compile+link clean and the native gate
  (check-build-linux.sh) is green at the fix commit.
- CI (this push): the i386 `linux-native` job builds the charter binary and
  runs `--smoke`, whose verdict now includes the MPQ gates: missing-MPQ
  clean-null + the fixture open/read/close ("d2linux mpq self-test OK").
- owner field pass: `./d2linux --mpq-check=<game dir>` next to a 1.14d
  install — opens all seven data MPQs through ARCHIVE_LoadMPQ and reports
  per-archive verdicts. The FAIL set mirrors MPQ_LoadAllDataMpqFiles's own
  boot verdict (D2WinArchive.cpp:139-147): only d2data+d2sfx+d2speech
  (missing = exit 1, cleanly diagnosed); d2delta/d2kfixup/patch_d2/d2exp
  report ABSENT-tolerated, exactly what a modern install without the legacy
  MPQs meets. The native skeleton binary explains that the probe needs the
  i386 game slice instead of silently ignoring the flag.

## 8. Next slice (round 4, toward M2's title screen)

The D2Win rung: D2Win/Src/D2WinArchive.cpp (MPQ_LoadAllDataMpqFiles/
MPQ_LoadAllMediaMpqFiles — ApplicationMain's first calls) + D2WinMain (the
form manager) + the DC6/CelCmp image layer it pulls (IMAGE_LoadDC6), then
globals/D2Win.cpp (the UI handler tables) — and with those, the boot chain
rungs after it: globals/globals.cpp + Fog/Engine/Application.cpp (the strong
operator shims + SystemSurvey) + D2Client/Engine/Application.cpp
(ApplicationMain itself) + the D2GFX/D2gfx renderer front onto the DirectDraw
shim. The storm/MPQ routing this round built is the floor all of that stands
on: ApplicationMain's SFILE_InitializeFileSystem + MPQ_LoadAllDataMpqFiles
call straight into it.
