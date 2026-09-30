// Float-noise biome source for weak consoles (PS2 / EE).
//
// The stock 1.2.5 biome field is a GenLayer chain: GenLayer::func_48425_a builds
// roughly 26 layers (island seed, four zoom/add-island rounds, a river branch and
// a biome branch, RiverMix, then VoronoiZoom), and every consumer in
// WorldChunkManager -- biome id, temperature, rainfall -- reads through it. That
// chain runs uncached for every generated chunk, because the heightmap generator
// asks for a 20x20 halo and BiomeCache only serves aligned 16x16 tiles.
//
// It is also the wrong shape for this CPU. Each layer walks its own area calling
// initChunkSeed()/nextInt(), which are 64-bit multiplies and a 64-bit modulo per
// cell. The R5900 has no dmult, so every one of those multiplies is a __muldi3
// call, and there are thousands per chunk.
//
// This replaces the chain with three all-float 2D octave fields -- continent,
// temperature, humidity -- and a threshold table, evaluated on a four-block
// lattice (BIOME_SAMPLE_SHIFT) rather than per block. It is the same shape Pocket
// Edition uses, and it costs a handful of Perlin samples instead of a 26-layer
// pyramid: a 20x20 halo goes from 400 evaluations per field to 36.
//
// It CHANGES the generated world: biome boundaries, and therefore the terrain
// heights the Lite generator blends from them, no longer match the GenLayer
// field for a given seed. Rivers and beaches disappear entirely, since those came
// from dedicated layers. Already-saved chunks are unaffected.
//
// The whole translation unit compiles to nothing when the profile is off.

#include "platform/PlatformTuning.h"

#if PLATFORM_FAST_BIOME_SOURCE

#if !PLATFORM_FLOAT_TERRAIN_NOISE
#  error "PLATFORM_FAST_BIOME_SOURCE needs the float octave sampler: NoiseGeneratorOctaves::getBiomeGenForCoordsFloat only exists under PLATFORM_FLOAT_TERRAIN_NOISE."
#endif

#include "WorldChunkManager.h"

#include "BiomeGenBase.h"
#include "NoiseGeneratorOctaves.h"
#include "java/Arithmetic.h"
#include "java/Random.h"

#include <algorithm>
#include <cmath>

namespace
{
	// Octave counts. Two is enough for a field that only feeds threshold
	// comparisons: a third octave moves biome edges by a few blocks and costs
	// another full Perlin sweep per sample row.
	constexpr int_t CONTINENT_OCTAVES   = 2;
	constexpr int_t CLIMATE_OCTAVES     = 2;

	// Feature size, in blocks, as an inverse scale. Continents are deliberately
	// four times coarser than climate so a single landmass spans several biomes.
	constexpr float CONTINENT_SCALE = 1.0f / 1024.0f;
	constexpr float CLIMATE_SCALE   = 1.0f / 256.0f;

	// The noise is evaluated on a lattice of 1 << SHIFT blocks and the result is
	// held across the cell, rather than sampled once per block.
	//
	// This is not a reduction in fidelity against the chain it replaces: GenLayer
	// ended in GenLayerVoronoiZoom, whose output cells are four blocks wide, so
	// four-block biome granularity is exactly what vanilla produced. Sampling per
	// block was simply wasted work -- a 20x20 halo cost 400 evaluations per field
	// where 36 carry the same information.
	//
	// The lattice is anchored to world coordinates, NOT to the requested origin.
	// That matters for agreement: the Lite generator asks for a 20x20 halo at
	// (chunkX*16 - 2) while BiomeCache asks for an aligned 16x16 tile, and both
	// must return the same biome for a block they share. Anchoring to the request
	// would put the two on different lattices and produce seams.
	constexpr int_t BIOME_SAMPLE_SHIFT = 2;

	// MathHelper-free squash from the raw octave sum to (0, 1).
	//
	// NoiseGeneratorOctaves divides each octave by its own amplitude, so the sum's
	// range grows with the octave count instead of staying in [-1, 1] -- there is
	// no constant that normalises it correctly for every configuration. This is
	// monotonic and bounded for any input, so a mis-estimated range costs biome
	// VARIETY (everything drifts toward the middle of the table) and can never
	// produce a degenerate all-ocean or all-desert world.
	//
	// GAIN sets how much of the table the field actually reaches. Raise it for
	// sharper, more varied biome boundaries; lower it for larger, calmer regions.
	constexpr float SQUASH_GAIN = 0.35f;

