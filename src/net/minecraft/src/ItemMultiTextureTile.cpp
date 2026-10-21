#include "ItemMultiTextureTile.h"

#include "Block.h"
#include "ItemStack.h"

ItemMultiTextureTile::ItemMultiTextureTile(int_t i, Block *block, const char *const *names, int_t nameCount) :
	ItemBlock(i),
	theBlock(block),
	field_82804_b(names),
	field_82804_b_count(nameCount)
{
	setMaxDamage(0);
	setHasSubtypes(true);
}

int_t ItemMultiTextureTile::getIconFromDamage(int_t i)
{
	return theBlock->getBlockTextureFromSideAndMetadata(2, i);
}

int_t ItemMultiTextureTile::getMetadata(int_t i)
{
	return i;
}

std::string ItemMultiTextureTile::getItemNameIS(ItemStack *itemstack)
{
	int_t var2 = itemstack->getItemDamage();
	if (var2 < 0 || var2 >= field_82804_b_count)
	{
		var2 = 0;
	}
	return ItemBlock::getItemName() + "." + field_82804_b[var2];
}
