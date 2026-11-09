// platform/linux/d2_linux_api.h — [d2linux M2] Win32 API surface subset for the
// linux build's shim layer. The PE line's windows.h + d2_platform.h carry the
// full SDK surface; this header only adds the subset the M1/M2 slice touches.
// Types are defined in d2_linux_compat.h (forced before d2_platform.h) and the
// platform shadow (d2_platform.h). This header only declares functions.
//
// PE path untouched: the whole file is wrapped in #ifndef _WIN32 and the
// mingw build never includes it (the forced-include chain stops at
// vp_host_api.h).
#ifndef D2_LINUX_API_H
#define D2_LINUX_API_H

#if !defined(_WIN32)

#include <stdint.h>

// Forward declarations for Win32 types (defined in d2_linux_compat.h and platform shadow)
struct HWND__; typedef struct HWND__* HWND;
struct HDC__; typedef struct HDC__* HDC;
struct HINSTANCE__; typedef struct HINSTANCE__* HINSTANCE;
struct HMENU__; typedef struct HMENU__* HMENU;
struct HKEY__; typedef struct HKEY__* HKEY;
struct HICON__; typedef struct HICON__* HICON;
struct HCURSOR__; typedef struct HCURSOR__* HCURSOR;
struct HBRUSH__; typedef struct HBRUSH__* HBRUSH;
struct HFONT__; typedef struct HFONT__* HFONT;
struct HMONITOR__; typedef struct HMONITOR__* HMONITOR;
struct HRGN__; typedef struct HRGN__* HRGN;
struct HDROP__; typedef struct HDROP__* HDROP;
struct HMETAFILE__; typedef struct HMETAFILE__* HMETAFILE;
struct HENHMETAFILE__; typedef struct HENHMETAFILE__* HENHMETAFILE;
struct HGLOBAL__; typedef struct HGLOBAL__* HGLOBAL;
struct HLOCAL__; typedef struct HLOCAL__* HLOCAL;
struct HRSRC__; typedef struct HRSRC__* HRSRC;
struct HACCEL__; typedef struct HACCEL__* HACCEL;
struct HPALETTE__; typedef struct HPALETTE__* HPALETTE;
struct HDROP__; typedef struct HDROP__* HDROP;
struct HFILE__; typedef struct HFILE__* HFILE;
struct HRESULT__; typedef struct HRESULT__* HRESULT;

typedef uint32_t DWORD;
typedef int32_t BOOL;
typedef uint32_t UINT;
typedef int32_t INT;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG;
typedef uint32_t WPARAM;
typedef int32_t LPARAM;
typedef int32_t LRESULT;
typedef void* HANDLE;
typedef HANDLE SC_HANDLE;
typedef HANDLE HINSTANCE;
typedef HANDLE HMODULE;
typedef HANDLE HKEY;
typedef HANDLE HGDIOBJ;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t* LPWSTR;
typedef uint16_t WORD;
typedef uint16_t ATOM;
typedef uint16_t WCHAR;
typedef uint32_t COLORREF;
typedef int32_t INT;
typedef uint32_t UINT;
typedef int32_t LONG;
typedef int64_t LONG_PTR;
typedef uint64_t DWORD_PTR;
typedef void* LPVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef uint32_t DWORD;
typedef int32_t BOOL;
typedef void* HANDLE;
typedef int32_t INT;
typedef uint32_t UINT;
typedef int32_t LONG;
typedef uint32_t WPARAM;
typedef int32_t LPARAM;
typedef int32_t LRESULT;
typedef void* HANDLE;
typedef HANDLE SC_HANDLE;
typedef void* HWND;
typedef const char* LPCSTR;
typedef uint32_t WPARAM;
typedef int32_t LPARAM;
typedef int32_t LRESULT;
typedef void* HMODULE;
typedef int32_t INT;
typedef uint32_t UINT;
typedef int32_t LONG;
typedef uint32_t UINT;
typedef int32_t INT;
typedef uint32_t DWORD;
typedef int32_t BOOL;
typedef void* HANDLE;
typedef int32_t INT;
typedef uint32_t UINT;
typedef int32_t LONG;
typedef uint32_t WPARAM;
typedef int32_t LPARAM;
typedef int32_t LRESULT;
typedef void* HANDLE;
typedef HANDLE SC_HANDLE;

