#include "EntityVillager.h"

#include "DataWatcher.h"
#include "EntityAIAvoidEntity.h"
#include "EntityAIFollowGolem.h"
#include "EntityAIMoveIndoors.h"
#include "EntityAIMoveTwardsRestriction.h"
#include "EntityAIOpenDoor.h"
#include "EntityAIPlay.h"
#include "EntityAIRestrictOpenDoor.h"
#include "EntityAISwimming.h"
#include "EntityAIVillagerMate.h"
#include "EntityAITradePlayer.h"
#include "EntityAILookAtTradePlayer.h"
#include "EntityAIWander.h"
#include "EntityAIWatchClosest.h"
#include "EntityAIWatchClosest2.h"
#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "IMerchant.h"
#include "Item.h"
#include "MerchantRecipe.h"
#include "MerchantRecipeList.h"
#include "ItemStack.h"
#include "NBTTagList.h"
#include "Block.h"
#include "Potion.h"
#include "PotionEffect.h"
#include "EntityZombie.h"
#include "NBTTagCompound.h"
#include "MathHelper.h"
#include "PathNavigate.h"
#include "Village.h"
#include "VillageCollection.h"
#include "World.h"

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
	if (buyingList) tag->setCompoundTag("Offers", buyingList->getRecipiesAsTags());
}

EntityVillager::~EntityVillager() = default;

void EntityVillager::readEntityFromNBT(NBTTagCompound *tag)
{
	EntityAgeable::readEntityFromNBT(tag);
	setProfession(tag->getInteger("Profession"));
	wealth = tag->getInteger("Riches");
	if (tag->hasKey("Offers")) buyingList.reset(new MerchantRecipeList(tag->getCompoundTag("Offers")));
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
	if (target == nullptr || worldObj == nullptr || worldObj->villageCollectionObj == nullptr)
		return;
	if (villageObj == nullptr)
		villageObj = worldObj->villageCollectionObj->findNearestVillage(MathHelper::floor_double(posX), MathHelper::floor_double(posY), MathHelper::floor_double(posZ), 32);
	if (villageObj != nullptr)
	{
		villageObj->addOrRenewAgressor(target);
		if (auto *player = dynamic_cast<EntityPlayer *>(target))
		{
			villageObj->setReputationForPlayer(player->username, isChild() ? -3 : -1);
			if (isEntityAlive()) worldObj->setEntityState(this, (byte_t)13);
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
		randomTickDivider = 70 + rand.nextInt(50);
		if (worldObj != nullptr && worldObj->villageCollectionObj != nullptr)
		{
			int_t x = MathHelper::floor_double(posX);
			int_t y = MathHelper::floor_double(posY);
			int_t z = MathHelper::floor_double(posZ);
			worldObj->villageCollectionObj->addVillagerPosition(x, y, z);
			Village *village = worldObj->villageCollectionObj->findNearestVillage(x, y, z, 32);
			villageObj = village;
			if (village == nullptr)
				detachHome();
			else
			{
				ChunkCoordinates center = village->getCenter();
				setHomeArea(center.x, center.y, center.z, (int_t)((float)village->getVillageRadius() * 0.6f));
				if (boostReputation) { boostReputation = false; village->modifyPlayerReputation(5); }
			}
		}
	}
	if (!isTrading() && timeUntilReset > 0 && --timeUntilReset <= 0)
	{
		if (needsInitialization)
		{
			if (buyingList && buyingList->size() > 1)
				for (int i = 0; i < buyingList->size(); ++i)
					if (MerchantRecipe *recipe = buyingList->get(i); recipe && recipe->isRecipeDisabled())
						recipe->increaseMaxTradeUses(getRNG().nextInt(6) + getRNG().nextInt(6) + 2);
			addDefaultEquipmentAndRecipies(1);
			needsInitialization = false;
			if (villageObj && !lastBuyingPlayer.empty())
			{
				worldObj->setEntityState(this, (byte_t)14);
				villageObj->setReputationForPlayer(lastBuyingPlayer, 1);
			}
		}
		addPotionEffect(new PotionEffect(Potion::regeneration->id, 200, 0));
	}
	EntityAgeable::updateAITick();
}

bool EntityVillager::interact(EntityPlayer *player)
{
	ItemStack *held = player ? player->inventory->getCurrentItem() : nullptr;
	if (held && Item::monsterPlacer && held->itemID == Item::monsterPlacer->shiftedIndex) return EntityAgeable::interact(player);
	if (!player || !isEntityAlive() || isTrading() || isChild() || worldObj == nullptr || !worldObj->isDaytime() || health < getMaxHealth())
		return EntityAgeable::interact(player);
	if (!worldObj->multiplayerWorld)
	{
		setCustomer(player);
		player->displayGUIMerchant(this, getRecipes(player));
	}
	return true;
}

void EntityVillager::setCustomer(EntityPlayer *player) { buyingPlayer = player; }
EntityPlayer *EntityVillager::getCustomer() const { return buyingPlayer; }
bool EntityVillager::isTrading() const { return buyingPlayer != nullptr; }
MerchantRecipeList *EntityVillager::getRecipes(EntityPlayer *)
{
	if (!buyingList) addDefaultEquipmentAndRecipies(1);
	return buyingList.get();
}
void EntityVillager::setRecipes(MerchantRecipeList *recipes) { buyingList.reset(recipes); }
void EntityVillager::useRecipe(MerchantRecipe *recipe)
{
	if (!recipe || !buyingList) return;
	recipe->incrementToolUses();
	MerchantRecipe *last = buyingList->get(buyingList->size() - 1);
	if (last && recipe->hasSameIDsAs(*last))
	{
		timeUntilReset = 40;
		needsInitialization = true;
		lastBuyingPlayer = buyingPlayer ? buyingPlayer->username : jstring();
	}
	if (recipe->getItemToBuy() && Item::emerald && recipe->getItemToBuy()->itemID == Item::emerald->shiftedIndex)
		wealth += recipe->getItemToBuy()->stackSize;
}

void EntityVillager::initCreature() { setProfession(worldObj ? worldObj->rand.nextInt(5) : getRNG().nextInt(5)); }

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
		worldObj->spawnParticle(particle, posX + (getRNG().nextFloat() * width * 2.0f) - width,
			posY + 1.0 + getRNG().nextFloat() * height, posZ + (getRNG().nextFloat() * width * 2.0f) - width,
			getRNG().nextGaussian() * 0.02, getRNG().nextGaussian() * 0.02, getRNG().nextGaussian() * 0.02);
}

