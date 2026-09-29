#include "LegacyCraftingScreen.h"

#include <algorithm>
#include <vector>
#include <string>

#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/BlockFlower.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/Item.h"
#include "net/minecraft/src/ItemStack.h"
#include "net/minecraft/src/InventoryPlayer.h"
#include "net/minecraft/src/EntityPlayer.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/RenderEngine.h"
#include "net/minecraft/src/RenderItem.h"
#include "net/minecraft/src/RenderHelper.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/ControlIcon.h"
#include "net/minecraft/src/UiStrings.h"
#include "net/minecraft/src/GuiInventory.h"
#include "net/minecraft/src/KeyBinding.h"
#include "net/minecraft/src/OpenGlHelper.h"
#include "platform/Input.h"
#include "platform/PlatformConfig.h"
#include "platform/RenderAPI.h"
#include "LegacyMenuHints.h"
#include "LegacySelectionCursor.h"
#include "LegacyUiTheme.h"
#include "pc/lwjgl/Keyboard.h"

#if PLATFORM_PS2
#include "ps2/input/Ps2PadKeyCodes.h"
#include "ps2/input/Ps2PadState.h"
#endif

RenderItem *LegacyCraftingScreen::itemRenderer = new RenderItem();

namespace
{

struct RecipeIngredient
{
    int_t itemId = 0;
    int_t count = 0;
    int_t itemDamage = -1;
};

struct RecipeVariant
{
    const char *name = nullptr;
    int_t resultId = 0;
    int_t resultCount = 1;
    int_t resultDamage = 0;
    bool requiresWorkbench = false;
    int_t gridWidth = 0;
    int_t gridHeight = 0;
    int_t gridItemIds[9] = {0};
    int_t gridItemDamage[9] = {0};
    int_t ingredientCount = 0;
    RecipeIngredient ingredients[6];

    ItemStack *resultStack = nullptr;
    ItemStack *gridStacks[9] = {nullptr};
};

struct RecipeGroup
{
    int_t variantCount = 0;
    RecipeVariant variants[8];
};

struct RecipeCategory
{
    const char *name = nullptr;
    int_t iconItemId = 0;
    int_t iconDamage = 0;
    int_t groupCount = 0;
    RecipeGroup groups[20];
    ItemStack *iconStack = nullptr;
};

static RecipeCategory s_categories[4];
static RecipeCategory s_categories2x2[4];
static bool s_recipesInitialized = false;

inline const RecipeCategory *getCategoriesTable(bool is2x2)
{
    return is2x2 ? s_categories2x2 : s_categories;
}

inline int_t bId(Block *b, int_t fallback)
{
    return b != nullptr ? b->blockID : fallback;
}

inline int_t iId(Item *it, int_t fallback)
{
    return it != nullptr ? it->shiftedIndex : fallback;
}

void initStaticRecipes()
{
    if (s_recipesInitialized)
        return;

    const int_t ID_STONE         = bId(Block::stone, 1);
    const int_t ID_COBBLE        = bId(Block::cobblestone, 4);
    const int_t ID_PLANKS        = bId(Block::planks, 5);
    const int_t ID_SAND          = bId(Block::sand, 12);
    const int_t ID_WOOD          = bId(Block::wood, 17);
    const int_t ID_GLASS         = bId(Block::glass, 20);
    const int_t ID_DISPENSER     = bId(Block::dispenser, 23);
    const int_t ID_SANDSTONE     = bId(Block::sandStone, 24);
    const int_t ID_NOTEBLOCK     = bId(Block::musicBlock, 25);
    const int_t ID_RAIL_POWERED  = bId(Block::railPowered, 27);
    const int_t ID_RAIL_DETECTOR = bId(Block::railDetector, 28);
    const int_t ID_STICKY_PISTON = bId(Block::pistonStickyBase, 29);
    const int_t ID_PISTON        = bId(Block::pistonBase, 33);
    const int_t ID_WOOL          = bId(Block::cloth, 35);
    const int_t ID_BROWN_MUSH    = bId(Block::mushroomBrown, 39);
    const int_t ID_RED_MUSH      = bId(Block::mushroomRed, 40);
    const int_t ID_GOLD_BLOCK    = bId(Block::blockGold, 41);
    const int_t ID_IRON_BLOCK    = bId(Block::blockSteel, 42);
    const int_t ID_SLAB          = bId(Block::stairSingle, 44);
    const int_t ID_BRICK_BLOCK   = bId(Block::brick, 45);
    const int_t ID_TNT           = bId(Block::tnt, 46);
    const int_t ID_BOOKSHELF     = bId(Block::bookShelf, 47);
    const int_t ID_TORCH         = bId(Block::torchWood, 50);
    const int_t ID_STAIR_WOOD    = bId(Block::stairCompactPlanks, 53);
    const int_t ID_CHEST         = bId(Block::chest, 54);
    const int_t ID_DIAMOND_BLOCK = bId(Block::blockDiamond, 57);
    const int_t ID_WORKBENCH     = bId(Block::workbench, 58);
    const int_t ID_FURNACE       = bId(Block::stoneOvenIdle, 61);
    const int_t ID_LADDER        = bId(Block::ladder, 65);
    const int_t ID_RAIL          = bId(Block::rail, 66);
    const int_t ID_STAIR_COBBLE  = bId(Block::stairCompactCobblestone, 67);
    const int_t ID_LEVER         = bId(Block::lever, 69);
    const int_t ID_PLATE_STONE   = bId(Block::pressurePlateStone, 70);
    const int_t ID_PLATE_WOOD    = bId(Block::pressurePlatePlanks, 72);
    const int_t ID_RED_TORCH     = bId(Block::torchRedstoneActive, 76);
    const int_t ID_BUTTON        = bId(Block::button, 77);
    const int_t ID_SNOW_BLOCK    = bId(Block::blockSnow, 80);
    const int_t ID_CLAY_BLOCK    = bId(Block::blockClay, 82);
    const int_t ID_JUKEBOX       = bId(Block::jukebox, 84);
    const int_t ID_FENCE         = bId(Block::fence, 85);
    const int_t ID_GLOWSTONE     = bId(Block::glowStone, 89);
    const int_t ID_TRAPDOOR      = bId(Block::trapdoor, 96);
    const int_t ID_STONE_BRICK   = bId(Block::stoneBrick, 98);
    const int_t ID_IRON_BARS     = bId(Block::fenceIron, 101);
    const int_t ID_GLASS_PANE    = bId(Block::thinGlass, 102);
    const int_t ID_FENCE_GATE    = bId(Block::fenceGate, 107);
    const int_t ID_STAIR_BRICK   = bId(Block::stairsBrick, 108);
    const int_t ID_STAIR_SBRICK  = bId(Block::stairsStoneBrickSmooth, 109);
    const int_t ID_NETHER_BRICK  = bId(Block::netherBrick, 112);
    const int_t ID_NETHER_FENCE  = bId(Block::netherFence, 113);

    const int_t ID_IRON_SHOVEL   = iId(Item::shovelSteel, 256);
    const int_t ID_IRON_PICKAXE  = iId(Item::pickaxeSteel, 257);
    const int_t ID_IRON_AXE      = iId(Item::axeSteel, 258);
    const int_t ID_FLINT_STEEL   = iId(Item::flintAndSteel, 259);
    const int_t ID_APPLE_RED     = iId(Item::appleRed, 260);
    const int_t ID_BOW           = iId(Item::bow, 261);
    const int_t ID_ARROW         = iId(Item::arrow, 262);
    const int_t ID_COAL          = iId(Item::coal, 263);
    const int_t ID_DIAMOND       = iId(Item::diamond, 264);
    const int_t ID_IRON_INGOT    = iId(Item::ingotIron, 265);
    const int_t ID_GOLD_INGOT    = iId(Item::ingotGold, 266);
    const int_t ID_IRON_SWORD    = iId(Item::swordSteel, 267);
    const int_t ID_WOOD_SWORD    = iId(Item::swordWood, 268);
    const int_t ID_WOOD_SHOVEL   = iId(Item::shovelWood, 269);
    const int_t ID_WOOD_PICKAXE  = iId(Item::pickaxeWood, 270);
    const int_t ID_WOOD_AXE      = iId(Item::axeWood, 271);
    const int_t ID_STONE_SWORD   = iId(Item::swordStone, 272);
    const int_t ID_STONE_SHOVEL  = iId(Item::shovelStone, 273);
    const int_t ID_STONE_PICKAXE = iId(Item::pickaxeStone, 274);
    const int_t ID_STONE_AXE     = iId(Item::axeStone, 275);
    const int_t ID_DIAM_SWORD    = iId(Item::swordDiamond, 276);
    const int_t ID_DIAM_SHOVEL   = iId(Item::shovelDiamond, 277);
    const int_t ID_DIAM_PICKAXE  = iId(Item::pickaxeDiamond, 278);
    const int_t ID_DIAM_AXE      = iId(Item::axeDiamond, 279);
    const int_t ID_STICK         = iId(Item::stick, 280);
    const int_t ID_BOWL          = iId(Item::bowlEmpty, 281);
    const int_t ID_STEW          = iId(Item::bowlSoup, 282);
    const int_t ID_GOLD_SWORD    = iId(Item::swordGold, 283);
    const int_t ID_GOLD_SHOVEL   = iId(Item::shovelGold, 284);
    const int_t ID_GOLD_PICKAXE  = iId(Item::pickaxeGold, 285);
    const int_t ID_GOLD_AXE      = iId(Item::axeGold, 286);
    const int_t ID_STRING        = iId(Item::silk, 287);
    const int_t ID_FEATHER       = iId(Item::feather, 288);
    const int_t ID_GUNPOWDER     = iId(Item::gunpowder, 289);
    const int_t ID_WOOD_HOE      = iId(Item::hoeWood, 290);
    const int_t ID_STONE_HOE     = iId(Item::hoeStone, 291);
    const int_t ID_IRON_HOE      = iId(Item::hoeSteel, 292);
    const int_t ID_DIAM_HOE      = iId(Item::hoeDiamond, 293);
    const int_t ID_GOLD_HOE      = iId(Item::hoeGold, 294);
    const int_t ID_WHEAT         = iId(Item::wheat, 296);
    const int_t ID_BREAD         = iId(Item::bread, 297);
    const int_t ID_LEATH_HELMET  = iId(Item::helmetLeather, 298);
    const int_t ID_LEATH_CHEST   = iId(Item::plateLeather, 299);
    const int_t ID_LEATH_LEGS    = iId(Item::legsLeather, 300);
    const int_t ID_LEATH_BOOTS   = iId(Item::bootsLeather, 301);
    const int_t ID_IRON_HELMET   = iId(Item::helmetSteel, 306);
    const int_t ID_IRON_CHEST    = iId(Item::plateSteel, 307);
    const int_t ID_IRON_LEGS     = iId(Item::legsSteel, 308);
    const int_t ID_IRON_BOOTS    = iId(Item::bootsSteel, 309);
    const int_t ID_DIAM_HELMET   = iId(Item::helmetDiamond, 310);
    const int_t ID_DIAM_CHEST    = iId(Item::plateDiamond, 311);
    const int_t ID_DIAM_LEGS     = iId(Item::legsDiamond, 312);
    const int_t ID_DIAM_BOOTS    = iId(Item::bootsDiamond, 313);
    const int_t ID_GOLD_HELMET   = iId(Item::helmetGold, 314);
    const int_t ID_GOLD_CHEST    = iId(Item::plateGold, 315);
    const int_t ID_GOLD_LEGS     = iId(Item::legsGold, 316);
    const int_t ID_GOLD_BOOTS    = iId(Item::bootsGold, 317);
    const int_t ID_FLINT         = iId(Item::flint, 318);
    const int_t ID_PAINTING      = iId(Item::painting, 321);
    const int_t ID_GOLD_APPLE    = iId(Item::appleGold, 322);
    const int_t ID_SIGN          = iId(Item::sign, 323);
    const int_t ID_WOOD_DOOR     = iId(Item::doorWood, 324);
    const int_t ID_BUCKET        = iId(Item::bucketEmpty, 325);
    const int_t ID_MINECART      = iId(Item::minecartEmpty, 328);
    const int_t ID_IRON_DOOR     = iId(Item::doorSteel, 330);
    const int_t ID_REDSTONE      = iId(Item::redstone, 331);
    const int_t ID_SNOWBALL      = iId(Item::snowball, 332);
    const int_t ID_BOAT          = iId(Item::boat, 333);
    const int_t ID_LEATHER       = iId(Item::leather, 334);
    const int_t ID_MILK          = iId(Item::bucketMilk, 335);
    const int_t ID_BRICK_ITEM    = iId(Item::brick, 336);
    const int_t ID_CLAY_BALL     = iId(Item::clay, 337);
    const int_t ID_REED          = iId(Item::reed, 338);
    const int_t ID_PAPER         = iId(Item::paper, 339);
    const int_t ID_BOOK          = iId(Item::book, 340);
    const int_t ID_SLIMEBALL     = iId(Item::slimeBall, 341);
    const int_t ID_CART_CHEST    = iId(Item::minecartCrate, 342);
    const int_t ID_CART_FURNACE  = iId(Item::minecartPowered, 343);
    const int_t ID_EGG           = iId(Item::egg, 344);
    const int_t ID_COMPASS       = iId(Item::compass, 345);
    const int_t ID_FISHING_ROD   = iId(Item::fishingRod, 346);
    const int_t ID_CLOCK         = iId(Item::pocketSundial, 347);
    const int_t ID_GLOW_DUST     = iId(Item::lightStoneDust, 348);
    const int_t ID_SUGAR         = iId(Item::sugar, 353);
    const int_t ID_CAKE          = iId(Item::cake, 354);
    const int_t ID_BED           = iId(Item::bed, 355);
    const int_t ID_REPEATER      = iId(Item::redstoneRepeater, 356);
    const int_t ID_MAP           = iId(Item::mapItem, 358);
    const int_t ID_SHEARS        = iId(Item::shears, 359);
    const int_t ID_GOLD_NUGGET   = iId(Item::goldNugget, 371);

    // ==========================================
    // TAB 0: Structures / Blocks
    // ==========================================
    RecipeCategory &cat0 = s_categories[0];
    cat0.name = "Structures";
    cat0.iconItemId = ID_PLANKS;
    cat0.iconDamage = 0;
    cat0.groupCount = 15;

    // G0: Planks (4 variants)
    {
        RecipeGroup &g = cat0.groups[0];
        g.variantCount = 4;
        const char *names[4] = {"Oak Planks", "Spruce Planks", "Birch Planks", "Jungle Planks"};
        for (int i = 0; i < 4; ++i)
        {
            RecipeVariant &v = g.variants[i];
            v.name = names[i];
            v.resultId = ID_PLANKS;
            v.resultCount = 4;
            v.resultDamage = i;
            v.requiresWorkbench = false;
            v.gridWidth = 1; v.gridHeight = 1;
            v.gridItemIds[0] = ID_WOOD; v.gridItemDamage[0] = i;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_WOOD, 1, i};
        }
    }

