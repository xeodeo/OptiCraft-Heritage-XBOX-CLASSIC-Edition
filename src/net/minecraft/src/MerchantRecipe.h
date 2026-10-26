#pragma once

#include <memory>

class ItemStack;
class NBTTagCompound;

class MerchantRecipe
{
public:
	MerchantRecipe(ItemStack *buy, ItemStack *sell);
	MerchantRecipe(ItemStack *buy, ItemStack *secondBuy, ItemStack *sell);
	explicit MerchantRecipe(NBTTagCompound *tag);
	ItemStack *getItemToBuy() const;
	ItemStack *getSecondItemToBuy() const;
	ItemStack *getItemToSell() const;
	bool hasSecondItemToBuy() const;
	bool hasSameIDsAs(const MerchantRecipe &other) const;
	bool hasSameItemsAs(const MerchantRecipe &other) const;
	void incrementToolUses();
	void increaseMaxTradeUses(int amount);
	bool isRecipeDisabled() const;
	void disableRecipe();
	void readFromTags(NBTTagCompound *tag);
	NBTTagCompound *writeToTags() const;

private:
	std::unique_ptr<ItemStack> itemToBuy;
	std::unique_ptr<ItemStack> secondItemToBuy;
	std::unique_ptr<ItemStack> itemToSell;
	int toolUses = 0;
	int maxTradeUses = 7;
};
