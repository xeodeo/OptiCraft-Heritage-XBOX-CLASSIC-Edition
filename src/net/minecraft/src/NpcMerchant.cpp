#include "NpcMerchant.h"

#include "EntityPlayer.h"
#include "InventoryMerchant.h"
#include "MerchantRecipe.h"
#include "MerchantRecipeList.h"

NpcMerchant::NpcMerchant(EntityPlayer *player)
	: theMerchantInventory(new InventoryMerchant(player, this)), customer(player), recipeList(nullptr)
{
}

NpcMerchant::~NpcMerchant() = default;

EntityPlayer *NpcMerchant::getCustomer() const
{
	return customer;
}

void NpcMerchant::setCustomer(EntityPlayer *)
{
}

MerchantRecipeList *NpcMerchant::getRecipes(EntityPlayer *)
{
	return recipeList;
}

void NpcMerchant::setRecipes(MerchantRecipeList *recipes)
{
	recipeList = recipes;
}

void NpcMerchant::useRecipe(MerchantRecipe *)
{
}
