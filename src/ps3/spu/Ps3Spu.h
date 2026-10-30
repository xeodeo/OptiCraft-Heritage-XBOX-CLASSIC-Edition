#pragma once

// PS3 SPU job runtime, PPU side (SPU side: src/ps3/spu/tasks/SpuMain.cpp).
//
// One SPU thread group of PS3_SPU_THREADS raw SPU threads, started on first
// use, each blocked on its signal-notification register until it gets a job.
// A job is a 128-byte aligned Ps3SpuJob whose header names the task and the
// effective address of its input; the task DMAs its own data in and out.
//
//   Ps3SpuJob job;               // must stay alive until waited
//   if (ps3SpuSubmit(job, PS3_SPU_JOB_PERLIN_NOISE, &input))
//       ok = ps3SpuWait(job);    // true = the SPU did it, false = run it on the PPU
//
// Every submitted job must be waited. Safe from any PPU thread. Without the SPU
// task image (PS3_SPU_TASKS=0) or if the group cannot start, ps3SpuAvailable()
// is false and the callers keep their PPU code.

#include <cstdint>

#include "ps3/spu/tasks/SpuJob.h"

struct alignas(128) Ps3SpuJob
{
    Ps3SpuJobHeader header;
    std::uint64_t submitMicros = 0;
    int slot = -1;
};

// Starts the SPU group on the first call; false if SPUs are unavailable.
bool ps3SpuAvailable();
bool ps3SpuSubmit(Ps3SpuJob &job, std::uint32_t type, const void *input);
// Blocks until the job is done. True when the SPU completed it.
bool ps3SpuWait(Ps3SpuJob &job);
int ps3SpuThreadCount();
// Quit path: stops and joins the SPU group, bounded (the XMB reboots the
// console when quitting takes too long).
void ps3SpuShutdown();

// Accumulated since the last call (for the 5 s stats line).
struct Ps3SpuStats
{
    long jobs = 0;
    long failed = 0;
    long spuMicros = 0;     // SPU-side time (decrementer)
    long waitMicros = 0;    // PPU time blocked in ps3SpuWait
};
Ps3SpuStats ps3SpuTakeStats();