    // G1: Sticks (1 variant)
    {
        RecipeGroup &g = cat0.groups[1];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Sticks";
        v.resultId = ID_STICK;
        v.resultCount = 4;
        v.requiresWorkbench = false;
        v.gridWidth = 1; v.gridHeight = 2;
        v.gridItemIds[0] = ID_PLANKS; v.gridItemIds[1] = ID_PLANKS;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_PLANKS, 2, -1};
    }

    // G2: Crafting Table (1 variant)
    {
        RecipeGroup &g = cat0.groups[2];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Crafting Table";
        v.resultId = ID_WORKBENCH;
        v.resultCount = 1;
        v.requiresWorkbench = false;
        v.gridWidth = 2; v.gridHeight = 2;
        v.gridItemIds[0] = ID_PLANKS; v.gridItemIds[1] = ID_PLANKS;
        v.gridItemIds[2] = ID_PLANKS; v.gridItemIds[3] = ID_PLANKS;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_PLANKS, 4, -1};
    }

    // G3: Chest (1 variant)
    {
        RecipeGroup &g = cat0.groups[3];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Chest";
        v.resultId = ID_CHEST;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        for (int i = 0; i < 9; ++i) v.gridItemIds[i] = (i == 4) ? 0 : ID_PLANKS;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_PLANKS, 8, -1};
    }

    // G4: Furnace (1 variant)
    {
        RecipeGroup &g = cat0.groups[4];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Furnace";
        v.resultId = ID_FURNACE;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        for (int i = 0; i < 9; ++i) v.gridItemIds[i] = (i == 4) ? 0 : ID_COBBLE;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_COBBLE, 8, -1};
    }

    // G5: Slabs (6 variants)
    {
        RecipeGroup &g = cat0.groups[5];
        g.variantCount = 6;
        const char *names[6] = {"Stone Slab", "Sandstone Slab", "Wooden Slab", "Cobblestone Slab", "Brick Slab", "Stone Brick Slab"};
        const int_t mats[6] = {ID_STONE, ID_SANDSTONE, ID_PLANKS, ID_COBBLE, ID_BRICK_BLOCK, ID_STONE_BRICK};
        for (int i = 0; i < 6; ++i)
        {
            RecipeVariant &v = g.variants[i];
            v.name = names[i];
            v.resultId = ID_SLAB;
            v.resultCount = 6;
            v.resultDamage = i;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 1;
            v.gridItemIds[0] = mats[i]; v.gridItemIds[1] = mats[i]; v.gridItemIds[2] = mats[i];
            v.ingredientCount = 1;
            v.ingredients[0] = {mats[i], 3, -1};
        }
    }

    // G6: Stairs (4 variants)
    {
        RecipeGroup &g = cat0.groups[6];
        g.variantCount = 4;
        const char *names[4] = {"Wooden Stairs", "Cobblestone Stairs", "Brick Stairs", "Stone Brick Stairs"};
        const int_t res[4] = {ID_STAIR_WOOD, ID_STAIR_COBBLE, ID_STAIR_BRICK, ID_STAIR_SBRICK};
        const int_t mats[4] = {ID_PLANKS, ID_COBBLE, ID_BRICK_BLOCK, ID_STONE_BRICK};
        for (int i = 0; i < 4; ++i)
        {
            RecipeVariant &v = g.variants[i];
            v.name = names[i];
            v.resultId = res[i];
            v.resultCount = 4;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            v.gridItemIds[0] = mats[i];
            v.gridItemIds[3] = mats[i]; v.gridItemIds[4] = mats[i];
            v.gridItemIds[6] = mats[i]; v.gridItemIds[7] = mats[i]; v.gridItemIds[8] = mats[i];
            v.ingredientCount = 1;
            v.ingredients[0] = {mats[i], 6, -1};
        }
    }

    // G7: Doors & Trapdoors (3 variants)
    {
        RecipeGroup &g = cat0.groups[7];
        g.variantCount = 3;
        // Wooden Door
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Wooden Door";
            v.resultId = ID_WOOD_DOOR;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 2; v.gridHeight = 3;
            v.gridItemIds[0] = ID_PLANKS; v.gridItemIds[1] = ID_PLANKS;
            v.gridItemIds[3] = ID_PLANKS; v.gridItemIds[4] = ID_PLANKS;
            v.gridItemIds[6] = ID_PLANKS; v.gridItemIds[7] = ID_PLANKS;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_PLANKS, 6, -1};
        }
        // Iron Door
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Iron Door";
            v.resultId = ID_IRON_DOOR;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 2; v.gridHeight = 3;
            v.gridItemIds[0] = ID_IRON_INGOT; v.gridItemIds[1] = ID_IRON_INGOT;
            v.gridItemIds[3] = ID_IRON_INGOT; v.gridItemIds[4] = ID_IRON_INGOT;
            v.gridItemIds[6] = ID_IRON_INGOT; v.gridItemIds[7] = ID_IRON_INGOT;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_IRON_INGOT, 6, -1};
        }
        // Trapdoor
        {
            RecipeVariant &v = g.variants[2];
            v.name = "Trapdoor";
            v.resultId = ID_TRAPDOOR;
            v.resultCount = 2;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 2;
            for (int i = 0; i < 6; ++i) v.gridItemIds[i] = ID_PLANKS;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_PLANKS, 6, -1};
        }
    }

    // G8: Fences (3 variants)
    {
        RecipeGroup &g = cat0.groups[8];
        g.variantCount = 3;
        // Wood Fence
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Oak Fence";
            v.resultId = ID_FENCE;
            v.resultCount = 2;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 2;
            for (int i = 0; i < 6; ++i) v.gridItemIds[i] = ID_STICK;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_STICK, 6, -1};
        }
        // Nether Fence
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Nether Brick Fence";
            v.resultId = ID_NETHER_FENCE;
            v.resultCount = 6;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 2;
            for (int i = 0; i < 6; ++i) v.gridItemIds[i] = ID_NETHER_BRICK;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_NETHER_BRICK, 6, -1};
        }
        // Fence Gate
        {
            RecipeVariant &v = g.variants[2];
            v.name = "Fence Gate";
            v.resultId = ID_FENCE_GATE;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 2;
            v.gridItemIds[0] = ID_STICK; v.gridItemIds[1] = ID_PLANKS; v.gridItemIds[2] = ID_STICK;
            v.gridItemIds[3] = ID_STICK; v.gridItemIds[4] = ID_PLANKS; v.gridItemIds[5] = ID_STICK;
            v.ingredientCount = 2;
            v.ingredients[0] = {ID_STICK, 4, -1};
            v.ingredients[1] = {ID_PLANKS, 2, -1};
        }
    }

    // G9: Torches (2 variants)
    {
        RecipeGroup &g = cat0.groups[9];
        g.variantCount = 2;
        const char *names[2] = {"Torch (Coal)", "Torch (Charcoal)"};
        for (int i = 0; i < 2; ++i)
        {
            RecipeVariant &v = g.variants[i];
            v.name = names[i];
            v.resultId = ID_TORCH;
            v.resultCount = 4;
            v.requiresWorkbench = false;
            v.gridWidth = 1; v.gridHeight = 2;
            v.gridItemIds[0] = ID_COAL; v.gridItemDamage[0] = i;
            v.gridItemIds[1] = ID_STICK;
            v.ingredientCount = 2;
            v.ingredients[0] = {ID_COAL, 1, i};
            v.ingredients[1] = {ID_STICK, 1, -1};
        }
    }

    // G10: Ladder (1 variant)
    {
        RecipeGroup &g = cat0.groups[10];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Ladder";
        v.resultId = ID_LADDER;
        v.resultCount = 3;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        v.gridItemIds[0] = ID_STICK; v.gridItemIds[2] = ID_STICK;
        v.gridItemIds[3] = ID_STICK; v.gridItemIds[4] = ID_STICK; v.gridItemIds[5] = ID_STICK;
        v.gridItemIds[6] = ID_STICK; v.gridItemIds[8] = ID_STICK;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_STICK, 7, -1};
    }

    // G11: Glass Pane & Iron Bars (2 variants)
    {
        RecipeGroup &g = cat0.groups[11];
        g.variantCount = 2;
        // Glass Pane
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Glass Pane";
            v.resultId = ID_GLASS_PANE;
            v.resultCount = 16;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 2;
            for (int i = 0; i < 6; ++i) v.gridItemIds[i] = ID_GLASS;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_GLASS, 6, -1};
        }
        // Iron Bars
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Iron Bars";
            v.resultId = ID_IRON_BARS;
            v.resultCount = 16;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 2;
            for (int i = 0; i < 6; ++i) v.gridItemIds[i] = ID_IRON_INGOT;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_IRON_INGOT, 6, -1};
        }
    }

    // G12: Bookshelf (1 variant)
    {
        RecipeGroup &g = cat0.groups[12];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Bookshelf";
        v.resultId = ID_BOOKSHELF;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        for (int i = 0; i < 3; ++i) v.gridItemIds[i] = ID_PLANKS;
        for (int i = 3; i < 6; ++i) v.gridItemIds[i] = ID_BOOK;
        for (int i = 6; i < 9; ++i) v.gridItemIds[i] = ID_PLANKS;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_PLANKS, 6, -1};
        v.ingredients[1] = {ID_BOOK, 3, -1};
    }

    // G13: Sandstone & Stone Bricks (3 variants)
    {
        RecipeGroup &g = cat0.groups[13];
        g.variantCount = 3;
        // Sandstone
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Sandstone";
            v.resultId = ID_SANDSTONE;
            v.resultCount = 1;
            v.requiresWorkbench = false;
            v.gridWidth = 2; v.gridHeight = 2;
            for (int i = 0; i < 4; ++i) v.gridItemIds[i] = ID_SAND;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_SAND, 4, -1};
        }
        // Smooth Sandstone
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Smooth Sandstone";
            v.resultId = ID_SANDSTONE;
            v.resultCount = 4;
            v.resultDamage = 2;
            v.requiresWorkbench = false;
            v.gridWidth = 2; v.gridHeight = 2;
            for (int i = 0; i < 4; ++i) v.gridItemIds[i] = ID_SANDSTONE;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_SANDSTONE, 4, 0};
        }
        // Stone Bricks
        {
            RecipeVariant &v = g.variants[2];
            v.name = "Stone Bricks";
            v.resultId = ID_STONE_BRICK;
            v.resultCount = 4;
            v.requiresWorkbench = false;
            v.gridWidth = 2; v.gridHeight = 2;
            for (int i = 0; i < 4; ++i) v.gridItemIds[i] = ID_STONE;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_STONE, 4, -1};
        }
    }

    // G14: Wool (1 variant)
    {
        RecipeGroup &g = cat0.groups[14];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Wool";
        v.resultId = ID_WOOL;
        v.resultCount = 1;
        v.requiresWorkbench = false;
        v.gridWidth = 2; v.gridHeight = 2;
        for (int i = 0; i < 4; ++i) v.gridItemIds[i] = ID_STRING;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_STRING, 4, -1};
    }

    // ==========================================
    // TAB 1: Tools, Weapons & Armor
    // ==========================================
    RecipeCategory &cat1 = s_categories[1];
    cat1.name = "Tools & Weapons";
    cat1.iconItemId = ID_IRON_PICKAXE;
    cat1.iconDamage = 0;
    cat1.groupCount = 13;

    auto setup5TierTool = [&](RecipeGroup &g, const char *toolNames[5], const int_t resIds[5],
                              int_t s0, int_t s1, int_t s2, int_t s3, int_t s4, int_t s5, int_t s6, int_t s7, int_t s8,
                              int_t matCount)
    {
        g.variantCount = 5;
        const int_t mats[5] = {ID_PLANKS, ID_COBBLE, ID_IRON_INGOT, ID_DIAMOND, ID_GOLD_INGOT};
        for (int i = 0; i < 5; ++i)
        {
            RecipeVariant &v = g.variants[i];
            v.name = toolNames[i];
            v.resultId = resIds[i];
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            const int_t templateGrid[9] = {s0, s1, s2, s3, s4, s5, s6, s7, s8};
            for (int k = 0; k < 9; ++k)
            {
                if (templateGrid[k] == 1) v.gridItemIds[k] = mats[i];
                else if (templateGrid[k] == 2) v.gridItemIds[k] = ID_STICK;
                else v.gridItemIds[k] = 0;
            }
            v.ingredientCount = 2;
            v.ingredients[0] = {mats[i], matCount, -1};
            v.ingredients[1] = {ID_STICK, (s7 == 2 && s4 == 2) ? 2 : 1, -1};
        }
    };

    // G0: Pickaxes
    {
        const char *names[5] = {"Wooden Pickaxe", "Stone Pickaxe", "Iron Pickaxe", "Diamond Pickaxe", "Golden Pickaxe"};
        const int_t res[5] = {ID_WOOD_PICKAXE, ID_STONE_PICKAXE, ID_IRON_PICKAXE, ID_DIAM_PICKAXE, ID_GOLD_PICKAXE};
        setup5TierTool(cat1.groups[0], names, res, 1, 1, 1, 0, 2, 0, 0, 2, 0, 3);
    }
    // G1: Shovels
    {
        const char *names[5] = {"Wooden Shovel", "Stone Shovel", "Iron Shovel", "Diamond Shovel", "Golden Shovel"};
        const int_t res[5] = {ID_WOOD_SHOVEL, ID_STONE_SHOVEL, ID_IRON_SHOVEL, ID_DIAM_SHOVEL, ID_GOLD_SHOVEL};
        setup5TierTool(cat1.groups[1], names, res, 0, 1, 0, 0, 2, 0, 0, 2, 0, 1);
    }
    // G2: Axes
    {
        const char *names[5] = {"Wooden Axe", "Stone Axe", "Iron Axe", "Diamond Axe", "Golden Axe"};
        const int_t res[5] = {ID_WOOD_AXE, ID_STONE_AXE, ID_IRON_AXE, ID_DIAM_AXE, ID_GOLD_AXE};
        setup5TierTool(cat1.groups[2], names, res, 1, 1, 0, 1, 2, 0, 0, 2, 0, 3);
    }
    // G3: Hoes
    {
        const char *names[5] = {"Wooden Hoe", "Stone Hoe", "Iron Hoe", "Diamond Hoe", "Golden Hoe"};
        const int_t res[5] = {ID_WOOD_HOE, ID_STONE_HOE, ID_IRON_HOE, ID_DIAM_HOE, ID_GOLD_HOE};
        setup5TierTool(cat1.groups[3], names, res, 1, 1, 0, 0, 2, 0, 0, 2, 0, 2);
    }
    // G4: Swords
    {
        const char *names[5] = {"Wooden Sword", "Stone Sword", "Iron Sword", "Diamond Sword", "Golden Sword"};
        const int_t res[5] = {ID_WOOD_SWORD, ID_STONE_SWORD, ID_IRON_SWORD, ID_DIAM_SWORD, ID_GOLD_SWORD};
        setup5TierTool(cat1.groups[4], names, res, 0, 1, 0, 0, 1, 0, 0, 2, 0, 2);
    }

    // G5: Bow & Arrows (2 variants)
    {
        RecipeGroup &g = cat1.groups[5];
        g.variantCount = 2;
        // Bow
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Bow";
            v.resultId = ID_BOW;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            v.gridItemIds[0] = 0;        v.gridItemIds[1] = ID_STICK; v.gridItemIds[2] = ID_STRING;
            v.gridItemIds[3] = ID_STICK; v.gridItemIds[4] = 0;        v.gridItemIds[5] = ID_STRING;
            v.gridItemIds[6] = 0;        v.gridItemIds[7] = ID_STICK; v.gridItemIds[8] = ID_STRING;
            v.ingredientCount = 2;
            v.ingredients[0] = {ID_STICK, 3, -1};
            v.ingredients[1] = {ID_STRING, 3, -1};
        }
        // Arrows
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Arrows";
            v.resultId = ID_ARROW;
            v.resultCount = 4;
            v.requiresWorkbench = true;
            v.gridWidth = 1; v.gridHeight = 3;
            v.gridItemIds[0] = ID_FLINT;
            v.gridItemIds[1] = ID_STICK;
            v.gridItemIds[2] = ID_FEATHER;
            v.ingredientCount = 3;
            v.ingredients[0] = {ID_FLINT, 1, -1};
            v.ingredients[1] = {ID_STICK, 1, -1};
            v.ingredients[2] = {ID_FEATHER, 1, -1};
        }
    }

    auto setupArmor = [&](RecipeGroup &g, const char *names[4], const int_t resIds[4],
                          int_t s0, int_t s1, int_t s2, int_t s3, int_t s4, int_t s5, int_t s6, int_t s7, int_t s8,
                          int_t matCount)
    {
        g.variantCount = 4;
        const int_t mats[4] = {ID_LEATHER, ID_IRON_INGOT, ID_DIAMOND, ID_GOLD_INGOT};
        for (int i = 0; i < 4; ++i)
        {
            RecipeVariant &v = g.variants[i];
            v.name = names[i];
            v.resultId = resIds[i];
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            const int_t templateGrid[9] = {s0, s1, s2, s3, s4, s5, s6, s7, s8};
            for (int k = 0; k < 9; ++k)
                v.gridItemIds[k] = (templateGrid[k] == 1) ? mats[i] : 0;
            v.ingredientCount = 1;
            v.ingredients[0] = {mats[i], matCount, -1};
        }
    };

    // G6: Helmets
    {
        const char *names[4] = {"Leather Cap", "Iron Helmet", "Diamond Helmet", "Golden Helmet"};
        const int_t res[4] = {ID_LEATH_HELMET, ID_IRON_HELMET, ID_DIAM_HELMET, ID_GOLD_HELMET};
        setupArmor(cat1.groups[6], names, res, 1, 1, 1, 1, 0, 1, 0, 0, 0, 5);
    }
    // G7: Chestplates
    {
        const char *names[4] = {"Leather Tunic", "Iron Chestplate", "Diamond Chestplate", "Golden Chestplate"};
        const int_t res[4] = {ID_LEATH_CHEST, ID_IRON_CHEST, ID_DIAM_CHEST, ID_GOLD_CHEST};
        setupArmor(cat1.groups[7], names, res, 1, 0, 1, 1, 1, 1, 1, 1, 1, 8);
    }
    // G8: Leggings
    {
        const char *names[4] = {"Leather Pants", "Iron Leggings", "Diamond Leggings", "Golden Leggings"};
        const int_t res[4] = {ID_LEATH_LEGS, ID_IRON_LEGS, ID_DIAM_LEGS, ID_GOLD_LEGS};
        setupArmor(cat1.groups[8], names, res, 1, 1, 1, 1, 0, 1, 1, 0, 1, 7);
    }
    // G9: Boots
    {
        const char *names[4] = {"Leather Boots", "Iron Boots", "Diamond Boots", "Golden Boots"};
        const int_t res[4] = {ID_LEATH_BOOTS, ID_IRON_BOOTS, ID_DIAM_BOOTS, ID_GOLD_BOOTS};
        setupArmor(cat1.groups[9], names, res, 0, 0, 0, 1, 0, 1, 1, 0, 1, 4);
    }

    // G10: Flint and Steel (1 variant)
    {
        RecipeGroup &g = cat1.groups[10];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Flint and Steel";
        v.resultId = ID_FLINT_STEEL;
        v.resultCount = 1;
        v.requiresWorkbench = false;
        v.gridWidth = 2; v.gridHeight = 2;
        v.gridItemIds[0] = ID_IRON_INGOT; v.gridItemIds[3] = ID_FLINT;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_IRON_INGOT, 1, -1};
        v.ingredients[1] = {ID_FLINT, 1, -1};
    }

    // G11: Shears (1 variant)
    {
        RecipeGroup &g = cat1.groups[11];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Shears";
        v.resultId = ID_SHEARS;
        v.resultCount = 1;
        v.requiresWorkbench = false;
        v.gridWidth = 2; v.gridHeight = 2;
        v.gridItemIds[0] = ID_IRON_INGOT; v.gridItemIds[3] = ID_IRON_INGOT;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_IRON_INGOT, 2, -1};
    }

    // G12: Fishing Rod (1 variant)
    {
        RecipeGroup &g = cat1.groups[12];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Fishing Rod";
        v.resultId = ID_FISHING_ROD;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        v.gridItemIds[2] = ID_STICK;
        v.gridItemIds[4] = ID_STICK; v.gridItemIds[5] = ID_STRING;
        v.gridItemIds[6] = ID_STICK; v.gridItemIds[8] = ID_STRING;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_STICK, 3, -1};
        v.ingredients[1] = {ID_STRING, 2, -1};
    }

    // ==========================================
    // TAB 2: Food & Mechanism
    // ==========================================
    RecipeCategory &cat2 = s_categories[2];
    cat2.name = "Food & Mechanisms";
    cat2.iconItemId = ID_BREAD;
    cat2.iconDamage = 0;
    cat2.groupCount = 14;

    // G0: Bread
    {
        RecipeGroup &g = cat2.groups[0];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Bread";
        v.resultId = ID_BREAD;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 1;
        v.gridItemIds[0] = ID_WHEAT; v.gridItemIds[1] = ID_WHEAT; v.gridItemIds[2] = ID_WHEAT;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_WHEAT, 3, -1};
    }
    // G1: Cake
    {
        RecipeGroup &g = cat2.groups[1];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Cake";
        v.resultId = ID_CAKE;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        v.gridItemIds[0] = ID_MILK;  v.gridItemIds[1] = ID_MILK;  v.gridItemIds[2] = ID_MILK;
        v.gridItemIds[3] = ID_SUGAR; v.gridItemIds[4] = ID_EGG;   v.gridItemIds[5] = ID_SUGAR;
        v.gridItemIds[6] = ID_WHEAT; v.gridItemIds[7] = ID_WHEAT; v.gridItemIds[8] = ID_WHEAT;
        v.ingredientCount = 4;
        v.ingredients[0] = {ID_MILK, 3, -1};
        v.ingredients[1] = {ID_SUGAR, 2, -1};
        v.ingredients[2] = {ID_EGG, 1, -1};
        v.ingredients[3] = {ID_WHEAT, 3, -1};
    }
    // G2: Golden Apple
    {
        RecipeGroup &g = cat2.groups[2];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Golden Apple";
        v.resultId = ID_GOLD_APPLE;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        for (int i = 0; i < 9; ++i) v.gridItemIds[i] = (i == 4) ? ID_APPLE_RED : ID_GOLD_NUGGET;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_APPLE_RED, 1, -1};
        v.ingredients[1] = {ID_GOLD_NUGGET, 8, -1};
    }
    // G3: Bowl & Mushroom Stew
    {
        RecipeGroup &g = cat2.groups[3];
        g.variantCount = 2;
        // Bowl
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Bowl";
            v.resultId = ID_BOWL;
            v.resultCount = 4;
            v.requiresWorkbench = false;
            v.gridWidth = 3; v.gridHeight = 2;
            v.gridItemIds[0] = ID_PLANKS; v.gridItemIds[2] = ID_PLANKS;
            v.gridItemIds[4] = ID_PLANKS;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_PLANKS, 3, -1};
        }
        // Stew
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Mushroom Stew";
            v.resultId = ID_STEW;
            v.resultCount = 1;
            v.requiresWorkbench = false;
            v.gridWidth = 2; v.gridHeight = 2;
            v.gridItemIds[0] = ID_RED_MUSH;   v.gridItemIds[1] = ID_BROWN_MUSH;
            v.gridItemIds[2] = ID_BOWL;
            v.ingredientCount = 3;
            v.ingredients[0] = {ID_BOWL, 1, -1};
            v.ingredients[1] = {ID_RED_MUSH, 1, -1};
            v.ingredients[2] = {ID_BROWN_MUSH, 1, -1};
        }
    }
    // G4: Sugar
    {
        RecipeGroup &g = cat2.groups[4];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Sugar";
        v.resultId = ID_SUGAR;
        v.resultCount = 1;
        v.requiresWorkbench = false;
        v.gridWidth = 1; v.gridHeight = 1;
        v.gridItemIds[0] = ID_REED;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_REED, 1, -1};
    }
    // G5: Pressure Plates (2 variants)
    {
        RecipeGroup &g = cat2.groups[5];
        g.variantCount = 2;
        // Stone Plate
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Stone Pressure Plate";
            v.resultId = ID_PLATE_STONE;
            v.resultCount = 1;
            v.requiresWorkbench = false;
            v.gridWidth = 2; v.gridHeight = 1;
            v.gridItemIds[0] = ID_STONE; v.gridItemIds[1] = ID_STONE;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_STONE, 2, -1};
        }
        // Wood Plate
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Wooden Pressure Plate";
            v.resultId = ID_PLATE_WOOD;
            v.resultCount = 1;
            v.requiresWorkbench = false;
            v.gridWidth = 2; v.gridHeight = 1;
            v.gridItemIds[0] = ID_PLANKS; v.gridItemIds[1] = ID_PLANKS;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_PLANKS, 2, -1};
        }
    }
    // G6: Button
    {
        RecipeGroup &g = cat2.groups[6];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Button";
        v.resultId = ID_BUTTON;
        v.resultCount = 1;
        v.requiresWorkbench = false;
        v.gridWidth = 1; v.gridHeight = 2;
        v.gridItemIds[0] = ID_STONE; v.gridItemIds[1] = ID_STONE;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_STONE, 2, -1};
    }
    // G7: Lever
    {
        RecipeGroup &g = cat2.groups[7];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Lever";
        v.resultId = ID_LEVER;
        v.resultCount = 1;
        v.requiresWorkbench = false;
        v.gridWidth = 1; v.gridHeight = 2;
        v.gridItemIds[0] = ID_STICK; v.gridItemIds[1] = ID_COBBLE;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_STICK, 1, -1};
        v.ingredients[1] = {ID_COBBLE, 1, -1};
    }
    // G8: Redstone Torch
    {
        RecipeGroup &g = cat2.groups[8];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Redstone Torch";
        v.resultId = ID_RED_TORCH;
        v.resultCount = 1;
        v.requiresWorkbench = false;
        v.gridWidth = 1; v.gridHeight = 2;
        v.gridItemIds[0] = ID_REDSTONE; v.gridItemIds[1] = ID_STICK;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_REDSTONE, 1, -1};
        v.ingredients[1] = {ID_STICK, 1, -1};
    }
    // G9: Redstone Repeater
    {
        RecipeGroup &g = cat2.groups[9];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Redstone Repeater";
        v.resultId = ID_REPEATER;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 2;
        v.gridItemIds[0] = ID_RED_TORCH; v.gridItemIds[1] = ID_REDSTONE; v.gridItemIds[2] = ID_RED_TORCH;
        v.gridItemIds[3] = ID_STONE;     v.gridItemIds[4] = ID_STONE;    v.gridItemIds[5] = ID_STONE;
        v.ingredientCount = 3;
        v.ingredients[0] = {ID_RED_TORCH, 2, -1};
        v.ingredients[1] = {ID_REDSTONE, 1, -1};
        v.ingredients[2] = {ID_STONE, 3, -1};
    }
    // G10: Pistons (2 variants)
    {
        RecipeGroup &g = cat2.groups[10];
        g.variantCount = 2;
        // Piston
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Piston";
            v.resultId = ID_PISTON;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            v.gridItemIds[0] = ID_PLANKS; v.gridItemIds[1] = ID_PLANKS;     v.gridItemIds[2] = ID_PLANKS;
            v.gridItemIds[3] = ID_COBBLE; v.gridItemIds[4] = ID_IRON_INGOT; v.gridItemIds[5] = ID_COBBLE;
            v.gridItemIds[6] = ID_COBBLE; v.gridItemIds[7] = ID_REDSTONE;   v.gridItemIds[8] = ID_COBBLE;
            v.ingredientCount = 4;
            v.ingredients[0] = {ID_PLANKS, 3, -1};
            v.ingredients[1] = {ID_COBBLE, 4, -1};
            v.ingredients[2] = {ID_IRON_INGOT, 1, -1};
            v.ingredients[3] = {ID_REDSTONE, 1, -1};
        }
        // Sticky Piston
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Sticky Piston";
            v.resultId = ID_STICKY_PISTON;
            v.resultCount = 1;
            v.requiresWorkbench = false;
            v.gridWidth = 1; v.gridHeight = 2;
            v.gridItemIds[0] = ID_SLIMEBALL; v.gridItemIds[1] = ID_PISTON;
            v.ingredientCount = 2;
            v.ingredients[0] = {ID_SLIMEBALL, 1, -1};
            v.ingredients[1] = {ID_PISTON, 1, -1};
        }
    }
    // G11: Dispenser
    {
        RecipeGroup &g = cat2.groups[11];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Dispenser";
        v.resultId = ID_DISPENSER;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        v.gridItemIds[0] = ID_COBBLE; v.gridItemIds[1] = ID_COBBLE; v.gridItemIds[2] = ID_COBBLE;
        v.gridItemIds[3] = ID_COBBLE; v.gridItemIds[4] = ID_BOW;    v.gridItemIds[5] = ID_COBBLE;
        v.gridItemIds[6] = ID_COBBLE; v.gridItemIds[7] = ID_REDSTONE; v.gridItemIds[8] = ID_COBBLE;
        v.ingredientCount = 3;
        v.ingredients[0] = {ID_COBBLE, 7, -1};
        v.ingredients[1] = {ID_BOW, 1, -1};
        v.ingredients[2] = {ID_REDSTONE, 1, -1};
    }
    // G12: TNT
    {
        RecipeGroup &g = cat2.groups[12];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "TNT";
        v.resultId = ID_TNT;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        v.gridItemIds[0] = ID_GUNPOWDER; v.gridItemIds[1] = ID_SAND;      v.gridItemIds[2] = ID_GUNPOWDER;
        v.gridItemIds[3] = ID_SAND;      v.gridItemIds[4] = ID_GUNPOWDER; v.gridItemIds[5] = ID_SAND;
        v.gridItemIds[6] = ID_GUNPOWDER; v.gridItemIds[7] = ID_SAND;      v.gridItemIds[8] = ID_GUNPOWDER;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_GUNPOWDER, 5, -1};
        v.ingredients[1] = {ID_SAND, 4, -1};
    }
    // G13: Note Block & Jukebox (2 variants)
    {
        RecipeGroup &g = cat2.groups[13];
        g.variantCount = 2;
        // Note Block
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Note Block";
            v.resultId = ID_NOTEBLOCK;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            for (int i = 0; i < 9; ++i) v.gridItemIds[i] = (i == 4) ? ID_REDSTONE : ID_PLANKS;
            v.ingredientCount = 2;
            v.ingredients[0] = {ID_PLANKS, 8, -1};
            v.ingredients[1] = {ID_REDSTONE, 1, -1};
        }
        // Jukebox
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Jukebox";
            v.resultId = ID_JUKEBOX;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            for (int i = 0; i < 9; ++i) v.gridItemIds[i] = (i == 4) ? ID_DIAMOND : ID_PLANKS;
            v.ingredientCount = 2;
            v.ingredients[0] = {ID_PLANKS, 8, -1};
            v.ingredients[1] = {ID_DIAMOND, 1, -1};
        }
    }

    // ==========================================
    // TAB 3: Transport & Misc
    // ==========================================
    RecipeCategory &cat3 = s_categories[3];
    cat3.name = "Transport & Misc";
    cat3.iconItemId = ID_MINECART;
    cat3.iconDamage = 0;
    cat3.groupCount = 11;

    // G0: Minecarts (3 variants)
    {
        RecipeGroup &g = cat3.groups[0];
        g.variantCount = 3;
        // Minecart
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Minecart";
            v.resultId = ID_MINECART;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 2;
            v.gridItemIds[0] = ID_IRON_INGOT; v.gridItemIds[2] = ID_IRON_INGOT;
            v.gridItemIds[3] = ID_IRON_INGOT; v.gridItemIds[4] = ID_IRON_INGOT; v.gridItemIds[5] = ID_IRON_INGOT;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_IRON_INGOT, 5, -1};
        }
        // Powered Minecart
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Powered Minecart";
            v.resultId = ID_CART_FURNACE;
            v.resultCount = 1;
            v.requiresWorkbench = false;
            v.gridWidth = 1; v.gridHeight = 2;
            v.gridItemIds[0] = ID_FURNACE; v.gridItemIds[1] = ID_MINECART;
            v.ingredientCount = 2;
            v.ingredients[0] = {ID_FURNACE, 1, -1};
            v.ingredients[1] = {ID_MINECART, 1, -1};
        }
        // Storage Minecart
        {
            RecipeVariant &v = g.variants[2];
            v.name = "Storage Minecart";
            v.resultId = ID_CART_CHEST;
            v.resultCount = 1;
            v.requiresWorkbench = false;
            v.gridWidth = 1; v.gridHeight = 2;
            v.gridItemIds[0] = ID_CHEST; v.gridItemIds[1] = ID_MINECART;
            v.ingredientCount = 2;
            v.ingredients[0] = {ID_CHEST, 1, -1};
            v.ingredients[1] = {ID_MINECART, 1, -1};
        }
    }
    // G1: Rails (3 variants)
    {
        RecipeGroup &g = cat3.groups[1];
        g.variantCount = 3;
        // Standard Rail
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Rail";
            v.resultId = ID_RAIL;
            v.resultCount = 16;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            v.gridItemIds[0] = ID_IRON_INGOT; v.gridItemIds[2] = ID_IRON_INGOT;
            v.gridItemIds[3] = ID_IRON_INGOT; v.gridItemIds[4] = ID_STICK; v.gridItemIds[5] = ID_IRON_INGOT;
            v.gridItemIds[6] = ID_IRON_INGOT; v.gridItemIds[8] = ID_IRON_INGOT;
            v.ingredientCount = 2;
            v.ingredients[0] = {ID_IRON_INGOT, 6, -1};
            v.ingredients[1] = {ID_STICK, 1, -1};
        }
        // Powered Rail
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Powered Rail";
            v.resultId = ID_RAIL_POWERED;
            v.resultCount = 6;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            v.gridItemIds[0] = ID_GOLD_INGOT; v.gridItemIds[2] = ID_GOLD_INGOT;
            v.gridItemIds[3] = ID_GOLD_INGOT; v.gridItemIds[4] = ID_STICK; v.gridItemIds[5] = ID_GOLD_INGOT;
            v.gridItemIds[6] = ID_GOLD_INGOT; v.gridItemIds[7] = ID_REDSTONE; v.gridItemIds[8] = ID_GOLD_INGOT;
            v.ingredientCount = 3;
            v.ingredients[0] = {ID_GOLD_INGOT, 6, -1};
            v.ingredients[1] = {ID_STICK, 1, -1};
            v.ingredients[2] = {ID_REDSTONE, 1, -1};
        }
        // Detector Rail
        {
            RecipeVariant &v = g.variants[2];
            v.name = "Detector Rail";
            v.resultId = ID_RAIL_DETECTOR;
            v.resultCount = 6;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            v.gridItemIds[0] = ID_IRON_INGOT; v.gridItemIds[2] = ID_IRON_INGOT;
            v.gridItemIds[3] = ID_IRON_INGOT; v.gridItemIds[4] = ID_PLATE_STONE; v.gridItemIds[5] = ID_IRON_INGOT;
            v.gridItemIds[6] = ID_IRON_INGOT; v.gridItemIds[7] = ID_REDSTONE; v.gridItemIds[8] = ID_IRON_INGOT;
            v.ingredientCount = 3;
            v.ingredients[0] = {ID_IRON_INGOT, 6, -1};
            v.ingredients[1] = {ID_PLATE_STONE, 1, -1};
            v.ingredients[2] = {ID_REDSTONE, 1, -1};
        }
    }
    // G2: Boat
    {
        RecipeGroup &g = cat3.groups[2];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Boat";
        v.resultId = ID_BOAT;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 2;
        v.gridItemIds[0] = ID_PLANKS; v.gridItemIds[2] = ID_PLANKS;
        v.gridItemIds[3] = ID_PLANKS; v.gridItemIds[4] = ID_PLANKS; v.gridItemIds[5] = ID_PLANKS;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_PLANKS, 5, -1};
    }
    // G3: Bucket
    {
        RecipeGroup &g = cat3.groups[3];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Bucket";
        v.resultId = ID_BUCKET;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 2;
        v.gridItemIds[0] = ID_IRON_INGOT; v.gridItemIds[2] = ID_IRON_INGOT;
        v.gridItemIds[4] = ID_IRON_INGOT;
        v.ingredientCount = 1;
        v.ingredients[0] = {ID_IRON_INGOT, 3, -1};
    }
    // G4: Compass
    {
        RecipeGroup &g = cat3.groups[4];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Compass";
        v.resultId = ID_COMPASS;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        v.gridItemIds[1] = ID_IRON_INGOT;
        v.gridItemIds[3] = ID_IRON_INGOT; v.gridItemIds[4] = ID_REDSTONE; v.gridItemIds[5] = ID_IRON_INGOT;
        v.gridItemIds[7] = ID_IRON_INGOT;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_IRON_INGOT, 4, -1};
        v.ingredients[1] = {ID_REDSTONE, 1, -1};
    }
    // G5: Clock
    {
        RecipeGroup &g = cat3.groups[5];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Clock";
        v.resultId = ID_CLOCK;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        v.gridItemIds[1] = ID_GOLD_INGOT;
        v.gridItemIds[3] = ID_GOLD_INGOT; v.gridItemIds[4] = ID_REDSTONE; v.gridItemIds[5] = ID_GOLD_INGOT;
        v.gridItemIds[7] = ID_GOLD_INGOT;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_GOLD_INGOT, 4, -1};
        v.ingredients[1] = {ID_REDSTONE, 1, -1};
    }
    // G6: Map
    {
        RecipeGroup &g = cat3.groups[6];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Map";
        v.resultId = ID_MAP;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        for (int i = 0; i < 9; ++i) v.gridItemIds[i] = (i == 4) ? ID_COMPASS : ID_PAPER;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_PAPER, 8, -1};
        v.ingredients[1] = {ID_COMPASS, 1, -1};
    }
    // G7: Paper & Book (2 variants)
    {
        RecipeGroup &g = cat3.groups[7];
        g.variantCount = 2;
        // Paper
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Paper";
            v.resultId = ID_PAPER;
            v.resultCount = 3;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 1;
            v.gridItemIds[0] = ID_REED; v.gridItemIds[1] = ID_REED; v.gridItemIds[2] = ID_REED;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_REED, 3, -1};
        }
        // Book
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Book";
            v.resultId = ID_BOOK;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 1; v.gridHeight = 3;
            v.gridItemIds[0] = ID_PAPER; v.gridItemIds[1] = ID_PAPER; v.gridItemIds[2] = ID_PAPER;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_PAPER, 3, -1};
        }
    }
    // G8: Painting
    {
        RecipeGroup &g = cat3.groups[8];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Painting";
        v.resultId = ID_PAINTING;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 3;
        for (int i = 0; i < 9; ++i) v.gridItemIds[i] = (i == 4) ? ID_WOOL : ID_STICK;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_STICK, 8, -1};
        v.ingredients[1] = {ID_WOOL, 1, -1};
    }
    // G9: Bed
    {
        RecipeGroup &g = cat3.groups[9];
        g.variantCount = 1;
        RecipeVariant &v = g.variants[0];
        v.name = "Bed";
        v.resultId = ID_BED;
        v.resultCount = 1;
        v.requiresWorkbench = true;
        v.gridWidth = 3; v.gridHeight = 2;
        v.gridItemIds[0] = ID_WOOL;   v.gridItemIds[1] = ID_WOOL;   v.gridItemIds[2] = ID_WOOL;
        v.gridItemIds[3] = ID_PLANKS; v.gridItemIds[4] = ID_PLANKS; v.gridItemIds[5] = ID_PLANKS;
        v.ingredientCount = 2;
        v.ingredients[0] = {ID_WOOL, 3, -1};
        v.ingredients[1] = {ID_PLANKS, 3, -1};
    }
    // G10: Mineral Blocks & Ingot Unpacking (8 variants)
    {
        RecipeGroup &g = cat3.groups[10];
        g.variantCount = 8;
        // 0: Iron Block
        {
            RecipeVariant &v = g.variants[0];
            v.name = "Block of Iron";
            v.resultId = ID_IRON_BLOCK;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            for (int i = 0; i < 9; ++i) v.gridItemIds[i] = ID_IRON_INGOT;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_IRON_INGOT, 9, -1};
        }
        // 1: Iron Ingots (from Block)
        {
            RecipeVariant &v = g.variants[1];
            v.name = "Iron Ingots";
            v.resultId = ID_IRON_INGOT;
            v.resultCount = 9;
            v.requiresWorkbench = false;
            v.gridWidth = 1; v.gridHeight = 1;
            v.gridItemIds[0] = ID_IRON_BLOCK;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_IRON_BLOCK, 1, -1};
        }
        // 2: Gold Block
        {
            RecipeVariant &v = g.variants[2];
            v.name = "Block of Gold";
            v.resultId = ID_GOLD_BLOCK;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            for (int i = 0; i < 9; ++i) v.gridItemIds[i] = ID_GOLD_INGOT;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_GOLD_INGOT, 9, -1};
        }
        // 3: Gold Ingots (from Block)
        {
            RecipeVariant &v = g.variants[3];
            v.name = "Gold Ingots";
            v.resultId = ID_GOLD_INGOT;
            v.resultCount = 9;
            v.requiresWorkbench = false;
            v.gridWidth = 1; v.gridHeight = 1;
            v.gridItemIds[0] = ID_GOLD_BLOCK;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_GOLD_BLOCK, 1, -1};
        }
        // 4: Diamond Block
        {
            RecipeVariant &v = g.variants[4];
            v.name = "Block of Diamond";
            v.resultId = ID_DIAMOND_BLOCK;
            v.resultCount = 1;
            v.requiresWorkbench = true;
            v.gridWidth = 3; v.gridHeight = 3;
            for (int i = 0; i < 9; ++i) v.gridItemIds[i] = ID_DIAMOND;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_DIAMOND, 9, -1};
        }
        // 5: Diamonds (from Block)
        {
            RecipeVariant &v = g.variants[5];
            v.name = "Diamonds";
            v.resultId = ID_DIAMOND;
            v.resultCount = 9;
            v.requiresWorkbench = false;
            v.gridWidth = 1; v.gridHeight = 1;
            v.gridItemIds[0] = ID_DIAMOND_BLOCK;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_DIAMOND_BLOCK, 1, -1};
        }
        // 6: Glowstone
        {
            RecipeVariant &v = g.variants[6];
            v.name = "Glowstone";
            v.resultId = ID_GLOWSTONE;
            v.resultCount = 1;
            v.requiresWorkbench = false;
            v.gridWidth = 2; v.gridHeight = 2;
            for (int i = 0; i < 4; ++i) v.gridItemIds[i] = ID_GLOW_DUST;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_GLOW_DUST, 4, -1};
        }
        // 7: Clay Block
        {
            RecipeVariant &v = g.variants[7];
            v.name = "Clay Block";
            v.resultId = ID_CLAY_BLOCK;
            v.resultCount = 1;
            v.requiresWorkbench = false;
            v.gridWidth = 2; v.gridHeight = 2;
            for (int i = 0; i < 4; ++i) v.gridItemIds[i] = ID_CLAY_BALL;
            v.ingredientCount = 1;
            v.ingredients[0] = {ID_CLAY_BALL, 4, -1};
        }
    }

    // Allocate persistent ItemStack objects for zero-allocation rendering
    for (int c = 0; c < 4; ++c)
    {
        RecipeCategory &cat = s_categories[c];
        if (cat.iconItemId > 0)
            cat.iconStack = new ItemStack(cat.iconItemId, 1, cat.iconDamage);
        for (int g = 0; g < cat.groupCount; ++g)
        {
            RecipeGroup &grp = cat.groups[g];
            for (int v = 0; v < grp.variantCount; ++v)
            {
                RecipeVariant &var = grp.variants[v];
                if (var.resultId > 0)
                    var.resultStack = new ItemStack(var.resultId, var.resultCount, var.resultDamage);
                for (int s = 0; s < 9; ++s)
                {
                    if (var.gridItemIds[s] > 0)
                        var.gridStacks[s] = new ItemStack(var.gridItemIds[s], 1, var.gridItemDamage[s] == -1 ? 0 : var.gridItemDamage[s]);
                }
            }
        }
    }

    // Precalculate s_categories2x2 with strict 2x2 non-workbench recipes
    for (int c = 0; c < 4; ++c)
    {
        RecipeCategory &srcCat = s_categories[c];
        RecipeCategory &dstCat = s_categories2x2[c];
        dstCat.name = srcCat.name;
        dstCat.iconItemId = srcCat.iconItemId;
        dstCat.iconDamage = srcCat.iconDamage;
        dstCat.iconStack = srcCat.iconStack;
        dstCat.groupCount = 0;

        for (int g = 0; g < srcCat.groupCount; ++g)
        {
            const RecipeGroup &srcGrp = srcCat.groups[g];
            int_t validCount = 0;
            for (int v = 0; v < srcGrp.variantCount; ++v)
            {
                const RecipeVariant &var = srcGrp.variants[v];
                if (var.gridWidth <= 2 && var.gridHeight <= 2 && !var.requiresWorkbench)
                    validCount++;
            }

            if (validCount > 0)
            {
                RecipeGroup &dstGrp = dstCat.groups[dstCat.groupCount];
                dstGrp.variantCount = 0;
                for (int v = 0; v < srcGrp.variantCount; ++v)
                {
                    const RecipeVariant &var = srcGrp.variants[v];
                    if (var.gridWidth <= 2 && var.gridHeight <= 2 && !var.requiresWorkbench)
                    {
                        dstGrp.variants[dstGrp.variantCount++] = var;
                    }
                }
                dstCat.groupCount++;
            }
        }

        // Ensure 2x2 tab icon shows a valid 2x2 item
        if (dstCat.groupCount > 0 && dstCat.groups[0].variantCount > 0)
        {
            const RecipeVariant &firstVar = dstCat.groups[0].variants[0];
            dstCat.iconItemId = firstVar.resultId;
            dstCat.iconDamage = firstVar.resultDamage;
            dstCat.iconStack = firstVar.resultStack;
        }
    }

    s_recipesInitialized = true;
}

} // namespace

