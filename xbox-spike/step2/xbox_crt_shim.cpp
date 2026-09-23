// Runtime glue between the VS2022 static CRT (UCRT + vcruntime + libcpmt) and
// the Xbox XAPI/kernel. Every Win32 import the modern CRT needs but XAPI does
// not export is implemented here; xbox_imports.asm publishes the matching
// __imp__Name@N pointers so dllimport call sites resolve statically.
//
// No Windows or XDK headers on purpose: the modern CRT's view of Win32 types
// (x86 desktop layout) is what matters, and we declare kernel calls ourselves.

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <intrin.h>

typedef int BOOL;
typedef unsigned long DWORD;
typedef long LONG;
typedef unsigned short WORD;
typedef void* HANDLE;
typedef wchar_t WCHAR;
typedef unsigned int UINT;

#define TRUE 1
#define FALSE 0
#define WINAPI __stdcall
#define NTAPI __stdcall
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define MAX_PATH 260

// ---------------------------------------------------------------------------
// Xbox kernel (xboxkrnl.exe) and XAPI (xapilib.lib) functions we call.
// ---------------------------------------------------------------------------
struct XBOX_RTL_CRITICAL_SECTION {
    uint8_t Synchronization[16];  // KEVENT dispatcher header
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
};

struct XBOX_EXCEPTION_RECORD {
    DWORD ExceptionCode;
    DWORD ExceptionFlags;
    XBOX_EXCEPTION_RECORD* ExceptionRecord;
    void* ExceptionAddress;
    DWORD NumberParameters;
    uintptr_t ExceptionInformation[15];
};

struct XBOX_MEMORY_BASIC_INFORMATION {
    void* BaseAddress;
    void* AllocationBase;
    DWORD AllocationProtect;
    size_t RegionSize;
    DWORD State;
    DWORD Protect;
    DWORD Type;
};

extern "C" {
void NTAPI RtlInitializeCriticalSection(XBOX_RTL_CRITICAL_SECTION* cs);
void NTAPI RtlEnterCriticalSection(XBOX_RTL_CRITICAL_SECTION* cs);
void NTAPI RtlLeaveCriticalSection(XBOX_RTL_CRITICAL_SECTION* cs);
BOOL NTAPI RtlTryEnterCriticalSection(XBOX_RTL_CRITICAL_SECTION* cs);
void NTAPI RtlRaiseException(XBOX_EXCEPTION_RECORD* rec);
void NTAPI KeQuerySystemTime(int64_t* time);
void* NTAPI KeGetCurrentThread(void);
LONG NTAPI NtYieldExecution(void);
LONG NTAPI NtQueryVirtualMemory(void* base, XBOX_MEMORY_BASIC_INFORMATION* info);
LONG NTAPI NtProtectVirtualMemory(void** base, size_t* size, DWORD newProtect, DWORD* oldProtect);
void NTAPI HalReturnToFirmware(int routine);

void* NTAPI RtlAllocateHeap(HANDLE heap, DWORD flags, size_t size);
void* NTAPI RtlReAllocateHeap(HANDLE heap, DWORD flags, void* p, size_t size);
size_t NTAPI RtlSizeHeap(HANDLE heap, DWORD flags, const void* p);

// XAPI Win32 subset.
DWORD WINAPI GetTickCount(void);
DWORD WINAPI TlsAlloc(void);
BOOL WINAPI TlsFree(DWORD);
void* WINAPI TlsGetValue(DWORD);
BOOL WINAPI TlsSetValue(DWORD, void*);
void WINAPI SetLastError(DWORD);
DWORD WINAPI GetLastError(void);
void WINAPI OutputDebugStringA(const char*);
BOOL WINAPI CloseHandle(HANDLE);
HANDLE WINAPI CreateFileA(const char*, DWORD, DWORD, void*, DWORD, DWORD, HANDLE);
HANDLE WINAPI CreateEventA(void*, BOOL, BOOL, const char*);
HANDLE WINAPI CreateSemaphoreA(void*, LONG, LONG, const char*);
void WINAPI Sleep(DWORD);
HANDLE WINAPI GetProcessHeap(void);
__declspec(noreturn) void WINAPI ExitThread(DWORD);
BOOL WINAPI HeapFree(HANDLE, DWORD, void*);
}

