#pragma once

#include "EntityMob.h"

class World;
class NBTTagCompound;
class ItemStack;
class EntityLiving;
class EntityPlayer;
class DamageSource;

// net.minecraft.src.EntityZombie
class EntityZombie : public EntityMob
{
public:
	explicit EntityZombie(World *world);
	~EntityZombie() override;

	void onUpdate() override;
	void onLivingUpdate() override;
	bool isAIEnabled() override;
	int_t getMaxHealth() const override;
	int_t getTotalArmorValue() const override;
	float getSpeedModifier() override;
	const char *getEntityTexture() override;
	bool isChild() override;
	void setChild(bool child);
	bool isVillager() const;
	void setVillager(bool villager);
	bool isConverting() const;
	void startConversion(int_t time);
	void initCreature() override;
	bool interact(EntityPlayer *entityPlayer) override;
	void onKillEntity(EntityLiving *entityLiving) override;
	void handleHealthUpdate(byte_t byte0) override;
	bool attackEntityAsMob(Entity *entity) override;
	void onDeath(const DamageSource &source) override;
	ItemStack *getHeldItem() override;
	EnumCreatureAttribute getCreatureAttribute() const override;
	void writeEntityToNBT(NBTTagCompound *tag) override;
	void readEntityFromNBT(NBTTagCompound *tag) override;

protected:
	void entityInit() override;
	jstring getLivingSound() override;
	jstring getHurtSound() override;
	jstring getDeathSound() override;
	int_t getDropItemId() override;
	void dropRareDrop(int_t lootingRoll) override;
	void convertToVillager();
	int_t getConversionTimeBoost();

private:
	int_t conversionTime;
	ItemStack *heldItemInstance;
	bool canPickUpLoot;
};
