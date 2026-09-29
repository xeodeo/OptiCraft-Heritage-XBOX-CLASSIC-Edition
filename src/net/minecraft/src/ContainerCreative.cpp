#include "ContainerCreative.h"

#include "Block.h"
#include "BlockDeadBush.h"
#include "BlockFlower.h"
#include "BlockGrass.h"
#include "BlockLeaves.h"
#include "BlockLilyPad.h"
#include "BlockMycelium.h"
#include "BlockTallGrass.h"
#include "BlockVine.h"
#include "EntityList.h"
#include "EntityPlayer.h"
#include "GuiContainerCreative.h"
#include "InventoryBasic.h"
#include "InventoryPlayer.h"
#include "Item.h"
#include "ItemStack.h"
#include "Slot.h"
#include "Minecraft.h"
#include "GameSettings.h"

#include <cmath>

static int_t getCreativeCategory(ItemStack *stack)
{
    if (stack == nullptr)
        return CREATIVE_TAB_MATERIALS;

    const int_t id = stack->itemID;

    if (id < 256)
    {
        // Redstone and mechanisms
        if ((Block::tnt != nullptr && id == Block::tnt->blockID) ||
            (Block::dispenser != nullptr && id == Block::dispenser->blockID) ||
            (Block::musicBlock != nullptr && id == Block::musicBlock->blockID) ||
            (Block::jukebox != nullptr && id == Block::jukebox->blockID) ||
            (Block::pistonStickyBase != nullptr && id == Block::pistonStickyBase->blockID) ||
            (Block::pistonBase != nullptr && id == Block::pistonBase->blockID) ||
            (Block::rail != nullptr && id == Block::rail->blockID) ||
            (Block::railPowered != nullptr && id == Block::railPowered->blockID) ||
            (Block::railDetector != nullptr && id == Block::railDetector->blockID) ||
            (Block::lever != nullptr && id == Block::lever->blockID) ||
            (Block::pressurePlateStone != nullptr && id == Block::pressurePlateStone->blockID) ||
            (Block::pressurePlatePlanks != nullptr && id == Block::pressurePlatePlanks->blockID) ||
            (Block::torchRedstoneActive != nullptr && id == Block::torchRedstoneActive->blockID) ||
            (Block::button != nullptr && id == Block::button->blockID) ||
            (Block::trapdoor != nullptr && id == Block::trapdoor->blockID) ||
            (Block::redstoneLampIdle != nullptr && id == Block::redstoneLampIdle->blockID))
        {
            return CREATIVE_TAB_REDSTONE_TRANSPORT;
        }

        // Decorative blocks
        if ((Block::leaves != nullptr && id == Block::leaves->blockID) ||
            (Block::sapling != nullptr && id == Block::sapling->blockID) ||
            (Block::deadBush != nullptr && id == Block::deadBush->blockID) ||
            (Block::sponge != nullptr && id == Block::sponge->blockID) ||
            (Block::plantYellow != nullptr && id == Block::plantYellow->blockID) ||
            (Block::plantRed != nullptr && id == Block::plantRed->blockID) ||
            (Block::mushroomBrown != nullptr && id == Block::mushroomBrown->blockID) ||
            (Block::mushroomRed != nullptr && id == Block::mushroomRed->blockID) ||
            (Block::cactus != nullptr && id == Block::cactus->blockID) ||
            (Block::melon != nullptr && id == Block::melon->blockID) ||
            (Block::pumpkin != nullptr && id == Block::pumpkin->blockID) ||
            (Block::pumpkinLantern != nullptr && id == Block::pumpkinLantern->blockID) ||
            (Block::vine != nullptr && id == Block::vine->blockID) ||
            (Block::fenceIron != nullptr && id == Block::fenceIron->blockID) ||
            (Block::thinGlass != nullptr && id == Block::thinGlass->blockID) ||
            (Block::netherFence != nullptr && id == Block::netherFence->blockID) ||
            (Block::waterlily != nullptr && id == Block::waterlily->blockID) ||
            (Block::tallGrass != nullptr && id == Block::tallGrass->blockID) ||
            (Block::chest != nullptr && id == Block::chest->blockID) ||
            (Block::workbench != nullptr && id == Block::workbench->blockID) ||
            (Block::cloth != nullptr && id == Block::cloth->blockID) ||
            (Block::fence != nullptr && id == Block::fence->blockID) ||
            (Block::fenceGate != nullptr && id == Block::fenceGate->blockID) ||
            (Block::ladder != nullptr && id == Block::ladder->blockID) ||
            (Block::torchWood != nullptr && id == Block::torchWood->blockID) ||
            (Block::enchantmentTable != nullptr && id == Block::enchantmentTable->blockID) ||
            (Block::bookShelf != nullptr && id == Block::bookShelf->blockID) ||
            (Block::stoneOvenIdle != nullptr && id == Block::stoneOvenIdle->blockID))
        {
            return CREATIVE_TAB_DECORATION;
        }

        return CREATIVE_TAB_BUILDING;
    }

    // Items
    // Redstone / Transportation
    if ((Item::redstone != nullptr && id == Item::redstone->shiftedIndex) ||
        (Item::redstoneRepeater != nullptr && id == Item::redstoneRepeater->shiftedIndex) ||
        (Item::minecartEmpty != nullptr && id == Item::minecartEmpty->shiftedIndex) ||
        (Item::minecartCrate != nullptr && id == Item::minecartCrate->shiftedIndex) ||
        (Item::minecartPowered != nullptr && id == Item::minecartPowered->shiftedIndex) ||
        (Item::boat != nullptr && id == Item::boat->shiftedIndex) ||
        (Item::doorWood != nullptr && id == Item::doorWood->shiftedIndex) ||
        (Item::doorSteel != nullptr && id == Item::doorSteel->shiftedIndex))
    {
        return CREATIVE_TAB_REDSTONE_TRANSPORT;
    }

    // Food, Tools, Weapons, Armor
    if ((Item::appleRed != nullptr && id == Item::appleRed->shiftedIndex) ||
        (Item::appleGold != nullptr && id == Item::appleGold->shiftedIndex) ||
        (Item::bread != nullptr && id == Item::bread->shiftedIndex) ||
        (Item::porkRaw != nullptr && id == Item::porkRaw->shiftedIndex) ||
        (Item::porkCooked != nullptr && id == Item::porkCooked->shiftedIndex) ||
        (Item::fishRaw != nullptr && id == Item::fishRaw->shiftedIndex) ||
        (Item::fishCooked != nullptr && id == Item::fishCooked->shiftedIndex) ||
        (Item::cake != nullptr && id == Item::cake->shiftedIndex) ||
        (Item::cookie != nullptr && id == Item::cookie->shiftedIndex) ||
        (Item::melon != nullptr && id == Item::melon->shiftedIndex) ||
        (Item::beefRaw != nullptr && id == Item::beefRaw->shiftedIndex) ||
        (Item::beefCooked != nullptr && id == Item::beefCooked->shiftedIndex) ||
        (Item::chickenRaw != nullptr && id == Item::chickenRaw->shiftedIndex) ||
        (Item::chickenCooked != nullptr && id == Item::chickenCooked->shiftedIndex) ||
        (Item::rottenFlesh != nullptr && id == Item::rottenFlesh->shiftedIndex) ||
        (Item::bowlSoup != nullptr && id == Item::bowlSoup->shiftedIndex) ||
        (Item::swordWood != nullptr && id == Item::swordWood->shiftedIndex) ||
        (Item::swordStone != nullptr && id == Item::swordStone->shiftedIndex) ||
        (Item::swordSteel != nullptr && id == Item::swordSteel->shiftedIndex) ||
        (Item::swordDiamond != nullptr && id == Item::swordDiamond->shiftedIndex) ||
        (Item::swordGold != nullptr && id == Item::swordGold->shiftedIndex) ||
        (Item::bow != nullptr && id == Item::bow->shiftedIndex) ||
        (Item::arrow != nullptr && id == Item::arrow->shiftedIndex) ||
        (Item::fishingRod != nullptr && id == Item::fishingRod->shiftedIndex) ||
        (Item::flintAndSteel != nullptr && id == Item::flintAndSteel->shiftedIndex) ||
        (Item::shears != nullptr && id == Item::shears->shiftedIndex) ||
        (Item::compass != nullptr && id == Item::compass->shiftedIndex) ||
        (Item::pocketSundial != nullptr && id == Item::pocketSundial->shiftedIndex) ||
        (Item::mapItem != nullptr && id == Item::mapItem->shiftedIndex) ||
        (Item::pickaxeWood != nullptr && id == Item::pickaxeWood->shiftedIndex) ||
        (Item::pickaxeStone != nullptr && id == Item::pickaxeStone->shiftedIndex) ||
        (Item::pickaxeSteel != nullptr && id == Item::pickaxeSteel->shiftedIndex) ||
        (Item::pickaxeDiamond != nullptr && id == Item::pickaxeDiamond->shiftedIndex) ||
        (Item::pickaxeGold != nullptr && id == Item::pickaxeGold->shiftedIndex) ||
        (Item::shovelWood != nullptr && id == Item::shovelWood->shiftedIndex) ||
        (Item::shovelStone != nullptr && id == Item::shovelStone->shiftedIndex) ||
        (Item::shovelSteel != nullptr && id == Item::shovelSteel->shiftedIndex) ||
        (Item::shovelDiamond != nullptr && id == Item::shovelDiamond->shiftedIndex) ||
        (Item::shovelGold != nullptr && id == Item::shovelGold->shiftedIndex) ||
        (Item::axeWood != nullptr && id == Item::axeWood->shiftedIndex) ||
        (Item::axeStone != nullptr && id == Item::axeStone->shiftedIndex) ||
        (Item::axeSteel != nullptr && id == Item::axeSteel->shiftedIndex) ||
        (Item::axeDiamond != nullptr && id == Item::axeDiamond->shiftedIndex) ||
        (Item::axeGold != nullptr && id == Item::axeGold->shiftedIndex) ||
        (Item::hoeWood != nullptr && id == Item::hoeWood->shiftedIndex) ||
        (Item::hoeStone != nullptr && id == Item::hoeStone->shiftedIndex) ||
        (Item::hoeSteel != nullptr && id == Item::hoeSteel->shiftedIndex) ||
        (Item::hoeDiamond != nullptr && id == Item::hoeDiamond->shiftedIndex) ||
        (Item::hoeGold != nullptr && id == Item::hoeGold->shiftedIndex) ||
        (Item::helmetLeather != nullptr && id == Item::helmetLeather->shiftedIndex) ||
        (Item::plateLeather != nullptr && id == Item::plateLeather->shiftedIndex) ||
        (Item::legsLeather != nullptr && id == Item::legsLeather->shiftedIndex) ||
        (Item::bootsLeather != nullptr && id == Item::bootsLeather->shiftedIndex) ||
        (Item::helmetChain != nullptr && id == Item::helmetChain->shiftedIndex) ||
        (Item::plateChain != nullptr && id == Item::plateChain->shiftedIndex) ||
        (Item::legsChain != nullptr && id == Item::legsChain->shiftedIndex) ||
        (Item::bootsChain != nullptr && id == Item::bootsChain->shiftedIndex) ||
        (Item::helmetSteel != nullptr && id == Item::helmetSteel->shiftedIndex) ||
        (Item::plateSteel != nullptr && id == Item::plateSteel->shiftedIndex) ||
        (Item::legsSteel != nullptr && id == Item::legsSteel->shiftedIndex) ||
        (Item::bootsSteel != nullptr && id == Item::bootsSteel->shiftedIndex) ||
        (Item::helmetDiamond != nullptr && id == Item::helmetDiamond->shiftedIndex) ||
        (Item::plateDiamond != nullptr && id == Item::plateDiamond->shiftedIndex) ||
        (Item::legsDiamond != nullptr && id == Item::legsDiamond->shiftedIndex) ||
        (Item::bootsDiamond != nullptr && id == Item::bootsDiamond->shiftedIndex) ||
        (Item::helmetGold != nullptr && id == Item::helmetGold->shiftedIndex) ||
        (Item::plateGold != nullptr && id == Item::plateGold->shiftedIndex) ||
        (Item::legsGold != nullptr && id == Item::legsGold->shiftedIndex) ||
        (Item::bootsGold != nullptr && id == Item::bootsGold->shiftedIndex))
    {
        return CREATIVE_TAB_FOOD_COMBAT;
    }

    // Decoration items
    if ((Item::painting != nullptr && id == Item::painting->shiftedIndex) ||
        (Item::sign != nullptr && id == Item::sign->shiftedIndex) ||
        (Item::bed != nullptr && id == Item::bed->shiftedIndex))
    {
        return CREATIVE_TAB_DECORATION;
    }

    return CREATIVE_TAB_MATERIALS;
}

