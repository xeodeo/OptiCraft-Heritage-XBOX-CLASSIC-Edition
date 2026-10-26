#pragma once
#include "Item.h"
class NBTTagCompound;
class ItemEditableBook : public Item { public: explicit ItemEditableBook(int id) : Item(id) { setMaxStackSize(1); } ItemStack *onItemRightClick(ItemStack*,World*,EntityPlayer*) override; bool hasEffect(ItemStack*) override; std::string getItemDisplayName(ItemStack*) override; static bool validBookTagContents(NBTTagCompound*); };
