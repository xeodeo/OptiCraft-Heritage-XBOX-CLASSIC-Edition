#include "ItemEditableBook.h"
#include "ItemWritableBook.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "NBTTagString.h"
#include "StatCollector.h"

ItemEditableBook::ItemEditableBook(int id)
    : Item(id)
{
    setMaxStackSize(1);
}

bool ItemEditableBook::validBookTagContents(NBTTagCompound *tag)
{
    if (!ItemWritableBook::validBookTagPages(tag))
    {
        return false;
    }
    if (tag == nullptr || !tag->hasKey("title"))
    {
        return false;
    }
    jstring title = tag->getString("title");
    if (title.empty() || title.length() > 16)
    {
        return false;
    }
    return tag->hasKey("author");
}

std::string ItemEditableBook::getItemDisplayName(ItemStack *itemstack)
{
    if (itemstack != nullptr && itemstack->hasTagCompound())
    {
        NBTTagCompound *tag = itemstack->getTagCompound();
        NBTTagString *titleTag = dynamic_cast<NBTTagString *>(tag->getTag("title"));
        if (titleTag != nullptr && !titleTag->data.empty())
        {
            return titleTag->data;
        }
    }
    return Item::getItemDisplayName(itemstack);
}

void ItemEditableBook::addInformation(ItemStack *itemstack, std::vector<std::string> &information)
{
    if (itemstack != nullptr && itemstack->hasTagCompound())
    {
        NBTTagCompound *tag = itemstack->getTagCompound();
        NBTTagString *authorTag = dynamic_cast<NBTTagString *>(tag->getTag("author"));
        if (authorTag != nullptr && !authorTag->data.empty())
        {
            information.push_back(std::string("\xc2\xa7""7") + StatCollector::translateToLocalFormatted("book.byAuthor", authorTag->data));
        }
    }
}

ItemStack *ItemEditableBook::onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *entityplayer)
{
    if (entityplayer != nullptr)
    {
        entityplayer->displayGUIBook(itemstack);
    }
    return itemstack;
}

bool ItemEditableBook::getShareTag()
{
    return true;
}

bool ItemEditableBook::hasEffect(ItemStack *itemstack)
{
    return true;
}
