#include "EntityAIVillagerMate.h"

#include "java/Arithmetic.h"

#include "AxisAlignedBB.h"
#include "Entity.h"
#include "EntityLiving.h"
#include "EntityLookHelper.h"
#include "EntityVillager.h"
#include "MathHelper.h"
#include "PathNavigate.h"
#include "Village.h"
#include "VillageCollection.h"
#include "World.h"

EntityAIVillagerMate::EntityAIVillagerMate(EntityVillager *villager)
	: villagerObj(villager), worldObj(villager != nullptr ? villager->worldObj : nullptr), mateEntityId(-1), matingTimeout(0)
{
	setMutexBits(3);
}

EntityVillager *EntityAIVillagerMate::resolveMate() const
{
	if (worldObj == nullptr || mateEntityId < 0)
		return nullptr;
	return dynamic_cast<EntityVillager *>(worldObj->getEntityByID(mateEntityId));
}

Village *EntityAIVillagerMate::resolveVillage() const
{
	if (villagerObj == nullptr || worldObj == nullptr || worldObj->villageCollectionObj == nullptr)
		return nullptr;
	return worldObj->villageCollectionObj->findNearestVillage(
		MathHelper::floor_double(villagerObj->posX), MathHelper::floor_double(villagerObj->posY),
		MathHelper::floor_double(villagerObj->posZ), 0);
}

bool EntityAIVillagerMate::shouldExecute()
{
	mateEntityId = -1;
	if (villagerObj == nullptr || worldObj == nullptr || villagerObj->getGrowingAge() != 0)
		return false;
	if (villagerObj->getRNG().nextInt(500) != 0)
		return false;
	Village *village = resolveVillage();
	if (village == nullptr || !checkSufficientDoorsPresentForNewVillager())
		return false;
	Entity *nearest = worldObj->findNearestEntityWithinAABB(typeid(EntityVillager),
		villagerObj->boundingBox->expand(8.0, 3.0, 8.0), villagerObj);
	EntityVillager *mate = dynamic_cast<EntityVillager *>(nearest);
	if (mate == nullptr || mate->getGrowingAge() != 0)
		return false;
	mateEntityId = mate->entityId;
	return true;
}

void EntityAIVillagerMate::startExecuting()
{
	matingTimeout = 300;
	if (villagerObj != nullptr)
		villagerObj->setIsMatingFlag(true);
}

void EntityAIVillagerMate::resetTask()
{
	mateEntityId = -1;
	matingTimeout = 0;
	if (villagerObj != nullptr)
		villagerObj->setIsMatingFlag(false);
}

bool EntityAIVillagerMate::continueExecuting()
{
	EntityVillager *mate = resolveMate();
	return mate != nullptr && !mate->isDead && matingTimeout >= 0 && checkSufficientDoorsPresentForNewVillager() &&
		villagerObj != nullptr && villagerObj->getGrowingAge() == 0;
}

void EntityAIVillagerMate::updateTask()
{
	EntityVillager *mate = resolveMate();
	if (villagerObj == nullptr || mate == nullptr)
		return;
	--matingTimeout;
	villagerObj->getLookHelper()->setLookPositionWithEntity(mate, 10.0f, 30.0f);
	if (villagerObj->getDistanceSqToEntity(mate) > 2.25)
		villagerObj->getNavigator()->tryMoveToEntityLiving(mate, 0.25f);
	else if (matingTimeout == 0 && mate->getIsMatingFlag())
		giveBirth(mate);
	if (villagerObj->getRNG().nextInt(35) == 0)
		worldObj->setEntityState(villagerObj, static_cast<byte_t>(12));
}

bool EntityAIVillagerMate::checkSufficientDoorsPresentForNewVillager() const
{
	Village *village = resolveVillage();
	if (village == nullptr || !village->isMatingSeason())
		return false;
	int_t capacity = JavaArithmetic::doubleToInt((double)(float)village->getNumVillageDoors() * 0.35);
	return village->getNumVillagers() < capacity;
}

void EntityAIVillagerMate::giveBirth(EntityVillager *mate)
{
	if (villagerObj == nullptr || mate == nullptr || worldObj == nullptr)
		return;
	EntityVillager *child = new EntityVillager(worldObj);
	mate->setGrowingAge(6000);
	villagerObj->setGrowingAge(6000);
	child->setGrowingAge(-24000);
	child->setProfession(villagerObj->getRNG().nextInt(5));
	child->setLocationAndAngles(villagerObj->posX, villagerObj->posY, villagerObj->posZ, 0.0f, 0.0f);
	if (!worldObj->entityJoinedWorld(child))
	{
		delete child;
		return;
	}
	worldObj->setEntityState(child, static_cast<byte_t>(12));
}
