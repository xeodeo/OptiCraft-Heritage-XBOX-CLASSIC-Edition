#include "RecipesCrafting.h"

#include "Block.h"
#include "CraftingManager.h"
#include "Item.h"
#include "ItemStack.h"

RecipesCrafting::RecipesCrafting()
{
}

void RecipesCrafting::addRecipes(CraftingManager *craftingmanager)
{
	craftingmanager->addRecipe(new ItemStack(Block::chest),
	                           { "###", "# #", "###", '#', Block::planks });
	craftingmanager->addRecipe(new ItemStack(Block::stoneOvenIdle),
	                           { "###", "# #", "###", '#', Block::cobblestone });
	craftingmanager->addRecipe(new ItemStack(Block::workbench),
	                           { "##", "##", '#', Block::planks });
	craftingmanager->addRecipe(new ItemStack(Block::sandStone),
	                           { "##", "##", '#', Block::sand });
	craftingmanager->addRecipe(new ItemStack(Block::sandStone, 4, 2),
	                           { "##", "##", '#', Block::sandStone });
	craftingmanager->addRecipe(new ItemStack(Block::sandStone, 1, 1),
	                           { "#", "#", '#', new ItemStack(Block::stairSingle, 1, 1) });
	craftingmanager->addRecipe(new ItemStack(Block::blockNetherQuartz, 1, 1),
	                           { "#", "#", '#', new ItemStack(Block::stairSingle, 1, 7) });
	craftingmanager->addRecipe(new ItemStack(Block::blockNetherQuartz, 2, 2),
	                           { "#", "#", '#', new ItemStack(Block::blockNetherQuartz, 1, 0) });
	craftingmanager->addRecipe(new ItemStack(Block::stoneBrick, 4),
	                           { "##", "##", '#', Block::stone });
	craftingmanager->addRecipe(new ItemStack(Block::fenceIron, 16),
	                           { "###", "###", '#', Item::ingotIron });
	craftingmanager->addRecipe(new ItemStack(Block::thinGlass, 16),
	                           { "###", "###", '#', Block::glass });
	craftingmanager->addRecipe(new ItemStack(Block::redstoneLampIdle),
	                           { " R ", "RGR", " R ", 'R', Item::redstone, 'G', Block::glowStone });
	craftingmanager->addRecipe(new ItemStack(Block::netherBrick, 1),
	                           { "NN", "NN", 'N', Item::netherrackBrick });
	craftingmanager->addRecipe(new ItemStack(Block::blockNetherQuartz, 1),
	                           { "##", "##", '#', Item::netherQuartz });
	craftingmanager->addRecipe(new ItemStack(Block::stairsSandStone, 4),
	                           { "#  ", "## ", "###", '#', Block::sandStone });
	craftingmanager->addRecipe(new ItemStack(Block::stairsNetherQuartz, 4),
	                           { "#  ", "## ", "###", '#', Block::blockNetherQuartz });
	craftingmanager->addRecipe(new ItemStack(Block::cobblestoneWall, 6, 0),
	                           { "###", "###", '#', Block::cobblestone });
	craftingmanager->addRecipe(new ItemStack(Block::cobblestoneWall, 6, 1),
	                           { "###", "###", '#', Block::cobblestoneMossy });
	craftingmanager->addRecipe(new ItemStack(Item::flowerPot, 1),
	                           { "# #", " # ", '#', Item::brick });
	craftingmanager->addRecipe(new ItemStack(Block::anvil, 1),
	                           { "III", " i ", "iii", 'I', Block::blockSteel, 'i', Item::ingotIron });
	craftingmanager->addRecipe(new ItemStack(Block::enderChest, 1),
	                           { "###", "#E#", "###", '#', Block::obsidian, 'E', Item::eyeOfEnder });
	craftingmanager->addRecipe(new ItemStack(Block::beacon, 1),
	                           { "GGG", "GSG", "OOO", 'G', Block::glass, 'S', Item::netherStar, 'O', Block::obsidian });
}