#define ERROR_NOT_SUPPORTED 50
#define ERROR_CALL_NOT_IMPLEMENTED 120
#define ERROR_INVALID_PARAMETER 87
#define ERROR_INSUFFICIENT_BUFFER 122

// ---------------------------------------------------------------------------
// Control Flow Guard / CastGuard: the Xbox has neither, and guard_support.obj
// carries 64-byte aligned sections, so satisfy its symbols here.
// ---------------------------------------------------------------------------
extern "C" {
static void __fastcall GuardCheckNop(void*) {}
static void __cdecl CastGuardNop(void*) {}
void* __guard_check_icall_fptr = (void*)&GuardCheckNop;
void* __guard_xfg_check_icall_fptr = (void*)&GuardCheckNop;
void* __castguard_check_failure_os_handled_fptr = (void*)&CastGuardNop;
void* __castguard_check_failure_user_handled_fptr = (void*)&CastGuardNop;
int __CastGuardVftablesStart = 0;
int __CastGuardVftablesEnd = 0;
}

// ---------------------------------------------------------------------------
// Small helpers.
// ---------------------------------------------------------------------------
namespace {

// Wide path -> narrow. Xbox file names are ASCII; anything else becomes '_'.
bool narrowPath(const WCHAR* in, char* out, size_t outLen)
{
    if (!in) {
        return false;
    }
    size_t i = 0;
    for (; in[i] && i + 1 < outLen; ++i) {
        out[i] = (in[i] < 0x80) ? (char)in[i] : '_';
    }
    out[i] = 0;
    return in[i] == 0;
}

void widenString(const char* in, WCHAR* out, size_t outLen)
{
    size_t i = 0;
    for (; in[i] && i + 1 < outLen; ++i) {
        out[i] = (WCHAR)(unsigned char)in[i];
    }
    out[i] = 0;
}

HANDLE processHeap()
{
    return GetProcessHeap();
}

}  // namespace

