#include "CreativeTabs.h"

#include "Block.h"
#include "Enchantment.h"
#include "EnchantmentData.h"
#include "Item.h"
#include "ItemEnchantedBook.h"
#include "ItemStack.h"
#include "StringTranslate.h"

CreativeTabs *CreativeTabs::creativeTabArray[12] = { nullptr };
CreativeTabs *CreativeTabs::tabBlock = nullptr;
CreativeTabs *CreativeTabs::tabDecorations = nullptr;
CreativeTabs *CreativeTabs::tabRedstone = nullptr;
CreativeTabs *CreativeTabs::tabTransport = nullptr;
CreativeTabs *CreativeTabs::tabMisc = nullptr;
CreativeTabs *CreativeTabs::tabAllSearch = nullptr;
CreativeTabs *CreativeTabs::tabFood = nullptr;
CreativeTabs *CreativeTabs::tabTools = nullptr;
CreativeTabs *CreativeTabs::tabCombat = nullptr;
CreativeTabs *CreativeTabs::tabBrewing = nullptr;
CreativeTabs *CreativeTabs::tabMaterials = nullptr;
CreativeTabs *CreativeTabs::tabInventory = nullptr;

void CreativeTabs::initialize()
{
    if (tabBlock != nullptr)
        return;

    tabBlock = new CreativeTabBlock(0, "buildingBlocks");
    tabDecorations = new CreativeTabDeco(1, "decorations");
    tabRedstone = new CreativeTabRedstone(2, "redstone");
    tabTransport = new CreativeTabTransport(3, "transportation");
    tabMisc = new CreativeTabMisc(4, "misc");
    tabAllSearch = (new CreativeTabSearch(5, "search"))->setBackgroundImageName("search.png");
    tabFood = new CreativeTabFood(6, "food");
    tabTools = new CreativeTabTools(7, "tools");
    tabCombat = new CreativeTabCombat(8, "combat");
    tabBrewing = new CreativeTabBrewing(9, "brewing");
    tabMaterials = new CreativeTabMaterial(10, "materials");
    tabInventory = (new CreativeTabInventory(11, "inventory"))
        ->setBackgroundImageName("survival_inv.png")
        ->setNoScrollbar()
        ->setNoTitle();
}

void CreativeTabs::cleanup()
{
    for (int i = 0; i < 12; ++i)
    {
        delete creativeTabArray[i];
        creativeTabArray[i] = nullptr;
    }
    tabBlock = nullptr;
    tabDecorations = nullptr;
    tabRedstone = nullptr;
    tabTransport = nullptr;
    tabMisc = nullptr;
    tabAllSearch = nullptr;
    tabFood = nullptr;
    tabTools = nullptr;
    tabCombat = nullptr;
    tabBrewing = nullptr;
    tabMaterials = nullptr;
    tabInventory = nullptr;
}

CreativeTabs::CreativeTabs(int index, const std::string &label)
    : tabIndex(index)
    , tabLabel(label)
    , backgroundImageName("list_items.png")
    , hasScrollbar(true)
    , drawTitle(true)
{
    if (index >= 0 && index < 12)
        creativeTabArray[index] = this;
}

std::string CreativeTabs::getTranslatedTabLabel() const
{
    std::string key = "itemGroup." + tabLabel;
    std::string trans = StringTranslate::getInstance()->translateKey(key);
    if (trans != key)
        return trans;

    if (tabLabel == "buildingBlocks") return "Building Blocks";
    if (tabLabel == "decorations") return "Decoration Blocks";
    if (tabLabel == "redstone") return "Redstone";
    if (tabLabel == "transportation") return "Transportation";
    if (tabLabel == "misc") return "Miscellaneous";
    if (tabLabel == "search") return "Search Items";
    if (tabLabel == "food") return "Foodstuffs";
    if (tabLabel == "tools") return "Tools";
    if (tabLabel == "combat") return "Combat";
    if (tabLabel == "brewing") return "Brewing";
    if (tabLabel == "materials") return "Materials";
    if (tabLabel == "inventory") return "Survival Inventory";
    return tabLabel;
}

Item *CreativeTabs::getTabIconItem()
{
    int iconIndex = getTabIconItemIndex();
    if (iconIndex >= 0 && iconIndex < Item::ITEM_REGISTRY_SIZE)
        return Item::itemsList[iconIndex];
    return nullptr;
}

int CreativeTabs::getTabIconItemIndex()
{
    return 1;
}