ContainerCreative::ContainerCreative(EntityPlayer *player)
    : currentTab(0)
{
    auto addItemStack = [this](ItemStack *stack) {
        if (stack == nullptr)
            return;
        masterList.push_back(stack);
        const int_t cat = getCreativeCategory(stack);
        if (cat >= 0 && cat < CREATIVE_TAB_COUNT)
            tabItems[cat].push_back(stack);
        tabItems[CREATIVE_TAB_ALL].push_back(stack);
    };

    Block *blocks[] = {
        Block::cobblestone, Block::stone, Block::oreDiamond, Block::oreGold, Block::oreIron, Block::oreCoal,
        Block::oreLapis, Block::oreRedstone, Block::stoneBrick, Block::stoneBrick, Block::stoneBrick,
        Block::stoneBrick, Block::blockClay, Block::blockDiamond, Block::blockGold, Block::blockSteel,
        Block::bedrock, Block::blockLapis, Block::brick, Block::cobblestoneMossy, Block::stairSingle,
        Block::stairSingle, Block::stairSingle, Block::stairSingle, Block::stairSingle, Block::stairSingle,
        Block::obsidian, Block::netherrack, Block::slowSand, Block::glowStone, Block::wood, Block::wood,
        Block::wood, Block::wood, Block::leaves, Block::leaves, Block::leaves, Block::leaves, Block::dirt,
        Block::grass, Block::sand, Block::sandStone, Block::sandStone, Block::sandStone, Block::gravel,
        Block::web, Block::planks, Block::planks, Block::planks, Block::planks, Block::sapling, Block::sapling,
        Block::sapling, Block::sapling, Block::deadBush, Block::sponge, Block::ice, Block::blockSnow,
        Block::plantYellow, Block::plantRed, Block::mushroomBrown, Block::mushroomRed, Block::cactus,
        Block::melon, Block::pumpkin, Block::pumpkinLantern, Block::vine, Block::fenceIron, Block::thinGlass,
        Block::netherBrick, Block::netherFence, Block::stairsNetherBrick, Block::endPortal, Block::whiteStone,
        Block::mycelium,
        Block::waterlily, Block::tallGrass, Block::tallGrass, Block::chest, Block::workbench, Block::glass,
        Block::tnt, Block::bookShelf, Block::cloth, Block::cloth, Block::cloth, Block::cloth, Block::cloth,
        Block::cloth, Block::cloth, Block::cloth, Block::cloth, Block::cloth, Block::cloth, Block::cloth,
        Block::cloth, Block::cloth, Block::cloth, Block::cloth, Block::dispenser, Block::stoneOvenIdle,
        Block::musicBlock, Block::jukebox, Block::pistonStickyBase, Block::pistonBase, Block::fence,
        Block::fenceGate, Block::ladder, Block::rail, Block::railPowered, Block::railDetector, Block::torchWood,
        Block::stairCompactPlanks, Block::stairCompactCobblestone, Block::stairsBrick,
        Block::stairsStoneBrickSmooth, Block::lever, Block::pressurePlateStone, Block::pressurePlatePlanks,
        Block::torchRedstoneActive, Block::button, Block::trapdoor, Block::enchantmentTable,
        Block::redstoneLampIdle
    };

    int_t clothMeta = 0;
    int_t slabMeta = 0;
    int_t logMeta = 0;
    int_t plankMeta = 0;
    int_t saplingMeta = 0;
    int_t stoneBrickMeta = 0;
    int_t sandstoneMeta = 0;
    int_t leavesMeta = 0;
    int_t tallGrassMeta = 1;

    for (Block *block : blocks)
    {
        if (block == nullptr)
            continue;

        int_t meta = 0;
        if (block == Block::cloth) meta = clothMeta++;
        else if (block == Block::stairSingle) meta = slabMeta++;
        else if (block == Block::wood) meta = logMeta++;
        else if (block == Block::planks) meta = plankMeta++;
        else if (block == Block::sapling) meta = saplingMeta++;
        else if (block == Block::stoneBrick) meta = stoneBrickMeta++;
        else if (block == Block::sandStone) meta = sandstoneMeta++;
        else if (block == Block::tallGrass) meta = tallGrassMeta++;
        else if (block == Block::leaves) meta = leavesMeta++;
        addItemStack(new ItemStack(block, 1, meta));
    }

    for (int_t id = 256; id < Item::ITEM_LIST_SIZE; ++id)
    {
        Item *item = Item::itemsList[id];
        if (item != nullptr && item != Item::potion && item != Item::monsterPlacer)
            addItemStack(new ItemStack(item));
    }

    if (Item::dyePowder != nullptr)
    {
        for (int_t damage = 1; damage < 16; ++damage)
            addItemStack(new ItemStack(Item::dyePowder, 1, damage));
    }

    if (Item::monsterPlacer != nullptr)
    {
        for (int_t id : EntityList::getEntityEggIDs())
            addItemStack(new ItemStack(Item::monsterPlacer, 1, id));
    }

    InventoryBasic *creativeInventory = GuiContainerCreative::getInventory();
    for (int_t row = 0; row < 9; ++row)
    {
        for (int_t col = 0; col < 8; ++col)
            addSlot(new Slot(creativeInventory, col + row * 8, 8 + col * 18, 18 + row * 18));
    }

    for (int_t col = 0; col < 9; ++col)
        addSlot(new Slot(player->inventory, col, 8 + col * 18, 184));

    Minecraft *mc = Minecraft::getMinecraft();
    if (mc != nullptr && mc->gameSettings != nullptr && !mc->gameSettings->legacyCreative)
    {
        currentTab = CREATIVE_TAB_ALL;
        itemList = masterList;
        scrollTo(0.0f);
    }
    else
    {
        setCategory(CREATIVE_TAB_BUILDING);
    }
}

