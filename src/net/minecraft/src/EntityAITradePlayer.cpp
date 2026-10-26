#include "EntityAITradePlayer.h"
#include "EntityVillager.h"
#include "EntityPlayer.h"
#include "EntityLookHelper.h"
EntityAITradePlayer::EntityAITradePlayer(EntityVillager *v) : villager(v) { setMutexBits(5); }
bool EntityAITradePlayer::shouldExecute() { return villager && villager->isTrading(); }
bool EntityAITradePlayer::continueExecuting() { return shouldExecute(); }
void EntityAITradePlayer::startExecuting() {}
void EntityAITradePlayer::resetTask() {}
void EntityAITradePlayer::updateTask()
{
	EntityPlayer *player = villager ? villager->getCustomer() : nullptr;
	if (player) villager->getLookHelper()->setLookPositionWithEntity(player, 30.0f, 30.0f);
}
