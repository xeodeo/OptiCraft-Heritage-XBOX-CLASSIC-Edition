#pragma once

// The one way Xbox backend code includes the XDK: #include "XboxXtl.h", never
// <xtl.h> directly.
//
// The XDK's WinBase.h maps InterlockedIncrement & co. onto the compiler
// intrinsics (#define InterlockedIncrement _InterlockedIncrement) and then
// declares them with 2003 signatures (LONG*). The VS2022 STL declares the same
// intrinsics with `volatile long*`, so the two cannot coexist in one TU.
// Rename the XDK's declarations to inert names while XTL.h is parsed, then
// restore the intrinsic mapping so backend code gets the modern declarations.
// XboxCrtShim.cpp defines the renamed functions for any XDK inline helper that
// still references them.

#include <intrin.h>

#define _InterlockedCompareExchange XdkInterlockedCompareExchange
#define _InterlockedDecrement XdkInterlockedDecrement
#define _InterlockedExchange XdkInterlockedExchange
#define _InterlockedExchangeAdd XdkInterlockedExchangeAdd
#define _InterlockedIncrement XdkInterlockedIncrement

#include <xtl.h>

#undef _InterlockedCompareExchange
#undef _InterlockedDecrement
#undef _InterlockedExchange
#undef _InterlockedExchangeAdd
#undef _InterlockedIncrement
