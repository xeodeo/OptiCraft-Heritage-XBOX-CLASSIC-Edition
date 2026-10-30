/* OptiCraft PS3: the SPU side of the job runtime (PPU side: src/ps3/spu/Ps3Spu.cpp).
   Replaces 4J's SPURS job queue: every SPU thread of our group runs this loop,
   blocked on signal notification 1 until the PPU hands it a job header.
   Built with Sony's spu-lv2-gcc (see cmake/ps3_spu_build.sh). */

#include <stdint.h>
#include <spu_intrinsics.h>
#include <spu_mfcio.h>
#include <sys/spu_thread.h>
#include <cell/dma.h>

#include "SpuJob.h"
#include "Common/DmaData.h"
#include "PerlinNoise/PerlinNoiseJob.h"

static Ps3SpuJobHeader s_header __attribute__((aligned(128)));

int main(uint64_t slotCellEA, uint64_t, uint64_t, uint64_t)
{
	for (;;)
	{
		const uint32_t headerEA = spu_read_signal1();
		if (headerEA == PS3_SPU_SIGNAL_QUIT)
			break;

		DmaData_SPU::getAndWait(&s_header, headerEA, sizeof(s_header));

		spu_writech(SPU_WrDec, 0xFFFFFFFFu);
		const uint32_t decStart = spu_readch(SPU_RdDec);

		bool ok = false;
		switch (s_header.type)
		{
		case PS3_SPU_JOB_PERLIN_NOISE:
			ok = PerlinNoiseJob_run(s_header.eaData);
			break;
		default:
			break;
		}

		// The job's own transfers have completed (put-and-wait), so the
		// results are in main memory before the status says so.
		s_header.spuTicks = decStart - spu_readch(SPU_RdDec);
		s_header.status = ok ? PS3_SPU_STATUS_DONE : PS3_SPU_STATUS_FAILED;
		DmaData_SPU::putAndWait(&s_header, headerEA, sizeof(s_header));
		cellDmaPutUint32(PS3_SPU_SLOT_IDLE, slotCellEA, PS3_SPU_DMA_TAG, 0, 0);
		cellDmaWaitTagStatusAll(1 << PS3_SPU_DMA_TAG);
	}

	cellDmaPutUint32(PS3_SPU_SLOT_EXITED, slotCellEA, PS3_SPU_DMA_TAG, 0, 0);
	cellDmaWaitTagStatusAll(1 << PS3_SPU_DMA_TAG);
	sys_spu_thread_exit(0);
	return 0;
}