void EntityVillager::addDefaultEquipmentAndRecipies(int_t count)
{
	if (buyingList) recipeMultiplier = MathHelper::sqrt_float((float)buyingList->size()) * 0.2f;
	else recipeMultiplier = 0.0f;
	auto chance = [this](float base) { float v = base + recipeMultiplier; return v > 0.9f ? 0.9f - (v - 0.9f) : v; };
	auto randomCount = [this](int_t lo, int_t hi) { return lo + getRNG().nextInt(hi - lo); };
	MerchantRecipeList candidates;
	const int emerald = Item::emerald ? Item::emerald->shiftedIndex : -1;
	auto buy = [&](int id, int_t lo, int_t hi, float probability)
	{
		if (id >= 0 && getRNG().nextFloat() < chance(probability))
			candidates.add(new MerchantRecipe(new ItemStack(id, randomCount(lo, hi), 0), new ItemStack(emerald, 1, 0)));
	};
	auto sell = [&](int id, int_t lo, int_t hi, float probability)
	{
		if (id >= 0 && getRNG().nextFloat() < chance(probability))
			candidates.add(new MerchantRecipe(new ItemStack(emerald, randomCount(lo, hi), 0), new ItemStack(id, 1, 0)));
	};
	auto itemId = [](Item *item) { return item ? item->shiftedIndex : -1; };
	auto blockId = [](Block *block) { return block ? block->blockID : -1; };
	switch (getProfession())
	{
	case 0: // farmer
		buy(itemId(Item::wheat), 18, 23, 0.9f); buy(blockId(Block::cloth), 14, 23, 0.5f);
		buy(itemId(Item::chickenRaw), 14, 19, 0.5f); buy(itemId(Item::fishCooked), 9, 14, 0.4f);
		sell(itemId(Item::bread), 2, 5, 0.9f); sell(itemId(Item::melon), 4, 9, 0.3f);
		sell(itemId(Item::appleRed), 4, 9, 0.3f); sell(itemId(Item::cookie), 7, 11, 0.3f);
		sell(itemId(Item::shears), 3, 5, 0.3f); sell(itemId(Item::flintAndSteel), 3, 5, 0.3f);
		break;
	case 1: // librarian
		buy(itemId(Item::paper), 24, 37, 0.8f); buy(itemId(Item::book), 11, 14, 0.8f); buy(itemId(Item::writtenBook), 1, 2, 0.3f);
		sell(blockId(Block::bookShelf), 3, 5, 0.8f); sell(blockId(Block::glass), 3, 6, 0.2f);
		sell(itemId(Item::compass), 10, 13, 0.2f); sell(itemId(Item::pocketSundial), 10, 13, 0.2f);
		break;
	case 2: // priest
		sell(itemId(Item::eyeOfEnder), 7, 12, 0.3f); sell(itemId(Item::expBottle), 1, 5, 0.2f);
		sell(itemId(Item::redstone), 1, 5, 0.4f); sell(itemId(Item::lightStoneDust), 1, 4, 0.3f);
		buy(itemId(Item::rottenFlesh), 36, 65, 0.4f); break;
	case 3: // blacksmith
		buy(itemId(Item::coal), 16, 25, 0.7f); buy(itemId(Item::ingotIron), 8, 11, 0.5f);
		buy(itemId(Item::ingotGold), 8, 11, 0.5f); buy(itemId(Item::diamond), 4, 7, 0.5f);
		sell(itemId(Item::swordSteel), 7, 12, 0.5f); sell(itemId(Item::swordDiamond), 12, 15, 0.5f);
		sell(itemId(Item::pickaxeSteel), 7, 10, 0.5f); sell(itemId(Item::pickaxeDiamond), 10, 13, 0.5f);
		sell(itemId(Item::axeSteel), 6, 9, 0.3f); sell(itemId(Item::axeDiamond), 9, 13, 0.3f);
		sell(itemId(Item::shovelSteel), 4, 7, 0.2f); sell(itemId(Item::plateSteel), 10, 15, 0.2f);
		sell(itemId(Item::plateDiamond), 16, 20, 0.2f); sell(itemId(Item::bootsChain), 5, 8, 0.1f);
		break;
	case 4: // butcher
		buy(itemId(Item::coal), 16, 25, 0.7f); buy(itemId(Item::porkRaw), 14, 19, 0.5f);
		buy(itemId(Item::beefRaw), 14, 19, 0.5f); sell(itemId(Item::saddle), 6, 9, 0.1f);
		sell(itemId(Item::plateLeather), 4, 6, 0.3f); sell(itemId(Item::bootsLeather), 2, 5, 0.3f);
		sell(itemId(Item::porkCooked), 5, 8, 0.3f); sell(itemId(Item::beefCooked), 5, 8, 0.3f);
		break;
	}
	if (candidates.empty()) buy(itemId(Item::ingotGold), 1, 2, 1.0f);
	if (!buyingList) buyingList.reset(new MerchantRecipeList());
	for (int i = 0; i < count && i < candidates.size(); ++i)
	{
		MerchantRecipe *recipe = candidates.get(0);
		// Lists have no remove operation; consume each candidate once by advancing through their indexed view.
		recipe = candidates.get(i);
		if (recipe) buyingList->addToListWithCheck(new MerchantRecipe(recipe->getItemToBuy(), recipe->getSecondItemToBuy(), recipe->getItemToSell()));
	}
}

void EntityVillager::onDeath(Entity *entity)
{
	if (villageObj && entity)
	{
		if (auto *player = dynamic_cast<EntityPlayer *>(entity)) villageObj->setReputationForPlayer(player->username, -2);
		else if (dynamic_cast<EntityLiving *>(entity)) villageObj->endMatingSeason();
	}
	else if (villageObj && worldObj && worldObj->getClosestPlayerToEntity(this, 16.0)) villageObj->endMatingSeason();
	EntityAgeable::onDeath(entity);
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
