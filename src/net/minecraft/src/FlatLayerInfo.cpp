#include "FlatLayerInfo.h"

#include <string>

FlatLayerInfo::FlatLayerInfo(int_t count, int_t blockId)
	: layerCount(count), layerFillBlock(blockId), layerFillBlockMeta(0), layerMinimumY(0)
{
}

FlatLayerInfo::FlatLayerInfo(int_t count, int_t blockId, int_t blockMeta)
	: layerCount(count), layerFillBlock(blockId), layerFillBlockMeta(blockMeta), layerMinimumY(0)
{
}

jstring FlatLayerInfo::toString() const
{
	std::string out = std::to_string(layerFillBlock);
	if (layerCount > 1)
	{
		out = std::to_string(layerCount) + "x" + out;
	}
	if (layerFillBlockMeta > 0)
	{
		out += ":" + std::to_string(layerFillBlockMeta);
	}
	return out;
}
