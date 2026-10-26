#include "BlockCloth.h"
#include "ItemStack.h"
#include "Material.h"

BlockCloth::BlockCloth() : Block(35, 64, Material::cloth)
{
}

int_t BlockCloth::getBlockTextureFromSideAndMetadata(int_t i, int_t j)
{
    if (j == 0) return blockIndexInTexture;
    j = ~(j & 0xf);
    return 113 + ((j & 8) >> 3) + (j & 7) * 16;
}

int_t BlockCloth::damageDropped(int_t i)
{
    return i;
}

int_t BlockCloth::getColorFromDamage(int_t i)
{
    return ~i & 0xf;
}

int_t BlockCloth::getColorFromItemStack(int_t i)
{
    return ~i & 0xf;
}

int_t BlockCloth::getBlockFromDye(int_t i)
{
	return ~i & 15;
}

void BlockCloth::getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list)
{
	(void)tab;
	for (int_t i = 0; i < 16; ++i)
		list.push_back(new ItemStack(id, 1, i));
}

int_t BlockCloth::getDyeFromBlock(int_t i)
{
	return ~i & 15;
}
