#include "ChocoCraftItems.h"
#include "EntityChocobo.h"
#include "mods/ModManager.h"

#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/BlockGrass.h"
#include "net/minecraft/src/CreativeTabs.h"
#include "net/minecraft/src/Material.h"
#include "net/minecraft/src/ItemBlock.h"
#include "net/minecraft/src/ItemStack.h"
#include "net/minecraft/src/CraftingManager.h"
#include "net/minecraft/src/FurnaceRecipes.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/EntityPlayer.h"
#include "net/minecraft/src/EntityItem.h"
#include "net/minecraft/src/IBlockAccess.h"
#include "net/minecraft/src/StringTranslate.h"
#include "platform/Log.h"

#include <cmath>

namespace
{
bool chocoIsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);
}
} // namespace

// ============================================================================
// BlockGysahlGreens (ChocoCraft 4.1.5 BlockGysahlStem / BlockGysahlGreen)
// ============================================================================

BlockGysahlGreens::BlockGysahlGreens(int_t id, int_t texIndex)
    : BlockCrops(id, texIndex)
{
    setHardness(0.0f);
    setStepSound(Block::soundGrassFootstep);
    setBlockName("gysahlGreens");
    setBlockBounds(0.15f, 0.0f, 0.15f, 0.85f, 0.65f, 0.85f);
}

bool BlockGysahlGreens::canThisPlantGrowOnThisBlockID(int_t blockId)
{
    return (Block::tilledField != nullptr && blockId == Block::tilledField->blockID) ||
           (Block::grass != nullptr && blockId == Block::grass->blockID) ||
           (Block::dirt != nullptr && blockId == Block::dirt->blockID);
}

bool BlockGysahlGreens::canBlockStay(World *world, int_t x, int_t y, int_t z)
{
    if (world == nullptr)
        return false;
    return canThisPlantGrowOnThisBlockID(world->getBlockId(x, y - 1, z));
}

int_t BlockGysahlGreens::getBlockTextureFromSideAndMetadata(int_t side, int_t meta)
{
    (void)side;
    if (meta < 0 || meta > 7) meta = 7;
    return 88 + meta;
}

int_t BlockGysahlGreens::getBlockColor()
{
    return 0x76E84C;
}

int_t BlockGysahlGreens::getRenderColor(int_t meta)
{
    return (meta >= 7) ? 0xC8F03C : 0x5CE052;
}

int_t BlockGysahlGreens::colorMultiplier(IBlockAccess *access, int_t x, int_t y, int_t z)
{
    int_t meta = (access != nullptr) ? access->getBlockMetadata(x, y, z) : 7;
    return getRenderColor(meta);
}

int_t BlockGysahlGreens::getSeedItem()
{
    return ChocoCraftItems::gysahlSeeds ? ChocoCraftItems::gysahlSeeds->shiftedIndex : Item::seeds->shiftedIndex;
}

int_t BlockGysahlGreens::getCropItem()
{
    return ChocoCraftItems::gysahlGreens ? ChocoCraftItems::gysahlGreens->shiftedIndex : Item::wheat->shiftedIndex;
}

void BlockGysahlGreens::updateTick(World *world, int_t x, int_t y, int_t z, Random &random)
{
    if (!ModManager::getInstance().isModEnabled("chococraft"))
        return;
    BlockCrops::updateTick(world, x, y, z, random);
}

void BlockGysahlGreens::dropBlockAsItemWithChance(World *world, int_t x, int_t y, int_t z, int_t meta, float chance, int_t fortune)
{
    if (world == nullptr || world->multiplayerWorld)
        return;
    if (!ModManager::getInstance().isModEnabled("chococraft"))
        return;

    // Mature Gysahl: drop Green Gysahl with ChocoCraft 4.1.5 mutation chances to Loverly / Golden Gysahl!
    if (meta >= 7)
    {
        Item *dropItem = ChocoCraftItems::gysahlGreens;
        const int roll = world->rand.nextInt(1000);
        if (roll < 100) // 10% mutation rate (DEFAULT_GYSAHL_GREEN_MUTATION_RATE = 100)
        {
            if (world->rand.nextInt(1000) < 100 && ChocoCraftItems::gysahlGolden != nullptr)
                dropItem = ChocoCraftItems::gysahlGolden;
            else if (ChocoCraftItems::gysahlLoverly != nullptr)
                dropItem = ChocoCraftItems::gysahlLoverly;
        }
        if (dropItem != nullptr)
            dropBlockAsItem_do(world, x, y, z, new ItemStack(dropItem, 1, 0));
    }

    // Drop seeds (3 + fortune rolls, exact BlockGysahlStem.java)
    const int_t tries = 3 + fortune;
    for (int_t i = 0; i < tries; ++i)
    {
        if (world->rand.nextInt(15) <= meta && ChocoCraftItems::gysahlSeeds != nullptr)
            dropBlockAsItem_do(world, x, y, z, new ItemStack(ChocoCraftItems::gysahlSeeds, 1, 0));
    }
    (void)chance;
}