LegacyCraftingScreen::LegacyCraftingScreen(InventoryPlayer *playerInventory, World *worldObj,
                                           int_t x, int_t y, int_t z, bool is2x2, EntityPlayer *player)
    : inventory(playerInventory), world(worldObj), posX(x), posY(y), posZ(z),
      is2x2Mode(is2x2), entityPlayer(player), selectedCategory(0),
      craftHoldTicks(0), ps2ActionReleaseLatch(true),
      guiLeft(0), guiTop(0), xSize(276), ySize(188),
      ownerPlayerIndex(-1)
{
    initStaticRecipes();

    for (int i = 0; i < 4; ++i)
    {
        selectedGroup[i] = 0;
        scrollOffset[i] = 0;
        for (int j = 0; j < 32; ++j)
            selectedVariant[i][j] = 0;
    }

    if (player != nullptr && mc != nullptr && player == static_cast<EntityPlayer*>(mc->thePlayer2))
        ownerPlayerIndex = 1;
}

LegacyCraftingScreen::~LegacyCraftingScreen()
{
}

int LegacyCraftingScreen::getOwnerPlayerIndex() const
{
    if (ownerPlayerIndex >= 0)
        return ownerPlayerIndex;
    if (entityPlayer != nullptr && mc != nullptr && entityPlayer == static_cast<EntityPlayer*>(mc->thePlayer2))
        return 1;
    if (mc != nullptr && mc->isScreenOwnedByPlayer2())
        return 1;
    return 0;
}

