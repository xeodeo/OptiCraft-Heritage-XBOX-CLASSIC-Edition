#include "EntityVillager.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "AxisAlignedBB.h"
#include "Block.h"
#include "ChunkCoordinates.h"
#include "DamageSource.h"
#include "DataWatcher.h"
#include "Enchantment.h"
#include "EnchantmentHelper.h"
#include "EntityAIAvoidEntity.h"
#include "EntityAIFollowGolem.h"
#include "EntityAILookAtTradePlayer.h"
#include "EntityAIMoveIndoors.h"
#include "EntityAIMoveTwardsRestriction.h"
#include "EntityAIOpenDoor.h"
#include "EntityAIPlay.h"
#include "EntityAIRestrictOpenDoor.h"
#include "EntityAISwimming.h"
#include "EntityAITradePlayer.h"
#include "EntityAIVillagerMate.h"
#include "EntityAIWander.h"
#include "EntityAIWatchClosest.h"
#include "EntityAIWatchClosest2.h"
#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "EntityZombie.h"
#include "IMob.h"
#include "Item.h"
#include "ItemStack.h"
#include "MathHelper.h"
#include "MerchantRecipe.h"
#include "MerchantRecipeList.h"
#include "NBTTagCompound.h"
#include "PathNavigate.h"
#include "Potion.h"
#include "PotionEffect.h"
#include "GuiMerchant.h"
#include "Village.h"
#include "VillageCollection.h"
#include "World.h"