typedef uint32_t DWORD;
typedef int32_t BOOL;
typedef void* HANDLE;
typedef int32_t INT;
typedef uint32_t UINT;
typedef int32_t LONG;
typedef uint32_t WPARAM;
typedef int32_t LPARAM;
typedef int32_t LRESULT;
typedef void* HANDLE;
typedef HANDLE SC_HANDLE;

// Forward declarations for struct types
struct _EXCEPTION_RECORD; typedef struct _EXCEPTION_RECORD EXCEPTION_RECORD, *PEXCEPTION_RECORD;
struct _CONTEXT; typedef struct _CONTEXT CONTEXT, *LPCONTEXT, *PCONTEXT;
struct _EXCEPTION_POINTERS; typedef struct _EXCEPTION_POINTERS EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;
struct _SYSTEM_INFO; typedef struct _SYSTEM_INFO _SYSTEM_INFO, *LPSYSTEM_INFO;
struct _SERVICE_STATUS; typedef struct _SERVICE_STATUS SERVICE_STATUS, *LPSERVICE_STATUS;
struct _SERVICE_TABLE_ENTRYA; typedef struct _SERVICE_TABLE_ENTRYA SERVICE_TABLE_ENTRYA, *LPSERVICE_TABLE_ENTRYA;
struct _WINDOWPLACEMENT; typedef struct _WINDOWPLACEMENT WINDOWPLACEMENT, *LPWINDOWPLACEMENT;
struct _DEVMODEA; typedef struct _DEVMODEA DEVMODEA, *LPDEVMODEA;
struct _MONITORINFO; typedef struct _MONITORINFO MONITORINFO, *LPMONITORINFO;
struct _TRACKMOUSEEVENT; typedef struct _TRACKMOUSEEVENT TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;
struct _SYSTEM_INFO; typedef struct _SYSTEM_INFO _SYSTEM_INFO, *LPSYSTEM_INFO;
struct _EXCEPTION_RECORD; typedef struct _EXCEPTION_RECORD EXCEPTION_RECORD, *PEXCEPTION_RECORD;
struct _CONTEXT; typedef struct _CONTEXT CONTEXT, *LPCONTEXT, *PCONTEXT;
struct _EXCEPTION_POINTERS; typedef struct _EXCEPTION_POINTERS EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

typedef void* HANDLE;
typedef HANDLE SC_HANDLE;
typedef void* HWND;
typedef void* HDC;
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef void* HKEY;
typedef void* HGDIOBJ;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HBRUSH;
typedef void* HFONT;
typedef void* HMONITOR;
typedef void* HRGN;
typedef void* HDROP;
typedef void* HMETAFILE;
typedef void* HENHMETAFILE;
typedef void* HGLOBAL;
typedef void* HLOCAL;
typedef void* HRSRC;
typedef void* HACCEL;
typedef void* HPALETTE;
typedef void* HDROP;
typedef void* HFILE;
typedef void* HRESULT;

typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t* LPWSTR;
typedef uint16_t WORD;
typedef uint16_t ATOM;
typedef uint16_t WCHAR;
typedef uint32_t COLORREF;
typedef int32_t INT;
typedef uint32_t UINT;
typedef int32_t LONG;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG;
typedef uint32_t WPARAM;
typedef int32_t LPARAM;
typedef int32_t LRESULT;
typedef void* HANDLE;
typedef HANDLE SC_HANDLE;
typedef void* HWND;
typedef void* HDC;
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef void* HKEY;
typedef void* HGDIOBJ;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HBRUSH;
typedef void* HFONT;
typedef void* HMONITOR;
typedef void* HRGN;
typedef void* HDROP.
typedef void* HMETAFILE.
typedef void* HENHMETAFILE.
typedef void* HGLOBAL.
typedef void* HLOCAL.
typedef void* HRSRC.
typedef void* HACCEL.
typedef void* HPALETTE.
typedef void* HDROP.
typedef void* HFILE.
typedef void* HRESULT.

typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR.
typedef const char* LPCSTR.
typedef const wchar_t* LPCWSTR.
typedef wchar_t* LPWSTR.
typedef uint16_t WORD.
typedef uint16_t ATOM.
typedef uint16_t WCHAR.
typedef uint32_t COLORREF.
typedef int32_t INT.
typedef uint32_t UINT.
typedef int32_t LONG.
typedef int64_t LONGLONG.
typedef uint64_t ULONGLONG.
typedef uint32_t WPARAM.
typedef int32_t LPARAM.
typedef int32_t LRESULT.
typedef void* HANDLE.
typedef HANDLE SC_HANDLE.
typedef void* HWND.
typedef void* HDC.
typedef void* HINSTANCE.
typedef void* HMODULE.
typedef void* HKEY.
typedef void* HGDIOBJ.
typedef void* HICON.
typedef void* HCURSOR.
typedef void* HBRUSH.
typedef void* HFONT.
typedef void* HMONITOR.
typedef void* HRGN.
typedef void* HDROP.
typedef void* HMETAFILE.
typedef void* HENHMETAFILE.
typedef void* HGLOBAL.
typedef void* HLOCAL.
typedef void* HRSRC.
typedef void* HACCEL.
typedef void* HPALETTE.
typedef void* HDROP.
typedef void* HFILE.
typedef void* HRESULT.

typedef void* LPVOID.
typedef const void* LPCVOID.
typedef char* LPSTR.
typedef const char* LPCSTR.
typedef const wchar_t* LPCWSTR.
typedef wchar_t* LPWSTR.
typedef uint16_t WORD.
typedef uint16_t ATOM.
typedef uint16_t WCHAR.
typedef uint32_t COLORREF.
typedef int32_t INT.
typedef uint32_t UINT.
typedef int32_t LONG.
typedef int64_t LONGLONG.
typedef uint64_t ULONGLONG.
typedef uint32_t WPARAM.
typedef int32_t LPARAM.
typedef int32_t LRESULT.
typedef void* HANDLE.
typedef HANDLE SC_HANDLE.
typedef void* HWND.
typedef void* HDC.
typedef void* HINSTANCE.
typedef void* HMODULE.
typedef void* HKEY.
typedef void* HGDIOBJ.
typedef void* HICON.
typedef void* HCURSOR.
typedef void* HBRUSH.
typedef void* HFONT.
typedef void* HMONITOR.
typedef void* HRGN.
typedef void* HDROP.
typedef void* HMETAFILE.
typedef void* HENHMETAFILE.
typedef void* HGLOBAL.
typedef void* HLOCAL.
typedef void* HRSRC.
typedef void* HACCEL.
typedef void* HPALETTE.
typedef void* HDROP.
typedef void* HFILE.
typedef void* HRESULT.

typedef void* LPVOID.
typedef const void* LPCVOID.
typedef char* LPSTR.
typedef const char* LPCSTR.
typedef const wchar_t* LPCWSTR.
typedef wchar_t* LPWSTR.
typedef uint16_t WORD.
typedef uint16_t ATOM.
typedef uint16_t WCHAR.
typedef uint32_t COLORREF.
typedef int32_t INT.
typedef uint32_t UINT.
typedef int32_t LONG.
typedef int64_t LONGLONG.
typedef uint64_t ULONGLONG.
typedef uint32_t WPARAM.
typedef int32_t LPARAM.
typedef int32_t LRESULT.
typedef void* HANDLE.
typedef HANDLE SC_HANDLE.
typedef void* HWND.
typedef void* HDC.
typedef void* HINSTANCE.
typedef void* HMODULE.
typedef void* HKEY.
typedef void* HGDIOBJ.
typedef void* HICON.
typedef void* HCURSOR.
typedef void* HBRUSH.
typedef void* HFONT.
typedef void* HMONITOR.
typedef void* HRGN.
typedef void* HDROP.
typedef void* HMETAFILE.
typedef void* HENHMETAFILE.
typedef void* HGLOBAL.
typedef void* HLOCAL.
typedef void* HRSRC.
typedef void* HACCEL.
typedef void* HPALETTE.
typedef void* HDROP.
typedef void* HFILE.
typedef void* HRESULT.

