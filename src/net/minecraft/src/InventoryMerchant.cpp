#include "InventoryMerchant.h"

#include "EntityPlayer.h"
#include "IMerchant.h"
#include "ItemStack.h"
#include "MerchantRecipe.h"
#include "MerchantRecipeList.h"

InventoryMerchant::InventoryMerchant(EntityPlayer *player, IMerchant *merchant) : thePlayer(player), theMerchant(merchant) {}
InventoryMerchant::~InventoryMerchant() { for (ItemStack *stack : inventory) delete stack; }
int_t InventoryMerchant::getSizeInventory() { return 3; }
ItemStack *InventoryMerchant::getStackInSlot(int_t slot) { return slot >= 0 && slot < 3 ? inventory[slot] : nullptr; }
ItemStack *InventoryMerchant::getStackInSlotOnClosing(int_t slot)
{
	if (slot < 0 || slot >= 3) return nullptr;
	ItemStack *stack = inventory[slot]; inventory[slot] = nullptr;
	if (slot < 2) resetRecipeAndSlots();
	return stack;
}
ItemStack *InventoryMerchant::decrStackSize(int_t slot, int_t count)
{
	if (slot < 0 || slot >= 3 || inventory[slot] == nullptr) return nullptr;
	if (slot == 2 || inventory[slot]->stackSize <= count)
	{
		ItemStack *result = inventory[slot]; inventory[slot] = nullptr;
		if (slot < 2) resetRecipeAndSlots();
		return result;
	}
	ItemStack *result = inventory[slot]->splitStack(count);
	if (inventory[slot]->stackSize <= 0) { delete inventory[slot]; inventory[slot] = nullptr; }
	if (slot < 2) resetRecipeAndSlots();
	return result;
}
void InventoryMerchant::setInventorySlotContents(int_t slot, ItemStack *stack)
{
	if (slot < 0 || slot >= 3) { delete stack; return; }
	if (inventory[slot] != stack) delete inventory[slot];
	inventory[slot] = stack;
	if (stack && stack->stackSize > 64) stack->stackSize = 64;
	if (slot < 2) resetRecipeAndSlots();
}
std::string InventoryMerchant::getInvName() { return "mob.villager"; }
int_t InventoryMerchant::getInventoryStackLimit() { return 64; }
void InventoryMerchant::onInventoryChanged() { resetRecipeAndSlots(); }
bool InventoryMerchant::canInteractWith(EntityPlayer *player) { return theMerchant && theMerchant->getCustomer() == player; }
bool InventoryMerchant::isStackValidForSlot(int_t, ItemStack *) { return true; }
MerchantRecipe *InventoryMerchant::getCurrentRecipe() const { return currentRecipe; }
void InventoryMerchant::setCurrentRecipeIndex(int_t index) { currentRecipeIndex = index; resetRecipeAndSlots(); }
void InventoryMerchant::resetRecipeAndSlots()
{
	currentRecipe = nullptr;
	ItemStack *first = inventory[0], *second = inventory[1];
	if (!first) { first = second; second = nullptr; }
	ItemStack *replacement = nullptr;
	if (first && theMerchant && thePlayer)
	{
		MerchantRecipeList *recipes = theMerchant->getRecipes(thePlayer);
		MerchantRecipe *recipe = recipes ? recipes->canRecipeBeUsed(first, second, currentRecipeIndex) : nullptr;
		if (!recipe && second) recipe = recipes->canRecipeBeUsed(second, first, currentRecipeIndex);
		if (recipe && !recipe->isRecipeDisabled()) { currentRecipe = recipe; replacement = recipe->getItemToSell()->copy(); }
	}
	delete inventory[2]; inventory[2] = replacement;
}
