#include "StructureScatteredFeatureStones.h"

#include "Block.h"
#include "java/Random.h"

void StructureScatteredFeatureStones::selectBlocks(Random &random, int_t, int_t, int_t, bool)
{
	if (random.nextFloat() < 0.4f)
	{
		selectedBlockId = Block::cobblestone != nullptr ? Block::cobblestone->blockID : 4;
	}
	else
	{
		selectedBlockId = Block::cobblestoneMossy != nullptr ? Block::cobblestoneMossy->blockID : 48;
	}
	selectedBlockMetaData = 0;
}
