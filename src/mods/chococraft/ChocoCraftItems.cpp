#include "ChocoCraftItems.h"

#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/BlockGrass.h"
#include "net/minecraft/src/ItemBlock.h"
#include "net/minecraft/src/ItemStack.h"
#include "net/minecraft/src/CraftingManager.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/IBlockAccess.h"
#include "net/minecraft/src/StringTranslate.h"
#include "platform/Log.h"

namespace
{
bool chocoIsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);
}
}

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
    const int_t below = world->getBlockId(x, y - 1, z);
    return canThisPlantGrowOnThisBlockID(below);
}

int_t BlockGysahlGreens::getBlockTextureFromSideAndMetadata(int_t side, int_t meta)
{
    (void)side;
    if (meta < 0) meta = 7;
    if (meta > 7) meta = 7;
    // Reuse vanilla wheat crop stages (88..95) with Gysahl green-gold tint!
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

ItemChocoTinted::ItemChocoTinted(int id, int_t tintRgb, const char *enName, const char *esName)
    : Item(id)
    , m_tint(tintRgb)
    , m_enName(enName)
    , m_esName(esName)
{
}

std::string ItemChocoTinted::getItemDisplayName(ItemStack *)
{
    return chocoIsSpanish() ? m_esName : m_enName;
}

ItemGysahlGreens::ItemGysahlGreens(int id)
    : ItemFood(id, 2, 0.3f, false)
{
    setIconCoord(9, 5); // Leafy wheat-bundle silhouette, tinted vibrant Gysahl green
    setItemName("gysahlGreens");
}

std::string ItemGysahlGreens::getItemDisplayName(ItemStack *)
{
    return chocoIsSpanish() ? "Verdura Gysahl" : "Gysahl Greens";
}

ItemGysahlSeeds::ItemGysahlSeeds(int id, int cropBlockId)
    : ItemSeeds(id, cropBlockId, Block::tilledField ? Block::tilledField->blockID : 60)
{
    setIconCoord(9, 0); // Seed silhouette, tinted Gysahl emerald
    setItemName("gysahlSeeds");
}

std::string ItemGysahlSeeds::getItemDisplayName(ItemStack *)
{
    return chocoIsSpanish() ? "Semillas Gysahl" : "Gysahl Seeds";
}

namespace ChocoCraftItems
{
BlockGysahlGreens *blockGysahl = nullptr;
ItemGysahlGreens  *gysahlGreens = nullptr;
ItemGysahlSeeds   *gysahlSeeds = nullptr;
ItemChocoTinted   *chocoboSaddle = nullptr;
ItemChocoTinted   *chocoboSaddlebags = nullptr;
ItemChocoTinted   *chocoboFeather = nullptr;
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
            Item::itemsList[BLOCK_GYSAHL_ID] = new ItemBlock(BLOCK_GYSAHL_ID - 256);
    }

    if (Item::itemsList[256 + ITEM_GYSAHL_GREENS_RAW] == nullptr)
        gysahlGreens = new ItemGysahlGreens(ITEM_GYSAHL_GREENS_RAW);

    if (Item::itemsList[256 + ITEM_GYSAHL_SEEDS_RAW] == nullptr)
        gysahlSeeds = new ItemGysahlSeeds(ITEM_GYSAHL_SEEDS_RAW, BLOCK_GYSAHL_ID);

    if (Item::itemsList[256 + ITEM_SADDLE_RAW] == nullptr)
    {
        chocoboSaddle = new ItemChocoTinted(ITEM_SADDLE_RAW, 0xE8B44A, "Chocobo Saddle", "Silla de Chocobo");
        chocoboSaddle->setMaxStackSize(1)->setIconCoord(8, 6)->setItemName("chocoboSaddle");
    }

    if (Item::itemsList[256 + ITEM_SADDLEBAGS_RAW] == nullptr)
    {
        chocoboSaddlebags = new ItemChocoTinted(ITEM_SADDLEBAGS_RAW, 0xC88A3B, "Chocobo Saddlebags", "Alforjas de Chocobo");
        chocoboSaddlebags->setMaxStackSize(1)->setIconCoord(7, 6)->setItemName("chocoboSaddlebags");
    }

    if (Item::itemsList[256 + ITEM_FEATHER_RAW] == nullptr)
    {
        chocoboFeather = new ItemChocoTinted(ITEM_FEATHER_RAW, 0xFFE844, "Chocobo Feather", "Pluma de Chocobo");
        chocoboFeather->setIconCoord(8, 1)->setItemName("chocoboFeather");
    }

    // Register ChocoCraft crafting recipes
    CraftingManager *cm = CraftingManager::getInstance();
    if (cm != nullptr && chocoboSaddle != nullptr && chocoboSaddlebags != nullptr && gysahlGreens != nullptr && gysahlSeeds != nullptr)
    {
        // Chocobo Saddle: Leather + Chocobo Feather (or normal Feather) + String
        cm->addRecipe(new ItemStack(chocoboSaddle, 1),
            { "LFL", "SLS", 'L', Item::leather, 'F', chocoboFeather, 'S', Item::silk });
        cm->addRecipe(new ItemStack(chocoboSaddle, 1),
            { "LFL", "SLS", 'L', Item::leather, 'F', Item::feather, 'S', Item::silk });
        // Chocobo Saddlebags: Leather + Chest + Leather
        cm->addRecipe(new ItemStack(chocoboSaddlebags, 1),
            { "LCL", 'L', Item::leather, 'C', Block::chest });
        // Gysahl Greens -> 2 Gysahl Seeds
        cm->addShapelessRecipe(new ItemStack(gysahlSeeds, 2), { gysahlGreens });
    }

    s_registered = true;
    MC_LOG_INFO("mods", "ChocoCraft items & recipes registered (block=%d, items=%d..%d)\n",
                static_cast<int>(BLOCK_GYSAHL_ID),
                static_cast<int>(256 + ITEM_GYSAHL_GREENS_RAW),
                static_cast<int>(256 + ITEM_FEATHER_RAW));
}
} // namespace ChocoCraftItems
