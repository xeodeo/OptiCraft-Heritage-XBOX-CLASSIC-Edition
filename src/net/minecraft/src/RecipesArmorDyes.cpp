#include "RecipesArmorDyes.h"

#include <algorithm>
#include <vector>

#include "BlockCloth.h"
#include "EntitySheep.h"
#include "EnumArmorMaterial.h"
#include "InventoryCrafting.h"
#include "Item.h"
#include "ItemArmor.h"
#include "ItemStack.h"

bool RecipesArmorDyes::matches(InventoryCrafting *inventory)
{
	if (inventory == nullptr || Item::dyePowder == nullptr)
		return false;

	ItemStack *armorStack = nullptr;
	int_t dyeCount = 0;

	for (int_t i = 0; i < inventory->getSizeInventory(); ++i)
	{
		ItemStack *stack = inventory->getStackInSlot(i);
		if (stack == nullptr)
			continue;

		ItemArmor *armor = dynamic_cast<ItemArmor *>(stack->getItem());
		if (armor != nullptr)
		{
			if (armor->material != EnumArmorMaterial::CLOTH || armorStack != nullptr)
				return false;
			armorStack = stack;
		}
		else
		{
			if (stack->itemID != Item::dyePowder->shiftedIndex)
				return false;
			++dyeCount;
		}
	}

	return armorStack != nullptr && dyeCount > 0;
}

ItemStack *RecipesArmorDyes::getCraftingResult(InventoryCrafting *inventory)
{
	if (inventory == nullptr || Item::dyePowder == nullptr)
		return nullptr;

	ItemStack *result = nullptr;
	ItemArmor *armor = nullptr;
	int_t rgb[3] = { 0, 0, 0 };
	int_t maxSum = 0;
	int_t colorCount = 0;

	for (int_t i = 0; i < inventory->getSizeInventory(); ++i)
	{
		ItemStack *stack = inventory->getStackInSlot(i);
		if (stack == nullptr)
			continue;

		ItemArmor *candidate = dynamic_cast<ItemArmor *>(stack->getItem());
		if (candidate != nullptr)
		{
			if (candidate->material != EnumArmorMaterial::CLOTH || result != nullptr)
			{
				delete result;
				return nullptr;
			}
			armor = candidate;
			result = stack->copy();
			result->stackSize = 1;

			if (armor->hasColor(stack))
			{
				const int_t cur = armor->getColor(result);
				const float r = (float)((cur >> 16) & 255) / 255.0f;
				const float g = (float)((cur >> 8) & 255) / 255.0f;
				const float b = (float)(cur & 255) / 255.0f;
				maxSum = (int_t)((float)maxSum + std::max(r, std::max(g, b)) * 255.0f);
				rgb[0] = (int_t)((float)rgb[0] + r * 255.0f);
				rgb[1] = (int_t)((float)rgb[1] + g * 255.0f);
				rgb[2] = (int_t)((float)rgb[2] + b * 255.0f);
				++colorCount;
			}
		}
		else
		{
			if (stack->itemID != Item::dyePowder->shiftedIndex)
			{
				delete result;
				return nullptr;
			}
			const int_t fleeceIdx = BlockCloth::getColorFromDamage(stack->getItemDamage()) & 15;
			const float *fleece = EntitySheep::fleeceColorTable[fleeceIdx];
			const int_t r = (int_t)(fleece[0] * 255.0f);
			const int_t g = (int_t)(fleece[1] * 255.0f);
			const int_t b = (int_t)(fleece[2] * 255.0f);
			maxSum += std::max(r, std::max(g, b));
			rgb[0] += r;
			rgb[1] += g;
			rgb[2] += b;
			++colorCount;
		}
	}

	if (armor == nullptr || result == nullptr || colorCount <= 0)
	{
		delete result;
		return nullptr;
	}

	int_t avgR = rgb[0] / colorCount;
	int_t avgG = rgb[1] / colorCount;
	int_t avgB = rgb[2] / colorCount;
	const float targetMax = (float)maxSum / (float)colorCount;
	const float curMax = (float)std::max(avgR, std::max(avgG, avgB));
	if (curMax > 0.0f)
	{
		avgR = (int_t)((float)avgR * targetMax / curMax);
		avgG = (int_t)((float)avgG * targetMax / curMax);
		avgB = (int_t)((float)avgB * targetMax / curMax);
	}
	const int_t finalColor = (((avgR & 255) << 16) | ((avgG & 255) << 8) | (avgB & 255));
	armor->setColor(result, finalColor);
	return result;
}

int_t RecipesArmorDyes::computeBlendedColor(ItemStack *armorStack, int_t dyeDamage)
{
	int_t rgb[3] = { 0, 0, 0 };
	int_t maxSum = 0;
	int_t colorCount = 0;

	if (armorStack != nullptr)
	{
		ItemArmor *armor = dynamic_cast<ItemArmor *>(armorStack->getItem());
		if (armor != nullptr && armor->hasColor(armorStack))
		{
			const int_t cur = armor->getColor(armorStack);
			const float r = (float)((cur >> 16) & 255) / 255.0f;
			const float g = (float)((cur >> 8) & 255) / 255.0f;
			const float b = (float)(cur & 255) / 255.0f;
			maxSum = (int_t)((float)maxSum + std::max(r, std::max(g, b)) * 255.0f);
			rgb[0] = (int_t)((float)rgb[0] + r * 255.0f);
			rgb[1] = (int_t)((float)rgb[1] + g * 255.0f);
			rgb[2] = (int_t)((float)rgb[2] + b * 255.0f);
			++colorCount;
		}
	}

	const int_t fleeceIdx = BlockCloth::getColorFromDamage(dyeDamage) & 15;
	const float *fleece = EntitySheep::fleeceColorTable[fleeceIdx];
	const int_t r = (int_t)(fleece[0] * 255.0f);
	const int_t g = (int_t)(fleece[1] * 255.0f);
	const int_t b = (int_t)(fleece[2] * 255.0f);
	maxSum += std::max(r, std::max(g, b));
	rgb[0] += r;
	rgb[1] += g;
	rgb[2] += b;
	++colorCount;

	int_t avgR = rgb[0] / colorCount;
	int_t avgG = rgb[1] / colorCount;
	int_t avgB = rgb[2] / colorCount;
	const float targetMax = (float)maxSum / (float)colorCount;
	const float curMax = (float)std::max(avgR, std::max(avgG, avgB));
	if (curMax > 0.0f)
	{
		avgR = (int_t)((float)avgR * targetMax / curMax);
		avgG = (int_t)((float)avgG * targetMax / curMax);
		avgB = (int_t)((float)avgB * targetMax / curMax);
	}
	return ((avgR & 255) << 16) | ((avgG & 255) << 8) | (avgB & 255);
}

int_t RecipesArmorDyes::getRecipeSize()
{
	return 10;
}

ItemStack *RecipesArmorDyes::getRecipeOutput()
{
	return nullptr;
}