extern "C" {

// ---------------------------------------------------------------------------
// Critical sections. The Xbox RTL_CRITICAL_SECTION (28 bytes) is larger than
// the desktop x86 CRITICAL_SECTION (24 bytes) the CRT allocates, so the CRT's
// storage holds a pointer to a heap-allocated kernel critical section.
// ---------------------------------------------------------------------------
struct DesktopCriticalSection {
    XBOX_RTL_CRITICAL_SECTION* impl;
    uint32_t pad[5];
};
static_assert(sizeof(DesktopCriticalSection) == 24, "desktop CRITICAL_SECTION is 24 bytes on x86");

BOOL WINAPI InitializeCriticalSectionEx(DesktopCriticalSection* cs, DWORD, DWORD)
{
    auto* impl = (XBOX_RTL_CRITICAL_SECTION*)RtlAllocateHeap(processHeap(), 0, sizeof(XBOX_RTL_CRITICAL_SECTION));
    if (!impl) {
        return FALSE;
    }
    RtlInitializeCriticalSection(impl);
    cs->impl = impl;
    return TRUE;
}

BOOL WINAPI InitializeCriticalSectionAndSpinCount(DesktopCriticalSection* cs, DWORD spin)
{
    return InitializeCriticalSectionEx(cs, spin, 0);
}

void WINAPI InitializeCriticalSection(DesktopCriticalSection* cs)
{
    InitializeCriticalSectionEx(cs, 0, 0);
}

void WINAPI EnterCriticalSection(DesktopCriticalSection* cs)
{
    RtlEnterCriticalSection(cs->impl);
}

BOOL WINAPI TryEnterCriticalSection(DesktopCriticalSection* cs)
{
    return RtlTryEnterCriticalSection(cs->impl);
}

void WINAPI LeaveCriticalSection(DesktopCriticalSection* cs)
{
    RtlLeaveCriticalSection(cs->impl);
}

void WINAPI DeleteCriticalSection(DesktopCriticalSection* cs)
{
    if (cs->impl) {
        HeapFree(processHeap(), 0, cs->impl);
        cs->impl = nullptr;
    }
}

// ---------------------------------------------------------------------------
// SRW locks (exclusive only is used by the CRT/STL): pointer-sized spin lock.
// ---------------------------------------------------------------------------
void WINAPI AcquireSRWLockExclusive(volatile LONG* lock)
{
    while (_InterlockedCompareExchange(lock, 1, 0) != 0) {
        NtYieldExecution();
    }
}

BOOL WINAPI TryAcquireSRWLockExclusive(volatile LONG* lock)
{
    return _InterlockedCompareExchange(lock, 1, 0) == 0;
}

void WINAPI ReleaseSRWLockExclusive(volatile LONG* lock)
{
    _InterlockedExchange(lock, 0);
}

// ---------------------------------------------------------------------------
// One-time init.
// ---------------------------------------------------------------------------
typedef BOOL(WINAPI* InitOnceFn)(void*, void*, void**);

BOOL WINAPI InitOnceExecuteOnce(volatile LONG* once, InitOnceFn fn, void* param, void** ctx)
{
    // 0 = not run, 1 = running, 2 = done.
    for (;;) {
        LONG prev = _InterlockedCompareExchange(once, 1, 0);
        if (prev == 0) {
            BOOL ok = fn((void*)once, param, ctx);
            _InterlockedExchange(once, ok ? 2 : 0);
            return ok;
        }
        if (prev == 2) {
            return TRUE;
        }
        NtYieldExecution();
    }
}

// ---------------------------------------------------------------------------
// Interlocked singly linked lists (type_info name cache): spin-protected.
// SLIST_HEADER on x86 is 8 bytes: {Next, Depth:16, Sequence:16}.
// ---------------------------------------------------------------------------
struct SListEntry {
    SListEntry* Next;
};
struct SListHeader {
    SListEntry* Next;
    volatile LONG lock;
};

void WINAPI InitializeSListHead(SListHeader* head)
{
    head->Next = nullptr;
    head->lock = 0;
}

SListEntry* WINAPI InterlockedPushEntrySList(SListHeader* head, SListEntry* entry)
{
    AcquireSRWLockExclusive(&head->lock);
    SListEntry* prev = head->Next;
    entry->Next = prev;
    head->Next = entry;
    ReleaseSRWLockExclusive(&head->lock);
    return prev;
}

SListEntry* WINAPI InterlockedFlushSList(SListHeader* head)
{
    AcquireSRWLockExclusive(&head->lock);
    SListEntry* all = head->Next;
    head->Next = nullptr;
    ReleaseSRWLockExclusive(&head->lock);
    return all;
}

// ---------------------------------------------------------------------------
// TLS / FLS. FLS maps onto TLS; fiber-local destructor callbacks are dropped
// (per-thread CRT data leaks when a thread exits, which the game tolerates).
// ---------------------------------------------------------------------------
DWORD WINAPI FlsAlloc(void*)
{
    return TlsAlloc();
}
BOOL WINAPI FlsFree(DWORD i)
{
    return TlsFree(i);
}
void* WINAPI FlsGetValue(DWORD i)
{
    return TlsGetValue(i);
}
BOOL WINAPI FlsSetValue(DWORD i, void* v)
{
    return TlsSetValue(i, v);
}
BOOL WINAPI IsThreadAFiber(void)
{
    return FALSE;
}

// ---------------------------------------------------------------------------
// Heap: HeapAlloc & co. are macros over Rtl*Heap in XAPI.
// ---------------------------------------------------------------------------
void* WINAPI HeapAlloc(HANDLE heap, DWORD flags, size_t size)
{
    return RtlAllocateHeap(heap, flags, size);
}
void* WINAPI HeapReAlloc(HANDLE heap, DWORD flags, void* p, size_t size)
{
    return RtlReAllocateHeap(heap, flags, p, size);
}
size_t WINAPI HeapSize(HANDLE heap, DWORD flags, const void* p)
{
    return RtlSizeHeap(heap, flags, p);
}

// ---------------------------------------------------------------------------
// Process / thread identity and lifetime.
// ---------------------------------------------------------------------------
HANDLE WINAPI GetCurrentProcess(void)
{
    return (HANDLE)(intptr_t)-1;
}
DWORD WINAPI GetCurrentProcessId(void)
{
    return 1;
}
HANDLE WINAPI GetCurrentThread(void)
{
    return (HANDLE)(intptr_t)-2;
}
DWORD WINAPI GetCurrentProcessorNumber(void)
{
    return 0;
}
void WINAPI FlushProcessWriteBuffers(void)
{
    volatile LONG barrier = 0;
    _InterlockedExchange(&barrier, 1);
}

__declspec(noreturn) void WINAPI ExitProcess(UINT code)
{
    char msg[64];
    const char* prefix = "OptiCraft: ExitProcess ";
    size_t n = strlen(prefix);
    memcpy(msg, prefix, n);
    msg[n] = (char)('0' + (code % 10));
    msg[n + 1] = '\n';
    msg[n + 2] = 0;
    OutputDebugStringA(msg);
    HalReturnToFirmware(2);  // HalQuickRebootRoutine: back to the dashboard
    for (;;) {
    }
}

BOOL WINAPI TerminateProcess(HANDLE, UINT code)
{
    ExitProcess(code);
}

// ---------------------------------------------------------------------------
// Exceptions / diagnostics.
// ---------------------------------------------------------------------------
BOOL WINAPI IsDebuggerPresent(void)
{
    return FALSE;
}
BOOL WINAPI IsProcessorFeaturePresent(DWORD feature)
{
    // Pentium III: MMX (3) and SSE (6) yes; SSE2 (10), fastfail (23) no.
    return feature == 3 || feature == 6;
}
void* WINAPI EncodePointer(void* p)
{
    return p;
}
void* WINAPI DecodePointer(void* p)
{
    return p;
}
void WINAPI OutputDebugStringW(const WCHAR* s)
{
    char buf[512];
    narrowPath(s, buf, sizeof(buf));
    OutputDebugStringA(buf);
}
DWORD WINAPI FormatMessageA(DWORD, const void*, DWORD, DWORD, char* buf, DWORD size, void*)
{
    if (buf && size) {
        buf[0] = 0;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Time.
// ---------------------------------------------------------------------------
void WINAPI GetSystemTimeAsFileTime(int64_t* ft)
{
    KeQuerySystemTime(ft);
}
uint64_t WINAPI GetTickCount64(void)
{
    // GetTickCount wraps after 49.7 days of uptime; fine for a console session.
    return GetTickCount();
}
int WINAPI GetDateFormatW(DWORD, DWORD, const void*, const WCHAR*, WCHAR*, int)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return 0;
}
int WINAPI GetTimeFormatW(DWORD, DWORD, const void*, const WCHAR*, WCHAR*, int)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return 0;
}

// ---------------------------------------------------------------------------
// Virtual memory (Xbox Nt* calls take no process handle).
// ---------------------------------------------------------------------------
size_t WINAPI VirtualQuery(const void* addr, XBOX_MEMORY_BASIC_INFORMATION* info, size_t len)
{
    if (len < sizeof(*info)) {
        return 0;
    }
    return NtQueryVirtualMemory((void*)addr, info) >= 0 ? sizeof(*info) : 0;
}
BOOL WINAPI VirtualProtect(void* addr, size_t size, DWORD prot, DWORD* old)
{
    void* base = addr;
    size_t sz = size;
    return NtProtectVirtualMemory(&base, &sz, prot, old) >= 0;
}

// ---------------------------------------------------------------------------
// Modules: there is exactly one image and no DLL loader.
// ---------------------------------------------------------------------------
HANDLE WINAPI GetModuleHandleW(const WCHAR*)
{
    return nullptr;
}
BOOL WINAPI GetModuleHandleExW(DWORD, const WCHAR*, HANDLE* out)
{
    if (out) {
        *out = nullptr;
    }
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
HANDLE WINAPI LoadLibraryExW(const WCHAR*, HANDLE, DWORD)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return nullptr;
}
BOOL WINAPI FreeLibrary(HANDLE)
{
    return TRUE;
}
void* WINAPI GetProcAddress(HANDLE, const char*)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return nullptr;
}
DWORD WINAPI GetModuleFileNameW(HANDLE, WCHAR* buf, DWORD size)
{
    static const char kPath[] = "D:\\default.xbe";
    if (!buf || size == 0) {
        return 0;
    }
    widenString(kPath, buf, size);
    return (DWORD)wcslen(buf);
}