namespace
{
struct TradeRange { int_t minCount; int_t maxCount; };

const std::unordered_map<int_t, TradeRange> &getVillagerStockList()
{
	static const std::unordered_map<int_t, TradeRange> stock = {
		{Item::coal->shiftedIndex, {16, 24}},
		{Item::ingotIron->shiftedIndex, {8, 10}},
		{Item::ingotGold->shiftedIndex, {8, 10}},
		{Item::diamond->shiftedIndex, {4, 6}},
		{Item::paper->shiftedIndex, {24, 36}},
		{Item::book->shiftedIndex, {11, 13}},
		{Item::writtenBook->shiftedIndex, {1, 1}},
		{Item::enderPearl->shiftedIndex, {3, 4}},
		{Item::eyeOfEnder->shiftedIndex, {2, 3}},
		{Item::porkRaw->shiftedIndex, {14, 18}},
		{Item::beefRaw->shiftedIndex, {14, 18}},
		{Item::chickenRaw->shiftedIndex, {14, 18}},
		{Item::fishCooked->shiftedIndex, {9, 13}},
		{Item::seeds->shiftedIndex, {34, 48}},
		{Item::melonSeeds->shiftedIndex, {30, 38}},
		{Item::pumpkinSeeds->shiftedIndex, {30, 38}},
		{Item::wheat->shiftedIndex, {18, 22}},
		{Block::cloth->blockID, {14, 22}},
		{Item::rottenFlesh->shiftedIndex, {36, 64}}
	};
	return stock;
}

const std::unordered_map<int_t, TradeRange> &getBlacksmithSellingList()
{
	static const std::unordered_map<int_t, TradeRange> selling = {
		{Item::flintAndSteel->shiftedIndex, {3, 4}},
		{Item::shears->shiftedIndex, {3, 4}},
		{Item::swordSteel->shiftedIndex, {7, 11}},
		{Item::swordDiamond->shiftedIndex, {12, 14}},
		{Item::axeSteel->shiftedIndex, {6, 8}},
		{Item::axeDiamond->shiftedIndex, {9, 12}},
		{Item::pickaxeSteel->shiftedIndex, {7, 9}},
		{Item::pickaxeDiamond->shiftedIndex, {10, 12}},
		{Item::shovelSteel->shiftedIndex, {4, 6}},
		{Item::shovelDiamond->shiftedIndex, {7, 8}},
		{Item::hoeSteel->shiftedIndex, {4, 6}},
		{Item::hoeDiamond->shiftedIndex, {7, 8}},
		{Item::bootsSteel->shiftedIndex, {4, 6}},
		{Item::bootsDiamond->shiftedIndex, {7, 8}},
		{Item::helmetSteel->shiftedIndex, {4, 6}},
		{Item::helmetDiamond->shiftedIndex, {7, 8}},
		{Item::plateSteel->shiftedIndex, {10, 14}},
		{Item::plateDiamond->shiftedIndex, {16, 19}},
		{Item::legsSteel->shiftedIndex, {8, 10}},
		{Item::legsDiamond->shiftedIndex, {11, 14}},
		{Item::bootsChain->shiftedIndex, {5, 7}},
		{Item::helmetChain->shiftedIndex, {5, 7}},
		{Item::plateChain->shiftedIndex, {11, 15}},
		{Item::legsChain->shiftedIndex, {9, 11}},
		{Item::bread->shiftedIndex, {-4, -2}},
		{Item::melon->shiftedIndex, {-8, -4}},
		{Item::appleRed->shiftedIndex, {-8, -4}},
		{Item::cookie->shiftedIndex, {-10, -7}},
		{Block::glass->blockID, {-5, -3}},
		{Block::bookShelf->blockID, {3, 4}},
		{Item::plateLeather->shiftedIndex, {4, 5}},
		{Item::bootsLeather->shiftedIndex, {2, 4}},
		{Item::helmetLeather->shiftedIndex, {2, 4}},
		{Item::legsLeather->shiftedIndex, {2, 4}},
		{Item::saddle->shiftedIndex, {6, 8}},
		{Item::expBottle->shiftedIndex, {-4, -1}},
		{Item::redstone->shiftedIndex, {-4, -1}},
		{Item::compass->shiftedIndex, {10, 12}},
		{Item::pocketSundial->shiftedIndex, {10, 12}},
		{Block::glowStone->blockID, {-3, -1}},
		{Item::porkCooked->shiftedIndex, {-7, -5}},
		{Item::beefCooked->shiftedIndex, {-7, -5}},
		{Item::chickenCooked->shiftedIndex, {-8, -6}},
		{Item::eyeOfEnder->shiftedIndex, {7, 11}},
		{Item::arrow->shiftedIndex, {-12, -8}}
	};
	return selling;
}

int_t getRandomCountForItem(int_t itemId, Random &rand)
{
	const auto &stock = getVillagerStockList();
	auto it = stock.find(itemId);
	if (it == stock.end())
		return 1;
	const TradeRange &r = it->second;
	if (r.minCount >= r.maxCount)
		return r.minCount;
	return r.minCount + rand.nextInt(r.maxCount - r.minCount);
}

int_t getRandomCountForBlacksmithItem(int_t itemId, Random &rand)
{
	const auto &selling = getBlacksmithSellingList();
	auto it = selling.find(itemId);
	if (it == selling.end())
		return 1;
	const TradeRange &r = it->second;
	if (r.minCount >= r.maxCount)
		return r.minCount;
	return r.minCount + rand.nextInt(r.maxCount - r.minCount);
}

void addMerchantItem(MerchantRecipeList *recipes, int_t itemId, Random &rand, float chance)
{
	if (rand.nextFloat() < chance && Item::emerald != nullptr)
	{
		int_t count = getRandomCountForItem(itemId, rand);
		recipes->add(new MerchantRecipe(new ItemStack(itemId, count, 0), new ItemStack(Item::emerald->shiftedIndex, 1, 0)));
	}
}

void addBlacksmithItem(MerchantRecipeList *recipes, int_t itemId, Random &rand, float chance)
{
	if (rand.nextFloat() < chance && Item::emerald != nullptr)
	{
		int_t count = getRandomCountForBlacksmithItem(itemId, rand);
		ItemStack *buy = nullptr;
		ItemStack *sell = nullptr;
		if (count < 0)
		{
			buy = new ItemStack(Item::emerald->shiftedIndex, 1, 0);
			sell = new ItemStack(itemId, -count, 0);
		}
		else
		{
			buy = new ItemStack(Item::emerald->shiftedIndex, count, 0);
			sell = new ItemStack(itemId, 1, 0);
		}
		recipes->add(new MerchantRecipe(buy, sell));
	}
}
}

