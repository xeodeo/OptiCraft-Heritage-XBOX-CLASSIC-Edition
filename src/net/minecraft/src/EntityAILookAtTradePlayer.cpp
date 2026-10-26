#include "EntityAILookAtTradePlayer.h"
#include "EntityVillager.h"
#include "EntityPlayer.h"
#include "EntityLookHelper.h"
EntityAILookAtTradePlayer::EntityAILookAtTradePlayer(EntityVillager *v) : villager(v) {}
bool EntityAILookAtTradePlayer::shouldExecute() { return villager && villager->isTrading() && villager->getCustomer(); }
bool EntityAILookAtTradePlayer::continueExecuting() { return shouldExecute(); }
void EntityAILookAtTradePlayer::updateTask()
{
	if (villager && villager->getCustomer()) villager->getLookHelper()->setLookPositionWithEntity(villager->getCustomer(), 10.0f, 30.0f);
}
