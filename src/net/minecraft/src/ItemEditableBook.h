#pragma once
#include "Item.h"
class NBTTagCompound;
class ItemEditableBook : public Item {
public:
    explicit ItemEditableBook(int id) : Item(id) { setMaxStackSize(1); }
    ItemStack *onItemRightClick(ItemStack*,World*,EntityPlayer*) override;
    bool hasEffect(ItemStack*) override;
    std::string getItemDisplayName(ItemStack*) override;
    void addInformation(ItemStack*,std::vector<std::string>&) override;
    static bool validBookTagContents(NBTTagCompound*);
    static std::string title(NBTTagCompound*);
    static std::string author(NBTTagCompound*);
    static int pageCount(NBTTagCompound*);
    static bool isSigned(NBTTagCompound*);
    static bool hasPages(NBTTagCompound*);
    static int titleLength(NBTTagCompound*);
    static bool canOpen(NBTTagCompound*);
};