// Function pointer types
typedef LONG (__stdcall *LPTOP_LEVEL_EXCEPTION_FILTER)(PEXCEPTION_POINTERS lpExceptionPointers);
typedef LONG (__stdcall *PVECTORED_EXCEPTION_HANDLER)(PEXCEPTION_POINTERS ExceptionInfo);
typedef DWORD (__stdcall *LPHANDLER_FUNCTION)(DWORD dwControl);
typedef void (__stdcall *LPSERVICE_MAIN_FUNCTIONA)(DWORD dwNumServicesArgs, LPCSTR *lpServiceArgVectors);

typedef HANDLE (__stdcall *LPHANDLER_FUNCTION)(DWORD dwControl);
typedef HANDLE (__stdcall *LPHANDLER_FUNCTION)(DWORD dwControl);

// Basic types already defined above
// typedef int errno_t;

// Point and Rect
typedef struct tagPOINT {
    int32_t x;
    int32_t y;
} POINT, *LPPOINT;

typedef struct tagRECT {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} RECT, *LPRECT;

typedef struct tagSIZE {
    int32_t cx;
    int32_t cy;
} SIZE, *LPSIZE;

typedef struct tagPOINT POINT, *LPPOINT;
typedef struct tagRECT RECT, *LPRECT;

// WNDCLASSA
typedef struct tagWNDCLASSA {
    uint32_t style;
    void* lpfnWndProc;
    int32_t cbClsExtra;
    int32_t cbWndExtra;
    void* hInstance;
    void* hIcon;
    void* hCursor;
    void* hbrBackground;
    char* lpszMenuName;
    char* lpszClassName;
} WNDCLASSA, *LPWNDCLASSA;

// WINDOWPLACEMENT
typedef struct _WINDOWPLACEMENT {
    uint32_t length;
    uint32_t flags;
    uint32_t showCmd;
    POINT ptMinPosition;
    POINT ptMaxPosition;
    RECT rcNormalPosition;
    RECT rcDevice;
} WINDOWPLACEMENT, *LPWINDOWPLACEMENT;

// DEVMODEA
typedef struct _DEVMODEA {
    char dmDeviceName[32];
    uint16_t dmSpecVersion;
    uint16_t dmDriverVersion;
    uint32_t dmSize;
    uint32_t dmDriverExtra;
    uint32_t dmFields;
    int32_t dmPositionX;
    int32_t dmPositionY;
    uint32_t dmDisplayOrientation;
    uint32_t dmDisplayFixedOutput;
    int16_t dmColor;
    int16_t dmDuplex;
    int16_t dmYResolution;
    int16_t dmTTOption;
    int16_t dmCollate;
    char dmFormName[32];
    uint16_t dmLogPixels;
    uint32_t dmBitsPerPel;
    uint32_t dmPelsWidth;
    uint32_t dmPelsHeight;
    uint32_t dmDisplayFlags;
    uint32_t dmDisplayFrequency;
    uint32_t dmICMMethod;
    uint32_t dmICMIntent;
    uint32_t dmMediaType;
    uint32_t dmDitherType;
    uint32_t dmReserved1;
    uint32_t dmReserved2;
    uint32_t dmPanningWidth;
    uint32_t dmPanningHeight;
} DEVMODEA, *LPDEVMODEA;

// MONITORINFO
typedef struct _MONITORINFO {
    uint32_t cbSize;
    RECT rcMonitor;
    RECT rcWork;
    uint32_t dwFlags;
} MONITORINFO, *LPMONITORINFO;

// TRACKMOUSEEVENT
typedef struct _TRACKMOUSEEVENT {
    uint32_t cbSize;
    uint32_t dwFlags;
    HWND hwndTrack;
    uint32_t dwHoverTime;
} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

