#pragma once

#include "EntityCreature.h"

class NBTTagCompound;
class EntityPlayer;

// net.minecraft.src.EntityAgeable
class EntityAgeable : public EntityCreature
{
public:
	explicit EntityAgeable(World *world);

	bool interact(EntityPlayer *player) override;
	virtual EntityAgeable *createChild(EntityAgeable *other) { (void)other; return nullptr; }
	int_t getGrowingAge();
	void setGrowingAge(int_t age);
	void writeEntityToNBT(NBTTagCompound *tag) override;
	void readEntityFromNBT(NBTTagCompound *tag) override;
	void onLivingUpdate() override;
	bool isChild() override;
	void setScaleForAge(bool child);

protected:
	void entityInit() override;
	void setSize(float width, float height) override;

private:
	void setScale(float scale);

	float field_98056_d = -1.0f;
	float field_98057_e = 0.0f;
};
