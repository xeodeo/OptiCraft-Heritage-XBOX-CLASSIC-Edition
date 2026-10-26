#pragma once

#include "EntityAgeable.h"
#include "IMerchant.h"
#include <memory>

// net.minecraft.src.EntityVillager
class Village;
class EntityPlayer;
class MerchantRecipe;
class MerchantRecipeList;

class EntityVillager : public EntityAgeable, public IMerchant
{
public:
	explicit EntityVillager(World *world);
	EntityVillager(World *world, int_t profession);
	~EntityVillager() override;

	static constexpr int_t CLASS_ID = 28;
	int_t getEntityClassID() const override { return CLASS_ID; }

	const char *getEntityTexture() override;
	void writeEntityToNBT(NBTTagCompound *tag) override;
	void readEntityFromNBT(NBTTagCompound *tag) override;
	void onLivingUpdate() override;
	bool attackEntityFrom(Entity *entity, int_t damage) override;
	void setRevengeTarget(EntityLiving *target) override;
	bool isAIEnabled() override;
	int_t getMaxHealth() const;

	void setProfession(int_t profession);
	int_t getProfession();
	bool getIsMatingFlag() const;
	void setIsMatingFlag(bool value);
	void setIsPlayingFlag(bool value);
	bool getIsPlayingFlag() const;
	bool interact(EntityPlayer *player) override;
	void setCustomer(EntityPlayer *player) override;
	EntityPlayer *getCustomer() const override;
	MerchantRecipeList *getRecipes(EntityPlayer *player) override;
	void useRecipe(MerchantRecipe *recipe) override;
	void setRecipes(MerchantRecipeList *recipes);
	bool isTrading() const;
	void initCreature() override;
	void handleHealthUpdate(byte_t state) override;
	void onDeath(Entity *entity) override;

protected:
	void entityInit() override;
	void updateAITick() override;
	bool canDespawn() override;
	jstring getLivingSound() override;
	jstring getHurtSound() override;
	jstring getDeathSound() override;

private:
	int_t randomTickDivider;
	bool isMatingFlag;
	bool isPlayingFlag;
	Village *villageObj = nullptr;
	EntityPlayer *buyingPlayer = nullptr;
	std::unique_ptr<MerchantRecipeList> buyingList;
	int_t timeUntilReset = 0;
	bool needsInitialization = false;
	int_t wealth = 0;
	jstring lastBuyingPlayer;
	bool boostReputation = false;
	float recipeMultiplier = 0.0f;
	void addDefaultEquipmentAndRecipies(int_t count);
	void generateTradeParticles(const char *particle);
};