// ============================================================================
// BlockChocoboStraw (ChocoCraft 4.1.5 BlockStraw)
// ============================================================================

BlockChocoboStraw::BlockChocoboStraw(int_t id, int_t texIndex)
    : Block(id, texIndex, Material::plants)
{
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    setHardness(0.0f);
    setStepSound(Block::soundGrassFootstep);
    setBlockName("chocoboStraw");
}

bool BlockChocoboStraw::canPlaceBlockAt(World *world, int_t x, int_t y, int_t z)
{
    if (world == nullptr || !ModManager::getInstance().isModEnabled("chococraft"))
        return false;
    const int_t belowId = world->getBlockId(x, y - 1, z);
    Block *below = (belowId > 0 && belowId < 256) ? Block::blocksList[belowId] : nullptr;
    return below != nullptr && below->isOpaqueCube();
}

void BlockChocoboStraw::onNeighborBlockChange(World *world, int_t x, int_t y, int_t z, int_t neighborId)
{
    (void)neighborId;
    if (!ModManager::getInstance().isModEnabled("chococraft"))
        return;
    if (world != nullptr && !canPlaceBlockAt(world, x, y, z))
    {
        dropBlockAsItem(world, x, y, z, world->getBlockMetadata(x, y, z), 0);
        world->setBlockWithNotify(x, y, z, 0);
    }
}

// ============================================================================
// Items with real ChocoCraft 4.1.5 16x16 PNG icons (+ tinted fallback)
// ============================================================================

ItemChocoIcon::ItemChocoIcon(int id, int_t fallbackTint, const char *customTex, const char *enName, const char *esName)
    : Item(id)
    , m_tint(fallbackTint)
    , m_customTex(customTex)
    , m_enName(enName)
    , m_esName(esName)
{
}

std::string ItemChocoIcon::getItemDisplayName(ItemStack *)
{
    return chocoIsSpanish() ? m_esName : m_enName;
}

ItemChocoBlock::ItemChocoBlock(int id, const char *customTex, const char *enName, const char *esName)
    : ItemBlock(id)
    , m_customTex(customTex)
    , m_enName(enName)
    , m_esName(esName)
{
}

std::string ItemChocoBlock::getItemDisplayName(ItemStack *)
{
    return chocoIsSpanish() ? m_esName : m_enName;
}

ItemChocoFood::ItemChocoFood(int id, int healAmount, float sat, bool wolfFood, int_t fallbackTint,
                             const char *customTex, const char *enName, const char *esName)
    : ItemFood(id, healAmount, sat, wolfFood)
    , m_tint(fallbackTint)
    , m_customTex(customTex)
    , m_enName(enName)
    , m_esName(esName)
{
}

std::string ItemChocoFood::getItemDisplayName(ItemStack *)
{
    return chocoIsSpanish() ? m_esName : m_enName;
}

ItemStack *ItemChocoFood::onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *player)
{
    if (!ModManager::getInstance().isModEnabled("chococraft"))
        return itemstack;
    return ItemFood::onItemRightClick(itemstack, world, player);
}

ItemStack *ItemChocoFood::onFoodEaten(ItemStack *itemstack, World *world, EntityPlayer *player)
{
    if (!ModManager::getInstance().isModEnabled("chococraft"))
        return itemstack;
    return ItemFood::onFoodEaten(itemstack, world, player);
}

ItemGysahlSeeds::ItemGysahlSeeds(int id, int cropBlockId)
    : ItemSeeds(id, cropBlockId, Block::tilledField ? Block::tilledField->blockID : 60)
{
    setIconCoord(9, 0);
    setItemName("gysahlSeeds");
}

std::string ItemGysahlSeeds::getItemDisplayName(ItemStack *)
{
    return chocoIsSpanish() ? "Semillas de Gysahl" : "Gysahl Seeds";
}

