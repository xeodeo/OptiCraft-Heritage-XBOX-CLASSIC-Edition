#pragma once

#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/BlockCrops.h"
#include "net/minecraft/src/Item.h"
#include "net/minecraft/src/ItemBlock.h"
#include "net/minecraft/src/ItemFood.h"
#include "net/minecraft/src/ItemSeeds.h"
#include "net/minecraft/src/ItemArmor.h"
#include "net/minecraft/src/BlockFlower.h"

class BlockGysahlGreens : public BlockCrops
{
public:
    BlockGysahlGreens(int_t id, int_t texIndex);

    bool canThisPlantGrowOnThisBlockID(int_t blockId) override;
    bool canBlockStay(World *world, int_t x, int_t y, int_t z) override;
    int_t getRenderType() override { return 1; }
    int_t getBlockTextureFromSideAndMetadata(int_t side, int_t meta) override;
    int_t getBlockColor() override;
    int_t getRenderColor(int_t meta) override;
    int_t colorMultiplier(IBlockAccess *access, int_t x, int_t y, int_t z) override;
    bool usesDefaultColorMultiplier() const override { return false; }
    void updateTick(World *world, int_t x, int_t y, int_t z, Random &random) override;
    void dropBlockAsItemWithChance(World *world, int_t x, int_t y, int_t z, int_t meta, float chance, int_t fortune) override;

protected:
    int_t getSeedItem();
    int_t getCropItem();
};

// ChocoCraft 4.1.5 BlockGysahlGreen: the wild Gysahl bush (a BlockBush), generated in the
// world and placed from the Gysahl Green item. Drawn with its own texture (terrain tile
// TILE_GYSAHL_GREEN, filled at runtime from blocks/gysahl_green.png).
class BlockGysahlGreenBush : public BlockFlower
{
public:
    BlockGysahlGreenBush(int_t id, int_t texIndex);
    int_t idDropped(int_t meta, Random &random) override;
    void updateTick(World *world, int_t x, int_t y, int_t z, Random &random) override;
};

class BlockChocoboStraw : public Block
{
public:
    BlockChocoboStraw(int_t id, int_t texIndex);

    AxisAlignedBB *getCollisionBoundingBoxFromPool(World *world, int_t x, int_t y, int_t z) override { (void)world; (void)x; (void)y; (void)z; return nullptr; }
    bool isOpaqueCube() override { return false; }
    bool renderAsNormalBlock() override { return false; }
    bool canPlaceBlockAt(World *world, int_t x, int_t y, int_t z) override;
    void onNeighborBlockChange(World *world, int_t x, int_t y, int_t z, int_t neighborId) override;
    // Own texture (terrain tile TILE_STRAW, from blocks/chocoboStraw.png), no tint.
    int_t quantityDropped(Random &random) override { (void)random; return 1; }
};

class ItemChocoIcon : public Item
{
public:
    ItemChocoIcon(int id, int_t fallbackTint, const char *customTex, const char *enName, const char *esName);
    const char *getCustomIconTexture() const override { return m_customTex; }
    int getColorFromDamage(int damage) override { (void)damage; return m_tint; }
    int getColorFromDamage(int damage, int renderPass) override { (void)damage; (void)renderPass; return m_tint; }
    std::string getItemDisplayName(ItemStack *stack) override;

private:
    int_t m_tint;
    const char *m_customTex;
    const char *m_enName;
    const char *m_esName;
};

// Item form of the ChocoCraft blocks: real name (en/es) and the original flat icon.
class ItemChocoBlock : public ItemBlock
{
public:
    ItemChocoBlock(int id, const char *customTex, const char *enName, const char *esName);
    const char *getCustomIconTexture() const override { return m_customTex; }
    std::string getItemDisplayName(ItemStack *stack) override;

private:
    const char *m_customTex;
    const char *m_enName;
    const char *m_esName;
};