// HGDIOBJ
typedef void* HGDIOBJ;

// errno_t
typedef int errno_t;

// _SYSTEM_INFO
typedef struct _SYSTEM_INFO {
    union {
        uint32_t dwOemId;
        struct {
            uint16_t wProcessorArchitecture;
            uint16_t wReserved;
        };
    };
    uint32_t dwPageSize;
    void* lpMinimumApplicationAddress;
    void* lpMaximumApplicationAddress;
    uintptr_t dwActiveProcessorMask;
    uint32_t dwNumberOfProcessors;
    uint32_t dwProcessorType;
    uint32_t dwAllocationGranularity;
    uint16_t wProcessorLevel;
    uint16_t wProcessorRevision;
} _SYSTEM_INFO, *LPSYSTEM_INFO;

// Exception handling
typedef struct _EXCEPTION_RECORD {
    uint32_t ExceptionCode;
    uint32_t ExceptionFlags;
    struct _EXCEPTION_RECORD* ExceptionRecord;
    void* ExceptionAddress;
    uint32_t NumberParameters;
    uintptr_t ExceptionInformation[15];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

typedef struct _CONTEXT {
    uint32_t ContextFlags;
    uint32_t Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
    uint32_t FloatSave[28];
    uint32_t SegGs, SegFs, SegEs, SegDs;
    uint32_t Edi, Esi, Ebx, Edx, Ecx, Eax;
    uint32_t Ebp, Eip, SegCs, EFlags, Esp, SegSs;
    uint8_t ExtendedRegisters[512];
} CONTEXT, *LPCONTEXT;

typedef struct _EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PCONTEXT ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

typedef LONG (__stdcall *PVECTORED_EXCEPTION_HANDLER)(PEXCEPTION_POINTERS ExceptionInfo);

// Function pointer types
typedef LONG (__stdcall *LPTOP_LEVEL_EXCEPTION_FILTER)(PEXCEPTION_POINTERS lpExceptionPointers);
typedef DWORD (__stdcall *LPHANDLER_FUNCTION)(DWORD dwControl);
typedef void (__stdcall *LPSERVICE_MAIN_FUNCTIONA)(DWORD dwNumServicesArgs, LPCSTR *lpServiceArgVectors);
typedef DWORD (__stdcall *LPHANDLER_FUNCTION)(DWORD dwControl);

// Service API
typedef DWORD (__stdcall *LPHANDLER_FUNCTION)(DWORD dwControl);
typedef HANDLE (__stdcall *LPHANDLER_FUNCTION)(DWORD dwControl);

// Service API functions
HANDLE __stdcall RegisterServiceCtrlHandlerA(LPCSTR lpServiceName, LPHANDLER_FUNCTION lpHandlerProc);
SC_HANDLE __stdcall OpenSCManagerA(LPCSTR lpMachineName, LPCSTR lpDatabaseName, DWORD dwDesiredAccess);
SC_HANDLE __stdcall OpenServiceA(SC_HANDLE hSCManager, LPCSTR lpServiceName, DWORD dwDesiredAccess);
BOOL __stdcall CloseServiceHandle(SC_HANDLE hSCObject);
BOOL __stdcall StartServiceA(SC_HANDLE hService, DWORD dwNumServiceArgs, LPCSTR *lpServiceArgVectors);
BOOL __stdcall ControlService(SC_HANDLE hService, DWORD dwControl, LPSERVICE_STATUS lpServiceStatus);
BOOL __stdcall QueryServiceStatus(SC_HANDLE hService, LPSERVICE_STATUS lpServiceStatus);
BOOL __stdcall DeleteService(SC_HANDLE hService);
SC_HANDLE __stdcall CreateServiceA(SC_HANDLE hSCManager, LPCSTR lpServiceName, LPCSTR lpDisplayName, DWORD dwDesiredAccess, DWORD dwServiceType, DWORD dwStartType, DWORD dwErrorControl, LPCSTR lpBinaryPathName, LPCSTR lpLoadOrderGroup, LPDWORD lpdwTagId, LPCSTR lpDependencies, LPCSTR lpServiceStartName, LPCSTR lpPassword);
BOOL __stdcall StartServiceCtrlDispatcherA(const SERVICE_TABLE_ENTRYA *lpServiceTable);

// Window/Rect functions
BOOL __stdcall GetWindowRect(HWND hWnd, LPRECT lpRect);
BOOL __stdcall AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
BOOL __stdcall CopyRect(LPRECT lprcDst, const RECT* lprcSrc);
BOOL __stdcall OffsetRect(LPRECT lprc, int dx, int dy);
BOOL __stdcall InflateRect(LPRECT lprc, int dx, int dy);
BOOL __stdcall IntersectRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);
BOOL __stdcall UnionRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);
BOOL __stdcall PtInRect(const RECT* lprc, POINT pt);
BOOL __stdcall GetCursorPos(LPPOINT lpPoint);
BOOL __stdcall TrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack);
BOOL __stdcall PtInRect(const RECT* lprc, POINT pt);

