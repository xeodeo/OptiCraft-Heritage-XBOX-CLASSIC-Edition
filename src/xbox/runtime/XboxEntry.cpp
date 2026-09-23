// Xbox process entry for images built with the VS2022 CRT.
//
// Mirrors the XDK's xapi0.c startup:
//   1. On the kernel's initial thread (which has no TLS block), size the
//      static TLS block from _tls_used and store the Xbox TLS index.
//   2. Start the real main thread through XAPI CreateThread, which allocates
//      that TLS block, and let the initial thread return.
//   3. On the new thread: kernel patches, XAPI init, XDK runtime initializers
//      (.CRT$RI*), then the modern CRT startup, which initializes the UCRT,
//      runs C++ static constructors (.CRT$XC*) and calls main().

#include <stddef.h>
#include <stdint.h>

extern "C" {

struct TlsDirectory {
    uintptr_t StartAddressOfRawData;
    uintptr_t EndAddressOfRawData;
    uintptr_t AddressOfIndex;
    uintptr_t AddressOfCallBacks;
    uint32_t SizeOfZeroFill;
    uint32_t Characteristics;
};

extern const TlsDirectory _tls_used;  // vcruntime tlssup.obj
extern unsigned long XapiTlsSize;     // xapilib

void __stdcall XapiApplyKernelPatches(void);
void __stdcall XapiInitProcess(void);
void __stdcall XapiBootToDash(unsigned long reason, unsigned long param1, unsigned long param2);
void* __stdcall CreateThread(void* sa, size_t stack, unsigned long(__stdcall* start)(void*), void* param, unsigned long flags, unsigned long* id);
int __stdcall CloseHandle(void* h);

int mainCRTStartup(void);  // VS2022 libcmt: UCRT init, static ctors, main()

// XDK library initializers live in .CRT$RIA..RIZ and were run by the XDK's
// _rtinit. xapi0dat.obj (which defines these bounds) also defines the
// .CRT$XI/XC bounds and would clash with the modern CRT, so define our own.
typedef void(__cdecl* InitFn)(void);
#pragma section(".CRT$RIA", long, read)
#pragma section(".CRT$RIZ", long, read)
__declspec(allocate(".CRT$RIA")) InitFn __xri_a = nullptr;
__declspec(allocate(".CRT$RIZ")) InitFn __xri_z = nullptr;

// C++ static initializers compiled into XDK libraries (DirectSound's memory
// counters, for one) sit in read-write .CRT$XCU contributions. The modern CRT
// table (__xc_a..__xc_z) is read-only data, so the linker groups the XDK ones
// into a separate .CRT section that mainCRTStartup never walks. These markers
// carry the same read-write attributes, so they land in that group and
// bracket it (XCT* < XCU < XCV* in section order).
#pragma section(".CRT$XCT_XDK", long, read, write)
#pragma section(".CRT$XCV_XDK", long, read, write)
__declspec(allocate(".CRT$XCT_XDK")) InitFn __xdk_xc_a = nullptr;
__declspec(allocate(".CRT$XCV_XDK")) InitFn __xdk_xc_z = nullptr;

static void RunXdkInitializers(void)
{
    for (InitFn* p = &__xri_a + 1; p < &__xri_z; ++p) {
        if (*p && *p != (InitFn)(intptr_t)-1) {
            (*p)();
        }
    }
    for (InitFn* p = &__xdk_xc_a + 1; p < &__xdk_xc_z; ++p) {
        if (*p) {
            (*p)();
        }
    }
}

static unsigned long __stdcall XboxMainThread(void*)
{
    XapiApplyKernelPatches();
    XapiInitProcess();
    RunXdkInitializers();
    mainCRTStartup();  // calls exit(), which does not return
    XapiBootToDash(1, 1, 0);
    return 0;
}

void XboxEntry(void)
{
    // Same arithmetic as the XDK: 16-byte aligned raw TLS data + zero fill,
    // plus a 4-byte slot; the index is negative so fs:[28h]-relative TLS
    // lookups land in the block XAPI places just below the thread's TlsData.
    unsigned long raw = (unsigned long)(_tls_used.EndAddressOfRawData - _tls_used.StartAddressOfRawData);
    unsigned long size = ((raw + _tls_used.SizeOfZeroFill + 15) & ~15ul) + 4;
    XapiTlsSize = size;
    *(long*)_tls_used.AddressOfIndex = (long)size / -4;

    void* thread = CreateThread(nullptr, 0, XboxMainThread, nullptr, 0, nullptr);
    if (!thread) {
        XapiBootToDash(1, 1, 0);
    }
    CloseHandle(thread);
}

}  // extern "C"