// ---------------------------------------------------------------------------
// Process environment: no command line, no environment, no console.
// ---------------------------------------------------------------------------
char* WINAPI GetCommandLineA(void)
{
    static char cmd[] = "default.xbe";
    return cmd;
}
WCHAR* WINAPI GetCommandLineW(void)
{
    static WCHAR cmd[] = L"default.xbe";
    return cmd;
}
WCHAR* WINAPI GetEnvironmentStringsW(void)
{
    static WCHAR empty[2] = {0, 0};
    return empty;
}
BOOL WINAPI FreeEnvironmentStringsW(WCHAR*)
{
    return TRUE;
}
BOOL WINAPI SetEnvironmentVariableW(const WCHAR*, const WCHAR*)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

struct StartupInfoW {
    DWORD cb;
    uint8_t rest[64];
};
void WINAPI GetStartupInfoW(StartupInfoW* si)
{
    memset(si, 0, sizeof(*si));
    si->cb = sizeof(*si);
}

HANDLE WINAPI GetStdHandle(DWORD)
{
    return INVALID_HANDLE_VALUE;
}
BOOL WINAPI SetStdHandle(DWORD, HANDLE)
{
    return TRUE;
}
DWORD WINAPI GetFileType(HANDLE h)
{
    // FILE_TYPE_DISK for real handles so fopen'd files behave; unknown otherwise.
    return (h && h != INVALID_HANDLE_VALUE) ? 1 : 0;
}
BOOL WINAPI GetConsoleMode(HANDLE, DWORD*)
{
    return FALSE;
}
UINT WINAPI GetConsoleOutputCP(void)
{
    return 0;
}
BOOL WINAPI ReadConsoleW(HANDLE, void*, DWORD, DWORD* read, void*)
{
    if (read) {
        *read = 0;
    }
    return FALSE;
}
BOOL WINAPI WriteConsoleW(HANDLE, const void*, DWORD, DWORD* written, void*)
{
    if (written) {
        *written = 0;
    }
    return FALSE;
}
BOOL WINAPI SetConsoleCtrlHandler(void*, BOOL)
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// Code pages & locale: single-byte Latin-1 "ACP", plus real UTF-8.
// ---------------------------------------------------------------------------
#define CP_UTF8 65001

