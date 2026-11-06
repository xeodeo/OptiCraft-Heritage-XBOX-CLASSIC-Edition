#pragma once
#include "EntityAIWatchClosest.h"
class EntityVillager;
// net.minecraft.src.EntityAILookAtTradePlayer
class EntityAILookAtTradePlayer : public EntityAIWatchClosest
{
public:
	explicit EntityAILookAtTradePlayer(EntityVillager *villager);
	bool shouldExecute() override;
private:
	EntityVillager *theMerchant;
};
