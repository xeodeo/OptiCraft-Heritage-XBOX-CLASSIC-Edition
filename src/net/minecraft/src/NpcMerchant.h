#pragma once

#include <memory>
#include "IMerchant.h"

class EntityPlayer;
class InventoryMerchant;
class MerchantRecipe;
class MerchantRecipeList;

// net.minecraft.src.NpcMerchant
class NpcMerchant : public IMerchant
{
public:
	explicit NpcMerchant(EntityPlayer *player);
	~NpcMerchant() override;

	EntityPlayer *getCustomer() const override;
	void setCustomer(EntityPlayer *player) override;
	MerchantRecipeList *getRecipes(EntityPlayer *player) override;
	void setRecipes(MerchantRecipeList *recipes) override;
	void useRecipe(MerchantRecipe *recipe) override;

private:
	std::unique_ptr<InventoryMerchant> theMerchantInventory;
	EntityPlayer *customer;
	MerchantRecipeList *recipeList;
};