UINT WINAPI GetACP(void)
{
    return 1252;
}
UINT WINAPI GetOEMCP(void)
{
    return 437;
}
BOOL WINAPI IsValidCodePage(UINT cp)
{
    return cp == 1252 || cp == 437 || cp == CP_UTF8 || cp == 20127;
}

struct CpInfo {
    UINT MaxCharSize;
    uint8_t DefaultChar[2];
    uint8_t LeadByte[12];
};
BOOL WINAPI GetCPInfo(UINT cp, CpInfo* info)
{
    memset(info, 0, sizeof(*info));
    info->MaxCharSize = (cp == CP_UTF8) ? 4 : 1;
    info->DefaultChar[0] = '?';
    return TRUE;
}

int WINAPI MultiByteToWideChar(UINT cp, DWORD, const char* src, int srcLen, WCHAR* dst, int dstLen)
{
    if (!src) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (srcLen < 0) {
        srcLen = (int)strlen(src) + 1;
    }
    int out = 0;
    const unsigned char* s = (const unsigned char*)src;
    int i = 0;
    while (i < srcLen) {
        uint32_t c = s[i++];
        if (cp == CP_UTF8 && c >= 0x80) {
            int extra = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : 0;
            c &= (extra == 3) ? 0x07 : (extra == 2) ? 0x0F : 0x1F;
            for (int k = 0; k < extra && i < srcLen; ++k) {
                c = (c << 6) | (s[i++] & 0x3F);
            }
            if (c > 0xFFFF) {
                c = '?';  // outside BMP: no surrogate pairs needed for game text
            }
        }
        if (dstLen) {
            if (out >= dstLen) {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            dst[out] = (WCHAR)c;
        }
        ++out;
    }
    return out;
}

int WINAPI WideCharToMultiByte(UINT cp, DWORD, const WCHAR* src, int srcLen, char* dst, int dstLen, const char*, BOOL* usedDefault)
{
    if (!src) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (srcLen < 0) {
        srcLen = (int)wcslen(src) + 1;
    }
    if (usedDefault) {
        *usedDefault = FALSE;
    }
    int out = 0;
    for (int i = 0; i < srcLen; ++i) {
        uint32_t c = src[i];
        unsigned char buf[3];
        int n;
        if (cp == CP_UTF8) {
            if (c < 0x80) {
                buf[0] = (unsigned char)c;
                n = 1;
            } else if (c < 0x800) {
                buf[0] = (unsigned char)(0xC0 | (c >> 6));
                buf[1] = (unsigned char)(0x80 | (c & 0x3F));
                n = 2;
            } else {
                buf[0] = (unsigned char)(0xE0 | (c >> 12));
                buf[1] = (unsigned char)(0x80 | ((c >> 6) & 0x3F));
                buf[2] = (unsigned char)(0x80 | (c & 0x3F));
                n = 3;
            }
        } else {
            if (c > 0xFF) {
                c = '?';
                if (usedDefault) {
                    *usedDefault = TRUE;
                }
            }
            buf[0] = (unsigned char)c;
            n = 1;
        }
        if (dstLen) {
            if (out + n > dstLen) {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            memcpy(dst + out, buf, (size_t)n);
        }
        out += n;
    }
    return out;
}

DWORD WINAPI GetUserDefaultLCID(void)
{
    return 0x0409;
}
BOOL WINAPI IsValidLocale(DWORD, DWORD)
{
    return TRUE;
}
BOOL WINAPI EnumSystemLocalesW(void*, DWORD)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
int WINAPI GetLocaleInfoW(DWORD, DWORD, WCHAR* buf, int len)
{
    if (buf && len) {
        buf[0] = 0;
    }
    SetLastError(ERROR_NOT_SUPPORTED);
    return 0;
}
int WINAPI GetLocaleInfoEx(const WCHAR*, DWORD, WCHAR* buf, int len)
{
    return GetLocaleInfoW(0, 0, buf, len);
}

int WINAPI CompareStringW(DWORD, DWORD, const WCHAR* a, int aLen, const WCHAR* b, int bLen)
{
    if (aLen < 0) {
        aLen = (int)wcslen(a);
    }
    if (bLen < 0) {
        bLen = (int)wcslen(b);
    }
    int n = aLen < bLen ? aLen : bLen;
    for (int i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? 1 : 3;  // CSTR_LESS_THAN / CSTR_GREATER_THAN
        }
    }
    return aLen == bLen ? 2 : (aLen < bLen ? 1 : 3);
}

#define LCMAP_LOWERCASE 0x100
#define LCMAP_UPPERCASE 0x200

int WINAPI LCMapStringW(DWORD, DWORD flags, const WCHAR* src, int srcLen, WCHAR* dst, int dstLen)
{
    if (srcLen < 0) {
        srcLen = (int)wcslen(src) + 1;
    }
    if (dstLen == 0) {
        return srcLen;
    }
    if (dstLen < srcLen) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }
    for (int i = 0; i < srcLen; ++i) {
        WCHAR c = src[i];
        if ((flags & LCMAP_LOWERCASE) && c >= 'A' && c <= 'Z') {
            c = (WCHAR)(c + 32);
        } else if ((flags & LCMAP_UPPERCASE) && c >= 'a' && c <= 'z') {
            c = (WCHAR)(c - 32);
        }
        dst[i] = c;
    }
    return srcLen;
}

