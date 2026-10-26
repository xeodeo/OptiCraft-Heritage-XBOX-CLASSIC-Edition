#include "MerchantRecipe.h"

#include "ItemStack.h"
#include "NBTTagCompound.h"

MerchantRecipe::MerchantRecipe(ItemStack *buy, ItemStack *sell) : MerchantRecipe(buy, nullptr, sell) {}
MerchantRecipe::MerchantRecipe(ItemStack *buy, ItemStack *secondBuy, ItemStack *sell)
	: itemToBuy(buy ? buy->copy() : nullptr), secondItemToBuy(secondBuy ? secondBuy->copy() : nullptr), itemToSell(sell ? sell->copy() : nullptr) {}
MerchantRecipe::MerchantRecipe(NBTTagCompound *tag) { readFromTags(tag); }
ItemStack *MerchantRecipe::getItemToBuy() const { return itemToBuy.get(); }
ItemStack *MerchantRecipe::getSecondItemToBuy() const { return secondItemToBuy.get(); }
ItemStack *MerchantRecipe::getItemToSell() const { return itemToSell.get(); }
bool MerchantRecipe::hasSecondItemToBuy() const { return secondItemToBuy != nullptr; }
bool MerchantRecipe::hasSameIDsAs(const MerchantRecipe &o) const
{
	return itemToBuy && o.itemToBuy && itemToSell && o.itemToSell && itemToBuy->itemID == o.itemToBuy->itemID && itemToSell->itemID == o.itemToSell->itemID &&
		((!secondItemToBuy && !o.secondItemToBuy) || (secondItemToBuy && o.secondItemToBuy && secondItemToBuy->itemID == o.secondItemToBuy->itemID));
}
bool MerchantRecipe::hasSameItemsAs(const MerchantRecipe &o) const
{
	return hasSameIDsAs(o) && (itemToBuy->stackSize < o.itemToBuy->stackSize || (secondItemToBuy && o.secondItemToBuy && secondItemToBuy->stackSize < o.secondItemToBuy->stackSize));
}
void MerchantRecipe::incrementToolUses() { ++toolUses; }
void MerchantRecipe::increaseMaxTradeUses(int amount) { maxTradeUses += amount; }
bool MerchantRecipe::isRecipeDisabled() const { return toolUses >= maxTradeUses; }
void MerchantRecipe::disableRecipe() { toolUses = maxTradeUses; }
void MerchantRecipe::readFromTags(NBTTagCompound *tag)
{
	if (!tag) return;
	itemToBuy.reset(ItemStack::loadItemStackFromNBT(tag->getCompoundTag("buy")));
	itemToSell.reset(ItemStack::loadItemStackFromNBT(tag->getCompoundTag("sell")));
	if (tag->hasKey("buyB")) secondItemToBuy.reset(ItemStack::loadItemStackFromNBT(tag->getCompoundTag("buyB")));
	toolUses = tag->hasKey("uses") ? tag->getInteger("uses") : 0;
	maxTradeUses = tag->hasKey("maxUses") ? tag->getInteger("maxUses") : 7;
}
NBTTagCompound *MerchantRecipe::writeToTags() const
{
	auto *tag = new NBTTagCompound();
	if (itemToBuy) tag->setCompoundTag("buy", itemToBuy->writeToNBT(new NBTTagCompound()));
	if (itemToSell) tag->setCompoundTag("sell", itemToSell->writeToNBT(new NBTTagCompound()));
	if (secondItemToBuy) tag->setCompoundTag("buyB", secondItemToBuy->writeToNBT(new NBTTagCompound()));
	tag->setInteger("uses", toolUses); tag->setInteger("maxUses", maxTradeUses);
	return tag;
}
