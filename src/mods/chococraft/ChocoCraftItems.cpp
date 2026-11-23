#include "ChocoCraftItems.h"
#include "EntityChocobo.h"
#include "GuiChocopedia.h"
#include "mods/ModManager.h"

#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/BlockGrass.h"
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
#include "net/minecraft/src/StepSound.h"
#include "net/minecraft/src/BlockTallGrass.h"
#include "net/minecraft/src/BlockDeadBush.h"
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

    if (meta >= 7)
    {
        Item *dropItem = ChocoCraftItems::gysahlGreens;
        const int roll = world->rand.nextInt(1000);
        if (roll < 100)
        {
            if (world->rand.nextInt(1000) < 100 && ChocoCraftItems::gysahlGolden != nullptr)
                dropItem = ChocoCraftItems::gysahlGolden;
            else if (ChocoCraftItems::gysahlLoverly != nullptr)
                dropItem = ChocoCraftItems::gysahlLoverly;
        }
        if (dropItem != nullptr)
            dropBlockAsItem_do(world, x, y, z, new ItemStack(dropItem, 1, 0));
    }

    const int_t tries = 3 + fortune;
    for (int_t i = 0; i < tries; ++i)
    {
        if (world->rand.nextInt(15) <= meta && ChocoCraftItems::gysahlSeeds != nullptr)
            dropBlockAsItem_do(world, x, y, z, new ItemStack(ChocoCraftItems::gysahlSeeds, 1, 0));
    }
    (void)chance;
}

// ============================================================================
// BlockGysahlGreenBush (ChocoCraft 4.1.5 BlockGysahlGreen, a BlockBush)
// ============================================================================

BlockGysahlGreenBush::BlockGysahlGreenBush(int_t id, int_t texIndex)
    : BlockFlower(id, texIndex)
{
    // BlockGysahlGreen: bounds 0..1 x 0..0.25, grass sound, instant break, random ticks.
    setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.25f, 1.0f);
    setHardness(0.0f);
    setStepSound(Block::soundGrassFootstep);
    setBlockName("gysahlGreenBlock");
}

int_t BlockGysahlGreenBush::idDropped(int_t meta, Random &random)
{
    (void)meta;
    (void)random;
    // The original drops its own ItemBlock; here that is the Gysahl Green item.
    if (!ModManager::getInstance().isModEnabled("chococraft") || ChocoCraftItems::gysahlGreens == nullptr)
        return -1;
    return ChocoCraftItems::gysahlGreens->shiftedIndex;
}

