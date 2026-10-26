#pragma once
#include "Item.h"
class NBTTagCompound;
class ItemWritableBook : public Item {
public:
    explicit ItemWritableBook(int id) : Item(id) { setMaxStackSize(1); }
    ItemStack *onItemRightClick(ItemStack*,World*,EntityPlayer*) override;
    bool getShareTag() const { return true; }
    static bool validBookTagPages(NBTTagCompound*);
    static int pageCount(NBTTagCompound*);
    static std::string sanitizePage(const std::string& page);
    static bool hasPages(NBTTagCompound* tag);
    static std::string pageAt(NBTTagCompound* tag, int index);
    static void appendPage(NBTTagCompound* tag, const std::string& page);
};
