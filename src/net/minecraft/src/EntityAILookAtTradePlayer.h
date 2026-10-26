#pragma once
#include "EntityAIBase.h"
class EntityVillager;
class EntityAILookAtTradePlayer : public EntityAIBase
{
public:
	explicit EntityAILookAtTradePlayer(EntityVillager *villager);
	bool shouldExecute() override;
	bool continueExecuting() override;
	void updateTask() override;
private:
	EntityVillager *villager;
};
