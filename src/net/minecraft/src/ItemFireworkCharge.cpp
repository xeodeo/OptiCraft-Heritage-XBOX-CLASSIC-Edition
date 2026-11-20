#include "ItemFireworkCharge.h"

#include "ItemDye.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "NBTTagIntArray.h"
#include "StatCollector.h"
#include "Texture152Atlas.h"

namespace
{
std::string trimString(const std::string &value)
{
    size_t start = 0;
    while (start < value.size() && (value[start] == ' ' || value[start] == '\t' || value[start] == '\r' || value[start] == '\n'))
        ++start;
    size_t end = value.size();
    while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r' || value[end - 1] == '\n'))
        --end;
    return value.substr(start, end - start);
}

std::string formatColorList(const std::vector<int_t> &colors, const std::string &prefix)
{
    std::string out = prefix;
    bool first = true;
    for (int_t color : colors)
    {
        if (!first)
            out += ", ";
        first = false;

        bool matched = false;
        for (int i = 0; i < 16; ++i)
        {
            if (color == ItemDye::worldChunkManager[i])
            {
                matched = true;
                out += StatCollector::translateToLocal("item.fireworksCharge." + ItemDye::dyeColors[i]);
                break;
            }
        }
        if (!matched)
        {
            out += StatCollector::translateToLocal("item.fireworksCharge.customColor");
        }
    }
    return out;
}
} // namespace

ItemFireworkCharge::ItemFireworkCharge(int id)
    : Item(id)
{
}

bool ItemFireworkCharge::func_46058_c()
{
    return true;
}

int ItemFireworkCharge::func_46057_a(int damage, int renderPass)
{
    if (renderPass > 0)
        return TEX152_ITEM_ICON("fireworksCharge_overlay.png");
    return Item::func_46057_a(damage, renderPass);
}

int ItemFireworkCharge::getColorFromItemStack(ItemStack *itemstack, int renderPass)
{
    if (renderPass != 1)
        return Item::getColorFromItemStack(itemstack, renderPass);

    NBTBase *tag = getExplosionTag(itemstack, "Colors");
    NBTTagIntArray *intArray = dynamic_cast<NBTTagIntArray *>(tag);
    if (intArray == nullptr || intArray->intArray.empty())
        return 9079434; // 0x8A8A8A default gray

    if (intArray->intArray.size() == 1)
        return intArray->intArray[0];

    int r = 0;
    int g = 0;
    int b = 0;
    for (int_t color : intArray->intArray)
    {
        r += (color & 0xFF0000) >> 16;
        g += (color & 0x00FF00) >> 8;
        b += (color & 0x0000FF);
    }

    const int count = static_cast<int>(intArray->intArray.size());
    r /= count;
    g /= count;
    b /= count;
    return (r << 16) | (g << 8) | b;
}

NBTBase *ItemFireworkCharge::getExplosionTag(ItemStack *itemstack, const std::string &key)
{
    if (itemstack == nullptr || !itemstack->hasTagCompound())
        return nullptr;

    NBTTagCompound *root = itemstack->getTagCompound();
    if (root == nullptr || !root->hasKey("Explosion"))
        return nullptr;

    NBTTagCompound *explosion = root->getCompoundTag("Explosion");
    if (explosion == nullptr)
        return nullptr;

    return explosion->getTag(jstring(key));
}

void ItemFireworkCharge::addInformation(ItemStack *itemstack, std::vector<std::string> &information)
{
    if (itemstack == nullptr || !itemstack->hasTagCompound())
        return;

    NBTTagCompound *root = itemstack->getTagCompound();
    if (root == nullptr || !root->hasKey("Explosion"))
        return;

    NBTTagCompound *explosion = root->getCompoundTag("Explosion");
    if (explosion != nullptr)
        addExplosionInfo(explosion, information);
}

void ItemFireworkCharge::addExplosionInfo(NBTTagCompound *explosion, std::vector<std::string> &information)
{
    if (explosion == nullptr)
        return;

    const byte_t shape = explosion->getByte("Type");
    if (shape >= 0 && shape <= 4)
    {
        information.push_back(trimString(StatCollector::translateToLocal("item.fireworksCharge.type." + std::to_string(static_cast<int>(shape)))));
    }
    else
    {
        information.push_back(trimString(StatCollector::translateToLocal("item.fireworksCharge.type")));
    }

    const std::vector<int_t> colors = explosion->getIntArray("Colors");
    if (!colors.empty())
    {
        information.push_back(formatColorList(colors, ""));
    }

    const std::vector<int_t> fadeColors = explosion->getIntArray("FadeColors");
    if (!fadeColors.empty())
    {
        information.push_back(formatColorList(fadeColors, StatCollector::translateToLocal("item.fireworksCharge.fadeTo") + " "));
    }

    if (explosion->getBoolean("Trail"))
    {
        information.push_back(StatCollector::translateToLocal("item.fireworksCharge.trail"));
    }

    if (explosion->getBoolean("Flicker"))
    {
        information.push_back(StatCollector::translateToLocal("item.fireworksCharge.flicker"));
    }
}