EntityVillager::EntityVillager(World *world)
	: EntityVillager(world, 0)
{
}

EntityVillager::EntityVillager(World *world, int_t profession)
	: EntityAgeable(world), randomTickDivider(0), isMatingFlag(false), isPlayingFlag(false)
{
	ensureEntityInit();
	setProfession(profession);
	texture = "/mob/villager/villager.png";
	moveSpeed = 0.5f;
	health = 20;
	prevHealth = 20;
	setSize(0.6f, 1.8f);
	if (getNavigator() != nullptr)
	{
		getNavigator()->setBreakDoors(true);
		getNavigator()->setAvoidsWater(true);
	}
	tasks.addTask(0, new EntityAISwimming(this));
	tasks.addTask(1, new EntityAIAvoidEntity(this, typeid(EntityZombie), 8.0f, 0.3f, 0.35f));
	tasks.addTask(1, new EntityAITradePlayer(this));
	tasks.addTask(1, new EntityAILookAtTradePlayer(this));
	tasks.addTask(2, new EntityAIMoveIndoors(this));
	tasks.addTask(3, new EntityAIRestrictOpenDoor(this));
	tasks.addTask(4, new EntityAIOpenDoor(this, true));
	tasks.addTask(5, new EntityAIMoveTwardsRestriction(this, 0.3f));
	tasks.addTask(6, new EntityAIVillagerMate(this));
	tasks.addTask(7, new EntityAIFollowGolem(this));
	tasks.addTask(8, new EntityAIPlay(this, 0.32f));
	tasks.addTask(9, new EntityAIWatchClosest2(this, typeid(EntityPlayer), 3.0f, 1.0f));
	tasks.addTask(9, new EntityAIWatchClosest2(this, typeid(EntityVillager), 5.0f, 0.02f));
	tasks.addTask(9, new EntityAIWander(this, 0.3f));
	tasks.addTask(10, new EntityAIWatchClosest(this, typeid(EntityLiving), 8.0f));
}

EntityVillager::~EntityVillager()
{
	if (buyingPlayer != nullptr)
		merchantScreensForget(this);
}

void EntityVillager::entityInit()
{
	EntityAgeable::entityInit();
	dataWatcher->addObject(16, int_t(0));
}

const char *EntityVillager::getEntityTexture()
{
	switch (getProfession())
	{
	case 0: return "/mob/villager/farmer.png";
	case 1: return "/mob/villager/librarian.png";
	case 2: return "/mob/villager/priest.png";
	case 3: return "/mob/villager/smith.png";
	case 4: return "/mob/villager/butcher.png";
	default: return EntityAgeable::getEntityTexture();
	}
}

void EntityVillager::writeEntityToNBT(NBTTagCompound *tag)
{
	EntityAgeable::writeEntityToNBT(tag);
	tag->setInteger("Profession", getProfession());
	tag->setInteger("Riches", wealth);
	if (buyingList)
		tag->setCompoundTag("Offers", buyingList->getRecipiesAsTags());
}

void EntityVillager::readEntityFromNBT(NBTTagCompound *tag)
{
	EntityAgeable::readEntityFromNBT(tag);
	setProfession(tag->getInteger("Profession"));
	wealth = tag->getInteger("Riches");
	if (tag->hasKey("Offers"))
		buyingList.reset(new MerchantRecipeList(tag->getCompoundTag("Offers")));
}

void EntityVillager::onLivingUpdate()
{
	EntityAgeable::onLivingUpdate();
}

bool EntityVillager::attackEntityFrom(Entity *entity, int_t damage)
{
	return EntityAgeable::attackEntityFrom(entity, damage);
}

void EntityVillager::setRevengeTarget(EntityLiving *target)
{
	EntityAgeable::setRevengeTarget(target);
	if (villageObj != nullptr && target != nullptr)
	{
		villageObj->addOrRenewAgressor(target);
		if (auto *player = dynamic_cast<EntityPlayer *>(target))
		{
			int_t rep = isChild() ? -3 : -1;
			villageObj->setReputationForPlayer(player->username, rep);
			if (isEntityAlive() && worldObj != nullptr)
			{
				worldObj->setEntityState(this, static_cast<byte_t>(13));
			}
		}
	}
}