void LegacyCraftingScreen::initGui()
{
    GuiScreen::initGui();
    xSize = 286;
    ySize = 188;
    guiLeft = (width - xSize) / 2;
    guiTop = (height - ySize) / 2;
    ensureSelectionVisible();
}

void LegacyCraftingScreen::onGuiClosed()
{
    GuiScreen::onGuiClosed();
}

void LegacyCraftingScreen::ensureSelectionVisible()
{
    const RecipeCategory *cats = getCategoriesTable(is2x2Mode);
    const int_t cat = selectedCategory;
    const int_t groupCount = cats[cat].groupCount;
    if (groupCount <= 0) return;

    if (selectedGroup[cat] < 0) selectedGroup[cat] = 0;
    if (selectedGroup[cat] >= groupCount) selectedGroup[cat] = groupCount - 1;

    const int_t cur = selectedGroup[cat];
    const int_t varCount = cats[cat].groups[cur].variantCount;
    if (selectedVariant[cat][cur] < 0) selectedVariant[cat][cur] = 0;
    if (selectedVariant[cat][cur] >= varCount) selectedVariant[cat][cur] = varCount - 1;

    const int_t visibleSlots = 10;
    if (cur < scrollOffset[cat])
        scrollOffset[cat] = cur;
    if (cur >= scrollOffset[cat] + visibleSlots)
        scrollOffset[cat] = cur - visibleSlots + 1;
}

