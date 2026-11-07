// platform/linux/d2_linux_static_init.cpp — [d2linux M2 round 3 fix] the
// linux twin of support/d2_static_init.cpp: the game's own C++ static
// initializers, in the PE binary's _initterm table order (66 entries at
// 006cc618; docs/data/cpp-static-init-order.tsv).
//
// Why a twin and not the PE file itself: support/d2_static_init.cpp is a PE
// TU (windows.h, the MSVC .CRT$XCU section) and its table names every module
// — including ones the i386 slice does not link yet (D2Client, D2Game,
// Bnclient, D2Launch, D2gfx, D2Win, D2Net, D2CMP, D2Common, SCMD). This twin
// runs the table's entries whose TUs ARE in the slice, preserving the
// table's relative order, and grows rung by rung with the slice (next: the
// D2Win archive/forms rung, then globals.cpp + the D2Client boot chain).
//
// Why it matters: without the pass, Storm's global list heads are zeroed
// BSS. The lists use Blizzard's complement-offset link encoding — the empty
// state is pNext = ~(&head + 4) (StaticInit_gSFileOpenFileList), not 0 — so
// the first SFILE_AllocFileRecord took the direct-pointer branch of
// SFILE_InsertFileRecordIntoList2 with nLinkOffset 0 and wrote through
// pointer 0: the 0xC0000005 the M2 round-3 smoke met in CI at
// SFILE_InsertFileRecordIntoList2+0xf3.
//
// Ordering contract (the PE file's own header note): run AFTER the
// fault-report layer installs, so a fault inside an initializer still
// reports. The fault installer carries init_priority(150) in
// d2_linux_crash.cpp; this constructor is priority 200. Both run before any
// unprioritized constructor.
//
// PE safety: this file compiles only in the linux build (build.cmake).

#include "Fog/Src/LogManager.h"
#include "Storm/Source/SComp.h"
#include "Storm/Source/SEVT.h"
#include "Storm/Source/SErr.h"
#include "Storm/Source/SFile.h"
#include "Storm/Source/SSystem.h"
#include "Storm/WindowHandle.h"
#include "D2Hell/D2Hell.h"

extern "C" __attribute__((constructor(200))) void D2RunCppStaticInitializers() {
    // [00]..[15] — the Fog/Storm family, table order 006ca700..006ca920.
    Fog::Src::LogManager::StaticInit_gLogMessageQueueCS();        // [00] 006ca700
    Fog::Src::LogManager::StaticInit_gLogFileCS();                // [01] 006ca720
    Fog::Src::LogManager::LOGMGR_InitMessageQueue();              // [02] 006ca740
    Storm::Source::SFile::StaticInit_gMpqArchiveList();           // [03] 006ca7b0
    Storm::Source::SFile::StaticInit_gSFileAsyncReqQueue();       // [04] 006ca7d0
    Storm::Source::SFile::StaticInit_CriticalSection_00779068();  // [05] 006ca7f0
    Storm::Source::SFile::StaticInit_gSFileOpenFileList();        // [06] 006ca810
    Storm::Source::SFile::StaticInit_gSFileAudioStreamList();     // [07] 006ca830
    Storm::Source::SFile::StaticInit_gSFileStreamRecordList();    // [08] 006ca850
    Storm::Source::SFile::StaticInit_CriticalSection_00779080();  // [09] 006ca870
    Storm::Source::SSystem::StaticInit_6ca890();                  // [10] 006ca890
    Storm::Source::SErr::StaticInit_gStormErrorCallbackList();    // [11] 006ca8a0
    Storm::Source::SComp::StaticInit_CriticalSection_00779214();  // [12] 006ca8c0
    Storm::WindowHandle::StaticInit_gWndTrackingList();           // [13] 006ca8e0
    Storm::Source::SEVT::StaticInit_gSevtBreakCmdList();          // [14] 006ca900
    Storm::Source::SEVT::StaticInit_CriticalSection_0077930c();   // [15] 006ca920

    // [16]..[49] — D2Launch / D2Client / D2gfx / D2Win entries: their TUs
    // join the slice at later rungs; the twin grows with the slice.

    // [50] 006cae5f — the D2Hell fault-handler list (the 12-byte head at
    // 0072fa14, server v892 naming); D2Hell.cpp is in the slice.
    D2Hell::StaticInit_gFaultHandlerList();

    // [51]..[64] — Bnclient / D2Net / D2Game / D2CMP / D2Common / SCMD
    // entries: later rungs.

    // [65] 006ca6f0 — the main-thread id cache (SSystem).
    Storm::Source::SSystem::SetCurrentThreadId();
}