void CreativeTabs::displayAllReleventItems(std::vector<ItemStack *> &list)
{
    for (int i = 0; i < Item::ITEM_REGISTRY_SIZE; ++i)
    {
        Item *item = Item::itemsList[i];
        if (item != nullptr && item->getCreativeTab() == this)
        {
            item->getSubItems(item->shiftedIndex, this, list);
        }
    }
}

void CreativeTabs::func_92116_a(std::vector<ItemStack *> &list, const std::vector<EnumEnchantmentType> &types)
{
    if (Item::enchantedBook == nullptr)
        return;
    ItemEnchantedBook *enchantedBookItem = dynamic_cast<ItemEnchantedBook *>(Item::enchantedBook);
    if (enchantedBookItem == nullptr)
        return;

    for (int i = 0; i < 256; ++i)
    {
        Enchantment *ench = Enchantment::enchantmentsList[i];
        if (ench != nullptr)
        {
            bool match = false;
            for (EnumEnchantmentType t : types)
            {
                if (ench->type == t || t == EnumEnchantmentType::all)
                {
                    match = true;
                    break;
                }
            }
            if (match)
            {
                list.push_back(enchantedBookItem->func_92111_a(EnchantmentData(ench, ench->getMaxLevel())));
            }
        }
    }
}

// Subclasses

CreativeTabBlock::CreativeTabBlock(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabBlock::getTabIconItemIndex()
{
    return Block::brick != nullptr ? Block::brick->blockID : 45;
}

CreativeTabDeco::CreativeTabDeco(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabDeco::getTabIconItemIndex()
{
    return Block::plantRed != nullptr ? Block::plantRed->blockID : 38;
}

CreativeTabRedstone::CreativeTabRedstone(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabRedstone::getTabIconItemIndex()
{
    return Item::redstone != nullptr ? Item::redstone->shiftedIndex : 331;
}

CreativeTabTransport::CreativeTabTransport(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabTransport::getTabIconItemIndex()
{
    return Block::railPowered != nullptr ? Block::railPowered->blockID : 27;
}

CreativeTabMisc::CreativeTabMisc(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabMisc::getTabIconItemIndex()
{
    return Item::bucketLava != nullptr ? Item::bucketLava->shiftedIndex : 327;
}

void CreativeTabMisc::displayAllReleventItems(std::vector<ItemStack *> &list)
{
    CreativeTabs::displayAllReleventItems(list);
    func_92116_a(list, { EnumEnchantmentType::all });
}

CreativeTabSearch::CreativeTabSearch(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabSearch::getTabIconItemIndex()
{
    return Item::compass != nullptr ? Item::compass->shiftedIndex : 345;
}

CreativeTabFood::CreativeTabFood(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabFood::getTabIconItemIndex()
{
    return Item::appleRed != nullptr ? Item::appleRed->shiftedIndex : 260;
}

CreativeTabTools::CreativeTabTools(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabTools::getTabIconItemIndex()
{
    return Item::axeSteel != nullptr ? Item::axeSteel->shiftedIndex : 258;
}

void CreativeTabTools::displayAllReleventItems(std::vector<ItemStack *> &list)
{
    CreativeTabs::displayAllReleventItems(list);
    func_92116_a(list, { EnumEnchantmentType::digger });
}

CreativeTabCombat::CreativeTabCombat(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabCombat::getTabIconItemIndex()
{
    return Item::swordGold != nullptr ? Item::swordGold->shiftedIndex : 283;
}

void CreativeTabCombat::displayAllReleventItems(std::vector<ItemStack *> &list)
{
    CreativeTabs::displayAllReleventItems(list);
    func_92116_a(list, {
        EnumEnchantmentType::armor,
        EnumEnchantmentType::armor_feet,
        EnumEnchantmentType::armor_head,
        EnumEnchantmentType::armor_legs,
        EnumEnchantmentType::armor_torso,
        EnumEnchantmentType::bow,
        EnumEnchantmentType::weapon
    });
}

CreativeTabBrewing::CreativeTabBrewing(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabBrewing::getTabIconItemIndex()
{
    return Item::potion != nullptr ? Item::potion->shiftedIndex : 373;
}

CreativeTabMaterial::CreativeTabMaterial(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabMaterial::getTabIconItemIndex()
{
    return Item::stick != nullptr ? Item::stick->shiftedIndex : 280;
}

CreativeTabInventory::CreativeTabInventory(int index, const std::string &label)
    : CreativeTabs(index, label)
{
}

int CreativeTabInventory::getTabIconItemIndex()
{
    return Block::chest != nullptr ? Block::chest->blockID : 54;
}
