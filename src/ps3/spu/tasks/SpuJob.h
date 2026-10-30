#pragma once

// Shared between the PPU runtime (src/ps3/spu/Ps3Spu.cpp, gcc 13, LP64) and
// the SPU dispatcher (SpuMain.cpp, Sony spu-lv2-gcc 4.1, C++03). Fixed-width
// fields only, no pointers, big-endian on both sides.
//
// Protocol, one job at a time per SPU thread:
//   PPU  claims a free slot cell (0 -> 1), fills a Ps3SpuJobHeader (status 0)
//        and writes its 32-bit effective address to the thread's signal 1.
//   SPU  DMAs the header in, runs the job (the job DMAs its own data in and its
//        results out and waits for them), writes the header back with the
//        status and its decrementer ticks, then writes 0 to its slot cell.
//   PPU  polls the header's status: 1 = done, 2 = the job refused its input.
// Signal value PS3_SPU_SIGNAL_QUIT makes the thread write 2 to its slot cell
// and exit.

#define PS3_SPU_SIGNAL_QUIT        1u

#define PS3_SPU_JOB_PERLIN_NOISE   1u

#define PS3_SPU_STATUS_PENDING     0u
#define PS3_SPU_STATUS_DONE        1u
#define PS3_SPU_STATUS_FAILED      2u

#define PS3_SPU_SLOT_IDLE          0u
#define PS3_SPU_SLOT_BUSY          1u
#define PS3_SPU_SLOT_EXITED        2u

// One slot cell per SPU thread: 16 bytes, the u32 at offset 0 is the state.
#define PS3_SPU_SLOT_CELL_BYTES    16

struct Ps3SpuJobHeader
{
	unsigned int       type;
	unsigned int       status;
	unsigned long long eaData;      // job-specific input, 16-byte aligned
	unsigned int       spuTicks;    // SPU decrementer ticks spent (timebase, 79.8 MHz)
	unsigned int       pad[3];
};

typedef char ps3_spu_job_header_size[(sizeof(Ps3SpuJobHeader) == 32) ? 1 : -1];