void LegacyCraftingScreen::changeCategory(int dir)
{
    selectedCategory = (selectedCategory + dir + 4) % 4;
    ensureSelectionVisible();
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
}

void LegacyCraftingScreen::changeVariant(int dir)
{
    const RecipeCategory *cats = getCategoriesTable(is2x2Mode);
    const int_t cat = selectedCategory;
    const int_t grp = selectedGroup[cat];
    if (grp < 0 || grp >= cats[cat].groupCount) return;
    const int_t varCount = cats[cat].groups[grp].variantCount;
    if (varCount <= 1) return;

    selectedVariant[cat][grp] = (selectedVariant[cat][grp] + dir + varCount) % varCount;
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
}

void LegacyCraftingScreen::handleNavigation(int dirX, int dirY)
{
    const RecipeCategory *cats = getCategoriesTable(is2x2Mode);
    const int_t cat = selectedCategory;
    const int_t groupCount = cats[cat].groupCount;
    if (groupCount <= 0) return;

    if (dirX != 0)
    {
        selectedGroup[cat] = (selectedGroup[cat] + dirX + groupCount) % groupCount;
        ensureSelectionVisible();
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
    }
    if (dirY != 0)
    {
        changeVariant(-dirY); // up is previous (-1), down is next (+1)
    }
}