class ItemChocoFood : public ItemFood
{
public:
    ItemChocoFood(int id, int healAmount, float sat, bool wolfFood, int_t fallbackTint,
                  const char *customTex, const char *enName, const char *esName);
    const char *getCustomIconTexture() const override { return m_customTex; }
    int getColorFromDamage(int damage) override { (void)damage; return m_tint; }
    int getColorFromDamage(int damage, int renderPass) override { (void)damage; (void)renderPass; return m_tint; }
    std::string getItemDisplayName(ItemStack *stack) override;
    ItemStack *onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *player) override;
    ItemStack *onFoodEaten(ItemStack *itemstack, World *world, EntityPlayer *player) override;

private:
    int_t m_tint;
    const char *m_customTex;
    const char *m_enName;
    const char *m_esName;
};

class ItemGysahlSeeds : public ItemSeeds
{
public:
    ItemGysahlSeeds(int id, int cropBlockId);
    const char *getCustomIconTexture() const override { return "/chococraft/textures/items/gysahl_seeds.png"; }
    int getColorFromDamage(int damage) override { (void)damage; return 0x55DD77; }
    int getColorFromDamage(int damage, int renderPass) override { (void)damage; (void)renderPass; return 0x55DD77; }
    std::string getItemDisplayName(ItemStack *stack) override;
    bool onItemUse(ItemStack *itemstack, EntityPlayer *player, World *world, int x, int y, int z, int side) override;
};

// Gysahl Green: feeds/tames chocobos and, like the original ItemBlock, plants the bush.
class ItemGysahlGreen : public ItemChocoIcon
{
public:
    explicit ItemGysahlGreen(int id);
    bool onItemUse(ItemStack *itemstack, EntityPlayer *player, World *world, int x, int y, int z, int side) override;
};

// Chocobo Feather: right-click on a bookshelf turns it into a Chocopedia (ChocoCraftEventCommon).
class ItemChocoboFeather : public ItemChocoIcon
{
public:
    explicit ItemChocoboFeather(int id);
    bool onItemUse(ItemStack *itemstack, EntityPlayer *player, World *world, int x, int y, int z, int side) override;
};

// The Chocopedia: right-click in the air opens the book; on a tamed chocobo, its page.
class ItemChocopedia : public ItemChocoIcon
{
public:
    explicit ItemChocopedia(int id);
    ItemStack *onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *player) override;
};

// Chocodisguise (ChocoboItemDisguise): feather armour, no protection, durability 1.
class ItemChocoDisguise : public ItemArmor
{
public:
    ItemChocoDisguise(int id, int armorType, const char *customTex, const char *enName, const char *esName);
    const char *getCustomIconTexture() const override { return m_customTex; }
    const char *getCustomArmorTexture(int layer) const override;
    int getItemEnchantability() override { return 0; }
    std::string getItemDisplayName(ItemStack *stack) override;

private:
    const char *m_customTex;
    const char *m_enName;
    const char *m_esName;
};

class ItemChocoboWhistle : public ItemChocoIcon
{
public:
    explicit ItemChocoboWhistle(int id);
    ItemStack *onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *player) override;
};

class ItemPurpleChocoboEgg : public ItemChocoIcon
{
public:
    explicit ItemPurpleChocoboEgg(int id);
    ItemStack *onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *player) override;
};

namespace ChocoCraftItems
{
    static constexpr int_t BLOCK_GYSAHL_ID = 210;
    static constexpr int_t BLOCK_STRAW_ID  = 211;
    static constexpr int_t BLOCK_GYSAHL_GREEN_ID = 212;

    // Spare terrain.png tiles (empty in 1.2.5 and in the 1.5.2 atlas) that get the
    // ChocoCraft block textures at runtime (ChocoCraftTileFX).
    static constexpr int_t TILE_GYSAHL_GREEN = 180;
    static constexpr int_t TILE_STRAW        = 181;