// GDI functions
HGDIOBJ __stdcall SelectObject(HDC hdc, HGDIOBJ h);
BOOL __stdcall DeleteObject(HGDIOBJ hObject);
HGDIOBJ __stdcall GetStockObject(int i);

// Display settings
LONG __stdcall ChangeDisplaySettingsA(DEVMODEA* lpDevMode, DWORD dwFlags);
BOOL __stdcall GetMonitorInfoA(void* hMonitor, MONITORINFO* lpmi);
BOOL __stdcall TrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack);

// Monitor/Window
BOOL __stdcall GetMonitorInfoA(void* hMonitor, MONITORINFO* lpmi);
BOOL __stdcall TrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack);
BOOL __stdcall PtInRect(const RECT* lprc, POINT pt);
BOOL __stdcall GetCursorPos(LPPOINT lpPoint);
BOOL __stdcall TrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack);

// System info
void __stdcall GetSystemInfo(LPSYSTEM_INFO lpSystemInfo);

// Exception handling
LPTOP_LEVEL_EXCEPTION_FILTER __stdcall SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
ULONG __stdcall AddVectoredExceptionHandler(ULONG First, PVECTORED_EXCEPTION_HANDLER Handler);

// Context
BOOL __stdcall GetThreadContext(HANDLE hThread, CONTEXT* lpContext);
BOOL __stdcall SetThreadContext(HANDLE hThread, const CONTEXT* lpContext);

// Exception filters
void CRT_ExceptionFilter1(EHExceptionRecord* pExceptionRecord, EHRegistrationNode* pRegistrationNode, CONTEXT* pContext, void* pDispatcherContext);
void CRT_ExceptionFilter2(EHExceptionRecord* pExceptionRecord, EHRegistrationNode* pRegistrationNode, CONTEXT* pContext, void* pDispatcherContext);
void CRT_ExceptionFilter3(EHExceptionRecord* pExceptionRecord, EHRegistrationNode* pRegistrationNode, CONTEXT* pContext, void* pDispatcherContext);

// Time
errno_t _localtime64_s(struct tm* tmDest, const long long* sourceTime);
void __stdcall GetSystemInfo(_SYSTEM_INFO* lpSystemInfo);
void __stdcall GetSystemInfo(LPSYSTEM_INFO lpSystemInfo);

// Cursor
BOOL __stdcall GetCursorPos(LPPOINT lpPoint);
BOOL __stdcall PtInRect(const RECT* lprc, POINT pt);

// Window functions
BOOL __stdcall GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT* lpwndpl);
LONG __stdcall ChangeDisplaySettingsA(DEVMODEA* lpDevMode, DWORD dwFlags);
BOOL __stdcall GetMonitorInfoA(void* hMonitor, MONITORINFO* lpmi);
BOOL __stdcall TrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack);

// GDI
HGDIOBJ __stdcall SelectObject(HDC hdc, HGDIOBJ h);
BOOL __stdcall DeleteObject(HGDIOBJ hObject);
HGDIOBJ __stdcall GetStockObject(int i);

