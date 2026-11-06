#include "EntityAITradePlayer.h"
#include "EntityVillager.h"
#include "EntityPlayer.h"
#include "PathNavigate.h"

EntityAITradePlayer::EntityAITradePlayer(EntityVillager *v) : villager(v) { setMutexBits(5); }

bool EntityAITradePlayer::shouldExecute()
{
	if (villager == nullptr || !villager->isEntityAlive()) return false;
	if (villager->isInWater()) return false;
	if (!villager->onGround) return false;
	if (villager->velocityChanged) return false;
	EntityPlayer *player = villager->getCustomer();
	if (player == nullptr) return false;
	// Java also requires player.openContainer instanceof Container, which is never false.
	return villager->getDistanceSqToEntity(player) <= 16.0;
}

void EntityAITradePlayer::startExecuting()
{
	if (PathNavigate *nav = villager->getNavigator()) nav->clearPathEntity();
}

void EntityAITradePlayer::resetTask() { villager->setCustomer(nullptr); }
