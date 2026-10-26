#include "ComponentScatteredFeature.h"

#include <algorithm>
#include "StructureBoundingBox.h"
#include "World.h"
#include "WorldProvider.h"
#include "java/Random.h"

ComponentScatteredFeature::ComponentScatteredFeature(Random &random, int_t x, int_t y, int_t z,
                                                     int_t sizeX, int_t sizeY, int_t sizeZ)
	: StructureComponent(0),
	  scatteredFeatureSizeX(sizeX),
	  scatteredFeatureSizeY(sizeY),
	  scatteredFeatureSizeZ(sizeZ),
	  field_74936_d(-1)
{
	coordBaseMode = random.nextInt(4);
	switch (coordBaseMode)
	{
	case 0:
	case 2:
		boundingBox = std::make_unique<StructureBoundingBox>(x, y, z, x + sizeX - 1, y + sizeY - 1, z + sizeZ - 1);
		break;
	default:
		boundingBox = std::make_unique<StructureBoundingBox>(x, y, z, x + sizeZ - 1, y + sizeY - 1, z + sizeX - 1);
		break;
	}
}

bool ComponentScatteredFeature::func_74935_a(World *world, const StructureBoundingBox &chunkBounds, int_t offset)
{
	if (field_74936_d >= 0)
		return true;

	if (world == nullptr || !boundingBox)
		return false;

	int_t total = 0;
	int_t count = 0;
	const int_t defaultGround = world->worldProvider != nullptr ? world->worldProvider->getAverageGroundLevel() : 64;

	for (int_t z = boundingBox->minZ; z <= boundingBox->maxZ; ++z)
	{
		for (int_t x = boundingBox->minX; x <= boundingBox->maxX; ++x)
		{
			if (chunkBounds.isVecInside(x, 64, z))
			{
				total += std::max(world->getTopSolidOrLiquidBlock(x, z), defaultGround);
				++count;
			}
		}
	}

	if (count == 0)
		return false;

	field_74936_d = total / count;
	boundingBox->offset(0, field_74936_d - boundingBox->minY + offset, 0);
	return true;
}
