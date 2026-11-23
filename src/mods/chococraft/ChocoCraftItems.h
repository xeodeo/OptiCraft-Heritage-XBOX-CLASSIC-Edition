#pragma once

#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/BlockCrops.h"
#include "net/minecraft/src/Item.h"
#include "net/minecraft/src/ItemFood.h"
#include "net/minecraft/src/ItemSeeds.h"

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
    void dropBlockAsItemWithChance(World *world, int_t x, int_t y, int_t z, int_t meta, float chance, int_t fortune) override;

protected:
    int_t getSeedItem();
    int_t getCropItem();
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
    int_t getBlockColor() override { return 0xE6C64C; }
    int_t getRenderColor(int_t meta) override { (void)meta; return 0xE6C64C; }
    int_t colorMultiplier(IBlockAccess *access, int_t x, int_t y, int_t z) override { (void)access; (void)x; (void)y; (void)z; return 0xE6C64C; }
    bool usesDefaultColorMultiplier() const override { return false; }
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

class ItemChocoFood : public ItemFood
{
public:
    ItemChocoFood(int id, int healAmount, float sat, bool wolfFood, int_t fallbackTint,
                  const char *customTex, const char *enName, const char *esName);
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

class ItemGysahlSeeds : public ItemSeeds
{
public:
    ItemGysahlSeeds(int id, int cropBlockId);
    const char *getCustomIconTexture() const override { return "/chococraft/textures/items/gysahl_seeds.png"; }
    int getColorFromDamage(int damage) override { (void)damage; return 0x55DD77; }
    int getColorFromDamage(int damage, int renderPass) override { (void)damage; (void)renderPass; return 0x55DD77; }
    std::string getItemDisplayName(ItemStack *stack) override;
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

    extern BlockGysahlGreens    *blockGysahl;
    extern BlockChocoboStraw    *blockStraw;
    extern ItemChocoFood        *gysahlGreens;
    extern ItemGysahlSeeds      *gysahlSeeds;
    extern ItemChocoIcon        *chocoboSaddle;
    extern ItemChocoIcon        *chocoboSaddlebags;
    extern ItemChocoIcon        *chocoboFeather;
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

    void registerAll();
    bool isRegistered();
}
