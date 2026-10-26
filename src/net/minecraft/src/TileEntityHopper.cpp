#include "TileEntityHopper.h"
#include "BlockHopper.h"
#include "World.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "ItemStack.h"
#include "EntityItem.h"
#include "EntityPlayer.h"
#include "Facing.h"
#include "ISidedInventory.h"
#include "AxisAlignedBB.h"
#include "MathHelper.h"
#include <algorithm>

TileEntityHopper::TileEntityHopper()
{
	for (int_t i = 0; i < 5; ++i)
	{
		hopperItemStacks[i] = nullptr;
	}
	transferCooldown = -1;
}

TileEntityHopper::~TileEntityHopper()
{
	for (int_t i = 0; i < 5; ++i)
	{
		delete hopperItemStacks[i];
		hopperItemStacks[i] = nullptr;
	}
}

void TileEntityHopper::readFromNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::readFromNBT(nbttagcompound);
	NBTTagList *itemsTag = nbttagcompound->getTagList("Items");
	for (int_t i = 0; i < 5; ++i)
	{
		delete hopperItemStacks[i];
		hopperItemStacks[i] = nullptr;
	}
	if (nbttagcompound->hasKey("CustomName"))
	{
		inventoryName = nbttagcompound->getString("CustomName");
	}
	transferCooldown = nbttagcompound->getInteger("TransferCooldown");

	if (itemsTag != nullptr)
	{
		for (int_t i = 0; i < itemsTag->tagCount(); ++i)
		{
			NBTTagCompound *slotTag = dynamic_cast<NBTTagCompound *>(itemsTag->tagAt(i));
			if (slotTag != nullptr)
			{
				int_t slot = slotTag->getByte("Slot") & 255;
				if (slot >= 0 && slot < 5)
				{
					hopperItemStacks[slot] = new ItemStack(slotTag);
				}
			}
		}
	}
}

void TileEntityHopper::writeToNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::writeToNBT(nbttagcompound);
	NBTTagList *itemsTag = new NBTTagList();
	for (int_t i = 0; i < 5; ++i)
	{
		if (hopperItemStacks[i] != nullptr)
		{
			NBTTagCompound *slotTag = new NBTTagCompound();
			slotTag->setByte("Slot", static_cast<char_t>(i));
			hopperItemStacks[i]->writeToNBT(slotTag);
			itemsTag->setTag(slotTag);
		}
	}
	nbttagcompound->setTag("Items", itemsTag);
	nbttagcompound->setInteger("TransferCooldown", transferCooldown);
	if (isInvNameLocalized())
	{
		nbttagcompound->setString("CustomName", inventoryName);
	}
}

int_t TileEntityHopper::getSizeInventory()
{
	return 5;
}

ItemStack *TileEntityHopper::getStackInSlot(int_t i)
{
	if (i >= 0 && i < 5)
		return hopperItemStacks[i];
	return nullptr;
}

ItemStack *TileEntityHopper::decrStackSize(int_t i, int_t j)
{
	if (i < 0 || i >= 5 || hopperItemStacks[i] == nullptr)
		return nullptr;

	if (hopperItemStacks[i]->stackSize <= j)
	{
		ItemStack *stack = hopperItemStacks[i];
		hopperItemStacks[i] = nullptr;
		return stack;
	}

	ItemStack *split = hopperItemStacks[i]->splitStack(j);
	if (hopperItemStacks[i]->stackSize == 0)
	{
		delete hopperItemStacks[i];
		hopperItemStacks[i] = nullptr;
	}
	return split;
}

ItemStack *TileEntityHopper::getStackInSlotOnClosing(int_t i)
{
	if (i >= 0 && i < 5 && hopperItemStacks[i] != nullptr)
	{
		ItemStack *stack = hopperItemStacks[i];
		hopperItemStacks[i] = nullptr;
		return stack;
	}
	return nullptr;
}

