// [d2boot] Diagnostic boot trace — see support/d2_boot.h.
// Always-on and unbuffered on purpose: the failure being chased is a silent
// exit, so every mark is flushed to disk the moment it is written.
// Each run OVERWRITES the previous log (CREATE_ALWAYS, not OPEN_ALWAYS +
// append): d2boot.log stays the trail of the most recent run only. The file
// is truncated once, at the first mark of the process — every mark after
// that still accumulates within the run. Two simultaneous instances would
// truncate each other's log; diagnostic tool, accepted trade-off.
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

#include "Redivivus/d2_boot.h"

static HANDLE g_hBootLog = INVALID_HANDLE_VALUE;
static char g_szBootLogPath[MAX_PATH] = "(unopened)";
static CRITICAL_SECTION g_csBootLog; // serialized: marks come from several threads
static BOOL g_bBootLogCsReady = FALSE;

static void BootLogOpen() {
    if (g_hBootLog != INVALID_HANDLE_VALUE) return;
    char szPath[MAX_PATH];
    DWORD n = GetModuleFileNameA(0, szPath, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return;
    while (n > 0 && szPath[n - 1] != '\\') n--;
    // [R161 SEC-03] bounded append: the exe path may already be MAX_PATH-1
    // long, and lstrcpyA/lstrcatA cannot check - an exe installed at a
    // near-260-char path overran the stack buffer by up to ~11 bytes here and
    // in the %TEMP% fallback below. "d2boot.log" needs 11 bytes; if the base
    // cannot hold them, skip the exe-dir attempt (the %TEMP% fallback runs
    // with the same reserve).
    if (n + 11 <= MAX_PATH) {
        lstrcpyA(szPath + n, "d2boot.log");
        // GENERIC_WRITE + CREATE_ALWAYS: fresh log every run (pointer starts at 0,
        // sequential WriteFile calls then accumulate the run's marks in order).
        g_hBootLog = CreateFileA(szPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    }
    if (g_hBootLog == INVALID_HANDLE_VALUE) {
        // Game dir not writable (or the exe path left no room) - fall back to
        // %TEMP% and say so in the log. GetTempPathA is capped 12 below
        // MAX_PATH so the suffix above always fits.
        char szTmp[MAX_PATH];
        DWORD m = GetTempPathA(MAX_PATH - 12, szTmp);
        if (m == 0 || m >= MAX_PATH - 12) return;
        lstrcatA(szTmp, "d2boot.log");
        g_hBootLog = CreateFileA(szTmp, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (g_hBootLog == INVALID_HANDLE_VALUE) return;
        lstrcpyA(szPath, szTmp);
    }
    lstrcpyA(g_szBootLogPath, szPath);
}

static void BootLogWrite(const char* pszLine) {
    // Serialized: marks come from several threads (render, sound, main).
    if (g_bBootLogCsReady) EnterCriticalSection(&g_csBootLog);
    BootLogOpen();
    if (g_hBootLog != INVALID_HANDLE_VALUE) {
        // [R161 ROB-05] wsprintfA truncates its OUTPUT at 1024 chars but takes
        // no destination size: a long input (a crash-report line routed through
        // D2BOOT_Mark can be up to 1023 chars) overran this 640-byte stack
        // buffer - precisely on the process-is-dying path, where the report
        // silently corrupted. Pre-truncate every input below the destination
        // size (the idiom d2_hangwatch.cpp already used for symbol names);
        // 580 + the 20-char timestamp prefix + NUL stays inside 640.
        char szTrunc[580];
        if (lstrlenA(pszLine) >= (int)sizeof(szTrunc)) {
            lstrcpynA(szTrunc, pszLine, (int)sizeof(szTrunc));
            pszLine = szTrunc;
        }
        char szBuf[640];
        // [R176] thread id in every line: the R175 field log interleaved read-thread and
        // auth-thread marks and cost a session's worth of thread attribution - marks come
        // from several threads, the prefix should say which.
        DWORD dwTid = GetCurrentThreadId();
        int n = wsprintfA(szBuf, "[%8lu ms][tid %lu] %s\r\n", (unsigned long)GetTickCount(),
                          (unsigned long)dwTid, pszLine);
        DWORD dwWritten = 0;
        WriteFile(g_hBootLog, szBuf, (DWORD)n, &dwWritten, 0);
        FlushFileBuffers(g_hBootLog);
    }
    if (g_bBootLogCsReady) LeaveCriticalSection(&g_csBootLog);
}

extern "C" void D2BOOT_Mark(const char* pszStage) {
    BootLogWrite(pszStage);
}

extern "C" void D2BOOT_MarkFmt(const char* pszFormat, ...) {
    // [R161 ROB-05] wvsprintfA writes at most 1024 output chars (its own cap,
    // no destination-size parameter) - the old 512-byte body could be overrun
    // by a long formatted mark. Size the body to the API cap; BootLogWrite
    // then pre-truncates for its own 640-byte line buffer.
    char szBody[1024];
    va_list args;
    va_start(args, pszFormat);
    wvsprintfA(szBody, pszFormat, args);
    va_end(args);
    BootLogWrite(szBody);
}

extern "C" void D2BOOT_Fail(const char* pszStage) {
    char szMsg[1024];
    wsprintfA(szMsg, "d2 boot diagnostic\n\nExited early at stage:\n  %s\n\n"
              "Full milestone trail: %s\n\n"
              "(This dialog is from the temporary boot-trace build.)", pszStage, g_szBootLogPath);
    BootLogWrite(szMsg);
    MessageBoxA(0, szMsg, "d2.exe boot diagnostic", MB_ICONERROR | MB_OK);
}

// VP-BEGIN [bn-ctl-replay R178]: the exact outbound SID_AUTH_INFO (0x50) frame stashed by
// NET_SID_CLIENT_Send (BnSend.cpp) at send time; the Safesock autopsy's control socket
// replays it verbatim onto a fresh connection to test whether the 10053 death follows the
// conversation content or the original flow's kernel state. See d2_boot.h for the contract.
char g_chCtlReplayAuthInfo[0x2000] = {};
int g_nCtlReplayAuthInfoLen = 0;
// VP-END [bn-ctl-replay R178]

// First mark proves the CRT and static initializers ran. Priority 130 puts it
// ahead of the crash (140) and trace (150) handlers on purpose.
__attribute__((init_priority(130))) static struct D2BootInit {
    D2BootInit() {
        BootLogOpen();
        InitializeCriticalSection(&g_csBootLog); // before any marks can race
        g_bBootLogCsReady = TRUE;
        char szLine[MAX_PATH + 64];
        wsprintfA(szLine, "static-init ok (CRT + globals); log = %s", g_szBootLogPath);
        BootLogWrite(szLine);
    }
} g_d2BootInit;