bool EntityVillager::isAIEnabled()
{
	return true;
}

int_t EntityVillager::getMaxHealth() const
{
	return 20;
}

void EntityVillager::updateAITick()
{
	if (--randomTickDivider <= 0)
	{
		int_t x = MathHelper::floor_double(posX);
		int_t y = MathHelper::floor_double(posY);
		int_t z = MathHelper::floor_double(posZ);
		if (worldObj != nullptr && worldObj->villageCollectionObj != nullptr)
		{
			worldObj->villageCollectionObj->addVillagerPosition(x, y, z);
			randomTickDivider = 70 + getRNG().nextInt(50);
			villageObj = worldObj->villageCollectionObj->findNearestVillage(x, y, z, 32);
			if (villageObj == nullptr)
			{
				detachHome();
			}
			else
			{
				ChunkCoordinates center = villageObj->getCenter();
				setHomeArea(center.x, center.y, center.z, static_cast<int_t>((float)villageObj->getVillageRadius() * 0.6f));
				if (field_82190_bM)
				{
					field_82190_bM = false;
					villageObj->modifyPlayerReputation(5);
				}
			}
		}
	}

	if (!isTrading() && timeUntilReset > 0)
	{
		--timeUntilReset;
		if (timeUntilReset <= 0)
		{
			if (needsInitialization)
			{
				if (buyingList && buyingList->size() > 1)
				{
					for (int i = 0; i < buyingList->size(); ++i)
					{
						MerchantRecipe *recipe = buyingList->get(i);
						if (recipe && recipe->isRecipeDisabled())
						{
							recipe->increaseMaxTradeUses(getRNG().nextInt(6) + getRNG().nextInt(6) + 2);
						}
					}
				}

				addDefaultEquipmentAndRecipies(1);
				needsInitialization = false;
				if (villageObj && !lastBuyingPlayer.empty())
				{
					worldObj->setEntityState(this, static_cast<byte_t>(14));
					villageObj->setReputationForPlayer(lastBuyingPlayer, 1);
				}
			}

			if (Potion::regeneration != nullptr)
				addPotionEffect(new PotionEffect(Potion::regeneration->id, 200, 0));
		}
	}

	EntityAgeable::updateAITick();
}

bool EntityVillager::interact(EntityPlayer *player)
{
	ItemStack *held = player ? player->inventory->getCurrentItem() : nullptr;
	bool isSpawnEgg = held && Item::monsterPlacer && held->itemID == Item::monsterPlacer->shiftedIndex;
	if (!isSpawnEgg && isEntityAlive() && !isTrading() && !isChild())
	{
		if (worldObj && !worldObj->multiplayerWorld)
		{
			setCustomer(player);
			player->displayGUIMerchant(this, func_94057_bL());
		}
		return true;
	}
	return EntityAgeable::interact(player);
}

void EntityVillager::setCustomer(EntityPlayer *player)
{
	buyingPlayer = player;
}

EntityPlayer *EntityVillager::getCustomer() const
{
	return buyingPlayer;
}

bool EntityVillager::isTrading() const
{
	return buyingPlayer != nullptr;
}

MerchantRecipeList *EntityVillager::getRecipes(EntityPlayer *)
{
	if (buyingList == nullptr)
		addDefaultEquipmentAndRecipies(1);
	return buyingList.get();
}

void EntityVillager::setRecipes(MerchantRecipeList *recipes)
{
	buyingList.reset(recipes);
}

