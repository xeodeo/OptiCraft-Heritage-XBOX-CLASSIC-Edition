#pragma once
#include "Item.h"
class NBTTagCompound;
class ItemWritableBook : public Item { public: explicit ItemWritableBook(int id) : Item(id) { setMaxStackSize(1); } ItemStack *onItemRightClick(ItemStack*,World*,EntityPlayer*) override; bool getShareTag() const { return true; } static bool validBookTagPages(NBTTagCompound*); };