bool ItemGysahlSeeds::onItemUse(ItemStack *itemstack, EntityPlayer *player, World *world, int x, int y, int z, int side)
{
    if (!ModManager::getInstance().isModEnabled("chococraft"))
        return false;
    return ItemSeeds::onItemUse(itemstack, player, world, x, y, z, side);
}

ItemChocoboWhistle::ItemChocoboWhistle(int id)
    : ItemChocoIcon(id, 0xFFD840, "/chococraft/textures/items/whistle.png", "Chocobo Whistle", "Silbato de Chocobo")
{
    setIconCoord(7, 2);
    setItemName("chocoboWhistle");
}

ItemStack *ItemChocoboWhistle::onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *player)
{
    if (world == nullptr || player == nullptr || !ModManager::getInstance().isModEnabled("chococraft"))
        return itemstack;

    world->playSoundAtEntity(player, "random.orb", 0.8f, 1.8f);
    if (!world->multiplayerWorld)
    {
        EntityChocobo *closest = nullptr;
        double bestDistSq = 128.0 * 128.0;
        for (Entity *ent : world->loadedEntityList)
        {
            EntityChocobo *choco = dynamic_cast<EntityChocobo *>(ent);
            if (choco == nullptr || choco->isDead || !choco->isTamed())
                continue;
            if (!choco->getOwnerName().empty() && choco->getOwnerName() != std::string(player->username))
                continue;
            const double d2 = choco->getDistanceSqToEntity(player);
            if (d2 < bestDistSq)
            {
                bestDistSq = d2;
                closest = choco;
            }
        }
        if (closest != nullptr)
        {
            closest->teleportToOwner(player);
            closest->setAiMode(ChocoboAiMode::Follow);
        }
    }
    return itemstack;
}

ItemPurpleChocoboEgg::ItemPurpleChocoboEgg(int id)
    : ItemChocoIcon(id, 0xA030D8, "/chococraft/textures/items/eggPurple.png", "Purple Chocobo Egg", "Huevo de Chocobo Morado")
{
    setIconCoord(12, 0);
    setItemName("eggPurple");
}

ItemStack *ItemPurpleChocoboEgg::onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *player)
{
    if (world == nullptr || player == nullptr || !ModManager::getInstance().isModEnabled("chococraft"))
        return itemstack;

    if (!player->capabilities.isCreativeMode && itemstack != nullptr)
        itemstack->stackSize--;

    world->playSoundAtEntity(player, "random.bow", 0.5f, 0.9f);
    if (!world->multiplayerWorld)
    {
        const float yawRad = player->rotationYaw * 3.14159265f / 180.0f;
        const double sx = player->posX - std::sin(yawRad) * 2.0;
        const double sz = player->posZ + std::cos(yawRad) * 2.0;
        EntityChocobo *baby = new EntityChocobo(world);
        baby->setColor(ChocoboColor::Purple);
        baby->setGrowingAge(-24000);
        baby->setTamed(true);
        baby->setOwnerName(player->username);
        baby->setLocationAndAngles(sx, player->posY + 0.2, sz, player->rotationYaw, 0.0f);
        world->entityJoinedWorld(baby);
    }
    return itemstack;
}

// ============================================================================
// Registration (Items, Blocks, Crafting & Smelting Recipes from ChocoCraft 4.1.5)
// ============================================================================

