#include "ChestItemRenderHelper.h"

#include "Block.h"
#include "TileEntityChest.h"
#include "TileEntityEnderChest.h"
#include "TileEntityRenderer.h"

ChestItemRenderHelper ChestItemRenderHelper::instance;

ChestItemRenderHelper::ChestItemRenderHelper()
	: chest(new TileEntityChest())
	, theEnderChest(new TileEntityEnderChest())
{
}

ChestItemRenderHelper::~ChestItemRenderHelper()
{
	delete chest;
	delete theEnderChest;
}

void ChestItemRenderHelper::renderChest(Block *block, int metadata, float brightness)
{
	(void)metadata;
	(void)brightness;
	if (block != nullptr && Block::enderChest != nullptr && block->blockID == Block::enderChest->blockID)
	{
		TileEntityRenderer::instance.renderTileEntityAt(theEnderChest, 0.0, 0.0, 0.0, 0.0f);
	}
	else
	{
		TileEntityRenderer::instance.renderTileEntityAt(chest, 0.0, 0.0, 0.0, 0.0f);
	}
}