BOOL WINAPI GetStringTypeW(DWORD type, const WCHAR* src, int len, WORD* out)
{
    // CT_CTYPE1 classification for Latin-1; everything above is "alpha".
    enum { C1_UPPER = 1, C1_LOWER = 2, C1_DIGIT = 4, C1_SPACE = 8, C1_PUNCT = 16, C1_CNTRL = 32, C1_BLANK = 64, C1_XDIGIT = 128, C1_ALPHA = 256, C1_DEFINED = 512 };
    if (type != 1) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }
    if (len < 0) {
        len = (int)wcslen(src) + 1;
    }
    for (int i = 0; i < len; ++i) {
        WCHAR c = src[i];
        WORD t = C1_DEFINED;
        if (c >= 'A' && c <= 'Z') {
            t |= C1_UPPER | C1_ALPHA;
        } else if (c >= 'a' && c <= 'z') {
            t |= C1_LOWER | C1_ALPHA;
        } else if (c >= '0' && c <= '9') {
            t |= C1_DIGIT;
        } else if (c == ' ' || c == '\t') {
            t |= C1_SPACE | C1_BLANK;
        } else if (c >= '\n' && c <= '\r') {
            t |= C1_SPACE | C1_CNTRL;
        } else if (c < 0x20 || c == 0x7F) {
            t |= C1_CNTRL;
        } else if (c < 0x80) {
            t |= C1_PUNCT;
        } else if (c >= 0xC0) {
            t |= C1_ALPHA;
        }
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            t |= C1_XDIGIT;
        }
        out[i] = t;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// Synchronization objects with Ex/W signatures.
