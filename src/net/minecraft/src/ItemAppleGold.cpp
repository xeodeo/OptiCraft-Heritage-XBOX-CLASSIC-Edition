#include "ItemAppleGold.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "Potion.h"
#include "PotionEffect.h"
#include "World.h"

ItemAppleGold::ItemAppleGold(int id, int healAmount, float saturationModifier, bool wolfFood) :
	ItemFood(id, healAmount, saturationModifier, wolfFood)
{
	setHasSubtypes(true);
}

bool ItemAppleGold::hasEffect(ItemStack *stack)
{
	return stack != nullptr && stack->getItemDamage() > 0;
}

const EnumRarity& ItemAppleGold::getRarity(ItemStack *stack)
{
	return stack != nullptr && stack->getItemDamage() > 0 ? EnumRarity::epic : EnumRarity::rare;
}

ItemStack *ItemAppleGold::onFoodEaten(ItemStack *stack, World *world, EntityPlayer *player)
{
	if (stack != nullptr && stack->getItemDamage() > 0)
	{
		if (world != nullptr && !world->multiplayerWorld && player != nullptr)
		{
			Potion::initPotions();
			player->addPotionEffect(new PotionEffect(Potion::regeneration->id, 600, 3));
			player->addPotionEffect(new PotionEffect(Potion::resistance->id, 6000, 0));
			player->addPotionEffect(new PotionEffect(Potion::fireResistance->id, 6000, 0));
		}
		return stack;
	}
	return ItemFood::onFoodEaten(stack, world, player);
}

std::vector<int> ItemAppleGold::getSubItemDamageValues() const { return {0, 1}; }
