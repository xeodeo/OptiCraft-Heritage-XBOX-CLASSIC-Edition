#include "ItemArmor.h"

#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "StatCollector.h"
#include "Texture152Atlas.h"

const int ItemArmor::maxDamageArray[4] = { 11, 16, 15, 13 };

ItemArmor::ItemArmor(int id, EnumArmorMaterial armorMaterial, int render, int type)
    : Item(id), material(armorMaterial), armorType(type),
      damageReduceAmount(EnumArmorMaterialHelper::getDamageReductionAmount(armorMaterial, type)),
      renderIndex(render)
{
    setMaxDamage(EnumArmorMaterialHelper::getDurability(armorMaterial, type));
    maxStackSize = 1;
}

int ItemArmor::getItemEnchantability()
{
    return EnumArmorMaterialHelper::getEnchantability(material);
}

bool ItemArmor::getIsRepairable(ItemStack* stack1, ItemStack* stack2)
{
    if (stack2 != nullptr && EnumArmorMaterialHelper::getArmorCraftingMaterial(material) == stack2->itemID)
    {
        return true;
    }
    return Item::getIsRepairable(stack1, stack2);
}

bool ItemArmor::func_46058_c()
{
    return material == EnumArmorMaterial::CLOTH;
}

int ItemArmor::func_46057_a(int damage, int renderPass)
{
    if (renderPass == 1 && material == EnumArmorMaterial::CLOTH)
    {
        switch (armorType)
        {
        case 0: return TEX152_ITEM_ICON("helmetCloth_overlay.png");
        case 1: return TEX152_ITEM_ICON("chestplateCloth_overlay.png");
        case 2: return TEX152_ITEM_ICON("leggingsCloth_overlay.png");
        case 3: return TEX152_ITEM_ICON("bootsCloth_overlay.png");
        default: break;
        }
    }
    return getIconFromDamage(damage);
}

int ItemArmor::getColorFromItemStack(ItemStack* itemstack, int renderPass)
{
    if (renderPass > 0)
    {
        return 0xFFFFFF;
    }
    const int color = getColor(itemstack);
    return color < 0 ? 0xFFFFFF : color;
}

void ItemArmor::addInformation(ItemStack* itemstack, std::vector<std::string>& information)
{
    if (hasColor(itemstack))
    {
        std::string dyed = StatCollector::translateToLocal("item.dyed");
        if (dyed.empty() || dyed == "item.dyed")
            dyed = "Dyed";
        information.push_back(dyed);
    }
}

ItemStack* ItemArmor::onItemRightClick(ItemStack* itemstack, World* world, EntityPlayer* entityplayer)
{
    (void)world;
    if (itemstack == nullptr || entityplayer == nullptr || entityplayer->inventory == nullptr)
        return itemstack;

    const int armorSlot = 3 - armorType;
    if (armorSlot >= 0 && armorSlot < 4 && entityplayer->inventory->armorInventory[armorSlot] == nullptr)
    {
        ItemStack* equipped = itemstack->copy();
        equipped->stackSize = 1;
        entityplayer->inventory->armorInventory[armorSlot] = equipped;
        entityplayer->inventory->onInventoryChanged();
        itemstack->stackSize = 0;
    }
    return itemstack;
}

bool ItemArmor::hasColor(ItemStack* stack) const
{
    if (material != EnumArmorMaterial::CLOTH || stack == nullptr || !stack->hasTagCompound())
        return false;
    NBTTagCompound* root = stack->getTagCompound();
    if (root == nullptr || !root->hasKey("display"))
        return false;
    NBTTagCompound* display = root->getCompoundTag("display");
    return display != nullptr && display->hasKey("color");
}

int ItemArmor::getColor(ItemStack* stack) const
{
    if (material != EnumArmorMaterial::CLOTH)
        return -1;
    if (stack == nullptr || !stack->hasTagCompound())
        return DEFAULT_LEATHER_COLOR;
    NBTTagCompound* root = stack->getTagCompound();
    if (root == nullptr || !root->hasKey("display"))
        return DEFAULT_LEATHER_COLOR;
    NBTTagCompound* display = root->getCompoundTag("display");
    if (display == nullptr || !display->hasKey("color"))
        return DEFAULT_LEATHER_COLOR;
    return display->getInteger("color");
}

void ItemArmor::setColor(ItemStack* stack, int color)
{
    if (material != EnumArmorMaterial::CLOTH || stack == nullptr)
        return;
    NBTTagCompound* root = stack->getTagCompound();
    if (root == nullptr)
    {
        root = new NBTTagCompound();
        stack->setTagCompound(root);
    }
    NBTTagCompound* display = nullptr;
    if (root->hasKey("display"))
    {
        display = root->getCompoundTag("display");
    }
    if (display == nullptr)
    {
        display = new NBTTagCompound();
        root->setCompoundTag("display", display);
    }
    display->setInteger("color", color);
}

void ItemArmor::removeColor(ItemStack* stack)
{
    if (material != EnumArmorMaterial::CLOTH || stack == nullptr || !stack->hasTagCompound())
        return;
    NBTTagCompound* root = stack->getTagCompound();
    if (root == nullptr || !root->hasKey("display"))
        return;
    NBTTagCompound* display = root->getCompoundTag("display");
    if (display != nullptr && display->hasKey("color"))
    {
        display->removeTag("color");
    }
}
