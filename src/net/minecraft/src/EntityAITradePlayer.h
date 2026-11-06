#pragma once
#include "EntityAIBase.h"
class EntityVillager;
// net.minecraft.src.EntityAITradePlayer
class EntityAITradePlayer : public EntityAIBase
{
public:
	explicit EntityAITradePlayer(EntityVillager *villager);
	bool shouldExecute() override;
	void startExecuting() override;
	void resetTask() override;
private:
	EntityVillager *villager;
};