void TileEntityHopper::setInventorySlotContents(int_t i, ItemStack *itemstack)
{
	if (i >= 0 && i < 5)
	{
		if (hopperItemStacks[i] != itemstack)
		{
			delete hopperItemStacks[i];
		}
		hopperItemStacks[i] = itemstack;
		if (itemstack != nullptr && itemstack->stackSize > getInventoryStackLimit())
		{
			itemstack->stackSize = getInventoryStackLimit();
		}
	}
}

std::string TileEntityHopper::getInvName()
{
	return isInvNameLocalized() ? inventoryName : "container.hopper";
}

int_t TileEntityHopper::getInventoryStackLimit()
{
	return 64;
}

void TileEntityHopper::onInventoryChanged()
{
	TileEntity::onInventoryChanged();
}

bool TileEntityHopper::canInteractWith(EntityPlayer *entityplayer)
{
	if (worldObj == nullptr || worldObj->getBlockTileEntity(xCoord, yCoord, zCoord) != this)
		return false;
	return entityplayer->getDistanceSq((double)xCoord + 0.5, (double)yCoord + 0.5, (double)zCoord + 0.5) <= 64.0;
}

void TileEntityHopper::updateEntity()
{
	if (worldObj != nullptr && !worldObj->multiplayerWorld)
	{
		transferCooldown--;
		if (!isCoolingDown())
		{
			setTransferCooldown(0);
			func_98045_j();
		}
	}
}

bool TileEntityHopper::func_98045_j()
{
	if (worldObj != nullptr && !worldObj->multiplayerWorld)
	{
		if (!isCoolingDown() && BlockHopper::getIsBlockNotPoweredFromMetadata(getBlockMetadata()))
		{
			bool changed = insertItemToInventory();
			changed = suckItemsIntoHopper(this) || changed;
			if (changed)
			{
				setTransferCooldown(8);
				onInventoryChanged();
				return true;
			}
		}
	}
	return false;
}

bool TileEntityHopper::insertItemToInventory()
{
	IInventory *outputInv = getOutputInventory();
	if (outputInv == nullptr)
	{
		return false;
	}

	int_t dir = BlockHopper::getDirectionFromMetadata(getBlockMetadata());
	int_t targetSide = FacingUtil::oppositeSide[dir];

	for (int_t slot = 0; slot < getSizeInventory(); ++slot)
	{
		if (getStackInSlot(slot) != nullptr)
		{
			ItemStack *original = getStackInSlot(slot)->copy();
			ItemStack *toInsert = decrStackSize(slot, 1);
			ItemStack *remaining = insertStack(outputInv, toInsert, targetSide);
			if (remaining == nullptr || remaining->stackSize == 0)
			{
				delete original;
				delete remaining;
				outputInv->onInventoryChanged();
				return true;
			}
			setInventorySlotContents(slot, original);
			delete remaining;
		}
	}
	return false;
}

IInventory *TileEntityHopper::getOutputInventory()
{
	int_t dir = BlockHopper::getDirectionFromMetadata(getBlockMetadata());
	return getInventoryAtLocation(
		getWorldObj(),
		(double)(xCoord + FacingUtil::offsetsXForSide[dir]),
		(double)(yCoord + FacingUtil::offsetsYForSide[dir]),
		(double)(zCoord + FacingUtil::offsetsZForSide[dir])
	);
}

