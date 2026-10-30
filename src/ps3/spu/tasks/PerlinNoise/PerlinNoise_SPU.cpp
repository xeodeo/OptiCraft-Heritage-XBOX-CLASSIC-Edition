#include "PerlinNoise_SPU.h"
#include "../Common/DmaData.h"
#include <alloca.h>

int64_t lfloor(double v)
{
	int64_t i = (int64_t) v;
	return v < i ? i - 1 : i;
}

void PerlinNoise_SPU::getRegion(unsigned long long bufferEA, int x, int y, int z, int xSize, int ySize, int zSize, double xScale, double yScale, double zScale)
{
	int bufLen = xSize * ySize * zSize;
	int bufMemSize = DmaData_SPU::roundUpDMASize(bufLen*sizeof(double));
	double* localBuffer = (double*)alloca(bufMemSize);
	for (int i = 0; i < bufLen; i++)
		localBuffer[i] = 0;


	double pow = 1;
	// OptiCraft: 1 / pow, kept by exact doublings (pow is 2^-i) so the SPU
	// never needs a double division; pow halves by an exact * 0.5.
	double invPow = 1;

	for (int i = 0; i < levels; i++)
	{
		//            value += noiseLevels[i].getValue(x * pow, y * pow, z * pow) / pow;
		double xx = x * pow * xScale;
		double yy = y * pow * yScale;
		double zz = z * pow * zScale;
		int64_t xb = lfloor(xx);
		int64_t zb = lfloor(zz);
		xx -= xb;
		zz -= zb;
		xb %= 16777216;
		zb %= 16777216;
		xx += xb;
		zz += zb;
		noiseLevels[i].add(localBuffer, xx, yy, zz, xSize, ySize, zSize, xScale * pow, yScale * pow, zScale * pow, invPow);
		pow *= 0.5;
		invPow *= 2.0;
	}
	DmaData_SPU::putAndWait(localBuffer, bufferEA, bufMemSize);
}
