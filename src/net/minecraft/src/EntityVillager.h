#pragma once

#include "EntityAgeable.h"
#include "IMerchant.h"
#include "DamageSource.h"
#include <memory>
#include <string>

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
	bool isMating() const { return getIsMatingFlag(); }
	void setMating(bool v) { setIsMatingFlag(v); }
	bool isPlaying() const { return getIsPlayingFlag(); }
	void setPlaying(bool v) { setIsPlayingFlag(v); }

	bool interact(EntityPlayer *player) override;
	void setCustomer(EntityPlayer *player) override;
	EntityPlayer *getCustomer() const override;
	MerchantRecipeList *getRecipes(EntityPlayer *player) override;
	void useRecipe(MerchantRecipe *recipe) override;
	void setRecipes(MerchantRecipeList *recipes) override;
	bool isTrading() const;
	void initCreature() override;
	void handleHealthUpdate(byte_t state) override;
	void onDeath(const DamageSource &source) override;
	void onDeath(Entity *entity) override;

	void func_82187_q();
	EntityVillager *func_90012_b(EntityAgeable *other);
	std::string func_94057_bL() const;

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
	bool field_82190_bM = false;
	float field_82191_bN = 0.0f;

	float func_82188_j(float base);
	void addDefaultEquipmentAndRecipies(int_t count);
	void generateTradeParticles(const char *particle);
};