namespace ChocoCraftItems
{
BlockGysahlGreens    *blockGysahl = nullptr;
BlockChocoboStraw    *blockStraw = nullptr;
ItemChocoIcon        *gysahlGreens = nullptr;
ItemGysahlSeeds      *gysahlSeeds = nullptr;
ItemChocoIcon        *chocoboSaddle = nullptr;
ItemChocoIcon        *chocoboSaddlebags = nullptr;
ItemChocoIcon        *chocoboFeather = nullptr;
ItemChocoIcon        *chocoboPackbags = nullptr;
ItemChocoIcon        *gysahlLoverly = nullptr;
ItemChocoIcon        *gysahlGolden = nullptr;
ItemChocoIcon        *gysahlPink = nullptr;
ItemChocoIcon        *gysahlRed = nullptr;
ItemChocoIcon        *gysahlCake = nullptr;
ItemChocoIcon        *gysahlPicklesRaw = nullptr;
ItemChocoFood        *gysahlPickles = nullptr;
ItemChocoFood        *chocoboLegRaw = nullptr;
ItemChocoFood        *chocoboLegCooked = nullptr;
ItemChocoboWhistle   *chocoboWhistle = nullptr;
ItemPurpleChocoboEgg *purpleChocoboEgg = nullptr;

static bool s_registered = false;

bool isRegistered()
{
    return s_registered;
}

void registerAll()
{
    if (s_registered)
        return;

    if (Block::blocksList[BLOCK_GYSAHL_ID] == nullptr)
    {
        blockGysahl = new BlockGysahlGreens(BLOCK_GYSAHL_ID, 88);
        Block::opaqueCubeLookup[BLOCK_GYSAHL_ID] = false;
        Block::lightOpacity[BLOCK_GYSAHL_ID] = 0;
        Block::canBlockGrass[BLOCK_GYSAHL_ID] = false;
        if (Item::itemsList[BLOCK_GYSAHL_ID] == nullptr)
            Item::itemsList[BLOCK_GYSAHL_ID] = new ItemChocoBlock(BLOCK_GYSAHL_ID - 256,
                "/chococraft/textures/blocks/gysahl_stem05.png", "Gysahl Stem", "Tallo de Gysahl");
    }

    if (Block::blocksList[BLOCK_STRAW_ID] == nullptr)
    {
        // Wheat (ripe) tinted straw-yellow: closest vanilla terrain tile to strawBlock.
        blockStraw = new BlockChocoboStraw(BLOCK_STRAW_ID, 95);
        Block::opaqueCubeLookup[BLOCK_STRAW_ID] = false;
        Block::lightOpacity[BLOCK_STRAW_ID] = 0;
        Block::canBlockGrass[BLOCK_STRAW_ID] = false;
        if (Item::itemsList[BLOCK_STRAW_ID] == nullptr)
            Item::itemsList[BLOCK_STRAW_ID] = new ItemChocoBlock(BLOCK_STRAW_ID - 256,
                "/chococraft/textures/blocks/chocoboStraw.png", "Straw", "Paja");
    }

    if (Item::itemsList[256 + ITEM_GYSAHL_GREENS_RAW] == nullptr)
    {
        // In ChocoCraft the Gysahl Green is not food (it is the gysahlGreenBlock item).
        gysahlGreens = new ItemChocoIcon(ITEM_GYSAHL_GREENS_RAW, 0x78EE44,
            "/chococraft/textures/blocks/gysahl_green.png", "Gysahl Green", "Verdura de Gysahl");
        gysahlGreens->setIconCoord(9, 5)->setItemName("gysahlGreens");
    }

    if (Item::itemsList[256 + ITEM_GYSAHL_SEEDS_RAW] == nullptr)
        gysahlSeeds = new ItemGysahlSeeds(ITEM_GYSAHL_SEEDS_RAW, BLOCK_GYSAHL_ID);

    if (Item::itemsList[256 + ITEM_SADDLE_RAW] == nullptr)
    {
        chocoboSaddle = new ItemChocoIcon(ITEM_SADDLE_RAW, 0xE8B44A,
            "/chococraft/textures/items/chocoboSaddle.png", "Chocobo Saddle", "Silla de Chocobo");
        chocoboSaddle->setMaxStackSize(5)->setIconCoord(8, 6)->setItemName("chocoboSaddle");
    }

    if (Item::itemsList[256 + ITEM_SADDLEBAGS_RAW] == nullptr)
    {
        chocoboSaddlebags = new ItemChocoIcon(ITEM_SADDLEBAGS_RAW, 0xC88A3B,
            "/chococraft/textures/items/saddlebag.png", "Chocobo Saddle Bags", "Alforjas de Chocobo");
        chocoboSaddlebags->setMaxStackSize(8)->setIconCoord(7, 6)->setItemName("chocoboSaddlebags");
    }

    if (Item::itemsList[256 + ITEM_FEATHER_RAW] == nullptr)
    {
        chocoboFeather = new ItemChocoIcon(ITEM_FEATHER_RAW, 0xFFE844,
            "/chococraft/textures/items/chocoboFeather.png", "Chocobo Feather", "Pluma de Chocobo");
        chocoboFeather->setIconCoord(8, 1)->setItemName("chocoboFeather");
    }

    if (Item::itemsList[256 + ITEM_PACKBAGS_RAW] == nullptr)
    {
        chocoboPackbags = new ItemChocoIcon(ITEM_PACKBAGS_RAW, 0xA86E34,
            "/chococraft/textures/items/packbag.png", "Chocobo Pack Bags", "Mochila de Chocobo");
        chocoboPackbags->setMaxStackSize(8)->setIconCoord(7, 6)->setItemName("chocoboPackbags");
    }

    if (Item::itemsList[256 + ITEM_GYSAHL_LOVERLY_RAW] == nullptr)
    {
        gysahlLoverly = new ItemChocoIcon(ITEM_GYSAHL_LOVERLY_RAW, 0xFF70A8,
            "/chococraft/textures/items/gysahl_loverly.png", "Loverly Gysahl", "Gysahl amorosa");
        gysahlLoverly->setIconCoord(9, 5)->setItemName("gysahlLoverly");
    }

    if (Item::itemsList[256 + ITEM_GYSAHL_GOLDEN_RAW] == nullptr)
    {
        gysahlGolden = new ItemChocoIcon(ITEM_GYSAHL_GOLDEN_RAW, 0xFFD820,
            "/chococraft/textures/items/gysahl_golden.png", "Golden Gysahl", "Gysahl dorada");
        gysahlGolden->setIconCoord(9, 5)->setItemName("gysahlGolden");
    }

    if (Item::itemsList[256 + ITEM_GYSAHL_PINK_RAW] == nullptr)
    {
        gysahlPink = new ItemChocoIcon(ITEM_GYSAHL_PINK_RAW, 0xFF88CC,
            "/chococraft/textures/items/gysahl_pink.png", "Pink Gysahl", "Gysahl rosa");
        gysahlPink->setIconCoord(9, 5)->setItemName("gysahlPink");
    }

    if (Item::itemsList[256 + ITEM_GYSAHL_RED_RAW] == nullptr)
    {
        gysahlRed = new ItemChocoIcon(ITEM_GYSAHL_RED_RAW, 0xE83030,
            "/chococraft/textures/items/gysahl_red.png", "Red Gysahl", "Gysahl roja");
        gysahlRed->setIconCoord(9, 5)->setItemName("gysahlRed");
    }

    if (Item::itemsList[256 + ITEM_GYSAHL_CAKE_RAW] == nullptr)
    {
        gysahlCake = new ItemChocoIcon(ITEM_GYSAHL_CAKE_RAW, 0x88E050,
            "/chococraft/textures/items/gysahl_cake.png", "Gysahl Cake", "Pastel de Gysahl");
        gysahlCake->setMaxStackSize(8)->setIconCoord(13, 1)->setItemName("gysahlCake");
    }

    if (Item::itemsList[256 + ITEM_GYSAHL_RAWPKL_RAW] == nullptr)
    {
        gysahlPicklesRaw = new ItemChocoIcon(ITEM_GYSAHL_RAWPKL_RAW, 0x90E070,
            "/chococraft/textures/items/gysahl_rawpickles.png", "Gysahl Raw Pickles", "Encurtido de Gysahl crudo");
        gysahlPicklesRaw->setIconCoord(9, 5)->setItemName("gysahlPicklesRaw");
    }

    if (Item::itemsList[256 + ITEM_GYSAHL_PICKLES_RAW] == nullptr)
    {
        gysahlPickles = new ItemChocoFood(ITEM_GYSAHL_PICKLES_RAW, 2, 0.4f, false, 0x48B838,
            "/chococraft/textures/items/gysahl_pickles.png", "Gysahl Pickles", "Encurtido de Gysahl");
        gysahlPickles->setIconCoord(9, 5)->setItemName("gysahlPickles");
    }

    if (Item::itemsList[256 + ITEM_LEG_RAW_RAW] == nullptr)
    {
        chocoboLegRaw = new ItemChocoFood(ITEM_LEG_RAW_RAW, 4, 0.3f, true, 0xE8A898,
            "/chococraft/textures/items/legRaw.png", "Raw Chocobo Leg", "Muslo de Chocobo Crudo");
        chocoboLegRaw->setIconCoord(9, 7)->setItemName("chocoboLegRaw");
    }

    if (Item::itemsList[256 + ITEM_LEG_COOKED_RAW] == nullptr)
    {
        chocoboLegCooked = new ItemChocoFood(ITEM_LEG_COOKED_RAW, 8, 0.8f, true, 0xC87838,
            "/chococraft/textures/items/legCooked.png", "Cooked Chocobo Leg", "Muslo de Chocobo Asado");
        chocoboLegCooked->setIconCoord(10, 7)->setItemName("chocoboLegCooked");
    }

    if (Item::itemsList[256 + ITEM_WHISTLE_RAW] == nullptr)
        chocoboWhistle = new ItemChocoboWhistle(ITEM_WHISTLE_RAW);

    if (Item::itemsList[256 + ITEM_PURPLE_EGG_RAW] == nullptr)
        purpleChocoboEgg = new ItemPurpleChocoboEgg(ITEM_PURPLE_EGG_RAW);

    // Assign Creative tabs (1.5.2) — dynamically hidden by CreativeTabs/GuiContainerCreative when ChocoCraft is OFF.
    CreativeTabs::initialize();
    if (blockStraw != nullptr && CreativeTabs::tabDecorations != nullptr)
        blockStraw->setCreativeTab(CreativeTabs::tabDecorations);
    if (CreativeTabs::tabFood != nullptr)
    {
        if (gysahlGreens != nullptr)     gysahlGreens->setCreativeTab(CreativeTabs::tabMisc);
        if (gysahlCake != nullptr)       gysahlCake->setCreativeTab(CreativeTabs::tabFood);
        if (gysahlPicklesRaw != nullptr) gysahlPicklesRaw->setCreativeTab(CreativeTabs::tabFood);
        if (gysahlPickles != nullptr)    gysahlPickles->setCreativeTab(CreativeTabs::tabFood);
        if (chocoboLegRaw != nullptr)    chocoboLegRaw->setCreativeTab(CreativeTabs::tabFood);
        if (chocoboLegCooked != nullptr) chocoboLegCooked->setCreativeTab(CreativeTabs::tabFood);
    }
    if (CreativeTabs::tabMisc != nullptr)
    {
        if (gysahlSeeds != nullptr)       gysahlSeeds->setCreativeTab(CreativeTabs::tabMisc);
        if (chocoboSaddle != nullptr)     chocoboSaddle->setCreativeTab(CreativeTabs::tabMisc);
        if (chocoboSaddlebags != nullptr) chocoboSaddlebags->setCreativeTab(CreativeTabs::tabMisc);
        if (chocoboPackbags != nullptr)   chocoboPackbags->setCreativeTab(CreativeTabs::tabMisc);
        if (chocoboFeather != nullptr)    chocoboFeather->setCreativeTab(CreativeTabs::tabMisc);
        if (gysahlLoverly != nullptr)     gysahlLoverly->setCreativeTab(CreativeTabs::tabMisc);
        if (gysahlGolden != nullptr)      gysahlGolden->setCreativeTab(CreativeTabs::tabMisc);
        if (gysahlPink != nullptr)        gysahlPink->setCreativeTab(CreativeTabs::tabMisc);
        if (gysahlRed != nullptr)         gysahlRed->setCreativeTab(CreativeTabs::tabMisc);
        if (chocoboWhistle != nullptr)    chocoboWhistle->setCreativeTab(CreativeTabs::tabMisc);
        if (purpleChocoboEgg != nullptr)  purpleChocoboEgg->setCreativeTab(CreativeTabs::tabMisc);
    }

    // ChocoCraft 4.1.5 crafting recipes (ChocoCraftRecipes.java)
    CraftingManager *cm = CraftingManager::getInstance();
    if (cm != nullptr && chocoboSaddle != nullptr && chocoboFeather != nullptr)
    {
        // Chocobo Saddle: "-X-" / " Y " (string + leather + chocobo feather) & vanilla saddle + chocobo feather
        cm->addRecipe(new ItemStack(chocoboSaddle, 1),
            { "-X-", " Y ", 'X', Item::leather, 'Y', chocoboFeather, '-', Item::silk });
        if (Item::saddle != nullptr)
            cm->addShapelessRecipe(new ItemStack(chocoboSaddle, 1), { Item::saddle, chocoboFeather });
        // Keep convenient fallback recipe using vanilla feather as well
        cm->addRecipe(new ItemStack(chocoboSaddle, 1),
            { "LFL", "SLS", 'L', Item::leather, 'F', Item::feather, 'S', Item::silk });

        // Chocobo Saddlebags: " Y " / "X X" / " X " (ChocoCraft 4.1.5) & LCL shortcut
        if (chocoboSaddlebags != nullptr)
        {
            cm->addRecipe(new ItemStack(chocoboSaddlebags, 1),
                { " Y ", "X X", " X ", 'X', Item::leather, 'Y', chocoboFeather });
            cm->addRecipe(new ItemStack(chocoboSaddlebags, 1),
                { "LCL", 'L', Item::leather, 'C', Block::chest });
        }

        // Chocobo Pack Bags (54 slots): "-Y-" / "C C" / "-X-"
        if (chocoboPackbags != nullptr && Block::cloth != nullptr)
        {
            cm->addRecipe(new ItemStack(chocoboPackbags, 1),
                { "-Y-", "C C", "-X-", 'C', Block::cloth, 'X', Item::leather, 'Y', chocoboFeather, '-', Item::silk });
        }

        // Chocobo Whistle: feather + gold ingot
        if (chocoboWhistle != nullptr && Item::ingotGold != nullptr)
            cm->addShapelessRecipe(new ItemStack(chocoboWhistle, 1), { chocoboFeather, Item::ingotGold });

        // Gysahl Greens -> 3 Gysahl Seeds
        if (gysahlSeeds != nullptr && gysahlGreens != nullptr)
            cm->addShapelessRecipe(new ItemStack(gysahlSeeds, 3), { gysahlGreens });

        // Gysahl Cake: "MGM" / "SES" / "WGW"
        if (gysahlCake != nullptr && gysahlGreens != nullptr && Item::bucketMilk != nullptr && Item::sugar != nullptr && Item::egg != nullptr)
        {
            cm->addRecipe(new ItemStack(gysahlCake, 1),
                { "MGM", "SES", "WGW", 'G', gysahlGreens, 'M', Item::bucketMilk, 'S', Item::sugar, 'W', Item::wheat, 'E', Item::egg });
        }

        // Gysahl Raw Pickles: Gysahl + Sugar -> 2
        if (gysahlPicklesRaw != nullptr && gysahlGreens != nullptr && Item::sugar != nullptr)
            cm->addShapelessRecipe(new ItemStack(gysahlPicklesRaw, 2), { gysahlGreens, Item::sugar });

        // Pink & Red Gysahl (for painting Gold Chocobo -> Pink / Red!)
        if (gysahlPink != nullptr && gysahlGreens != nullptr && Item::dyePowder != nullptr)
            cm->addShapelessRecipe(new ItemStack(gysahlPink, 1), { gysahlGreens, new ItemStack(Item::dyePowder, 1, 9) });
        if (gysahlRed != nullptr && gysahlGreens != nullptr && Item::dyePowder != nullptr)
            cm->addShapelessRecipe(new ItemStack(gysahlRed, 1), { gysahlGreens, new ItemStack(Item::dyePowder, 1, 1) });

        // Straw: "WW" -> 8
        if (blockStraw != nullptr)
            cm->addRecipe(new ItemStack(blockStraw, 8), { "WW", 'W', Item::wheat });

        // Arrows using Chocobo Feather
        if (Item::arrow != nullptr && Item::flint != nullptr && Item::stick != nullptr)
            cm->addRecipe(new ItemStack(Item::arrow, 4), { " F ", " S ", " Y ", 'F', Item::flint, 'S', Item::stick, 'Y', chocoboFeather });
    }

    // ChocoCraft 4.1.5 smelting recipes
    FurnaceRecipes *fr = FurnaceRecipes::smelting();
    if (fr != nullptr)
    {
        if (chocoboLegRaw != nullptr && chocoboLegCooked != nullptr)
            fr->addSmelting(chocoboLegRaw->shiftedIndex, new ItemStack(chocoboLegCooked, 1));
        if (gysahlPicklesRaw != nullptr && gysahlPickles != nullptr)
            fr->addSmelting(gysahlPicklesRaw->shiftedIndex, new ItemStack(gysahlPickles, 1));
    }

    s_registered = true;
    MC_LOG_INFO("mods", "ChocoCraft 4.1.5 items, blocks & recipes registered (blocks=%d..%d, items=%d..%d)\n",
                static_cast<int>(BLOCK_GYSAHL_ID), static_cast<int>(BLOCK_STRAW_ID),
                static_cast<int>(256 + ITEM_GYSAHL_GREENS_RAW), static_cast<int>(256 + ITEM_PURPLE_EGG_RAW));
}
} // namespace ChocoCraftItems
