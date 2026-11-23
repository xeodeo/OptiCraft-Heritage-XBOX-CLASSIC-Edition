#pragma once

#include "java/Type.h"
#include "java/String.h"

// net.minecraft.src.FlatLayerInfo (Java 1.5.2)
class FlatLayerInfo
{
public:
	FlatLayerInfo(int_t count, int_t blockId);
	FlatLayerInfo(int_t count, int_t blockId, int_t blockMeta);

	int_t getLayerCount() const { return layerCount; }
	int_t getFillBlock() const { return layerFillBlock; }
	int_t getFillBlockMeta() const { return layerFillBlockMeta; }
	int_t getMinY() const { return layerMinimumY; }
	void setMinY(int_t y) { layerMinimumY = y; }

	jstring toString() const;

private:
	int_t layerCount;
	int_t layerFillBlock;
	int_t layerFillBlockMeta;
	int_t layerMinimumY;
};