void EntityVillager::useRecipe(MerchantRecipe *recipe)
{
	if (recipe == nullptr || buyingList == nullptr)
		return;

	recipe->incrementToolUses();
	MerchantRecipe *lastRecipe = buyingList->get(buyingList->size() - 1);
	if (lastRecipe && recipe->hasSameIDsAs(*lastRecipe))
	{
		timeUntilReset = 40;
		needsInitialization = true;
		if (buyingPlayer != nullptr)
			lastBuyingPlayer = buyingPlayer->username;
		else
			lastBuyingPlayer.clear();
	}

	if (recipe->getItemToBuy() != nullptr && Item::emerald != nullptr &&
		recipe->getItemToBuy()->itemID == Item::emerald->shiftedIndex)
	{
		wealth += recipe->getItemToBuy()->stackSize;
	}
}

void EntityVillager::initCreature()
{
	setProfession(worldObj ? worldObj->rand.nextInt(5) : getRNG().nextInt(5));
}

void EntityVillager::handleHealthUpdate(byte_t state)
{
	if (state == 12) generateTradeParticles("heart");
	else if (state == 13) generateTradeParticles("angryVillager");
	else if (state == 14) generateTradeParticles("happyVillager");
	else EntityAgeable::handleHealthUpdate(state);
}

void EntityVillager::generateTradeParticles(const char *particle)
{
	if (!worldObj) return;
	for (int i = 0; i < 5; ++i)
	{
		double motionX = getRNG().nextGaussian() * 0.02;
		double motionY = getRNG().nextGaussian() * 0.02;
		double motionZ = getRNG().nextGaussian() * 0.02;
		worldObj->spawnParticle(particle,
			posX + (double)(getRNG().nextFloat() * width * 2.0f) - (double)width,
			posY + 1.0 + (double)(getRNG().nextFloat() * height),
			posZ + (double)(getRNG().nextFloat() * width * 2.0f) - (double)width,
			motionX, motionY, motionZ);
	}
}

float EntityVillager::func_82188_j(float base)
{
	float v = base + field_82191_bN;
	return v > 0.9f ? 0.9f - (v - 0.9f) : v;
}

