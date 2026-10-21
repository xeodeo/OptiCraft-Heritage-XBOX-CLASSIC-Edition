#include "BlockPressurePlateWeighted.h"
#include "EntityItem.h"
#include "ItemStack.h"
#include "Material.h"
#include "World.h"
#include <algorithm>
#include <cmath>

BlockPressurePlateWeighted::BlockPressurePlateWeighted(int_t id, int_t textureIndex, Material *material, int_t maxItems)
	: BlockBasePressurePlate(id, textureIndex, material), maxItemsWeighted(maxItems)
{
}

int_t BlockPressurePlateWeighted::tickRate()
{
	return 10;
}

int_t BlockPressurePlateWeighted::getPowerSupply(int_t meta)
{
	return meta;
}

int_t BlockPressurePlateWeighted::getMetaFromWeight(int_t weight)
{
	return weight;
}

int_t BlockPressurePlateWeighted::getPlateState(World *world, int_t x, int_t y, int_t z)
{
	int_t total = 0;
	const auto& list = world->getEntitiesWithinAABB(typeid(EntityItem), getSensitiveAABB(x, y, z));
	for (Entity *e : list)
	{
		EntityItem *item = dynamic_cast<EntityItem*>(e);
		if (item != nullptr && item->item != nullptr)
		{
			total += item->item->stackSize;
			if (total >= maxItemsWeighted) break;
		}
	}
	if (total <= 0) return 0;
	float ratio = (float)std::min(maxItemsWeighted, total) / (float)maxItemsWeighted;
	return std::min(15, (int_t)std::ceil(ratio * 15.0f));
}