// ---------------------------------------------------------------------------
HANDLE WINAPI CreateEventExW(void* attrs, const WCHAR*, DWORD flags, DWORD)
{
    // CREATE_EVENT_MANUAL_RESET = 1, CREATE_EVENT_INITIAL_SET = 2
    return CreateEventA(attrs, (flags & 1) != 0, (flags & 2) != 0, nullptr);
}
HANDLE WINAPI CreateSemaphoreExW(void* attrs, LONG initial, LONG maximum, const WCHAR*, DWORD, DWORD)
{
    return CreateSemaphoreA(attrs, initial, maximum, nullptr);
}

// Thread pool: unavailable. The STL only uses these for timed waits on newer
// OSes and falls back when creation fails.
void* WINAPI CreateThreadpoolTimer(void*, void*, void*)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return nullptr;
}
void* WINAPI CreateThreadpoolWait(void*, void*, void*)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return nullptr;
}
void WINAPI SetThreadpoolTimer(void*, void*, DWORD, DWORD) {}
void WINAPI SetThreadpoolWait(void*, HANDLE, void*) {}
void WINAPI CloseThreadpoolTimer(void*) {}
void WINAPI CloseThreadpoolWait(void*) {}
void WINAPI WaitForThreadpoolTimerCallbacks(void*, BOOL) {}
void WINAPI FreeLibraryWhenCallbackReturns(void*, HANDLE) {}