bool LegacyCraftingScreen::playerHasIngredient(int_t itemId, int_t itemDamage) const
{
    if (itemId <= 0 || inventory == nullptr || inventory->mainInventory == nullptr)
        return false;

    for (int_t i = 0; i < 36; ++i)
    {
        ItemStack *st = inventory->mainInventory[i];
        if (st != nullptr && st->isValid() && st->itemID == itemId)
        {
            if (itemDamage == -1 || st->getItemDamage() == itemDamage)
                return true;
        }
    }
    return false;
}

bool LegacyCraftingScreen::canCraftCurrentRecipe() const
{
    const RecipeCategory *cats = getCategoriesTable(is2x2Mode);
    const int_t cat = selectedCategory;
    const int_t grp = selectedGroup[cat];
    if (grp < 0 || grp >= cats[cat].groupCount) return false;
    const int_t var = selectedVariant[cat][grp];
    if (var < 0 || var >= cats[cat].groups[grp].variantCount) return false;
    const RecipeVariant &recipe = cats[cat].groups[grp].variants[var];

    if (is2x2Mode && recipe.requiresWorkbench)
        return false;

    if (inventory == nullptr || inventory->mainInventory == nullptr)
        return false;

    for (int_t i = 0; i < recipe.ingredientCount; ++i)
    {
        const RecipeIngredient &ing = recipe.ingredients[i];
        if (ing.itemId <= 0 || ing.count <= 0)
            continue;
        int_t available = 0;
        for (int_t s = 0; s < 36; ++s)
        {
            ItemStack *st = inventory->mainInventory[s];
            if (st != nullptr && st->isValid() && st->itemID == ing.itemId)
            {
                if (ing.itemDamage == -1 || st->getItemDamage() == ing.itemDamage)
                    available += st->stackSize;
            }
        }
        if (available < ing.count)
            return false;
    }
    return true;
}

void LegacyCraftingScreen::craftCurrentRecipe()
{
    if (!canCraftCurrentRecipe())
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("note.bass", 1.0f, 0.8f);
        return;
    }

    const RecipeCategory *cats = getCategoriesTable(is2x2Mode);
    const int_t cat = selectedCategory;
    const int_t grp = selectedGroup[cat];
    if (grp < 0 || grp >= cats[cat].groupCount) return;
    const int_t var = selectedVariant[cat][grp];
    if (var < 0 || var >= cats[cat].groups[grp].variantCount) return;
    const RecipeVariant &recipe = cats[cat].groups[grp].variants[var];

    // Deduct ingredients
    for (int_t i = 0; i < recipe.ingredientCount; ++i)
    {
        const RecipeIngredient &ing = recipe.ingredients[i];
        if (ing.itemId <= 0 || ing.count <= 0)
            continue;
        int_t toTake = ing.count;
        for (int_t s = 0; s < 36 && toTake > 0; ++s)
        {
            ItemStack *st = inventory->mainInventory[s];
            if (st != nullptr && st->isValid() && st->itemID == ing.itemId)
            {
                if (ing.itemDamage == -1 || st->getItemDamage() == ing.itemDamage)
                {
                    int_t take = std::min(st->stackSize, toTake);
                    st->stackSize -= take;
                    toTake -= take;
                    if (st->stackSize <= 0)
                    {
                        delete st;
                        inventory->mainInventory[s] = nullptr;
                    }
                }
            }
        }
    }

    // Add crafted result to inventory
    ItemStack *result = new ItemStack(recipe.resultId, recipe.resultCount, recipe.resultDamage);
    if (!inventory->addItemStackToInventory(result))
    {
        if (result != nullptr && result->stackSize > 0)
        {
            EntityPlayer *p = entityPlayer ? entityPlayer : (mc ? static_cast<EntityPlayer*>(mc->thePlayer) : nullptr);
            if (p != nullptr)
                p->dropPlayerItem(result);
            else
                delete result;
        }
        else
        {
            delete result;
        }
    }
    inventory->inventoryChanged = true;

    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.pop", 1.0f, 1.0f);
}

void LegacyCraftingScreen::drawSlotRect(int_t sx, int_t sy)
{
    // Classic 18x18 Minecraft slot with sunken bevel
    drawRect(sx, sy, sx + 18, sy + 18, 0xff8b8b8b);
    drawRect(sx, sy, sx + 18, sy + 1, 0xff373737);
    drawRect(sx, sy, sx + 1, sy + 18, 0xff373737);
    drawRect(sx + 1, sy + 1, sx + 17, sy + 2, 0xff373737);
    drawRect(sx + 1, sy + 1, sx + 2, sy + 17, 0xff373737);
    drawRect(sx + 17, sy + 1, sx + 18, sy + 18, 0xffffffff);
    drawRect(sx + 1, sy + 17, sx + 18, sy + 18, 0xffffffff);
}

void LegacyCraftingScreen::drawTooltip(ItemStack *stack, int_t mouseX, int_t mouseY)
{
    if (stack == nullptr || fontRenderer == nullptr)
        return;

    std::vector<std::string> lines = stack->getItemNameandInformation();
    if (lines.empty())
        return;

    int_t tooltipWidth = 0;
    for (const std::string &l : lines)
        tooltipWidth = std::max(tooltipWidth, fontRenderer->getStringWidth(l));

    int_t tx = mouseX + 12;
    int_t ty = mouseY - 12;
    if (tx + tooltipWidth + 6 > width)
        tx = width - tooltipWidth - 6;
    if (ty < 4)
        ty = 4;

    int_t tooltipHeight = 8;
    if (lines.size() > 1)
        tooltipHeight += 2 + (static_cast<int_t>(lines.size()) - 1) * 10;

    const int_t bg = static_cast<int_t>(0xf0100010u);
    drawGradientRect(tx - 3, ty - 4, tx + tooltipWidth + 3, ty - 3, bg, bg);
    drawGradientRect(tx - 3, ty + tooltipHeight + 3, tx + tooltipWidth + 3, ty + tooltipHeight + 4, bg, bg);
    drawGradientRect(tx - 3, ty - 3, tx + tooltipWidth + 3, ty + tooltipHeight + 3, bg, bg);
    drawGradientRect(tx - 4, ty - 3, tx - 3, ty + tooltipHeight + 3, bg, bg);
    drawGradientRect(tx + tooltipWidth + 3, ty - 3, tx + tooltipWidth + 4, ty + tooltipHeight + 3, bg, bg);

    const int_t borderTop = 0x505000ff;
    const int_t borderBottom = (borderTop & 0x00fefefe) >> 1 | (borderTop & static_cast<int_t>(0xff000000u));
    drawGradientRect(tx - 3, ty - 2, tx - 2, ty + tooltipHeight + 2, borderTop, borderBottom);
    drawGradientRect(tx + tooltipWidth + 2, ty - 2, tx + tooltipWidth + 3, ty + tooltipHeight + 2, borderTop, borderBottom);
    drawGradientRect(tx - 3, ty - 3, tx + tooltipWidth + 3, ty - 2, borderTop, borderTop);
    drawGradientRect(tx - 3, ty + tooltipHeight + 2, tx + tooltipWidth + 3, ty + tooltipHeight + 3, borderBottom, borderBottom);

    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        fontRenderer->drawStringWithShadow(lines[i], tx, ty, 0xffffffff);
        ty += (i == 0) ? 12 : 10;
    }
}

void LegacyCraftingScreen::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    drawDefaultBackground();

    const int_t panelLeft = guiLeft;
    const int_t panelTop = guiTop + 24;
    const int_t panelRight = guiLeft + xSize;
    const int_t panelBottom = guiTop + ySize;

    // Draw main panel drop shadow and outer bevel
    drawRect(panelLeft + 2, panelTop + 2, panelRight + 2, panelBottom + 2, 0x68000000);
    drawRect(panelLeft - 1, panelTop - 1, panelRight + 1, panelBottom + 1, 0xff343434);
    drawRect(panelLeft, panelTop, panelRight, panelBottom, 0xffc6c6c6);
    drawRect(panelLeft + 2, panelTop + 1, panelRight - 2, panelTop + 2, 0xffededed);
    drawRect(panelLeft + 1, panelTop + 2, panelLeft + 2, panelBottom - 2, 0xffededed);
    drawRect(panelLeft + 2, panelBottom - 2, panelRight - 2, panelBottom - 1, 0xff858585);
    drawRect(panelRight - 2, panelTop + 2, panelRight - 1, panelBottom - 2, 0xff858585);

    // Setup item rendering context
    RenderHelper::enableGUIStandardItemLighting();
    renderPushMatrix();
    renderTranslate(0.0f, 0.0f, 0.0f);
    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    renderEnable(RenderCapability::RescaleNormal);
    OpenGlHelper::setLightmapTextureCoords(OpenGlHelper::lightmapTexUnit, 240.0f, 240.0f);

    ItemStack *hoveredStack = nullptr;

    const RecipeCategory *cats = getCategoriesTable(is2x2Mode);

    // -------------------------------------------------------------
    // Draw 4 Top Tabs
    // -------------------------------------------------------------
    renderDisable(RenderCapability::Lighting);
    for (int t = 0; t < 4; ++t)
    {
        const int_t tabX = guiLeft + 16 + t * 32;
        const bool active = (t == selectedCategory);
        const int_t tabY = active ? (guiTop + 4) : (guiTop + 7);
        const int_t tabH = active ? 22 : 18;

        drawRect(tabX - 1, tabY - 1, tabX + 29, tabY + tabH, 0xff343434);
        drawRect(tabX, tabY, tabX + 28, tabY + tabH, active ? 0xffc6c6c6 : 0xff9c9c9c);
        drawRect(tabX + 1, tabY + 1, tabX + 27, tabY + 2, active ? 0xffffffff : 0xffb8b8b8);
        drawRect(tabX + 1, tabY + 2, tabX + 2, tabY + tabH, active ? 0xffffffff : 0xffb8b8b8);
        drawRect(tabX + 27, tabY + 2, tabX + 28, tabY + tabH, active ? 0xff858585 : 0xff505050);

        if (active)
            drawRect(tabX + 1, panelTop, tabX + 27, panelTop + 2, 0xffc6c6c6);

        renderEnable(RenderCapability::Lighting);
        if (cats[t].iconStack != nullptr)
        {
            itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, cats[t].iconStack,
                                            tabX + 6, tabY + (active ? 3 : 2));
        }
        renderDisable(RenderCapability::Lighting);
    }

    // Shoulder button tab hints
