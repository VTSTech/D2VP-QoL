// mpq_fallback_harness.cpp — [d2linux M2 round 3 fix] standalone proof for the
// GetModuleFileNameA slash-direction fix, for boxes without i386 multilib
// (the CI gate itself re-verifies end to end through the real binary).
//
// It ports VERBATIM the path logic the smoke's MPQ fixture rides:
//   - D2Hell.cpp FindMpqFromDiskOrCd: SStrCopy -> SFileOpenArchive (relative)
//     -> GetModuleFileNameA -> strrchr(0x5C) truncate -> SStrPack(name)
//     -> SFileOpenArchive (module dir). The CD scan is out of scope here; the
//     module-dir rung is the one that must answer in CI.
//   - Storm/Source/SFile.cpp SFileOpenArchive: SFILE_FileExistsOnDisk probe
//     (GetFileAttributesA) then CreateFileA open.
//   - the shim: d2_posix_path (Win32 -> POSIX at the open/stat boundary) and
//     GetModuleFileNameA in BOTH modes — old (POSIX slashes, the CI failure)
//     and new (Win32 slashes, the fix).
//
// PASS = the NEW mode opens the fixture next to this binary AND the OLD mode
// provably does not (the differentiator). Exit 0 on PASS.

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

typedef char* LPSTR;
typedef const char* LPCSTR;

// ---- the shim's d2_posix_path, verbatim (d2_linux_shim_win32.cpp) ----------
static const char* d2_posix_path(char* szBuf, size_t cbBuf, LPCSTR lpFileName) {
    if (!lpFileName) return lpFileName;
    if (!strchr(lpFileName, '\\')) return lpFileName;
    size_t n = 0;
    for (const char* p = lpFileName; *p && n + 1 < cbBuf; ++p) {
        szBuf[n++] = (*p == '\\') ? '/' : *p;
    }
    szBuf[n] = '\0';
    return szBuf;
}

// ---- SFileOpenArchive's on-disk probe + open, modeled ----------------------
// SFILE_FileExistsOnDisk -> GetFileAttributesA (shim: d2_posix_path + stat),
// then the CreateFileA open (shim: d2_posix_path + open(2)). stat() covers
// both verdicts: what the boundary would open is what stat resolves.
static bool BoundaryOpen(LPCSTR lpWin32Path) {
    char szPath[1024];
    const char* pPosix = d2_posix_path(szPath, sizeof(szPath), lpWin32Path);
    if (!pPosix) return false;
    struct stat st;
    return stat(pPosix, &st) == 0 && S_ISREG(st.st_mode);
}

// ---- Storm's bounded strcat, verbatim semantics ----------------------------
static void SStrCopy(LPSTR dst, LPCSTR src) { strcpy(dst, src); }
static void SStrPack(LPSTR dst, LPCSTR src) { strcat(dst, src); }

// ---- GetModuleFileNameA, old vs new ----------------------------------------
static void GetModuleFileNameA_mode(LPSTR lpFilename, size_t nSize, bool bWin32Face) {
    ssize_t n = readlink("/proc/self/exe", lpFilename, nSize - 1);
    if (n <= 0) { lpFilename[0] = '\0'; return; }
    lpFilename[n] = '\0';
    if (bWin32Face) {
        for (char* p = lpFilename; *p; ++p) {
            if (*p == '/') *p = '\\';
        }
    }
}

// ---- D2Hell's FindMpqFromDiskOrCd module-dir rung, verbatim path shape -----
static bool FindMpqFromDiskOrCd(LPCSTR pszMpqName, bool bWin32Face, char* szTextOut) {
    char szText[260];
    SStrCopy(szText, pszMpqName);                       // relative attempt
    if (BoundaryOpen(szText)) { strcpy(szTextOut, szText); return true; }

    GetModuleFileNameA_mode(szText, 0x104, bWin32Face); // module dir fallback
    char* pszLastBackslash = strrchr(szText, 0x5c);     // the decomp's 0x5C
    if (pszLastBackslash) {
        pszLastBackslash[1] = '\0';
    }
    SStrPack(szText, pszMpqName);
    strcpy(szTextOut, szText);
    return BoundaryOpen(szText);
}

int main() {
    char szNew[260], szOld[260];
    bool bNew = FindMpqFromDiskOrCd("d2vp_smoke.mpq", true, szNew);
    bool bOld = FindMpqFromDiskOrCd("d2vp_smoke.mpq", false, szOld);

    std::printf("module-dir rung, NEW (Win32 face):  open=%s  path=%s\n",
                bNew ? "YES" : "no", szNew);
    std::printf("module-dir rung, OLD (POSIX face):  open=%s  path=%s\n",
                bOld ? "YES" : "no", szOld);

    if (bNew && !bOld) {
        std::printf("mpq_fallback_harness PASS (fix opens the fixture, pre-fix shape does not)\n");
        return 0;
    }
    std::printf("mpq_fallback_harness FAIL\n");
    return 1;
}
