#include "BlockPressurePlate.h"
#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "Material.h"
#include "World.h"

BlockPressurePlate::BlockPressurePlate(int_t id, int_t textureIndex, EnumMobType mobType, Material *material)
	: BlockBasePressurePlate(id, textureIndex, material), triggerMobType(mobType)
{
}

int_t BlockPressurePlate::getMetaFromWeight(int_t weight)
{
	return weight > 0 ? 1 : 0;
}

int_t BlockPressurePlate::getPowerSupply(int_t meta)
{
	return meta == 1 ? 15 : 0;
}

int_t BlockPressurePlate::getPlateState(World *world, int_t x, int_t y, int_t z)
{
	AxisAlignedBB *aabb = getSensitiveAABB(x, y, z);
	if (triggerMobType == EnumMobType::everything)
	{
		const auto& list = world->getEntitiesWithinAABBExcludingEntity(nullptr, aabb);
		if (!list.empty()) return 15;
	}
	else if (triggerMobType == EnumMobType::mobs)
	{
		const auto& list = world->getEntitiesWithinAABB(typeid(EntityLiving), aabb);
		if (!list.empty()) return 15;
	}
	else if (triggerMobType == EnumMobType::players)
	{
		const auto& list = world->getEntitiesWithinAABB(typeid(EntityPlayer), aabb);
		if (!list.empty()) return 15;
	}
	return 0;
}
