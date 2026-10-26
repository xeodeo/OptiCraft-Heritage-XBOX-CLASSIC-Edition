#include "ItemWritableBook.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "NBTTagString.h"

ItemWritableBook::ItemWritableBook(int id)
    : Item(id)
{
    setMaxStackSize(1);
}

ItemStack *ItemWritableBook::onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *entityplayer)
{
    if (entityplayer != nullptr)
    {
        entityplayer->displayGUIBook(itemstack);
    }
    return itemstack;
}

bool ItemWritableBook::getShareTag()
{
    return true;
}

bool ItemWritableBook::validBookTagPages(NBTTagCompound *var0)
{
    if (var0 == nullptr)
    {
        return false;
    }
    if (!var0->hasKey("pages"))
    {
        return false;
    }
    NBTTagList *var1 = (NBTTagList *)var0->getTag("pages");
    if (var1 == nullptr)
    {
        return false;
    }
    for (int var2 = 0; var2 < var1->tagCount(); var2++)
    {
        NBTTagString *var3 = (NBTTagString *)var1->tagAt(var2);
        if (var3 == nullptr)
        {
            return false;
        }
        if (var3->data.length() > 256)
        {
            return false;
        }
    }
    return true;
}