bool TileEntityHopper::suckItemsIntoHopper(Hopper *hopper)
{
	IInventory *aboveInv = getInventoryAboveHopper(hopper);
	if (aboveInv != nullptr)
	{
		const int_t side = 0; // extracted from bottom of container above
		ISidedInventory *sided = dynamic_cast<ISidedInventory *>(aboveInv);
		if (sided != nullptr)
		{
			std::vector<int_t> accessible = sided->getAccessibleSlotsFromSide(side);
			for (int_t slot : accessible)
			{
				if (func_102012_a(hopper, aboveInv, slot, side))
					return true;
			}
		}
		else
		{
			const int_t size = aboveInv->getSizeInventory();
			for (int_t slot = 0; slot < size; ++slot)
			{
				if (func_102012_a(hopper, aboveInv, slot, side))
					return true;
			}
		}
	}
	else
	{
		EntityItem *entityItem = func_96119_a(hopper->getWorldObj(), hopper->getXPos(), hopper->getYPos() + 1.0, hopper->getZPos());
		if (entityItem != nullptr)
		{
			return func_96114_a(hopper->getHopperInventory(), entityItem);
		}
	}
	return false;
}

bool TileEntityHopper::func_102012_a(Hopper *hopper, IInventory *inv, int_t slot, int_t side)
{
	ItemStack *stack = inv->getStackInSlot(slot);
	if (stack != nullptr && canExtractItemFromInventory(inv, stack, slot, side))
	{
		ItemStack *original = stack->copy();
		ItemStack *toInsert = inv->decrStackSize(slot, 1);
		ItemStack *remaining = insertStack(hopper->getHopperInventory(), toInsert, -1);
		if (remaining == nullptr || remaining->stackSize == 0)
		{
			delete original;
			delete remaining;
			inv->onInventoryChanged();
			return true;
		}
		inv->setInventorySlotContents(slot, original);
		delete remaining;
	}
	return false;
}

bool TileEntityHopper::func_96114_a(IInventory *inv, EntityItem *entityItem)
{
	if (entityItem == nullptr || entityItem->item == nullptr)
		return false;

	ItemStack *copy = entityItem->item->copy();
	ItemStack *remaining = insertStack(inv, copy, -1);
	if (remaining != nullptr && remaining->stackSize > 0)
	{
		entityItem->item->stackSize = remaining->stackSize;
		delete remaining;
		return false;
	}
	else
	{
		delete remaining;
		entityItem->setEntityDead();
		return true;
	}
}

ItemStack *TileEntityHopper::insertStack(IInventory *inv, ItemStack *stack, int_t side)
{
	if (inv == nullptr || stack == nullptr || stack->stackSize <= 0)
		return stack;

	ISidedInventory *sided = dynamic_cast<ISidedInventory *>(inv);
	if (sided != nullptr && side > -1)
	{
		std::vector<int_t> accessible = sided->getAccessibleSlotsFromSide(side);
		for (size_t i = 0; i < accessible.size() && stack != nullptr && stack->stackSize > 0; ++i)
		{
			stack = func_102014_c(inv, stack, accessible[i], side);
		}
	}
	else
	{
		const int_t size = inv->getSizeInventory();
		for (int_t slot = 0; slot < size && stack != nullptr && stack->stackSize > 0; ++slot)
		{
			stack = func_102014_c(inv, stack, slot, side);
		}
	}

	if (stack != nullptr && stack->stackSize == 0)
	{
		delete stack;
		stack = nullptr;
	}
	return stack;
}

ItemStack *TileEntityHopper::func_102014_c(IInventory *inv, ItemStack *stack, int_t slot, int_t side)
{
	ItemStack *inSlot = inv->getStackInSlot(slot);
	if (func_102015_a(inv, stack, slot, side))
	{
		bool changed = false;
		if (inSlot == nullptr)
		{
			inv->setInventorySlotContents(slot, stack);
			stack = nullptr;
			changed = true;
		}
		else if (areItemStacksEqualItem(inSlot, stack))
		{
			int_t maxStack = std::min(inv->getInventoryStackLimit(), inSlot->getMaxStackSize());
			int_t space = maxStack - inSlot->stackSize;
			int_t toMove = std::min(stack->stackSize, space);
			if (toMove > 0)
			{
				stack->stackSize -= toMove;
				inSlot->stackSize += toMove;
				changed = true;
			}
		}

		if (changed)
		{
			TileEntityHopper *hopper = dynamic_cast<TileEntityHopper *>(inv);
			if (hopper != nullptr)
			{
				hopper->setTransferCooldown(8);
			}
			inv->onInventoryChanged();
		}
	}
	return stack;
}

