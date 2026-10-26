#pragma once

#include <memory>
#include <vector>

class ItemStack;
class MerchantRecipe;
class NBTTagCompound;

class MerchantRecipeList
{
public:
	explicit MerchantRecipeList(NBTTagCompound *tag = nullptr);
	int size() const;
	bool empty() const;
	MerchantRecipe *get(int index) const;
	void add(MerchantRecipe *recipe);
	void addToListWithCheck(MerchantRecipe *recipe);
	MerchantRecipe *canRecipeBeUsed(ItemStack *first, ItemStack *second, int index) const;
	void readRecipiesFromTags(NBTTagCompound *tag);
	NBTTagCompound *getRecipiesAsTags() const;

private:
	std::vector<std::unique_ptr<MerchantRecipe>> recipes;
};