void BlockGysahlGreenBush::updateTick(World *world, int_t x, int_t y, int_t z, Random &random)
{
    if (!ModManager::getInstance().isModEnabled("chococraft"))
        return;
    BlockFlower::updateTick(world, x, y, z, random);
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

ItemGysahlGreen::ItemGysahlGreen(int id)
    : ItemChocoIcon(id, 0x78EE44, "/chococraft/textures/blocks/gysahl_green.png", "Gysahl Green", "Verdura de Gysahl")
{
}

// Same placement as ItemReed / ItemBlock: the Gysahl Green item is the bush's ItemBlock
// in ChocoCraft 4.1.5.
bool ItemGysahlGreen::onItemUse(ItemStack *itemstack, EntityPlayer *player, World *world, int x, int y, int z, int side)
{
    if (itemstack == nullptr || player == nullptr || world == nullptr || !ModManager::getInstance().isModEnabled("chococraft"))
        return false;
    Block *bush = ChocoCraftItems::blockGysahlGreen;
    if (bush == nullptr)
        return false;

    const int_t clicked = world->getBlockId(x, y, z);
    if (Block::snow != nullptr && clicked == Block::snow->blockID)
    {
        side = 1;
    }
    else if (!(Block::tallGrass != nullptr && clicked == Block::tallGrass->blockID) &&
             !(Block::deadBush != nullptr && clicked == Block::deadBush->blockID))
    {
        if (side == 0) y--;
        if (side == 1) y++;
        if (side == 2) z--;
        if (side == 3) z++;
        if (side == 4) x--;
        if (side == 5) x++;
    }

    if (!player->canPlayerEdit(x, y, z) || itemstack->stackSize == 0)
        return false;
    if (!world->canBlockBePlacedAt(bush->blockID, x, y, z, false, side))
        return false;
    if (world->setBlockWithNotify(x, y, z, bush->blockID))
    {
        if (world->getBlockId(x, y, z) == bush->blockID)
        {
            bush->onBlockPlaced(world, x, y, z, side);
            bush->onBlockPlacedBy(world, x, y, z, player);
        }
        world->playSoundEffect(x + 0.5f, y + 0.5f, z + 0.5f, bush->stepSound->getStepSound(),
                               (bush->stepSound->getVolume() + 1.0f) / 2.0f, bush->stepSound->getPitch() * 0.8f);
        if (!player->capabilities.isCreativeMode)
            itemstack->stackSize--;
    }
    return true;
}

ItemChocoboFeather::ItemChocoboFeather(int id)
    : ItemChocoIcon(id, 0xFFE844, "/chococraft/textures/items/chocoboFeather.png", "Chocobo Feather", "Pluma de Chocobo")
{
}

// ChocoCraftEventCommon.onFeatherRightClick: a Chocobo Feather used on a bookshelf
// takes the bookshelf and one feather and leaves a Chocopedia.
bool ItemChocoboFeather::onItemUse(ItemStack *itemstack, EntityPlayer *player, World *world, int x, int y, int z, int side)
{
    (void)side;
    if (itemstack == nullptr || player == nullptr || world == nullptr || !ModManager::getInstance().isModEnabled("chococraft"))
        return false;
    if (Block::bookShelf == nullptr || world->getBlockId(x, y, z) != Block::bookShelf->blockID ||
        ChocoCraftItems::chocopedia == nullptr)
        return false;
    if (world->multiplayerWorld)
        return true;
    world->setBlockWithNotify(x, y, z, 0);
    if (!player->capabilities.isCreativeMode)
        itemstack->stackSize--;
    EntityItem *book = new EntityItem(world, static_cast<double>(x), static_cast<double>(y), static_cast<double>(z),
                                      new ItemStack(ChocoCraftItems::chocopedia, 1, 0));
    world->entityJoinedWorld(book);
    return true;
}

ItemChocopedia::ItemChocopedia(int id)
    : ItemChocoIcon(id, 0xC08040, "/chococraft/textures/items/chocopedia.png", "Chocopedia", "Chocopedia")
{
}

ItemStack *ItemChocopedia::onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *player)
{
    (void)world;
    if (player == nullptr || !ModManager::getInstance().isModEnabled("chococraft"))
        return itemstack;
    // A click on a chocobo already opened its page (the item use follows the entity
    // click in this engine): only open the book when no screen is up for this player.
    if (!GuiChocopedia::isScreenOpenFor(player))
        GuiChocopedia::open(player, nullptr);
    return itemstack;
}

ItemChocoDisguise::ItemChocoDisguise(int id, int armorType, const char *customTex, const char *enName, const char *esName)
    : ItemArmor(id, EnumArmorMaterial::GOLD, 4, armorType)
    , m_customTex(customTex)
    , m_enName(enName)
    , m_esName(esName)
{
    // ChocoboArmourMaterial.CHOCOFEATHER: no protection, durability 1, not enchantable.
    damageReduceAmount = 0;
    setMaxDamage(1);
    maxStackSize = 1;
}

const char *ItemChocoDisguise::getCustomArmorTexture(int layer) const
{
    // Helmet, body and boots use chocoDisguise_1, the leggings chocoDisguise_2.
    return layer == 2 ? "/chococraft/textures/armor/chocoDisguise_2.png"
                      : "/chococraft/textures/armor/chocoDisguise_1.png";
}

