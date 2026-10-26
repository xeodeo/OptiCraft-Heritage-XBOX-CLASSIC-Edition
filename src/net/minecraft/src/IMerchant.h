#pragma once

class EntityPlayer;
class MerchantRecipe;
class MerchantRecipeList;

// net.minecraft.src.IMerchant
class IMerchant
{
public:
	virtual ~IMerchant() = default;
	virtual void setCustomer(EntityPlayer *player) = 0;
	virtual EntityPlayer *getCustomer() const = 0;
	virtual MerchantRecipeList *getRecipes(EntityPlayer *player) = 0;
	virtual void useRecipe(MerchantRecipe *recipe) = 0;
};