    static constexpr int_t ITEM_GYSAHL_GREENS_RAW  = 1900; // 2156
    static constexpr int_t ITEM_GYSAHL_SEEDS_RAW   = 1901; // 2157
    static constexpr int_t ITEM_SADDLE_RAW         = 1902; // 2158
    static constexpr int_t ITEM_SADDLEBAGS_RAW     = 1903; // 2159
    static constexpr int_t ITEM_FEATHER_RAW        = 1904; // 2160
    static constexpr int_t ITEM_PACKBAGS_RAW       = 1905; // 2161
    static constexpr int_t ITEM_GYSAHL_LOVERLY_RAW = 1906; // 2162
    static constexpr int_t ITEM_GYSAHL_GOLDEN_RAW  = 1907; // 2163
    static constexpr int_t ITEM_GYSAHL_PINK_RAW    = 1908; // 2164
    static constexpr int_t ITEM_GYSAHL_RED_RAW     = 1909; // 2165
    static constexpr int_t ITEM_GYSAHL_CAKE_RAW    = 1910; // 2166
    static constexpr int_t ITEM_GYSAHL_RAWPKL_RAW  = 1911; // 2167
    static constexpr int_t ITEM_GYSAHL_PICKLES_RAW = 1912; // 2168
    static constexpr int_t ITEM_LEG_RAW_RAW        = 1913; // 2169
    static constexpr int_t ITEM_LEG_COOKED_RAW     = 1914; // 2170
    static constexpr int_t ITEM_WHISTLE_RAW        = 1915; // 2171
    static constexpr int_t ITEM_PURPLE_EGG_RAW     = 1916; // 2172
    static constexpr int_t ITEM_CHOCOPEDIA_RAW     = 1917; // 2173
    static constexpr int_t ITEM_DISGUISE_HEAD_RAW  = 1918; // 2174
    static constexpr int_t ITEM_DISGUISE_BODY_RAW  = 1919; // 2175
    static constexpr int_t ITEM_DISGUISE_LEGS_RAW  = 1920; // 2176
    static constexpr int_t ITEM_DISGUISE_BOOTS_RAW = 1921; // 2177
    static constexpr int_t ITEM_FIRST_RAW          = ITEM_GYSAHL_GREENS_RAW;
    static constexpr int_t ITEM_LAST_RAW           = ITEM_DISGUISE_BOOTS_RAW;

    extern BlockGysahlGreens    *blockGysahl;
    extern BlockChocoboStraw    *blockStraw;
    extern BlockGysahlGreenBush *blockGysahlGreen;
    extern ItemGysahlGreen      *gysahlGreens;
    extern ItemGysahlSeeds      *gysahlSeeds;
    extern ItemChocoIcon        *chocoboSaddle;
    extern ItemChocoIcon        *chocoboSaddlebags;
    extern ItemChocoboFeather   *chocoboFeather;
    extern ItemChocoIcon        *chocoboPackbags;
    extern ItemChocoIcon        *gysahlLoverly;
    extern ItemChocoIcon        *gysahlGolden;
    extern ItemChocoIcon        *gysahlPink;
    extern ItemChocoIcon        *gysahlRed;
    extern ItemChocoIcon        *gysahlCake;
    extern ItemChocoIcon        *gysahlPicklesRaw;
    extern ItemChocoFood        *gysahlPickles;
    extern ItemChocoFood        *chocoboLegRaw;
    extern ItemChocoFood        *chocoboLegCooked;
    extern ItemChocoboWhistle   *chocoboWhistle;
    extern ItemPurpleChocoboEgg *purpleChocoboEgg;
    extern ItemChocopedia       *chocopedia;
    extern ItemChocoDisguise    *disguiseHead;
    extern ItemChocoDisguise    *disguiseBody;
    extern ItemChocoDisguise    *disguiseLegs;
    extern ItemChocoDisguise    *disguiseBoots;

    void registerAll();
    bool isRegistered();
}