bool TileEntityHopper::func_102015_a(IInventory *inv, ItemStack *stack, int_t slot, int_t side)
{
	if (!inv->isStackValidForSlot(slot, stack))
		return false;
	ISidedInventory *sided = dynamic_cast<ISidedInventory *>(inv);
	if (sided != nullptr)
		return sided->canInsertItem(slot, stack, side);
	return true;
}

bool TileEntityHopper::canExtractItemFromInventory(IInventory *inv, ItemStack *stack, int_t slot, int_t side)
{
	ISidedInventory *sided = dynamic_cast<ISidedInventory *>(inv);
	if (sided != nullptr)
		return sided->canExtractItem(slot, stack, side);
	return true;
}

IInventory *TileEntityHopper::getInventoryAboveHopper(Hopper *hopper)
{
	return getInventoryAtLocation(hopper->getWorldObj(), hopper->getXPos(), hopper->getYPos() + 1.0, hopper->getZPos());
}

IInventory *TileEntityHopper::getInventoryAtLocation(World *world, double x, double y, double z)
{
	if (world == nullptr)
		return nullptr;

	int_t bx = MathHelper::floor_double(x);
	int_t by = MathHelper::floor_double(y);
	int_t bz = MathHelper::floor_double(z);

	TileEntity *te = world->getBlockTileEntity(bx, by, bz);
	if (te != nullptr)
	{
		IInventory *inv = dynamic_cast<IInventory *>(te);
		if (inv != nullptr)
		{
			return inv;
		}
	}

	// No tile entity container found. Check for minecart container in AABB.
	AxisAlignedBB *box = AxisAlignedBB::getBoundingBoxFromPool(x, y, z, x + 1.0, y + 1.0, z + 1.0);
	const std::vector<Entity *> &entities = world->getEntitiesWithinAABBExcludingEntity(nullptr, box);
	for (Entity *ent : entities)
	{
		IInventory *inv = dynamic_cast<IInventory *>(ent);
		if (inv != nullptr)
		{
			return inv;
		}
	}

	return nullptr;
}

EntityItem *TileEntityHopper::func_96119_a(World *world, double x, double y, double z)
{
	if (world == nullptr)
		return nullptr;

	AxisAlignedBB *box = AxisAlignedBB::getBoundingBoxFromPool(x, y, z, x + 1.0, y + 1.0, z + 1.0);
	const std::vector<Entity *> &entities = world->getEntitiesWithinAABB(typeid(EntityItem), box);
	if (!entities.empty())
	{
		return dynamic_cast<EntityItem *>(entities[0]);
	}
	return nullptr;
}

bool TileEntityHopper::areItemStacksEqualItem(ItemStack *a, ItemStack *b)
{
	if (a == nullptr || b == nullptr)
		return false;
	if (a->itemID != b->itemID)
		return false;
	if (a->getItemDamage() != b->getItemDamage())
		return false;
	return ItemStack::areItemStackTagsEqual(a, b);
}

World *TileEntityHopper::getWorldObj()
{
	return worldObj;
}

double TileEntityHopper::getXPos()
{
	return (double)xCoord;
}

double TileEntityHopper::getYPos()
{
	return (double)yCoord;
}

double TileEntityHopper::getZPos()
{
	return (double)zCoord;
}

void TileEntityHopper::setTransferCooldown(int_t cooldown)
{
	transferCooldown = cooldown;
}

bool TileEntityHopper::isCoolingDown() const
{
	return transferCooldown > 0;
}

bool TileEntityHopper::isInvNameLocalized() const
{
	return !inventoryName.empty();
}

void TileEntityHopper::setInventoryName(const std::string &name)
{
	inventoryName = name;
}
