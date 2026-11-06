#include "EntityAILookAtTradePlayer.h"
#include "EntityVillager.h"
#include "EntityPlayer.h"

EntityAILookAtTradePlayer::EntityAILookAtTradePlayer(EntityVillager *v)
	: EntityAIWatchClosest(v, typeid(EntityPlayer), 8.0f), theMerchant(v) {}

bool EntityAILookAtTradePlayer::shouldExecute()
{
	if (!theMerchant->isTrading()) return false;
	closestEntityId = theMerchant->getCustomer()->entityId;
	return true;
}
