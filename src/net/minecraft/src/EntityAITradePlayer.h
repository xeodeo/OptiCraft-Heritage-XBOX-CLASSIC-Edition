#pragma once
#include "EntityAIBase.h"
class EntityVillager;
class EntityAITradePlayer : public EntityAIBase
{
public:
	explicit EntityAITradePlayer(EntityVillager *villager);
	bool shouldExecute() override;
	bool continueExecuting() override;
	void startExecuting() override;
	void resetTask() override;
	void updateTask() override;
private:
	EntityVillager *villager;
};