// ---------------------------------------------------------------------------
// Files: wide entry points forward to XAPI's ANSI versions.
// ---------------------------------------------------------------------------
HANDLE WINAPI CreateFileW(const WCHAR* name, DWORD access, DWORD share, void* sa, DWORD disp, DWORD flags, HANDLE tmpl)
{
    char path[MAX_PATH];
    if (!narrowPath(name, path, sizeof(path))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    return CreateFileA(path, access, share, sa, disp, flags, tmpl);
}

struct FindDataA {
    DWORD attrs;
    int64_t created, accessed, written;
    DWORD sizeHigh, sizeLow, reserved0, reserved1;
    char name[MAX_PATH];
    char altName[14];
};
struct FindDataW {
    DWORD attrs;
    int64_t created, accessed, written;
    DWORD sizeHigh, sizeLow, reserved0, reserved1;
    WCHAR name[MAX_PATH];
    WCHAR altName[14];
};

HANDLE WINAPI FindFirstFileA(const char*, FindDataA*);
BOOL WINAPI FindNextFileA(HANDLE, FindDataA*);

static void convertFindData(const FindDataA& a, FindDataW* w)
{
    w->attrs = a.attrs;
    w->created = a.created;
    w->accessed = a.accessed;
    w->written = a.written;
    w->sizeHigh = a.sizeHigh;
    w->sizeLow = a.sizeLow;
    w->reserved0 = a.reserved0;
    w->reserved1 = a.reserved1;
    widenString(a.name, w->name, MAX_PATH);
    widenString(a.altName, w->altName, 14);
}

HANDLE WINAPI FindFirstFileExW(const WCHAR* name, int, FindDataW* data, int, void*, DWORD)
{
    char path[MAX_PATH];
    if (!narrowPath(name, path, sizeof(path))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    FindDataA a;
    HANDLE h = FindFirstFileA(path, &a);
    if (h != INVALID_HANDLE_VALUE) {
        convertFindData(a, data);
    }
    return h;
}

BOOL WINAPI FindNextFileW(HANDLE h, FindDataW* data)
{
    FindDataA a;
    if (!FindNextFileA(h, &a)) {
        return FALSE;
    }
    convertFindData(a, data);
    return TRUE;
}

BOOL WINAPI FindClose(HANDLE h)
{
    return CloseHandle(h);
}

BOOL WINAPI GetFileInformationByHandleEx(HANDLE, int, void*, DWORD)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
BOOL WINAPI SetFileInformationByHandle(HANDLE, int, void*, DWORD)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
BOOL WINAPI CreateSymbolicLinkW(const WCHAR*, const WCHAR*, DWORD)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
DWORD WINAPI GetTempPathW(DWORD len, WCHAR* buf)
{
    static const char kTemp[] = "Z:\\";
    if (len < sizeof(kTemp)) {
        return sizeof(kTemp);
    }
    widenString(kTemp, buf, len);
    return sizeof(kTemp) - 1;
}

// ---------------------------------------------------------------------------
// Condition variables over the SRW spin lock: a generation counter that
// waiters poll with yields. Adequate for the game's worker handoff.
// ---------------------------------------------------------------------------
void WINAPI WakeConditionVariable(volatile LONG* cv)
{
    _InterlockedIncrement(cv);
}
void WINAPI WakeAllConditionVariable(volatile LONG* cv)
{
    _InterlockedIncrement(cv);
}
BOOL WINAPI SleepConditionVariableSRW(volatile LONG* cv, volatile LONG* lock, DWORD timeoutMs, DWORD flags)
{
    (void)flags;  // shared-mode waits are treated as exclusive
    LONG gen = *cv;
    ReleaseSRWLockExclusive(lock);
    DWORD start = GetTickCount();
    bool signaled = false;
    for (;;) {
        if (*cv != gen) {
            signaled = true;
            break;
        }
        if (timeoutMs != 0xFFFFFFFF && GetTickCount() - start >= timeoutMs) {
            break;
        }
        NtYieldExecution();
    }
    AcquireSRWLockExclusive(lock);
    if (!signaled) {
        SetLastError(1460);  // ERROR_TIMEOUT
        return FALSE;
    }
    return TRUE;
}

struct SystemInfo {
    WORD arch;
    WORD reserved;
    DWORD pageSize;
    void* minAddr;
    void* maxAddr;
    uintptr_t activeMask;
    DWORD numberOfProcessors;
    DWORD processorType;
    DWORD allocGranularity;
    WORD level;
    WORD revision;
};
void WINAPI GetNativeSystemInfo(SystemInfo* si)
{
    memset(si, 0, sizeof(*si));
    si->pageSize = 4096;
    si->minAddr = (void*)0x10000;
    si->maxAddr = (void*)0x7FFEFFFF;
    si->activeMask = 1;
    si->numberOfProcessors = 1;
    si->processorType = 586;  // PROCESSOR_INTEL_PENTIUM
    si->allocGranularity = 65536;
    si->level = 6;
}

int WINAPI LCMapStringEx(const WCHAR*, DWORD flags, const WCHAR* src, int srcLen, WCHAR* dst, int dstLen, void*, void*, intptr_t)
{
    return LCMapStringW(0, flags, src, srcLen, dst, dstLen);
}
int WINAPI CompareStringEx(const WCHAR*, DWORD flags, const WCHAR* a, int aLen, const WCHAR* b, int bLen, void*, void*, intptr_t)
{
    return CompareStringW(0, flags, a, aLen, b, bLen);
}

__declspec(noreturn) void WINAPI FreeLibraryAndExitThread(HANDLE, DWORD code)
{
    ExitThread(code);
}

}  // extern "C"

extern "C" {

// ---------------------------------------------------------------------------
// SEH validation. vcruntime bounds-checks SEH registration nodes
// (_ValidateEH3RN, used by _except_handler3) and exception CONTEXT / longjmp
// buffers (jbcxrval.obj) against the TIB stack range at fs:[4]/fs:[8], and
// inspects PE headers. On the Xbox fs:[4] is the TLS array (XDK
// __tls_array = 4) and the image is an XBE, so these checks always fail:
// exceptions go unhandled, or __fastfail (int 29h) hits a missing IDT entry
// and the kernel double faults (bug check 0x7F, arg 8). The kernel dispatcher
// already validates the chain against the real KTHREAD stack limits.
// ---------------------------------------------------------------------------
int __cdecl _ValidateEH3RN(void*)
{
    return 1;
}
void __cdecl __except_validate_context_record(void*) {}
void __cdecl __except_validate_jump_buffer(void*) {}

}  // extern "C"

extern "C" {

// UCRT peb_access.obj reads the Win32 PEB via fs:[18h]->+30h, which does not
// exist on the Xbox (kernel bug check 0x1E, access violation). There is no
// secure/elevated process mode and no application verifier.
bool __cdecl __acrt_is_secure_process(void)
{
    return false;
}
bool __cdecl __acrt_app_verifier_enabled(void)
{
    return false;
}

}  // extern "C"
