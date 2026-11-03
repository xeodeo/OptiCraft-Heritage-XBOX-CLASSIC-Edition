#include "CreativeTabs.h"

#include "Block.h"
#include "BlockFlower.h"
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

    initTabsForBlocksAndItems();
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
    if (iconIndex >= 0 && iconIndex < Item::ITEM_LIST_SIZE)
        return Item::itemsList[iconIndex];
    return nullptr;
}

int CreativeTabs::getTabIconItemIndex()
{
    return 1;
}

void CreativeTabs::displayAllReleventItems(std::vector<ItemStack *> &list)
{
    for (int i = 0; i < Item::ITEM_LIST_SIZE; ++i)
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
                if (ench->type == t)
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


void CreativeTabs::initTabsForBlocksAndItems()
{
    struct Entry { int id; int tab; };
    static const Entry s_blocks[] = {
        {   1,  0 }, // stone -> tabBlock
        {   2,  0 }, // grass -> tabBlock
        {   3,  0 }, // dirt -> tabBlock
        {   4,  0 }, // cobblestone -> tabBlock
        {   5,  0 }, // planks -> tabBlock
        {   6,  1 }, // sapling -> tabDecorations
        {   7,  0 }, // bedrock -> tabBlock
        {  12,  0 }, // sand -> tabBlock
        {  13,  0 }, // gravel -> tabBlock
        {  14,  0 }, // oreGold -> tabBlock
        {  15,  0 }, // oreIron -> tabBlock
        {  16,  0 }, // oreCoal -> tabBlock
        {  17,  0 }, // wood -> tabBlock
        {  18,  1 }, // leaves -> tabDecorations
        {  19,  0 }, // sponge -> tabBlock
        {  20,  0 }, // glass -> tabBlock
        {  21,  0 }, // oreLapis -> tabBlock
        {  22,  0 }, // blockLapis -> tabBlock
        {  23,  2 }, // dispenser -> tabRedstone
        {  24,  0 }, // sandStone -> tabBlock
        {  25,  2 }, // music -> tabRedstone
        {  27,  3 }, // railPowered -> tabTransport
        {  28,  3 }, // railDetector -> tabTransport
        {  29,  2 }, // pistonStickyBase -> tabRedstone
        {  30,  1 }, // web -> tabDecorations
        {  31,  1 }, // tallGrass -> tabDecorations
        {  32,  1 }, // deadBush -> tabDecorations
        {  33,  2 }, // pistonBase -> tabRedstone
        {  35,  0 }, // cloth -> tabBlock
        {  37,  1 }, // plantYellow -> tabDecorations
        {  38,  1 }, // plantRed -> tabDecorations
        {  39,  1 }, // mushroomBrown -> tabDecorations
        {  40,  1 }, // mushroomRed -> tabDecorations
        {  41,  0 }, // blockGold -> tabBlock
        {  42,  0 }, // blockIron -> tabBlock
        {  43,  0 }, // stoneDoubleSlab -> tabBlock
        {  44,  0 }, // stoneSingleSlab -> tabBlock
        {  45,  0 }, // brick -> tabBlock
        {  46,  2 }, // tnt -> tabRedstone
        {  47,  0 }, // bookShelf -> tabBlock
        {  48,  0 }, // cobblestoneMossy -> tabBlock
        {  49,  0 }, // obsidian -> tabBlock
        {  50,  1 }, // torchWood -> tabDecorations
        {  53,  0 }, // stairsWoodOak -> tabBlock
        {  54,  1 }, // chest -> tabDecorations
        {  56,  0 }, // oreDiamond -> tabBlock
        {  57,  0 }, // blockDiamond -> tabBlock
        {  58,  1 }, // workbench -> tabDecorations
        {  59,  1 }, // crops -> tabDecorations
        {  61,  1 }, // furnaceIdle -> tabDecorations
        {  65,  1 }, // ladder -> tabDecorations
        {  66,  3 }, // rail -> tabTransport
        {  67,  0 }, // stairsCobblestone -> tabBlock
        {  69,  2 }, // lever -> tabRedstone
        {  70,  2 }, // pressurePlateStone -> tabRedstone
        {  72,  2 }, // pressurePlatePlanks -> tabRedstone
        {  73,  0 }, // oreRedstone -> tabBlock
        {  75,  1 }, // torchRedstoneIdle -> tabDecorations
        {  76,  2 }, // torchRedstoneActive -> tabRedstone
        {  77,  2 }, // stoneButton -> tabRedstone
        {  78,  1 }, // snow -> tabDecorations
        {  79,  0 }, // ice -> tabBlock
        {  80,  0 }, // blockSnow -> tabBlock
        {  81,  1 }, // cactus -> tabDecorations
        {  82,  0 }, // blockClay -> tabBlock
        {  84,  1 }, // jukebox -> tabDecorations
        {  85,  1 }, // fence -> tabDecorations
        {  86,  0 }, // pumpkin -> tabBlock
        {  87,  0 }, // netherrack -> tabBlock
        {  88,  0 }, // slowSand -> tabBlock
        {  89,  0 }, // glowStone -> tabBlock
        {  91,  0 }, // pumpkinLantern -> tabBlock
        {  96,  2 }, // trapdoor -> tabRedstone
        {  97,  1 }, // silverfish -> tabDecorations
        {  98,  0 }, // stoneBrick -> tabBlock
        { 101,  1 }, // fenceIron -> tabDecorations
        { 102,  1 }, // thinGlass -> tabDecorations
        { 103,  0 }, // melon -> tabBlock
        { 104,  1 }, // pumpkinStem -> tabDecorations
        { 105,  1 }, // melonStem -> tabDecorations
        { 106,  1 }, // vine -> tabDecorations
        { 107,  2 }, // fenceGate -> tabRedstone
        { 108,  0 }, // stairsBrick -> tabBlock
        { 109,  0 }, // stairsStoneBrick -> tabBlock
        { 110,  0 }, // mycelium -> tabBlock
        { 111,  1 }, // waterlily -> tabDecorations
        { 112,  0 }, // netherBrick -> tabBlock
        { 113,  1 }, // netherFence -> tabDecorations
        { 114,  0 }, // stairsNetherBrick -> tabBlock
        { 115,  1 }, // netherStalk -> tabDecorations
        { 116,  1 }, // enchantmentTable -> tabDecorations
        { 120,  1 }, // endPortalFrame -> tabDecorations
        { 121,  0 }, // whiteStone -> tabBlock
        { 123,  2 }, // redstoneLampIdle -> tabRedstone
        { 125,  0 }, // woodDoubleSlab -> tabBlock
        { 126,  0 }, // woodSingleSlab -> tabBlock
        { 128,  0 }, // stairsSandStone -> tabBlock
        { 129,  0 }, // oreEmerald -> tabBlock
        { 130,  1 }, // enderChest -> tabDecorations
        { 131,  2 }, // tripWireSource -> tabRedstone
        { 133,  0 }, // blockEmerald -> tabBlock
        { 134,  0 }, // stairsWoodSpruce -> tabBlock
        { 135,  0 }, // stairsWoodBirch -> tabBlock
        { 136,  0 }, // stairsWoodJungle -> tabBlock
        { 138,  4 }, // beacon -> tabMisc
        { 139,  0 }, // cobblestoneWall -> tabBlock
        { 141,  1 }, // carrot -> tabDecorations
        { 142,  1 }, // potato -> tabDecorations
        { 143,  2 }, // woodenButton -> tabRedstone
        { 145,  1 }, // anvil -> tabDecorations
        { 146,  1 }, // chestTrapped -> tabDecorations
        { 147,  2 }, // pressurePlateGold -> tabRedstone
        { 148,  2 }, // pressurePlateIron -> tabRedstone
        { 151,  2 }, // daylightSensor -> tabRedstone
        { 152,  2 }, // blockRedstone -> tabRedstone
        { 153,  0 }, // oreNetherQuartz -> tabBlock
        { 154,  2 }, // hopperBlock -> tabRedstone
        { 155,  0 }, // blockNetherQuartz -> tabBlock
        { 156,  0 }, // stairsNetherQuartz -> tabBlock
        { 157,  3 }, // railActivator -> tabTransport
        { 158,  2 }, // dropper -> tabRedstone
    };
    for (const auto &e : s_blocks)
    {
        if (e.id >= 0 && e.id < Block::BLOCK_REGISTRY_SIZE && Block::blocksList[e.id] != nullptr)
            Block::blocksList[e.id]->setCreativeTab(creativeTabArray[e.tab]);
    }

    static const Entry s_items[] = {
        {  256,  7 }, // shovelIron -> tabTools
        {  257,  7 }, // pickaxeIron -> tabTools
        {  258,  7 }, // axeIron -> tabTools
        {  259,  7 }, // flintAndSteel -> tabTools
        {  260,  6 }, // appleRed -> tabFood
        {  261,  8 }, // bow -> tabCombat
        {  262,  8 }, // arrow -> tabCombat
        {  263, 10 }, // coal -> tabMaterials
        {  264, 10 }, // diamond -> tabMaterials
        {  265, 10 }, // ingotIron -> tabMaterials
        {  266, 10 }, // ingotGold -> tabMaterials
        {  267,  8 }, // swordIron -> tabCombat
        {  268,  8 }, // swordWood -> tabCombat
        {  269,  7 }, // shovelWood -> tabTools
        {  270,  7 }, // pickaxeWood -> tabTools
        {  271,  7 }, // axeWood -> tabTools
        {  272,  8 }, // swordStone -> tabCombat
        {  273,  7 }, // shovelStone -> tabTools
        {  274,  7 }, // pickaxeStone -> tabTools
        {  275,  7 }, // axeStone -> tabTools
        {  276,  8 }, // swordDiamond -> tabCombat
        {  277,  7 }, // shovelDiamond -> tabTools
        {  278,  7 }, // pickaxeDiamond -> tabTools
        {  279,  7 }, // axeDiamond -> tabTools
        {  280, 10 }, // stick -> tabMaterials
        {  281, 10 }, // bowlEmpty -> tabMaterials
        {  282,  6 }, // bowlSoup -> tabFood
        {  283,  8 }, // swordGold -> tabCombat
        {  284,  7 }, // shovelGold -> tabTools
        {  285,  7 }, // pickaxeGold -> tabTools
        {  286,  7 }, // axeGold -> tabTools
        {  287, 10 }, // silk -> tabMaterials
        {  288, 10 }, // feather -> tabMaterials
        {  289, 10 }, // gunpowder -> tabMaterials
        {  290,  7 }, // hoeWood -> tabTools
        {  291,  7 }, // hoeStone -> tabTools
        {  292,  7 }, // hoeIron -> tabTools
        {  293,  7 }, // hoeDiamond -> tabTools
        {  294,  7 }, // hoeGold -> tabTools
        {  295, 10 }, // seeds -> tabMaterials
        {  296, 10 }, // wheat -> tabMaterials
        {  297,  6 }, // bread -> tabFood
        {  298,  8 }, // helmetLeather -> tabCombat
        {  299,  8 }, // plateLeather -> tabCombat
        {  300,  8 }, // legsLeather -> tabCombat
        {  301,  8 }, // bootsLeather -> tabCombat
        {  302,  8 }, // helmetChain -> tabCombat
        {  303,  8 }, // plateChain -> tabCombat
        {  304,  8 }, // legsChain -> tabCombat
        {  305,  8 }, // bootsChain -> tabCombat
        {  306,  8 }, // helmetIron -> tabCombat
        {  307,  8 }, // plateIron -> tabCombat
        {  308,  8 }, // legsIron -> tabCombat
        {  309,  8 }, // bootsIron -> tabCombat
        {  310,  8 }, // helmetDiamond -> tabCombat
        {  311,  8 }, // plateDiamond -> tabCombat
        {  312,  8 }, // legsDiamond -> tabCombat
        {  313,  8 }, // bootsDiamond -> tabCombat
        {  314,  8 }, // helmetGold -> tabCombat
        {  315,  8 }, // plateGold -> tabCombat
        {  316,  8 }, // legsGold -> tabCombat
        {  317,  8 }, // bootsGold -> tabCombat
        {  318, 10 }, // flint -> tabMaterials
        {  319,  6 }, // porkRaw -> tabFood
        {  320,  6 }, // porkCooked -> tabFood
        {  321,  1 }, // painting -> tabDecorations
        {  322,  6 }, // appleGold -> tabFood
        {  323,  1 }, // sign -> tabDecorations
        {  324,  2 }, // doorWood -> tabRedstone
        {  325,  4 }, // bucketEmpty -> tabMisc
        {  326,  4 }, // bucketWater -> tabMisc
        {  327,  4 }, // bucketLava -> tabMisc
        {  328,  3 }, // minecartEmpty -> tabTransport
        {  329,  3 }, // saddle -> tabTransport
        {  330,  2 }, // doorIron -> tabRedstone
        {  331,  2 }, // redstone -> tabRedstone
        {  332,  4 }, // snowball -> tabMisc
        {  333,  3 }, // boat -> tabTransport
        {  334, 10 }, // leather -> tabMaterials
        {  335,  4 }, // bucketMilk -> tabMisc
        {  336, 10 }, // brick -> tabMaterials
        {  337, 10 }, // clay -> tabMaterials
        {  338, 10 }, // reed -> tabMaterials
        {  339,  4 }, // paper -> tabMisc
        {  340,  4 }, // book -> tabMisc
        {  341,  4 }, // slimeBall -> tabMisc
        {  342,  3 }, // minecartCrate -> tabTransport
        {  343,  3 }, // minecartPowered -> tabTransport
        {  344, 10 }, // egg -> tabMaterials
        {  345,  7 }, // compass -> tabTools
        {  346,  7 }, // fishingRod -> tabTools
        {  347,  7 }, // pocketSundial -> tabTools
        {  348, 10 }, // lightStoneDust -> tabMaterials
        {  349,  6 }, // fishRaw -> tabFood
        {  350,  6 }, // fishCooked -> tabFood
        {  351, 10 }, // dyePowder -> tabMaterials
        {  352,  4 }, // bone -> tabMisc
        {  353, 10 }, // sugar -> tabMaterials
        {  354,  6 }, // cake -> tabFood
        {  355,  1 }, // bed -> tabDecorations
        {  356,  2 }, // redstoneRepeater -> tabRedstone
        {  357,  6 }, // cookie -> tabFood
        {  359,  7 }, // shears -> tabTools
        {  360,  6 }, // melon -> tabFood
        {  361, 10 }, // pumpkinSeeds -> tabMaterials
        {  362, 10 }, // melonSeeds -> tabMaterials
        {  363,  6 }, // beefRaw -> tabFood
        {  364,  6 }, // beefCooked -> tabFood
        {  365,  6 }, // chickenRaw -> tabFood
        {  366,  6 }, // chickenCooked -> tabFood
        {  367,  6 }, // rottenFlesh -> tabFood
        {  368,  4 }, // enderPearl -> tabMisc
        {  369, 10 }, // blazeRod -> tabMaterials
        {  370,  9 }, // ghastTear -> tabBrewing
        {  371, 10 }, // goldNugget -> tabMaterials
        {  372, 10 }, // netherStalkSeeds -> tabMaterials
        {  373,  9 }, // potion -> tabBrewing
        {  374,  9 }, // glassBottle -> tabBrewing
        {  375,  6 }, // spiderEye -> tabFood
        {  376,  9 }, // fermentedSpiderEye -> tabBrewing
        {  377,  9 }, // blazePowder -> tabBrewing
        {  378,  9 }, // magmaCream -> tabBrewing
        {  379,  9 }, // brewingStand -> tabBrewing
        {  380,  9 }, // cauldron -> tabBrewing
        {  381,  4 }, // eyeOfEnder -> tabMisc
        {  382,  9 }, // speckledMelon -> tabBrewing
        {  383,  4 }, // monsterPlacer -> tabMisc
        {  384,  4 }, // expBottle -> tabMisc
        {  385,  4 }, // fireballCharge -> tabMisc
        {  386,  4 }, // writableBook -> tabMisc
        {  388, 10 }, // emerald -> tabMaterials
        {  389,  1 }, // itemFrame -> tabDecorations
        {  390,  1 }, // flowerPot -> tabDecorations
        {  391,  6 }, // carrot -> tabFood
        {  392,  6 }, // potato -> tabFood
        {  393,  6 }, // bakedPotato -> tabFood
        {  394,  6 }, // poisonousPotato -> tabFood
        {  395,  4 }, // emptyMap -> tabMisc
        {  396,  6 }, // goldenCarrot -> tabFood
        {  397,  1 }, // skull -> tabDecorations
        {  398,  3 }, // carrotOnAStick -> tabTransport
        {  399, 10 }, // netherStar -> tabMaterials
        {  400,  6 }, // pumpkinPie -> tabFood
        {  402,  4 }, // fireworkCharge -> tabMisc
        {  404,  2 }, // comparator -> tabRedstone
        {  405, 10 }, // netherrackBrick -> tabMaterials
        {  406, 10 }, // netherQuartz -> tabMaterials
        {  407,  3 }, // minecartTnt -> tabTransport
        {  408,  3 }, // minecartHopper -> tabTransport
        { 2256,  4 }, // record13 -> tabMisc
        { 2257,  4 }, // recordCat -> tabMisc
        { 2258,  4 }, // recordBlocks -> tabMisc
        { 2259,  4 }, // recordChirp -> tabMisc
        { 2260,  4 }, // recordFar -> tabMisc
        { 2261,  4 }, // recordMall -> tabMisc
        { 2262,  4 }, // recordMellohi -> tabMisc
        { 2263,  4 }, // recordStal -> tabMisc
        { 2264,  4 }, // recordStrad -> tabMisc
        { 2265,  4 }, // recordWard -> tabMisc
        { 2266,  4 }, // record11 -> tabMisc
        { 2267,  4 }, // recordWait -> tabMisc
    };
    for (const auto &e : s_items)
    {
        if (e.id >= 0 && e.id < Item::ITEM_LIST_SIZE && Item::itemsList[e.id] != nullptr)
            Item::itemsList[e.id]->setCreativeTab(creativeTabArray[e.tab]);
    }
}
