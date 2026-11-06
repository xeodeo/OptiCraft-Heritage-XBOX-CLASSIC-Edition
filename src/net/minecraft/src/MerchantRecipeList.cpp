#include "MerchantRecipeList.h"

#include "ItemStack.h"
#include "MerchantRecipe.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"

MerchantRecipeList::MerchantRecipeList(NBTTagCompound *tag) { if (tag) readRecipiesFromTags(tag); }
int MerchantRecipeList::size() const { return static_cast<int>(recipes.size()); }
bool MerchantRecipeList::empty() const { return recipes.empty(); }
MerchantRecipe *MerchantRecipeList::get(int index) const { return index >= 0 && index < size() ? recipes[index].get() : nullptr; }
void MerchantRecipeList::add(MerchantRecipe *recipe) { if (recipe) recipes.emplace_back(recipe); }
void MerchantRecipeList::addToListWithCheck(MerchantRecipe *recipe)
{
	if (!recipe) return;
	for (auto &current : recipes)
		if (recipe->hasSameIDsAs(*current))
		{
			if (recipe->hasSameItemsAs(*current)) current.reset(recipe); else delete recipe;
			return;
		}
	add(recipe);
}
MerchantRecipe *MerchantRecipeList::canRecipeBeUsed(ItemStack *first, ItemStack *second, int index) const
{
	auto matches = [](ItemStack *given, ItemStack *needed) { return given && needed && given->itemID == needed->itemID && given->stackSize >= needed->stackSize; };
	if (!first) return nullptr;
	if (index > 0 && index < size())
	{
		MerchantRecipe *candidate = get(index);
		if (candidate && matches(first, candidate->getItemToBuy()) &&
			((!candidate->hasSecondItemToBuy() && !second) || matches(second, candidate->getSecondItemToBuy()))) return candidate;
		return nullptr;
	}
	for (const auto &recipe : recipes)
		if (matches(first, recipe->getItemToBuy()) && ((!recipe->hasSecondItemToBuy() && !second) || matches(second, recipe->getSecondItemToBuy()))) return recipe.get();
	return nullptr;
}
void MerchantRecipeList::readRecipiesFromTags(NBTTagCompound *tag)
{
	if (!tag || !tag->hasKey("Recipes")) return;
	NBTTagList *list = tag->getTagList("Recipes");
	for (int i = 0; i < list->tagCount(); ++i)
		if (auto *entry = dynamic_cast<NBTTagCompound *>(list->tagAt(i)))
		{
			// An offer whose item no longer loads would leave a null stack every screen dereferences.
			auto *recipe = new MerchantRecipe(entry);
			if (recipe->getItemToBuy() && recipe->getItemToSell()) add(recipe); else delete recipe;
		}
}
NBTTagCompound *MerchantRecipeList::getRecipiesAsTags() const
{
	auto *tag = new NBTTagCompound(); auto *list = new NBTTagList();
	for (const auto &recipe : recipes) list->appendTag(recipe->writeToTags());
	tag->setTag("Recipes", list); return tag;
}
