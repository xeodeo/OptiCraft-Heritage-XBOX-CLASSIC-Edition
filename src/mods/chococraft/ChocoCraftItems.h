#pragma once

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
    int_t idDropped(int_t meta, Random &random) override;
};

class ItemChocoTinted : public Item
{
public:
    ItemChocoTinted(int id, int_t tintRgb, const char *enName, const char *esName);
    int getColorFromDamage(int damage) override { (void)damage; return m_tint; }
    int getColorFromDamage(int damage, int renderPass) override { (void)damage; (void)renderPass; return m_tint; }
    std::string getItemDisplayName(ItemStack *stack) override;

private:
    int_t m_tint;
    const char *m_enName;
    const char *m_esName;
};

class ItemGysahlGreens : public ItemFood
{
public:
    explicit ItemGysahlGreens(int id);
    int getColorFromDamage(int damage) override { (void)damage; return 0x78EE44; }
    int getColorFromDamage(int damage, int renderPass) override { (void)damage; (void)renderPass; return 0x78EE44; }
    std::string getItemDisplayName(ItemStack *stack) override;
};

class ItemGysahlSeeds : public ItemSeeds
{
public:
    ItemGysahlSeeds(int id, int cropBlockId);
    int getColorFromDamage(int damage) override { (void)damage; return 0x55DD77; }
    int getColorFromDamage(int damage, int renderPass) override { (void)damage; (void)renderPass; return 0x55DD77; }
    std::string getItemDisplayName(ItemStack *stack) override;
};

namespace ChocoCraftItems
{
    static constexpr int_t BLOCK_GYSAHL_ID = 210;
    static constexpr int_t ITEM_GYSAHL_GREENS_RAW = 1900; // shiftedIndex = 2156
    static constexpr int_t ITEM_GYSAHL_SEEDS_RAW  = 1901; // shiftedIndex = 2157
    static constexpr int_t ITEM_SADDLE_RAW        = 1902; // shiftedIndex = 2158
    static constexpr int_t ITEM_SADDLEBAGS_RAW    = 1903; // shiftedIndex = 2159
    static constexpr int_t ITEM_FEATHER_RAW       = 1904; // shiftedIndex = 2160

    extern BlockGysahlGreens *blockGysahl;
    extern ItemGysahlGreens  *gysahlGreens;
    extern ItemGysahlSeeds   *gysahlSeeds;
    extern ItemChocoTinted   *chocoboSaddle;
    extern ItemChocoTinted   *chocoboSaddlebags;
    extern ItemChocoTinted   *chocoboFeather;

    void registerAll();
    bool isRegistered();
}