#if PLATFORM_PS2
    fontRenderer->drawStringWithShadow("L1", guiLeft + 4, guiTop + 9, 0xffe0e0e0);
    fontRenderer->drawStringWithShadow("R1", guiLeft + 148, guiTop + 9, 0xffe0e0e0);
#elif PLATFORM_XBOX
    drawControlIcon(mc, controlIconTexture(mc, "White"), guiLeft + 4, guiTop + 6);
    drawControlIcon(mc, controlIconTexture(mc, "Black"), guiLeft + 148, guiTop + 6);
#elif PLATFORM_WII
    fontRenderer->drawStringWithShadow("L", guiLeft + 6, guiTop + 9, 0xffe0e0e0);
    fontRenderer->drawStringWithShadow("R", guiLeft + 148, guiTop + 9, 0xffe0e0e0);

#else
    fontRenderer->drawStringWithShadow("Q", guiLeft + 6, guiTop + 9, 0xffe0e0e0);
    fontRenderer->drawStringWithShadow("E", guiLeft + 148, guiTop + 9, 0xffe0e0e0);
#endif

    // Category title
    const RecipeCategory &currentCat = cats[selectedCategory];
    int_t catTitleW = fontRenderer->getStringWidth(currentCat.name);
    fontRenderer->drawStringWithShadow(currentCat.name, guiLeft + (xSize - catTitleW) / 2, panelTop + 4, 0xffffff00);

    // -------------------------------------------------------------
    // Horizontal Recipe Carousel
    // -------------------------------------------------------------
    const int_t carouselY = panelTop + 16;
    const int_t visibleCount = 10;
    const int_t carouselW = visibleCount * 22;
    const int_t carouselStartX = guiLeft + (xSize - carouselW) / 2;

    const int_t curGroup = selectedGroup[selectedCategory];
    const int_t scroll = scrollOffset[selectedCategory];

    for (int i = 0; i < visibleCount; ++i)
    {
        const int_t gIdx = scroll + i;
        if (gIdx >= currentCat.groupCount) break;

        const RecipeGroup &grp = currentCat.groups[gIdx];
        const int_t gx = carouselStartX + i * 22;
        const int_t gy = carouselY;
        const bool isSelected = (gIdx == curGroup);

        renderDisable(RenderCapability::Lighting);
        drawSlotRect(gx, gy);

        if (isSelected)
        {
            drawRect(gx - 1, gy - 1, gx + 19, gy, 0xffffff00);
            drawRect(gx - 1, gy + 18, gx + 19, gy + 19, 0xffffff00);
            drawRect(gx - 1, gy, gx, gy + 18, 0xffffff00);
            drawRect(gx + 18, gy, gx + 19, gy + 18, 0xffffff00);

            if (grp.variantCount > 1)
            {
                const int_t curVar = selectedVariant[selectedCategory][gIdx];

                drawRect(gx - 2, gy - 16, gx + 20, gy + 36, 0x90202020);
                drawRect(gx - 1, gy - 15, gx + 19, gy + 35, 0xff343434);
                drawRect(gx, gy - 14, gx + 18, gy + 34, 0xffa0a0a0);

                fontRenderer->drawString("^", gx + 7, gy - 12, curVar > 0 ? 0xffffff : 0x707070);
                fontRenderer->drawString("v", gx + 7, gy + 22, curVar < grp.variantCount - 1 ? 0xffffff : 0x707070);

                renderEnable(RenderCapability::Lighting);
                if (curVar > 0 && grp.variants[curVar - 1].resultStack != nullptr)
                {
                    itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine,
                                                    grp.variants[curVar - 1].resultStack, gx + 1, gy - 10);
                }
                if (curVar < grp.variantCount - 1 && grp.variants[curVar + 1].resultStack != nullptr)
                {
                    itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine,
                                                    grp.variants[curVar + 1].resultStack, gx + 1, gy + 14);
                }
                renderDisable(RenderCapability::Lighting);
            }
        }

        renderEnable(RenderCapability::Lighting);
        const int_t varIdx = selectedVariant[selectedCategory][gIdx];
        if (grp.variants[varIdx].resultStack != nullptr)
        {
            itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine,
                                            grp.variants[varIdx].resultStack, gx + 1, gy + 1);
        }
    }

    renderDisable(RenderCapability::Lighting);
    drawRect(guiLeft + 8, panelTop + 42, guiLeft + xSize - 8, panelTop + 43, 0xff858585);
    drawRect(guiLeft + 8, panelTop + 43, guiLeft + xSize - 8, panelTop + 44, 0xffffffff);

    // -------------------------------------------------------------
    // Lower Section: Left = Recipe Grid + Result, Right = Inventory
    // -------------------------------------------------------------
    if (curGroup >= 0 && curGroup < currentCat.groupCount)
    {
        const RecipeGroup &activeGroup = currentCat.groups[curGroup];
        const int_t curVar = selectedVariant[selectedCategory][curGroup];
        const RecipeVariant &activeVariant = activeGroup.variants[curVar];

        fontRenderer->drawString(activeVariant.name ? activeVariant.name : "Recipe", guiLeft + 14, panelTop + 48, 0x303030);
        fontRenderer->drawString("Inventory", guiLeft + 116, panelTop + 48, 0x303030);

        const bool canCraft = canCraftCurrentRecipe();
        const int_t matrixX = is2x2Mode ? (guiLeft + 16) : (guiLeft + 10);
        const int_t matrixY = is2x2Mode ? (panelTop + 66) : (panelTop + 60);
        const int_t arrowX = is2x2Mode ? (guiLeft + 58) : (guiLeft + 68);
        const int_t arrowY = is2x2Mode ? (matrixY + 11) : (matrixY + 20);
        const int_t resultX = is2x2Mode ? (guiLeft + 86) : (guiLeft + 92);
        const int_t resultY = is2x2Mode ? (matrixY + 7) : (matrixY + 16);

        if (is2x2Mode)
        {
            for (int r = 0; r < 2; ++r)
            {
                for (int c = 0; c < 2; ++c)
                {
                    const int slot = r * 2 + c;
                    const int sx = matrixX + c * 18;
                    const int sy = matrixY + r * 18;
                    drawSlotRect(sx, sy);

                    if (activeVariant.gridStacks[slot] != nullptr)
                    {
                        const bool hasIng = playerHasIngredient(activeVariant.gridItemIds[slot], activeVariant.gridItemDamage[slot]);
                        if (!hasIng)
                            drawRect(sx + 1, sy + 1, sx + 17, sy + 17, 0x60cc2020);

                        renderEnable(RenderCapability::Lighting);
                        itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, activeVariant.gridStacks[slot], sx + 1, sy + 1);
                        renderDisable(RenderCapability::Lighting);

                        if (!hasIng)
                            fontRenderer->drawString("!", sx + 2, sy + 1, 0xffff20);

                        if (mouseX >= sx && mouseX < sx + 18 && mouseY >= sy && mouseY < sy + 18)
                            hoveredStack = activeVariant.gridStacks[slot];
                    }
                }
            }
        }
        else
        {
            for (int r = 0; r < 3; ++r)
            {
                for (int c = 0; c < 3; ++c)
                {
                    const int slot = r * 3 + c;
                    const int sx = matrixX + c * 18;
                    const int sy = matrixY + r * 18;
                    drawSlotRect(sx, sy);

                    if (activeVariant.gridStacks[slot] != nullptr)
                    {
                        const bool hasIng = playerHasIngredient(activeVariant.gridItemIds[slot], activeVariant.gridItemDamage[slot]);
                        if (!hasIng)
                            drawRect(sx + 1, sy + 1, sx + 17, sy + 17, 0x60cc2020);

                        renderEnable(RenderCapability::Lighting);
                        itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, activeVariant.gridStacks[slot], sx + 1, sy + 1);
                        renderDisable(RenderCapability::Lighting);

                        if (!hasIng)
                            fontRenderer->drawString("!", sx + 2, sy + 1, 0xffff20);

                        if (mouseX >= sx && mouseX < sx + 18 && mouseY >= sy && mouseY < sy + 18)
                            hoveredStack = activeVariant.gridStacks[slot];
                    }
                }
            }
        }

        // Arrow pointing to result - Disable lighting so texture is not darkly shaded
        renderDisable(RenderCapability::Lighting);
        int_t craftTex = mc->renderEngine->getTexture("/gui/crafting.png");
        renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        mc->renderEngine->bindTexture(craftTex);
        drawTexturedModalRect(arrowX, arrowY, 89, 35, 22, 15);

        // Result Slot
        renderDisable(RenderCapability::Lighting);
        drawRect(resultX, resultY, resultX + 22, resultY + 22, 0xff8b8b8b);
        drawRect(resultX, resultY, resultX + 22, resultY + 1, 0xff373737);
        drawRect(resultX, resultY, resultX + 1, resultY + 22, 0xff373737);
        drawRect(resultX + 1, resultY + 1, resultX + 21, resultY + 2, 0xff373737);
        drawRect(resultX + 1, resultY + 1, resultX + 2, resultY + 21, 0xff373737);
        drawRect(resultX + 21, resultY + 1, resultX + 22, resultY + 22, 0xffffffff);
        drawRect(resultX + 1, resultY + 21, resultX + 22, resultY + 22, 0xffffffff);

        if (!canCraft)
            drawRect(resultX + 1, resultY + 1, resultX + 21, resultY + 21, 0x60cc2020);
        else
            drawRect(resultX + 1, resultY + 1, resultX + 21, resultY + 21, 0x3020c020);

        renderEnable(RenderCapability::Lighting);
        if (activeVariant.resultStack != nullptr)
        {
            itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, activeVariant.resultStack, resultX + 3, resultY + 3);
            itemRenderer->renderItemOverlayIntoGUI(fontRenderer, mc->renderEngine, activeVariant.resultStack, resultX + 3, resultY + 3);
        }
        renderDisable(RenderCapability::Lighting);

        if (!canCraft)
            fontRenderer->drawString("!", resultX + 3, resultY + 2, 0xffff20);

        if (mouseX >= resultX && mouseX < resultX + 22 && mouseY >= resultY && mouseY < resultY + 22)
            hoveredStack = activeVariant.resultStack;

        // Status description text below crafting matrix
        const int_t statusY = matrixY + (is2x2Mode ? 40 : 58);
        if (canCraft)
        {
#if PLATFORM_PS2
            fontRenderer->drawString("Ready [Cross]", guiLeft + 14, statusY, 0x207820);
#elif PLATFORM_WII
            fontRenderer->drawString("Ready [A]", guiLeft + 14, statusY, 0x207820);

#elif PLATFORM_XBOX
            fontRenderer->drawString("Ready [A]", guiLeft + 14, statusY, 0x207820);
#else
            fontRenderer->drawString("Ready [Enter]", guiLeft + 14, statusY, 0x207820);
#endif
        }
        else if (is2x2Mode && activeVariant.requiresWorkbench)
        {
            fontRenderer->drawString("Needs Crafting Table", guiLeft + 14, statusY, 0x902020);
        }
        else
        {
            fontRenderer->drawString("Missing Items", guiLeft + 14, statusY, 0x902020);
        }
    }

    // Right Area: Player Inventory (4 rows x 9 columns)
    const int_t invX = guiLeft + 116;
    const int_t invY = panelTop + 60;

    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 9; ++c)
        {
            const int slotIndex = (r < 3) ? (9 + r * 9 + c) : c;
            const int sx = invX + c * 18;
            const int sy = invY + r * 18 + (r == 3 ? 3 : 0);

            drawSlotRect(sx, sy);

            ItemStack *st = (inventory != nullptr && inventory->mainInventory != nullptr)
                                ? inventory->mainInventory[slotIndex]
                                : nullptr;
            if (st != nullptr && st->isValid())
            {
                renderEnable(RenderCapability::Lighting);
                itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, st, sx + 1, sy + 1);
                itemRenderer->renderItemOverlayIntoGUI(fontRenderer, mc->renderEngine, st, sx + 1, sy + 1);
                renderDisable(RenderCapability::Lighting);

                if (mouseX >= sx && mouseX < sx + 18 && mouseY >= sy && mouseY < sy + 18)
                    hoveredStack = st;
            }
        }
    }

    // Selection cursor centered on active recipe carousel slot
    renderDisable(RenderCapability::Lighting);
    const int_t activeSlotX = carouselStartX + (curGroup - scroll) * 22;
    legacyDrawSelectionCursorCentered(mc, activeSlotX + 9, carouselY + 9, 20, zLevel + 64.0f);

    renderDisable(RenderCapability::RescaleNormal);
    RenderHelper::disableStandardItemLighting();
    renderPopMatrix();

    // Tooltip display
    if (hoveredStack != nullptr)
        drawTooltip(hoveredStack, mouseX, mouseY);

    // Bottom Action Hints (static strings to avoid runtime heap allocation)
