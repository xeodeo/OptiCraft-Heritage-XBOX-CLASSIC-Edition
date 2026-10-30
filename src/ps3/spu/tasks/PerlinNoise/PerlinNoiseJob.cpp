/* 4J's PerlinNoiseJob. OptiCraft: the SPURS job entry (cellSpursJobQueueMain)
   became PerlinNoiseJob_run, called by our raw SPU thread dispatcher
   (../SpuMain.cpp) with the effective address of a PerlinNoise_DataIn. */

/* common headers */
#include <stdint.h>
#include <stdlib.h>
#include <spu_intrinsics.h>
#include <cell/dma.h>

#include "PerlinNoiseJob.h"
#include "../Common/DmaData.h"


// 33 KB: static, not on the stack.
static PerlinNoise_DataIn s_dataIn __attribute__((aligned(128)));

bool PerlinNoiseJob_run(unsigned long long eaDataIn)
{
	PerlinNoise_DataIn& dataIn = s_dataIn;
	DmaData_SPU::getAndWait(&dataIn, eaDataIn, sizeof(PerlinNoise_DataIn));

	if (dataIn.m_perlinData.levels < 0 || dataIn.m_perlinData.levels > PerlinNoise_SPU::MAX_NOISE_LEVELS ||
		dataIn.xSize * dataIn.ySize * dataIn.zSize > PS3_SPU_NOISE_MAX_VALUES)
		return false;

	dataIn.m_perlinData.getRegion(	dataIn.m_outputBufferEA,
									dataIn.x, dataIn.y, dataIn.z,
									dataIn.xSize, dataIn.ySize, dataIn.zSize,
									dataIn.xScale, dataIn.yScale, dataIn.zScale);
	return true;
}