std::string ItemChocoDisguise::getItemDisplayName(ItemStack *)
{
    return chocoIsSpanish() ? m_esName : m_enName;
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
BlockGysahlGreenBush *blockGysahlGreen = nullptr;
ItemGysahlGreen      *gysahlGreens = nullptr;
ItemGysahlSeeds      *gysahlSeeds = nullptr;
ItemChocoIcon        *chocoboSaddle = nullptr;
ItemChocoIcon        *chocoboSaddlebags = nullptr;
ItemChocoboFeather   *chocoboFeather = nullptr;
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
ItemChocopedia       *chocopedia = nullptr;
ItemChocoDisguise    *disguiseHead = nullptr;
ItemChocoDisguise    *disguiseBody = nullptr;
ItemChocoDisguise    *disguiseLegs = nullptr;
ItemChocoDisguise    *disguiseBoots = nullptr;

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
        // Own texture: spare terrain tile filled from blocks/chocoboStraw.png (ChocoCraftTileFX).
        blockStraw = new BlockChocoboStraw(BLOCK_STRAW_ID, TILE_STRAW);
        Block::opaqueCubeLookup[BLOCK_STRAW_ID] = false;
        Block::lightOpacity[BLOCK_STRAW_ID] = 0;
        Block::canBlockGrass[BLOCK_STRAW_ID] = false;
        if (Item::itemsList[BLOCK_STRAW_ID] == nullptr)
            Item::itemsList[BLOCK_STRAW_ID] = new ItemChocoBlock(BLOCK_STRAW_ID - 256,
                "/chococraft/textures/blocks/chocoboStraw.png", "Straw", "Paja");
    }

    if (Block::blocksList[BLOCK_GYSAHL_GREEN_ID] == nullptr)
    {
        blockGysahlGreen = new BlockGysahlGreenBush(BLOCK_GYSAHL_GREEN_ID, TILE_GYSAHL_GREEN);
        Block::opaqueCubeLookup[BLOCK_GYSAHL_GREEN_ID] = false;
        Block::lightOpacity[BLOCK_GYSAHL_GREEN_ID] = 0;
        Block::canBlockGrass[BLOCK_GYSAHL_GREEN_ID] = false;
        if (Item::itemsList[BLOCK_GYSAHL_GREEN_ID] == nullptr)
            Item::itemsList[BLOCK_GYSAHL_GREEN_ID] = new ItemChocoBlock(BLOCK_GYSAHL_GREEN_ID - 256,
                "/chococraft/textures/blocks/gysahl_green.png", "Gysahl Green", "Verdura de Gysahl");
    }

    if (Item::itemsList[256 + ITEM_GYSAHL_GREENS_RAW] == nullptr)
    {
        // In ChocoCraft the Gysahl Green is not food: it is the gysahlGreenBlock item
        // (plants the bush, tames and heals chocobos).
        gysahlGreens = new ItemGysahlGreen(ITEM_GYSAHL_GREENS_RAW);
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
        chocoboFeather = new ItemChocoboFeather(ITEM_FEATHER_RAW);
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

    if (Item::itemsList[256 + ITEM_CHOCOPEDIA_RAW] == nullptr)
    {
        chocopedia = new ItemChocopedia(ITEM_CHOCOPEDIA_RAW);
        chocopedia->setMaxStackSize(1)->setIconCoord(11, 3)->setItemName("chocopedia");
    }

    // Chocodisguise (helmet, body, legs, boots): ChocoboItemDisguise, 4.1.5 icons and armour textures.
    if (Item::itemsList[256 + ITEM_DISGUISE_HEAD_RAW] == nullptr)
    {
        disguiseHead = new ItemChocoDisguise(ITEM_DISGUISE_HEAD_RAW, 0,
            "/chococraft/textures/items/disguiseHead.png", "Chocodisguise Helmet", "Casco de Chocodisfraz");
        disguiseHead->setIconCoord(0, 0)->setItemName("disguiseHead");
    }
    if (Item::itemsList[256 + ITEM_DISGUISE_BODY_RAW] == nullptr)
    {
        disguiseBody = new ItemChocoDisguise(ITEM_DISGUISE_BODY_RAW, 1,
            "/chococraft/textures/items/disguiseBody.png", "Chocodisguise Body", "Cuerpo de Chocodisfraz");
        disguiseBody->setIconCoord(0, 1)->setItemName("disguiseBody");
    }
    if (Item::itemsList[256 + ITEM_DISGUISE_LEGS_RAW] == nullptr)
    {
        disguiseLegs = new ItemChocoDisguise(ITEM_DISGUISE_LEGS_RAW, 2,
            "/chococraft/textures/items/disguiseLegs.png", "Chocodisguise Legs", "Patas de Chocodisfraz");
        disguiseLegs->setIconCoord(0, 2)->setItemName("disguiseLegs");
    }
    if (Item::itemsList[256 + ITEM_DISGUISE_BOOTS_RAW] == nullptr)
    {
        disguiseBoots = new ItemChocoDisguise(ITEM_DISGUISE_BOOTS_RAW, 3,
            "/chococraft/textures/items/disguiseBoots.png", "Chocodisguise Boots", "Botas de Chocodisfraz");
        disguiseBoots->setIconCoord(0, 3)->setItemName("disguiseBoots");
    }

    CraftingManager *cm = CraftingManager::getInstance();
    if (cm != nullptr && chocoboSaddle != nullptr && chocoboFeather != nullptr)
    {
        cm->addRecipe(new ItemStack(chocoboSaddle, 1),
            { "-X-", " Y ", 'X', Item::leather, 'Y', chocoboFeather, '-', Item::silk });
        if (Item::saddle != nullptr)
            cm->addShapelessRecipe(new ItemStack(chocoboSaddle, 1), { Item::saddle, chocoboFeather });
        cm->addRecipe(new ItemStack(chocoboSaddle, 1),
            { "LFL", "SLS", 'L', Item::leather, 'F', Item::feather, 'S', Item::silk });

        if (chocoboSaddlebags != nullptr)
        {
            cm->addRecipe(new ItemStack(chocoboSaddlebags, 1),
                { " Y ", "X X", " X ", 'X', Item::leather, 'Y', chocoboFeather });
            cm->addRecipe(new ItemStack(chocoboSaddlebags, 1),
                { "LCL", 'L', Item::leather, 'C', Block::chest });
        }

        if (chocoboPackbags != nullptr && Block::cloth != nullptr)
        {
            cm->addRecipe(new ItemStack(chocoboPackbags, 1),
                { "-Y-", "C C", "-X-", 'C', Block::cloth, 'X', Item::leather, 'Y', chocoboFeather, '-', Item::silk });
        }

        // Chocopedia: "FGF" / "IBI" / "FLF" (feathers, gold ingot, ink sacs, book, lapis)
        if (chocopedia != nullptr && Item::book != nullptr && Item::ingotGold != nullptr && Item::dyePowder != nullptr)
        {
            cm->addRecipe(new ItemStack(chocopedia, 1),
                { "FGF", "IBI", "FLF", 'B', Item::book, 'F', chocoboFeather,
                  'I', new ItemStack(Item::dyePowder, 1, 0), 'L', new ItemStack(Item::dyePowder, 1, 4), 'G', Item::ingotGold });
        }

        // Chocodisguise from Chocobo Feathers (ChocoCraftRecipes)
        if (disguiseHead != nullptr)
            cm->addRecipe(new ItemStack(disguiseHead, 1), { "YYY", "Y Y", 'Y', chocoboFeather });
        if (disguiseBody != nullptr)
            cm->addRecipe(new ItemStack(disguiseBody, 1), { "Y Y", "YYY", "YYY", 'Y', chocoboFeather });
        if (disguiseLegs != nullptr)
            cm->addRecipe(new ItemStack(disguiseLegs, 1), { "YYY", "Y Y", "Y Y", 'Y', chocoboFeather });
        if (disguiseBoots != nullptr)
            cm->addRecipe(new ItemStack(disguiseBoots, 1), { "Y Y", "Y Y", 'Y', chocoboFeather });

        if (chocoboWhistle != nullptr && Item::ingotGold != nullptr)
            cm->addShapelessRecipe(new ItemStack(chocoboWhistle, 1), { chocoboFeather, Item::ingotGold });

        if (gysahlSeeds != nullptr && gysahlGreens != nullptr)
            cm->addShapelessRecipe(new ItemStack(gysahlSeeds, 3), { gysahlGreens });

        if (gysahlCake != nullptr && gysahlGreens != nullptr && Item::bucketMilk != nullptr && Item::sugar != nullptr && Item::egg != nullptr)
        {
            cm->addRecipe(new ItemStack(gysahlCake, 1),
                { "MGM", "SES", "WGW", 'G', gysahlGreens, 'M', Item::bucketMilk, 'S', Item::sugar, 'W', Item::wheat, 'E', Item::egg });
        }

        if (gysahlPicklesRaw != nullptr && gysahlGreens != nullptr && Item::sugar != nullptr)
            cm->addShapelessRecipe(new ItemStack(gysahlPicklesRaw, 2), { gysahlGreens, Item::sugar });

        if (gysahlPink != nullptr && gysahlGreens != nullptr && Item::dyePowder != nullptr)
            cm->addShapelessRecipe(new ItemStack(gysahlPink, 1), { gysahlGreens, new ItemStack(Item::dyePowder, 1, 9) });
        if (gysahlRed != nullptr && gysahlGreens != nullptr && Item::dyePowder != nullptr)
            cm->addShapelessRecipe(new ItemStack(gysahlRed, 1), { gysahlGreens, new ItemStack(Item::dyePowder, 1, 1) });

        if (blockStraw != nullptr)
            cm->addRecipe(new ItemStack(blockStraw, 8), { "WW", 'W', Item::wheat });

        if (Item::arrow != nullptr && Item::flint != nullptr && Item::stick != nullptr)
            cm->addRecipe(new ItemStack(Item::arrow, 4), { " F ", " S ", " Y ", 'F', Item::flint, 'S', Item::stick, 'Y', chocoboFeather });
    }

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
                static_cast<int>(BLOCK_GYSAHL_ID), static_cast<int>(BLOCK_GYSAHL_GREEN_ID),
                static_cast<int>(256 + ITEM_FIRST_RAW), static_cast<int>(256 + ITEM_LAST_RAW));
}
} // namespace ChocoCraftItems
