#pragma once

#include "java/Type.h"

// Runtime performance profile inspired by Pocket Edition / console profiles.
// Disabled by default on Xbox so vanilla visuals and baseline 60 fps stay
// untouched; when enabled in Performance Settings (Java or Legacy UI), sheds
// entity, particle, tile-entity, lightmap, ambient probe and weather work.
namespace PocketOptimizations
{
	bool enabled();

	int_t maxRenderDistanceFine();

	float entityRenderRadius(bool player);
	float tileEntityRenderRadius();
	float particleRenderDistance();

	int_t maxParticlesPerLayer(int_t platformLimit);
	int_t blockDestroyParticleGrid(int_t platformLimit);

	int_t randomDisplayProbeCount(int_t platformProbes);

	int_t dynamicTextureIntervalTicks(int_t platformInterval);
	int_t lightmapUpdateIntervalFrames();

	int_t weatherRenderRange(int_t platformRange);
	int_t rainSplashParticleCount(int_t count);
	int_t rainSplashRange(int_t range);

	int_t chunkCacheRenderDistance(int_t userTier);
}
