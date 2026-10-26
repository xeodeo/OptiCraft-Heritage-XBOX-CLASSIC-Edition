#include "EntityPainting.h"
#include "EnumArt.h"
#include "Item.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "World.h"
#include <vector>

EntityPainting::EntityPainting(World *world)
	: EntityHanging(world)
{
}

EntityPainting::EntityPainting(World *world, int_t i, int_t j, int_t k, int_t l)
	: EntityHanging(world, i, j, k, l)
{
	std::vector<const EnumArt *> arraylist;
	const EnumArt **aenumart = EnumArt::values;
	int i1 = EnumArt::valuesCount;
	for (int j1 = 0; j1 < i1; j1++)
	{
		const EnumArt *enumart = aenumart[j1];
		art = enumart;
		setDirection(l);
		if (onValidSurface())
		{
			arraylist.push_back(enumart);
		}
	}
	if (!arraylist.empty())
	{
		art = arraylist[rand.nextInt(arraylist.size())];
	}
	setDirection(l);
}

EntityPainting::EntityPainting(World *world, int_t i, int_t j, int_t k, int_t l, const jstring &s)
	: EntityPainting(world, i, j, k, l)
{
	const EnumArt **aenumart = EnumArt::values;
	int i1 = EnumArt::valuesCount;
	for (int j1 = 0; j1 < i1; j1++)
	{
		const EnumArt *enumart = aenumart[j1];
		if (enumart->title == s)
		{
			art = enumart;
			break;
		}
	}
	setDirection(l);
}

void EntityPainting::writeEntityToNBT(NBTTagCompound *nbttagcompound)
{
	if (art != nullptr)
	{
		nbttagcompound->setString("Motive", art->title);
	}
	EntityHanging::writeEntityToNBT(nbttagcompound);
}

void EntityPainting::readEntityFromNBT(NBTTagCompound *nbttagcompound)
{
	jstring s = nbttagcompound->getString("Motive");
	const EnumArt **aenumart = EnumArt::values;
	int i = EnumArt::valuesCount;
	for (int j = 0; j < i; j++)
	{
		const EnumArt *enumart = aenumart[j];
		if (enumart->title == s)
		{
			art = enumart;
			break;
		}
	}

	if (art == nullptr)
	{
		art = &EnumArt::Kebab;
	}
	EntityHanging::readEntityFromNBT(nbttagcompound);
}

int EntityPainting::func_82329_d()
{
	return art != nullptr ? art->sizeX : 16;
}

int EntityPainting::func_82330_g()
{
	return art != nullptr ? art->sizeY : 16;
}

void EntityPainting::dropItemStack()
{
	entityDropItem(new ItemStack(Item::painting), 0.0f);
}