void EntityVillager::addDefaultEquipmentAndRecipies(int_t count)
{
	if (buyingList != nullptr)
		field_82191_bN = MathHelper::sqrt_float((float)buyingList->size()) * 0.2f;
	else
		field_82191_bN = 0.0f;

	MerchantRecipeList candidates;
	switch (getProfession())
	{
	case 0: // Farmer
		addMerchantItem(&candidates, Item::wheat->shiftedIndex, getRNG(), func_82188_j(0.9f));
		addMerchantItem(&candidates, Block::cloth->blockID, getRNG(), func_82188_j(0.5f));
		addMerchantItem(&candidates, Item::chickenRaw->shiftedIndex, getRNG(), func_82188_j(0.5f));
		addMerchantItem(&candidates, Item::fishCooked->shiftedIndex, getRNG(), func_82188_j(0.4f));
		addBlacksmithItem(&candidates, Item::bread->shiftedIndex, getRNG(), func_82188_j(0.9f));
		addBlacksmithItem(&candidates, Item::melon->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::appleRed->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::cookie->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::shears->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::flintAndSteel->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::chickenCooked->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::arrow->shiftedIndex, getRNG(), func_82188_j(0.5f));
		if (getRNG().nextFloat() < func_82188_j(0.5f) && Item::emerald != nullptr)
		{
			candidates.add(new MerchantRecipe(
				new ItemStack(Block::gravel->blockID, 10, 0),
				new ItemStack(Item::emerald->shiftedIndex, 1, 0),
				new ItemStack(Item::flint->shiftedIndex, 4 + getRNG().nextInt(2), 0)));
		}
		break;
	case 1: // Librarian
		addMerchantItem(&candidates, Item::paper->shiftedIndex, getRNG(), func_82188_j(0.8f));
		addMerchantItem(&candidates, Item::book->shiftedIndex, getRNG(), func_82188_j(0.8f));
		addMerchantItem(&candidates, Item::writtenBook->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Block::bookShelf->blockID, getRNG(), func_82188_j(0.8f));
		addBlacksmithItem(&candidates, Block::glass->blockID, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::compass->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::pocketSundial->shiftedIndex, getRNG(), func_82188_j(0.2f));
		if (getRNG().nextFloat() < func_82188_j(0.07f) && Item::emerald != nullptr && Item::enchantedBook != nullptr)
		{
			int_t cost = 2 + getRNG().nextInt(15);
			candidates.add(new MerchantRecipe(
				new ItemStack(Item::book->shiftedIndex, 1, 0),
				new ItemStack(Item::emerald->shiftedIndex, cost, 0),
				new ItemStack(Item::enchantedBook->shiftedIndex, 1, 0)));
		}
		break;
	case 2: // Priest
		addBlacksmithItem(&candidates, Item::eyeOfEnder->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::expBottle->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::redstone->shiftedIndex, getRNG(), func_82188_j(0.4f));
		addBlacksmithItem(&candidates, Block::glowStone->blockID, getRNG(), func_82188_j(0.3f));
		{
			int_t priestGear[] = {
				Item::swordSteel->shiftedIndex,
				Item::swordDiamond->shiftedIndex,
				Item::plateSteel->shiftedIndex,
				Item::plateDiamond->shiftedIndex,
				Item::axeSteel->shiftedIndex,
				Item::axeDiamond->shiftedIndex,
				Item::pickaxeSteel->shiftedIndex,
				Item::pickaxeDiamond->shiftedIndex
			};
			for (int_t gearId : priestGear)
			{
				if (getRNG().nextFloat() < func_82188_j(0.05f) && Item::emerald != nullptr)
				{
					ItemStack *enchanted = new ItemStack(gearId, 1, 0);
					EnchantmentHelper::addRandomEnchantment(getRNG(), enchanted, 5 + getRNG().nextInt(15));
					candidates.add(new MerchantRecipe(
						new ItemStack(gearId, 1, 0),
						new ItemStack(Item::emerald->shiftedIndex, 2 + getRNG().nextInt(3), 0),
						enchanted));
				}
			}
		}
		break;
	case 3: // Blacksmith
		addMerchantItem(&candidates, Item::coal->shiftedIndex, getRNG(), func_82188_j(0.7f));
		addMerchantItem(&candidates, Item::ingotIron->shiftedIndex, getRNG(), func_82188_j(0.5f));
		addMerchantItem(&candidates, Item::ingotGold->shiftedIndex, getRNG(), func_82188_j(0.5f));
		addMerchantItem(&candidates, Item::diamond->shiftedIndex, getRNG(), func_82188_j(0.5f));
		addBlacksmithItem(&candidates, Item::swordSteel->shiftedIndex, getRNG(), func_82188_j(0.5f));
		addBlacksmithItem(&candidates, Item::swordDiamond->shiftedIndex, getRNG(), func_82188_j(0.5f));
		addBlacksmithItem(&candidates, Item::axeSteel->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::axeDiamond->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::pickaxeSteel->shiftedIndex, getRNG(), func_82188_j(0.5f));
		addBlacksmithItem(&candidates, Item::pickaxeDiamond->shiftedIndex, getRNG(), func_82188_j(0.5f));
		addBlacksmithItem(&candidates, Item::shovelSteel->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::shovelDiamond->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::hoeSteel->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::hoeDiamond->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::bootsSteel->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::bootsDiamond->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::helmetSteel->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::helmetDiamond->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::plateSteel->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::plateDiamond->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::legsSteel->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::legsDiamond->shiftedIndex, getRNG(), func_82188_j(0.2f));
		addBlacksmithItem(&candidates, Item::bootsChain->shiftedIndex, getRNG(), func_82188_j(0.1f));
		addBlacksmithItem(&candidates, Item::helmetChain->shiftedIndex, getRNG(), func_82188_j(0.1f));
		addBlacksmithItem(&candidates, Item::plateChain->shiftedIndex, getRNG(), func_82188_j(0.1f));
		addBlacksmithItem(&candidates, Item::legsChain->shiftedIndex, getRNG(), func_82188_j(0.1f));
		break;
	case 4: // Butcher
		addMerchantItem(&candidates, Item::coal->shiftedIndex, getRNG(), func_82188_j(0.7f));
		addMerchantItem(&candidates, Item::porkRaw->shiftedIndex, getRNG(), func_82188_j(0.5f));
		addMerchantItem(&candidates, Item::beefRaw->shiftedIndex, getRNG(), func_82188_j(0.5f));
		addBlacksmithItem(&candidates, Item::saddle->shiftedIndex, getRNG(), func_82188_j(0.1f));
		addBlacksmithItem(&candidates, Item::plateLeather->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::bootsLeather->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::helmetLeather->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::legsLeather->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::porkCooked->shiftedIndex, getRNG(), func_82188_j(0.3f));
		addBlacksmithItem(&candidates, Item::beefCooked->shiftedIndex, getRNG(), func_82188_j(0.3f));
		break;
	}

	if (candidates.empty() && Item::ingotGold != nullptr)
	{
		addMerchantItem(&candidates, Item::ingotGold->shiftedIndex, getRNG(), 1.0f);
	}

	std::vector<MerchantRecipe *> list;
	for (int i = 0; i < candidates.size(); ++i)
		list.push_back(candidates.get(i));
	for (int i = static_cast<int>(list.size()) - 1; i > 0; --i)
	{
		int j = getRNG().nextInt(i + 1);
		std::swap(list[i], list[j]);
	}

	if (!buyingList)
		buyingList.reset(new MerchantRecipeList());

	for (int i = 0; i < count && i < static_cast<int>(list.size()); ++i)
	{
		MerchantRecipe *r = list[i];
		if (r)
		{
			buyingList->addToListWithCheck(new MerchantRecipe(
				r->getItemToBuy() ? r->getItemToBuy()->copy() : nullptr,
				r->getSecondItemToBuy() ? r->getSecondItemToBuy()->copy() : nullptr,
				r->getItemToSell() ? r->getItemToSell()->copy() : nullptr));
		}
	}
}

