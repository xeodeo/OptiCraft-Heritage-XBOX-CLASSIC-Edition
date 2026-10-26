#include "ContainerMerchant.h"

#include "EntityPlayer.h"
#include "IMerchant.h"
#include "InventoryMerchant.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"
#include "Slot.h"
#include "SlotMerchantResult.h"
#include "World.h"

ContainerMerchant::ContainerMerchant(InventoryPlayer *inventory, IMerchant *merchant, World *world)
	: theMerchant(merchant), merchantInventory(new InventoryMerchant(inventory ? inventory->player : nullptr, merchant)), theWorld(world)
{
	addSlot(new Slot(merchantInventory, 0, 36, 53));
	addSlot(new Slot(merchantInventory, 1, 62, 53));
	addSlot(new SlotMerchantResult(inventory ? inventory->player : nullptr, merchant, merchantInventory, 2, 120, 53));
	if (inventory)
	{
		for (int row = 0; row < 3; ++row)
			for (int col = 0; col < 9; ++col)
				addSlot(new Slot(inventory, col + row * 9 + 9, 8 + col * 18, 84 + row * 18));
		for (int col = 0; col < 9; ++col) addSlot(new Slot(inventory, col, 8 + col * 18, 142));
	}
}
ContainerMerchant::~ContainerMerchant() { delete merchantInventory; }
InventoryMerchant *ContainerMerchant::getMerchantInventory() const { return merchantInventory; }
void ContainerMerchant::setCurrentRecipeIndex(int_t index) { merchantInventory->setCurrentRecipeIndex(index); }
void ContainerMerchant::onCraftMatrixChanged(IInventory *inventory) { (void)inventory; merchantInventory->resetRecipeAndSlots(); Container::onCraftMatrixChanged(merchantInventory); }
bool ContainerMerchant::isUsableByPlayer(EntityPlayer *player) { return theMerchant && theMerchant->getCustomer() == player; }
void ContainerMerchant::onCraftGuiClosed(EntityPlayer *player)
{
	Container::onCraftGuiClosed(player);
	if (theMerchant) theMerchant->setCustomer(nullptr);
	if (player && theWorld && !theWorld->multiplayerWorld)
		for (int slot = 0; slot < 2; ++slot)
			if (ItemStack *stack = merchantInventory->getStackInSlotOnClosing(slot)) player->dropPlayerItem(stack);
}