ContainerCreative::~ContainerCreative()
{
    InventoryBasic *creativeInventory = GuiContainerCreative::getInventory();
    if (creativeInventory != nullptr)
    {
        for (int_t i = 0; i < creativeInventory->getSizeInventory(); ++i)
            creativeInventory->setInventorySlotContents(i, nullptr);
    }

    for (ItemStack *stack : masterList)
        delete stack;
    masterList.clear();

    for (int_t t = 0; t < CREATIVE_TAB_COUNT; ++t)
        tabItems[t].clear();
    itemList.clear();
}

void ContainerCreative::setCategory(int_t tabIndex)
{
    Minecraft *mc = Minecraft::getMinecraft();
    if (mc != nullptr && mc->gameSettings != nullptr && !mc->gameSettings->legacyCreative)
    {
        currentTab = CREATIVE_TAB_ALL;
        itemList = masterList;
        scrollTo(0.0f);
        return;
    }

    if (tabIndex < 0)
        tabIndex = 0;
    if (tabIndex >= CREATIVE_TAB_COUNT)
        tabIndex = CREATIVE_TAB_COUNT - 1;

    currentTab = tabIndex;
    itemList = tabItems[currentTab];
    scrollTo(0.0f);
}

bool ContainerCreative::isUsableByPlayer(EntityPlayer *)
{
    return true;
}

void ContainerCreative::scrollTo(float_t scroll)
{
    int_t rows = (int_t)itemList.size() / 8 - 8 + 1;
    if (rows < 0)
        rows = 0;
    int_t firstRow = (int_t)((double)(scroll * (float_t)rows) + 0.5);
    if (firstRow < 0)
        firstRow = 0;

    InventoryBasic *creativeInventory = GuiContainerCreative::getInventory();
    for (int_t row = 0; row < 9; ++row)
    {
        for (int_t col = 0; col < 8; ++col)
        {
            int_t itemIndex = col + (row + firstRow) * 8;
            creativeInventory->setInventorySlotContents(
                col + row * 8,
                itemIndex >= 0 && itemIndex < (int_t)itemList.size() ? itemList[itemIndex] : nullptr);
        }
    }
}

void ContainerCreative::retrySlotClick(int_t, int_t, bool, EntityPlayer *)
{
}
