#include "PocketOptimizations.h"

#include "Config.h"
#include "platform/PlatformTuning.h"

namespace PocketOptimizations
{
bool enabled()
{
	return Config::isPocketOptimizations();
}

int_t maxRenderDistanceFine()
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
	return 64;
#else
	return 128;
#endif
}

float entityRenderRadius(bool player)
{
	return player ? 96.0f : 64.0f;
}

float tileEntityRenderRadius()
{
	return 64.0f;
}

float particleRenderDistance()
{
	return 48.0f;
}

int_t maxParticlesPerLayer(int_t platformLimit)
{
#if PLATFORM_PS2 || PLATFORM_WII
	const int_t cap = 128;
#elif PLATFORM_XBOX
	const int_t cap = 256;
#else
	const int_t cap = 512;
#endif
	return platformLimit < cap ? platformLimit : cap;
}

int_t blockDestroyParticleGrid(int_t platformLimit)
{
	return platformLimit < 2 ? platformLimit : 2;
}

int_t randomDisplayProbeCount(int_t platformProbes)
{
	if (platformProbes <= 0)
		return 0;
#if PLATFORM_PS2 || PLATFORM_WII
	const int_t divisor = 2;
#else
	const int_t divisor = 4;
#endif
	int_t count = platformProbes / divisor;
	if (count < 16 && platformProbes >= 16)
		count = 16;
	return count;
}

int_t dynamicTextureIntervalTicks(int_t platformInterval)
{
	return platformInterval > 2 ? platformInterval : 2;
}

int_t lightmapUpdateIntervalFrames()
{
	return 2;
}

int_t weatherRenderRange(int_t platformRange)
{
	const int_t range = platformRange >= 0 ? platformRange : 5;
	return range < 5 ? range : 5;
}

int_t rainSplashParticleCount(int_t count)
{
	return count > 0 ? count / 2 : count;
}

int_t rainSplashRange(int_t range)
{
	return range < 5 ? range : 5;
}

int_t chunkCacheRenderDistance(int_t userTier)
{
	if (!enabled())
		return userTier;

	const int_t fine = Config::getRenderDistanceFine();
	int_t fineTier = 3;
	if (fine >= 256)
		fineTier = 0;
	else if (fine >= 128)
		fineTier = 1;
	else if (fine >= 64)
		fineTier = 2;

	return fineTier > userTier ? fineTier : userTier;
}
}