// Window position
BOOL __stdcall GetWindowRect(HWND hWnd, LPRECT lpRect);
BOOL __stdcall AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
int __stdcall DrawTextA(HDC hdc, LPCSTR lpchText, int cchText, LPRECT lprc, UINT format);
BOOL __stdcall CopyRect(LPRECT lprcDst, const RECT* lprcSrc);
BOOL __stdcall OffsetRect(LPRECT lprc, int dx, int dy);
BOOL __stdcall InflateRect(LPRECT lprc, int dx, int dy);
BOOL __stdcall IntersectRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);
BOOL __stdcall UnionRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);

// Cursor
BOOL __stdcall GetCursorPos(LPPOINT lpPoint);
BOOL __stdcall PtInRect(const RECT* lprc, POINT pt);
BOOL __stdcall TrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack);

// System
void __stdcall GetSystemInfo(_SYSTEM_INFO* lpSystemInfo);
void __stdcall GetSystemInfo(LPSYSTEM_INFO lpSystemInfo);
BOOL __stdcall GetComputerNameA(LPSTR lpBuffer, LPDWORD nSize);
BOOL __stdcall GetUserNameA(LPSTR lpBuffer, LPDWORD nSize);

// Registry
LSTATUS __stdcall RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
LSTATUS __stdcall RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired, PHKEY phkResult);
LSTATUS __stdcall RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
LSTATUS __stdcall RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData);
LSTATUS __stdcall RegDeleteValueA(HKEY hKey, LPCSTR lpValueName);
LSTATUS __stdcall RegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey);
LSTATUS __stdcall RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
LSTATUS __stdcall RegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
LSTATUS __stdcall RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
LSTATUS __stdcall RegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
LSTATUS __stdcall RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);

// File I/O
HANDLE __stdcall OpenEventA(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
HANDLE __stdcall FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
BOOL __stdcall IsBadStringPtrA(LPCSTR lpsz, UINT ucchMax);
DWORD __stdcall GetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
BOOL __stdcall SetCurrentDirectoryA(LPCSTR lpPathName);
BOOL __stdcall GetPrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
BOOL __stdcall DeleteFileA(LPCSTR lpFileName);
BOOL __stdcall GetDiskFreeSpaceA(LPCSTR lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters);
DWORD __stdcall GetFileAttributesA(LPCSTR lpFileName);
DWORD __stdcall GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
HANDLE __stdcall CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL __stdcall ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
BOOL __stdcall WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
BOOL __stdcall CloseHandle(HANDLE hObject);
DWORD __stdcall GetFileType(HANDLE hFile);
DWORD __stdcall SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
BOOL __stdcall SetEndOfFile(HANDLE hFile);
BOOL __stdcall FlushFileBuffers(HANDLE hFile);
BOOL __stdcall GetFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation);
HANDLE __stdcall CreateFileMappingA(HANDLE hFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName);
LPVOID __stdcall MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, DWORD dwNumberOfBytesToMap);
BOOL __stdcall UnmapViewOfFile(LPCVOID lpBaseAddress);
BOOL __stdcall CloseHandle(HANDLE hObject);

// Memory
LPVOID __stdcall VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL __stdcall VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
BOOL __stdcall VirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpOldProtect);

// String
int __stdcall lstrcmpA(LPCSTR lpString1, LPCSTR lpString2);
int __stdcall lstrcmpiA(LPCSTR lpString1, LPCSTR lpString2);
LPSTR __stdcall lstrcpyA(LPSTR lpString1, LPCSTR lpString2);
LPSTR __stdcall lstrcpynA(LPSTR lpString1, LPCSTR lpString2, int cchMax);
LPSTR __stdcall lstrcatA(LPSTR lpString1, LPCSTR lpString2);
int __stdcall lstrlenA(LPCSTR lpString);
LPSTR __stdcall lstrchrA(LPCSTR lpString, char c);

