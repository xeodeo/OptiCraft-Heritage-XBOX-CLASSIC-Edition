#pragma once

// Per-thread copies of the little mutable state block rendering writes into the
// shared Block singletons (PLATFORM_THREADED_TERRAIN_MESH).
//
// RenderBlocks and Block::setBlockBoundsBasedOnState carve a block's pieces by
// rewriting its minX..maxZ, and the main thread does the same for collision,
// ray tracing and item rendering. The Legacy Console Edition kept a Tile's shape
// in thread-local storage for exactly this reason. Here the toolchain has no
// native TLS (gcc is built with --disable-tls; __thread would go through
// emutls and our pthread_getspecific, a syscall and a lock per access), so a
// value keeps two copies and picks one by the stack the caller runs on: the
// mesh worker's stack is a known range, recorded when the worker starts.
//
// Without the flag nothing here exists and Block keeps its plain doubles.

#include "platform/PlatformConfig.h"

#if PLATFORM_THREADED_TERRAIN_MESH

#include <cstdint>

// Stack range of the mesh worker thread; zero span while no worker runs. Written
// once by the worker as it starts (low first, then span), read by everyone.
extern std::uintptr_t g_platformMeshWorkerStackLow;
extern std::uintptr_t g_platformMeshWorkerStackSpan;

inline bool platformOnMeshWorker()
{
    std::uintptr_t sp;
#if defined(__powerpc64__) || defined(__PPU__)
    __asm__("mr %0,1" : "=r"(sp));
#else
    sp = reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0));
#endif
    return sp - g_platformMeshWorkerStackLow < g_platformMeshWorkerStackSpan;
}

// One value, one copy for the main thread and one for the mesh worker. Reads
// and writes go to the calling thread's copy; the main thread copies its value
// into the worker's (syncWorkerFromMain) before the worker first starts.
template <typename T>
class PlatformMeshThreadValue
{
public:
    explicit PlatformMeshThreadValue(T value = T()) : mainValue(value), workerValue(value) {}
    PlatformMeshThreadValue(const PlatformMeshThreadValue &) = default;

    // Assigning one Block's bound to another's must copy the calling thread's
    // view only, never both copies.
    PlatformMeshThreadValue &operator=(const PlatformMeshThreadValue &other)
    {
        current() = other.get();
        return *this;
    }
    PlatformMeshThreadValue &operator=(T value) { current() = value; return *this; }
    PlatformMeshThreadValue &operator+=(T value) { current() += value; return *this; }
    PlatformMeshThreadValue &operator-=(T value) { current() -= value; return *this; }
    PlatformMeshThreadValue &operator*=(T value) { current() *= value; return *this; }
    PlatformMeshThreadValue &operator/=(T value) { current() /= value; return *this; }

    operator T() const { return get(); }
    T get() const { return platformOnMeshWorker() ? workerValue : mainValue; }

    void syncWorkerFromMain() { workerValue = mainValue; }

private:
    T &current() { return platformOnMeshWorker() ? workerValue : mainValue; }

    T mainValue;
    T workerValue;
};

#endif