void EntityVillager::onDeath(const DamageSource &source)
{
	if (villageObj != nullptr)
	{
		Entity *attacker = source.getEntity();
		if (attacker != nullptr)
		{
			if (auto *player = dynamic_cast<EntityPlayer *>(attacker))
			{
				villageObj->setReputationForPlayer(player->username, -2);
			}
			else if (dynamic_cast<IMob *>(attacker))
			{
				villageObj->endMatingSeason();
			}
		}
		else
		{
			EntityPlayer *closest = worldObj ? worldObj->getClosestPlayerToEntity(this, 16.0) : nullptr;
			if (closest != nullptr)
			{
				villageObj->endMatingSeason();
			}
		}
	}

	EntityAgeable::onDeath(source);
}

void EntityVillager::onDeath(Entity *entity)
{
	if (entity != nullptr)
	{
		onDeath(DamageSource::causeMobDamage(dynamic_cast<EntityLiving *>(entity)));
	}
	else
	{
		onDeath(DamageSource::generic);
	}
}

void EntityVillager::func_82187_q()
{
	field_82190_bM = true;
}

EntityVillager *EntityVillager::func_90012_b(EntityAgeable *)
{
	auto *child = new EntityVillager(worldObj);
	child->initCreature();
	return child;
}

std::string EntityVillager::func_94057_bL() const
{
	return "";
}

bool EntityVillager::canDespawn()
{
	return false;
}

jstring EntityVillager::getLivingSound()
{
	return "mob.villager.default";
}

jstring EntityVillager::getHurtSound()
{
	return "mob.villager.defaulthurt";
}

jstring EntityVillager::getDeathSound()
{
	return "mob.villager.defaultdeath";
}

void EntityVillager::setProfession(int_t profession)
{
	dataWatcher->updateObject(16, profession);
}

int_t EntityVillager::getProfession()
{
	return dataWatcher->getWatchableObjectInt(16);
}

bool EntityVillager::getIsMatingFlag() const
{
	return isMatingFlag;
}

void EntityVillager::setIsMatingFlag(bool value)
{
	isMatingFlag = value;
}

void EntityVillager::setIsPlayingFlag(bool value)
{
	isPlayingFlag = value;
}

bool EntityVillager::getIsPlayingFlag() const
{
	return isPlayingFlag;
}