// Process/Thread
HANDLE __stdcall CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
DWORD __stdcall GetCurrentThreadId();
DWORD __stdcall GetCurrentProcessId();
HANDLE __stdcall GetCurrentProcess();
DWORD __stdcall WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
BOOL __stdcall TerminateThread(HANDLE hThread, DWORD dwExitCode);
BOOL __stdcall GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode);
void __stdcall Sleep(DWORD dwMilliseconds);

// Synchronization
HANDLE __stdcall CreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName);
HANDLE __stdcall CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
BOOL __stdcall SetEvent(HANDLE hEvent);
BOOL __stdcall ResetEvent(HANDLE hEvent);
DWORD __stdcall WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
BOOL __stdcall ReleaseMutex(HANDLE hMutex);
BOOL __stdcall CloseHandle(HANDLE hObject);

// Library
HMODULE __stdcall LoadLibraryA(LPCSTR lpLibFileName);
HMODULE __stdcall LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
FARPROC __stdcall GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
BOOL __stdcall FreeLibrary(HMODULE hModule);

// Error
DWORD __stdcall GetLastError();
void __stdcall SetLastError(DWORD dwErrorCode);

// Math
double __stdcall pow(double x, double y);
double __stdcall sqrt(double x);
double __stdcall sin(double x);
double __stdcall cos(double x);
double __stdcall tan(double x);
double __stdcall asin(double x);
double __stdcall acos(double x);
double __stdcall atan(double x);
double __stdcall atan2(double y, double x);
double __stdcall exp(double x);
double __stdcall log(double x);
double __stdcall log10(double x);
double __stdcall floor(double x);
double __stdcall ceil(double x);
double __stdcall fabs(double x);

// String formatting
int __stdcall _snprintf(char* buffer, size_t count, const char* format, ...);
int __stdcall _vsnprintf(char* buffer, size_t count, const char* format, va_list argptr);
int __stdcall sprintf(char* buffer, const char* format, ...);
int __stdcall vsprintf(char* buffer, const char* format, va_list argptr);
int __stdcall _stricmp(const char* s1, const char* s2);
int __stdcall _strcmpi(const char* s1, const char* s2);
int __stdcall strcmp(const char* s1, const char* s2);
int __stdcall strncmp(const char* s1, const char* s2, size_t n);
char* __stdcall strcpy(char* dest, const char* src);
char* __stdcall strncpy(char* dest, const char* src, size_t n);
char* __stdcall strcat(char* dest, const char* src);
char* __stdcall strchr(const char* s, int c);
char* __stdcall strrchr(const char* s, int c);
size_t __stdcall strlen(const char* s);
char* __stdcall _strlwr(char* s);
char* __stdcall _strupr(char* s);

// Memory
void* __stdcall malloc(size_t size);
void __stdcall free(void* ptr);
void* __stdcall realloc(void* ptr, size_t size);
void* __stdcall memset(void* ptr, int value, size_t num);
void* __stdcall memcpy(void* dest, const void* src, size_t count);
void* __stdcall memmove(void* dest, const void* src, size_t count);
int __stdcall memcmp(const void* ptr1, const void* ptr2, size_t num);

// File I/O
FILE* __stdcall fopen(const char* filename, const char* mode);
int __stdcall fclose(FILE* stream);
size_t __stdcall fread(void* ptr, size_t size, size_t count, FILE* stream);
size_t __stdcall fwrite(const void* ptr, size_t size, size_t count, FILE* stream);
int __stdcall fseek(FILE* stream, long offset, int origin);
long __stdcall ftell(FILE* stream);
int __stdcall fflush(FILE* stream);
int __stdcall feof(FILE* stream);
int __stdcall ferror(FILE* stream);

// Time
time_t __stdcall time(time_t* timer);
struct tm* __stdcall localtime(const time_t* timer);
time_t __stdcall mktime(struct tm* timeptr);
char* __stdcall ctime(const time_t* timer);
char* __stdcall asctime(const struct tm* timeptr);
size_t __stdcall strftime(char* s, size_t maxsize, const char* format, const struct tm* tm);

// Math constants
#define M_PI 3.14159265358979323846
#define M_E 2.71828182845904523536

#endif // !defined(_WIN32)

#endif // D2_LINUX_API_H