	float toUnitRange(float value)
	{
		const float scaled = value * SQUASH_GAIN;
		const float magnitude = scaled < 0.0f ? -scaled : scaled;
		return 0.5f + 0.5f * (scaled / (1.0f + magnitude));
	}

	// Threshold table. Ordered cold to hot, dry to wet within each band, using the
	// 1.2.5 biome registry. River, beach and the "hills" variants are absent by
	// construction: those came from dedicated GenLayers, not from a climate field.
	int_t selectBiomeId(float continent, float temperature, float humidity)
	{
		if (continent < 0.42f)
		{
			// The fast source has no dedicated island layer. Reserve a narrow,
			// temperate patch of deep ocean for the rare Mushroom Island biome.
			if (continent < 0.38f && temperature > 0.45f && temperature < 0.55f &&
				humidity > 0.45f && humidity < 0.55f)
				return BiomeGenBase::mushroomIsland->biomeID;
			return BiomeGenBase::ocean->biomeID;
		}

		if (temperature < 0.18f)
			return (humidity > 0.55f ? BiomeGenBase::taiga : BiomeGenBase::icePlains)->biomeID;
		if (temperature < 0.38f)
			return (humidity > 0.55f ? BiomeGenBase::taiga : BiomeGenBase::extremeHills)->biomeID;
		if (temperature < 0.62f)
		{
			if (humidity > 0.80f)
				return BiomeGenBase::swampland->biomeID;
			return (humidity > 0.45f ? BiomeGenBase::forest : BiomeGenBase::plains)->biomeID;
		}
		if (temperature < 0.82f)
		{
			if (humidity > 0.75f)
				return BiomeGenBase::jungle->biomeID;
			return (humidity > 0.40f ? BiomeGenBase::forest : BiomeGenBase::plains)->biomeID;
		}
		if (humidity < 0.30f)
			return BiomeGenBase::desert->biomeID;
		return (humidity > 0.75f ? BiomeGenBase::jungle : BiomeGenBase::plains)->biomeID;
	}
}

void WorldChunkManager::initFastBiomeSource(long_t worldSeed)
{
	// Own Random sequence, seeded off the world seed so the field is reproducible
	// and independent of however many draws the GenLayer chain made.
	Random random(worldSeed);
	fastContinentNoise = std::make_unique<NoiseGeneratorOctaves>(random, CONTINENT_OCTAVES);
	fastTemperatureNoise = std::make_unique<NoiseGeneratorOctaves>(random, CLIMATE_OCTAVES);
	fastHumidityNoise = std::make_unique<NoiseGeneratorOctaves>(random, CLIMATE_OCTAVES);
}

