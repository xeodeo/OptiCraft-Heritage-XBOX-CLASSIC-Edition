#include "TileEntityDaylightDetector.h"
#include "BlockDaylightDetector.h"
#include "World.h"

void TileEntityDaylightDetector::updateEntity()
{
	if (worldObj != nullptr && !worldObj->multiplayerWorld && (worldObj->getWorldTime() % 20LL == 0LL))
	{
		blockType = getBlockType();
		if (blockType != nullptr && dynamic_cast<BlockDaylightDetector *>(blockType) != nullptr)
		{
			static_cast<BlockDaylightDetector *>(blockType)->updateLightLevel(worldObj, xCoord, yCoord, zCoord);
		}
	}
}