#if PLATFORM_PS2
    if (is2x2Mode)
    {
        static const std::string buttons[] = {"L1/R1", "D-Pad", "Cross", "Triangle", "Circle"};
        static const std::string actions[] = {uiText("Category"), uiText("Navigate"), uiText("Craft"), uiText("Inventory"), uiText("Back")};
        drawControlHintRow(mc, width, legacyHintRowY(height), buttons, actions, 5);
    }
    else
    {
        static const std::string buttons[] = {"L1/R1", "D-Pad", "Cross", "Circle"};
        static const std::string actions[] = {uiText("Category"), uiText("Navigate"), uiText("Craft"), uiText("Back")};
        drawControlHintRow(mc, width, legacyHintRowY(height), buttons, actions, 4);
    }
#elif PLATFORM_WII
    static const std::string buttons[] = {"L/R", "D-Pad", "A", "B"};
    static const std::string actions[] = {uiText("Category"), uiText("Navigate"), uiText("Craft"), uiText("Back")};
    drawControlHintRow(mc, width, legacyHintRowY(height), buttons, actions, 4);
#elif PLATFORM_XBOX
    const std::string buttons[] = {"White/Black", "D-Pad", "A", "B"};
    const std::string actions[] = {uiText("Category"), uiText("Navigate"), uiText("Craft"), uiText("Back")};
    drawControlHintRow(mc, width, legacyHintRowY(height), buttons, actions, 4);
#else
    static const std::string buttons[] = {"Q/E", "Arrows", "Enter", "Esc"};
    static const std::string actions[] = {uiText("Category"), uiText("Navigate"), uiText("Craft"), uiText("Back")};
    drawControlHintRow(mc, width, legacyHintRowY(height), buttons, actions, 4);
#endif
}

void LegacyCraftingScreen::updateScreen()
{
    GuiScreen::updateScreen();

#if PLATFORM_PS2
    const Ps2PadSnapshot &ps2Pad = ps2PadGetSnapshot(getOwnerPlayerIndex());
    if (ps2Pad.connected)
    {
        unsigned short pressed = ps2Pad.pressed;
        if (ps2ActionReleaseLatch)
        {
            pressed &= ~PS2_PAD_CROSS;
            if ((ps2Pad.held & PS2_PAD_CROSS) == 0)
                ps2ActionReleaseLatch = false;
        }

        if (pressed & PS2_PAD_L1) changeCategory(-1);
        if (pressed & PS2_PAD_R1) changeCategory(1);
        if (pressed & PS2_PAD_LEFT)  handleNavigation(-1, 0);
        if (pressed & PS2_PAD_RIGHT) handleNavigation(1, 0);
        if (pressed & PS2_PAD_UP)    handleNavigation(0, 1);
        if (pressed & PS2_PAD_DOWN)  handleNavigation(0, -1);

        if (pressed & PS2_PAD_TRIANGLE)
        {
            if (is2x2Mode)
            {
                EntityPlayer *p = entityPlayer ? entityPlayer : (mc ? static_cast<EntityPlayer*>(mc->thePlayer) : nullptr);
                if (p != nullptr)
                {
                    if (mc->isSplitScreenActive())
                        mc->displayPlayerScreen(getOwnerPlayerIndex(), new GuiInventory(p));
                    else
                        mc->displayGuiScreen(new GuiInventory(p));
                    return;
                }
            }
        }

        if (pressed & PS2_PAD_CROSS)
            craftCurrentRecipe();

        if (pressed & (PS2_PAD_CIRCLE | PS2_PAD_SQUARE))
        {
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
            if (mc != nullptr && mc->isSplitScreenActive())
                mc->closePlayerScreen(getOwnerPlayerIndex());
            else if (mc != nullptr)
                mc->displayGuiScreen(nullptr);
            return;
        }

        // Hold-to-repeat crafting
        if (!ps2ActionReleaseLatch && (ps2Pad.held & PS2_PAD_CROSS) != 0)
        {
            ++craftHoldTicks;
            if (craftHoldTicks >= 10 && (craftHoldTicks % 3 == 0))
                craftCurrentRecipe();
        }
        else
        {
            craftHoldTicks = 0;
        }
        return;
    }
#endif

    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    if (pad.connected)
    {
        if (pad.pressed & PLATFORM_TEXT_PREV_PAGE) changeCategory(-1);
        if (pad.pressed & PLATFORM_TEXT_NEXT_PAGE) changeCategory(1);
        if (pad.pressed & PLATFORM_TEXT_LEFT)  handleNavigation(-1, 0);
        if (pad.pressed & PLATFORM_TEXT_RIGHT) handleNavigation(1, 0);
        if (pad.pressed & PLATFORM_TEXT_UP)    handleNavigation(0, 1);
        if (pad.pressed & PLATFORM_TEXT_DOWN)  handleNavigation(0, -1);
        if (pad.pressed & PLATFORM_TEXT_PREV_PAGE) changeCategory(-1);
        if (pad.pressed & PLATFORM_TEXT_NEXT_PAGE) changeCategory(1);
        if (pad.pressed & PLATFORM_TEXT_TYPE)  craftCurrentRecipe();

        if (pad.pressed & (PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_SHIFT))
        {
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
            if (mc != nullptr && mc->isSplitScreenActive())
                mc->closePlayerScreen(getOwnerPlayerIndex());
            else if (mc != nullptr)
                mc->displayGuiScreen(nullptr);
            return;
        }

        if (pad.held & PLATFORM_TEXT_TYPE)
        {
            ++craftHoldTicks;
            if (craftHoldTicks >= 10 && (craftHoldTicks % 3 == 0))
                craftCurrentRecipe();
        }
        else
        {
            craftHoldTicks = 0;
        }
    }
}

void LegacyCraftingScreen::keyTyped(char_t c, int_t key)
{
    const bool isCloseKey = (key == lwjgl::Keyboard::KEY_ESCAPE) ||
        (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->keyBindCrafting != nullptr && key == mc->gameSettings->keyBindCrafting->keyCode);
    if (isCloseKey)
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        if (mc != nullptr && mc->isSplitScreenActive())
            mc->closePlayerScreen(getOwnerPlayerIndex());
        else if (mc != nullptr)
            mc->displayGuiScreen(nullptr);
        return;
    }

    // Toggle to Inventory if in 2x2 hand crafting mode, or close workbench if in 3x3 mode
    const bool isInventoryKey = (key == lwjgl::Keyboard::KEY_I ||
        (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->keyBindInventory != nullptr && key == mc->gameSettings->keyBindInventory->keyCode));
    if (isInventoryKey)
    {
        if (is2x2Mode)
        {
            EntityPlayer *p = entityPlayer ? entityPlayer : (mc ? static_cast<EntityPlayer*>(mc->thePlayer) : nullptr);
            if (p != nullptr)
            {
                if (mc->isSplitScreenActive())
                    mc->displayPlayerScreen(getOwnerPlayerIndex(), new GuiInventory(p));
                else
                    mc->displayGuiScreen(new GuiInventory(p));
                return;
            }
        }
        else
        {
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
            if (mc != nullptr && mc->isSplitScreenActive())
                mc->closePlayerScreen(getOwnerPlayerIndex());
            else if (mc != nullptr)
                mc->displayGuiScreen(nullptr);
            return;
        }
    }

    if (key == lwjgl::Keyboard::KEY_Q || key == lwjgl::Keyboard::KEY_PRIOR)
    {
        changeCategory(-1);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_E || key == lwjgl::Keyboard::KEY_NEXT)
    {
        changeCategory(1);
        return;
    }

    if (key == lwjgl::Keyboard::KEY_LEFT || key == lwjgl::Keyboard::KEY_A)
    {
        handleNavigation(-1, 0);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_RIGHT || key == lwjgl::Keyboard::KEY_D)
    {
        handleNavigation(1, 0);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_UP || key == lwjgl::Keyboard::KEY_W)
    {
        handleNavigation(0, 1);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_DOWN || key == lwjgl::Keyboard::KEY_S)
    {
        handleNavigation(0, -1);
        return;
    }

    if (key == lwjgl::Keyboard::KEY_RETURN || key == lwjgl::Keyboard::KEY_SPACE)
    {
        craftCurrentRecipe();
        return;
    }

    GuiScreen::keyTyped(c, key);
}

void LegacyCraftingScreen::mouseClicked(int_t mouseX, int_t mouseY, int_t button)
{
    GuiScreen::mouseClicked(mouseX, mouseY, button);

    // 1. Click on Category Tabs
    for (int t = 0; t < 4; ++t)
    {
        const int_t tabX = guiLeft + 16 + t * 32;
        const int_t tabY = guiTop + 4;
        if (mouseX >= tabX && mouseX < tabX + 28 && mouseY >= tabY && mouseY < tabY + 22)
        {
            selectedCategory = t;
            ensureSelectionVisible();
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
            return;
        }
    }

    // 2. Click on Recipe Carousel Items
    const RecipeCategory *cats = getCategoriesTable(is2x2Mode);
    const int_t panelTop = guiTop + 24;
    const int_t carouselY = panelTop + 16;
    const int_t visibleCount = 10;
    const int_t carouselW = visibleCount * 22;
    const int_t carouselStartX = guiLeft + (xSize - carouselW) / 2;
    const int_t scroll = scrollOffset[selectedCategory];
    const RecipeCategory &currentCat = cats[selectedCategory];

    for (int i = 0; i < visibleCount; ++i)
    {
        const int_t gIdx = scroll + i;
        if (gIdx >= currentCat.groupCount) break;

        const int_t gx = carouselStartX + i * 22;
        const int_t gy = carouselY;

        // If clicking on active group's variant arrows or popups
        if (gIdx == selectedGroup[selectedCategory] && currentCat.groups[gIdx].variantCount > 1)
        {
            if (mouseX >= gx && mouseX < gx + 18)
            {
                if (mouseY >= gy - 16 && mouseY < gy)
                {
                    changeVariant(-1);
                    return;
                }
                if (mouseY >= gy + 18 && mouseY < gy + 34)
                {
                    changeVariant(1);
                    return;
                }
            }
        }

        if (mouseX >= gx && mouseX < gx + 18 && mouseY >= gy && mouseY < gy + 18)
        {
            selectedGroup[selectedCategory] = gIdx;
            ensureSelectionVisible();
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
            return;
        }
    }

    // 3. Click on Result Slot -> Craft
    const int_t matrixX = is2x2Mode ? (guiLeft + 16) : (guiLeft + 10);
    const int_t matrixY = is2x2Mode ? (panelTop + 66) : (panelTop + 60);
    const int_t arrowX = is2x2Mode ? (guiLeft + 58) : (guiLeft + 68);
    const int_t arrowY = is2x2Mode ? (matrixY + 11) : (matrixY + 20);
    const int_t resultX = is2x2Mode ? (guiLeft + 86) : (guiLeft + 92);
    const int_t resultY = is2x2Mode ? (matrixY + 7) : (matrixY + 16);

    if (mouseX >= resultX && mouseX < resultX + 22 && mouseY >= resultY && mouseY < resultY + 22)
    {
        craftCurrentRecipe();
        return;
    }
}