const std::vector<int_t> &WorldChunkManager::fastBiomeIdArea(int_t x, int_t z,
                                                             int_t width, int_t height)
{
	const int_t count = checkedBiomeAreaCount(width, height);
	if (fastBiomeIds.size() < static_cast<std::size_t>(count))
		fastBiomeIds.resize(static_cast<std::size_t>(count));

	if (!fastContinentNoise || !fastTemperatureNoise || !fastHumidityNoise)
	{
		std::fill(fastBiomeIds.begin(), fastBiomeIds.begin() + count,
		          BiomeGenBase::plains->biomeID);
		return fastBiomeIds;
	}

	// World-anchored lattice covering the request. Arithmetic shift is floor
	// division for a power-of-two stride, including at negative coordinates, so a
	// block keeps its cell across the world origin.
	const int_t cellX0 = JavaArithmetic::intShr(x, BIOME_SAMPLE_SHIFT);
	const int_t cellZ0 = JavaArithmetic::intShr(z, BIOME_SAMPLE_SHIFT);
	const int_t cellX1 = JavaArithmetic::intShr(JavaArithmetic::intAdd(x, width - 1), BIOME_SAMPLE_SHIFT);
	const int_t cellZ1 = JavaArithmetic::intShr(JavaArithmetic::intAdd(z, height - 1), BIOME_SAMPLE_SHIFT);
	const int_t sampleWidth = JavaArithmetic::intAdd(JavaArithmetic::intSub(cellX1, cellX0), 1);
	const int_t sampleHeight = JavaArithmetic::intAdd(JavaArithmetic::intSub(cellZ1, cellZ0), 1);
	const int_t sampleCount = checkedBiomeAreaCount(sampleWidth, sampleHeight);
	if (fastCoarseBiomeIds.size() < static_cast<std::size_t>(sampleCount))
		fastCoarseBiomeIds.resize(static_cast<std::size_t>(sampleCount));

	// Sample index s sits at noise coordinate (base + s) * scale, so passing the
	// CELL index as the base and a stride-multiplied scale puts sample s exactly
	// on world block (cellX0 + s) << SHIFT.
	constexpr float latticeStep = static_cast<float>(1 << BIOME_SAMPLE_SHIFT);
	fastContinentNoise->getBiomeGenForCoordsFloat(fastContinentField, cellX0, cellZ0,
		sampleWidth, sampleHeight,
		CONTINENT_SCALE * latticeStep, CONTINENT_SCALE * latticeStep, 1.0f);
	fastTemperatureNoise->getBiomeGenForCoordsFloat(fastTemperatureField, cellX0, cellZ0,
		sampleWidth, sampleHeight,
		CLIMATE_SCALE * latticeStep, CLIMATE_SCALE * latticeStep, 1.0f);
	fastHumidityNoise->getBiomeGenForCoordsFloat(fastHumidityField, cellX0, cellZ0,
		sampleWidth, sampleHeight,
		CLIMATE_SCALE * latticeStep, CLIMATE_SCALE * latticeStep, 1.0f);

	// getBiomeGenForCoordsFloat lays its output out X-major (index = i * sizeZ + k).
	// Resolve each lattice cell once here, then the expansion below is pure
	// indexing -- no float work per output block.
	int_t sampleX = 0;
	int_t sampleZ = 0;
	const float rInner = worldSizeType == 2 ? 394.0f : 92.0f;
	const float rOuter = worldSizeType == 2 ? 420.0f : 118.0f;
	const float rInnerSq = rInner * rInner;
	const float rOuterSq = rOuter * rOuter;
	for (int_t s = 0; s < sampleCount; ++s)
	{
		const std::size_t sample = static_cast<std::size_t>(s);
		float continent = toUnitRange(static_cast<float>(fastContinentField[sample]));
		const float temperature = toUnitRange(static_cast<float>(fastTemperatureField[sample]));
		const float humidity = toUnitRange(static_cast<float>(fastHumidityField[sample]));
		if (limitedWorld)
		{
			const int_t worldX = (cellX0 + sampleX) << BIOME_SAMPLE_SHIFT;
			const int_t worldZ = (cellZ0 + sampleZ) << BIOME_SAMPLE_SHIFT;
			const float dx = static_cast<float>(worldX);
			const float dz = static_cast<float>(worldZ);
			const float distSq = dx * dx + dz * dz;
			if (distSq < rInnerSq)
			{
				continent = 0.50f + continent * 0.35f;
			}
			else if (distSq < rOuterSq)
			{
				const float dist = std::sqrt(distSq);
				const float t = (dist - rInner) * (1.0f / 26.0f);
				const float land = 0.50f + continent * 0.35f;
				continent = land * (1.0f - t) + 0.30f * t;
			}
			else
			{
				continent = 0.30f;
			}
		}
		fastCoarseBiomeIds[sample] = selectBiomeId(continent, temperature, humidity);
		if (++sampleZ >= sampleHeight)
		{
			sampleZ = 0;
			++sampleX;
		}
	}

	// Consumers expect GenLayer's Z-major convention (index = j * width + i).
	for (int_t j = 0; j < height; ++j)
	{
		const int_t sampleZ = JavaArithmetic::intSub(
			JavaArithmetic::intShr(JavaArithmetic::intAdd(z, j), BIOME_SAMPLE_SHIFT), cellZ0);
		for (int_t i = 0; i < width; ++i)
		{
			const int_t sampleX = JavaArithmetic::intSub(
				JavaArithmetic::intShr(JavaArithmetic::intAdd(x, i), BIOME_SAMPLE_SHIFT), cellX0);
			fastBiomeIds[static_cast<std::size_t>(j * width + i)] =
				fastCoarseBiomeIds[static_cast<std::size_t>(sampleX * sampleHeight + sampleZ)];
		}
	}
	return fastBiomeIds;
}

#endif // PLATFORM_FAST_BIOME_SOURCE
