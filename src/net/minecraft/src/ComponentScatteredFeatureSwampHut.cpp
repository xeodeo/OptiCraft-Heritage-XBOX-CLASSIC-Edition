#include "ComponentScatteredFeatureSwampHut.h"

#include "Block.h"
#include "EntityWitch.h"
#include "StructureBoundingBox.h"
#include "World.h"
#include "java/Random.h"

ComponentScatteredFeatureSwampHut::ComponentScatteredFeatureSwampHut(Random &random, int_t x, int_t z)
	: ComponentScatteredFeature(random, x, 64, z, 7, 5, 9),
	  hasWitch(false)
{
}

bool ComponentScatteredFeatureSwampHut::addComponentParts(World *world, Random &, const StructureBoundingBox &chunkBounds)
{
	if (!func_74935_a(world, chunkBounds, 0))
		return false;

	const int_t planksId = Block::planks != nullptr ? Block::planks->blockID : 5;
	const int_t woodId = Block::wood != nullptr ? Block::wood->blockID : 17;
	const int_t fenceId = Block::fence != nullptr ? Block::fence->blockID : 85;
	const int_t flowerPotId = Block::flowerPot != nullptr ? Block::flowerPot->blockID : 140;
	const int_t workbenchId = Block::workbench != nullptr ? Block::workbench->blockID : 58;
	const int_t cauldronId = Block::cauldron != nullptr ? Block::cauldron->blockID : 118;
	const int_t stairsSpruceId = Block::stairsWoodSpruce != nullptr ? Block::stairsWoodSpruce->blockID : 134;

	// Spruce planks (metadata 1)
	fillWithMetadataBlocks(world, chunkBounds, 1, 1, 1, 5, 1, 7, planksId, 1, planksId, 1, false);
	fillWithMetadataBlocks(world, chunkBounds, 1, 4, 2, 5, 4, 7, planksId, 1, planksId, 1, false);
	fillWithMetadataBlocks(world, chunkBounds, 2, 1, 0, 4, 1, 0, planksId, 1, planksId, 1, false);
	fillWithMetadataBlocks(world, chunkBounds, 2, 2, 2, 3, 3, 2, planksId, 1, planksId, 1, false);
	fillWithMetadataBlocks(world, chunkBounds, 1, 2, 3, 1, 3, 6, planksId, 1, planksId, 1, false);
	fillWithMetadataBlocks(world, chunkBounds, 5, 2, 3, 5, 3, 6, planksId, 1, planksId, 1, false);
	fillWithMetadataBlocks(world, chunkBounds, 2, 2, 7, 4, 3, 7, planksId, 1, planksId, 1, false);

	// Wood pillars
	fillWithBlocks(world, chunkBounds, 1, 0, 2, 1, 3, 2, woodId, woodId, false);
	fillWithBlocks(world, chunkBounds, 5, 0, 2, 5, 3, 2, woodId, woodId, false);
	fillWithBlocks(world, chunkBounds, 1, 0, 7, 1, 3, 7, woodId, woodId, false);
	fillWithBlocks(world, chunkBounds, 5, 0, 7, 5, 3, 7, woodId, woodId, false);

	// Fences and decorations
	placeBlockAtCurrentPosition(world, fenceId, 0, 2, 3, 2, chunkBounds);
	placeBlockAtCurrentPosition(world, fenceId, 0, 3, 3, 7, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 1, 3, 4, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 5, 3, 4, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 5, 3, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, flowerPotId, 7, 1, 3, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, workbenchId, 0, 3, 2, 6, chunkBounds);
	placeBlockAtCurrentPosition(world, cauldronId, 0, 4, 2, 6, chunkBounds);
	placeBlockAtCurrentPosition(world, fenceId, 0, 1, 2, 1, chunkBounds);
	placeBlockAtCurrentPosition(world, fenceId, 0, 5, 2, 1, chunkBounds);

	// Roof with spruce stairs
	const int_t stairsNorth = getMetadataWithOffset(stairsSpruceId, 3);
	const int_t stairsWest = getMetadataWithOffset(stairsSpruceId, 1);
	const int_t stairsEast = getMetadataWithOffset(stairsSpruceId, 0);
	const int_t stairsSouth = getMetadataWithOffset(stairsSpruceId, 2);

	fillWithMetadataBlocks(world, chunkBounds, 0, 4, 1, 6, 4, 1, stairsSpruceId, stairsNorth, stairsSpruceId, stairsNorth, false);
	fillWithMetadataBlocks(world, chunkBounds, 0, 4, 2, 0, 4, 7, stairsSpruceId, stairsEast, stairsSpruceId, stairsEast, false);
	fillWithMetadataBlocks(world, chunkBounds, 6, 4, 2, 6, 4, 7, stairsSpruceId, stairsWest, stairsSpruceId, stairsWest, false);
	fillWithMetadataBlocks(world, chunkBounds, 0, 4, 8, 6, 4, 8, stairsSpruceId, stairsSouth, stairsSpruceId, stairsSouth, false);

	// Stilts downwards
	for (int_t z = 2; z <= 7; z += 5)
	{
		for (int_t x = 1; x <= 5; x += 4)
		{
			fillCurrentPositionBlocksDownwards(world, woodId, 0, x, -1, z, chunkBounds);
		}
	}

	// Spawn witch
	if (!hasWitch)
	{
		const int_t witchX = getXWithOffset(2, 5);
		const int_t witchY = getYWithOffset(2);
		const int_t witchZ = getZWithOffset(2, 5);
		if (chunkBounds.isVecInside(witchX, witchY, witchZ))
		{
			hasWitch = true;
			EntityWitch *witch = new EntityWitch(world);
			witch->setLocationAndAngles(static_cast<double>(witchX) + 0.5,
			                            static_cast<double>(witchY),
			                            static_cast<double>(witchZ) + 0.5,
			                            0.0f, 0.0f);
			witch->initCreature();
			world->spawnEntityInWorld(witch);
		}
	}

	return true;
}
