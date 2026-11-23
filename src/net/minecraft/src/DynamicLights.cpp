#include "DynamicLights.h"

#include "Block.h"
#include "Entity.h"
#include "EntityCreeper.h"
#include "EntityItem.h"
#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "EntityPlayerSP.h"
#include "EnumSkyBlock.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"
#include "MathHelper.h"
#include "Minecraft.h"
#include "World.h"

#include <algorithm>

namespace
{
#if PLATFORM_XBOX
constexpr int_t kMaxSources = 16;
constexpr int_t kScanIntervalTicks = 3;
#else
constexpr int_t kMaxSources = 64;
constexpr int_t kScanIntervalTicks = 2;
#endif
constexpr int_t kMinLight = 8;

struct Source
{
    Entity *entity;
    int_t x;
    int_t y;
    int_t z;
    int_t light;
};

Source s_active[kMaxSources] = {};
Source s_pending[kMaxSources] = {};
Source s_previous[kMaxSources] = {};
int_t s_scanCountdown = 0;
int_t s_activeCount = 0;
bool s_modEnabled = false;

int_t itemLight(int_t itemID)
{
    switch (itemID)
    {
    case 50:  return 15; // torch
    case 89:  return 12; // glowstone block
    case 91:  return 15; // jack-o-lantern
    case 76:  return 10; // redstone torch
    case 314: return 14; // golden helmet
    case 327: return 15; // lava bucket
    case 331: return 10; // redstone dust
    case 348: return 10; // glowstone dust
    default:  return 0;
    }
}

bool notWaterProof(int_t itemID)
{
    return itemID == 50 || itemID == 327;
}

int_t stackLight(const ItemStack *stack, bool inWater)
{
    if (stack == nullptr || stack->stackSize <= 0)
        return 0;
    const int_t light = itemLight(stack->itemID);
    if (light > 0 && inWater && notWaterProof(stack->itemID))
        return 0;
    return light;
}

int_t entityLight(Entity *entity)
{
    if (entity == nullptr || !entity->isEntityAlive())
        return 0;

    int_t light = entity->isBurning() ? 15 : 0;

    if (EntityPlayer *player = dynamic_cast<EntityPlayer *>(entity))
    {
        if (player->inventory != nullptr)
        {
            light = std::max(light, stackLight(player->inventory->getCurrentItem(), player->isInWater()));
            for (int_t slot = 0; slot < 4; ++slot)
                light = std::max(light, stackLight(player->inventory->armorItemInSlot(slot), false));
        }
    }
    else if (EntityItem *item = dynamic_cast<EntityItem *>(entity))
    {
        light = std::max(light, stackLight(item->getEntityItem(), item->isInWater()));
    }
    else if (EntityCreeper *creeper = dynamic_cast<EntityCreeper *>(entity))
    {
        if (creeper->getCreeperState() == 1)
            light = 15;
    }
    else if (EntityLiving *living = dynamic_cast<EntityLiving *>(entity))
    {
        light = std::max(light, stackLight(living->getHeldItem(), living->isInWater()));
    }

    if (light < kMinLight)
        return 0;
    return std::min<int_t>(light, 15);
}

bool sameSource(const Source &left, const Source &right)
{
    return left.entity == right.entity && left.x == right.x && left.y == right.y &&
           left.z == right.z && left.light == right.light;
}

bool contains(const Source *sources, int_t count, const Source &target)
{
    for (int_t i = 0; i < count; ++i)
        if (sameSource(sources[i], target))
            return true;
    return false;
}

void addPending(Entity *entity, int_t light, int_t &count)
{
    if (entity == nullptr || light < kMinLight || count >= kMaxSources)
        return;
    for (int_t i = 0; i < count; ++i)
        if (s_pending[i].entity == entity)
            return;

    Source &source = s_pending[count++];
    source.entity = entity;
    source.x = MathHelper::floor_double(entity->posX);
    source.y = MathHelper::floor_double(entity->posY);
    source.z = MathHelper::floor_double(entity->posZ);
    source.light = std::min<int_t>(light, 15);
}

int_t collectSources(Minecraft *mc, World *world)
{
    int_t count = 0;
    addPending(mc->thePlayer, entityLight(mc->thePlayer), count);
    if (mc->thePlayer2 != nullptr && mc->thePlayer2 != mc->thePlayer)
        addPending(mc->thePlayer2, entityLight(mc->thePlayer2), count);

    for (Entity *entity : world->loadedEntityList)
    {
        if (count >= kMaxSources)
            break;
        if (entity == mc->thePlayer || entity == mc->thePlayer2)
            continue;
        addPending(entity, entityLight(entity), count);
    }
    return count;
}

void relight(World *world, int_t x, int_t y, int_t z)
{
    if (world != nullptr)
        world->updateLightByType(EnumSkyBlock::Block, x, y, z);
}

void applyPending(World *world, int_t pendingCount)
{
    const int_t previousCount = s_activeCount;
    std::copy(s_active, s_active + previousCount, s_previous);
    std::copy(s_pending, s_pending + pendingCount, s_active);
    s_activeCount = pendingCount;
    DynamicLights::g_sourceCount = pendingCount;

    for (int_t i = 0; i < previousCount; ++i)
        if (!contains(s_active, pendingCount, s_previous[i]))
            relight(world, s_previous[i].x, s_previous[i].y, s_previous[i].z);

    for (int_t i = 0; i < pendingCount; ++i)
        if (!contains(s_previous, previousCount, s_pending[i]))
            relight(world, s_pending[i].x, s_pending[i].y, s_pending[i].z);
}

void clearSources(World *world, bool relightPositions)
{
    const int_t count = s_activeCount;
    std::copy(s_active, s_active + count, s_previous);
    s_activeCount = 0;
    DynamicLights::g_sourceCount = 0;

    if (!relightPositions)
        return;
    for (int_t i = 0; i < count; ++i)
        relight(world, s_previous[i].x, s_previous[i].y, s_previous[i].z);
}
} // namespace

namespace DynamicLights
{
bool g_enabled = false;
World *g_world = nullptr;
int_t g_sourceCount = 0;

void setModEnabled(bool enabled)
{
    s_modEnabled = enabled;
    if (!enabled)
    {
        if (g_world != nullptr && g_sourceCount > 0)
            clearSources(g_world, true);
        g_enabled = false;
        s_scanCountdown = 0;
    }
}

bool isModEnabled()
{
    return s_modEnabled;
}

int_t getLightValue(const World *world, int_t x, int_t y, int_t z)
{
    if (!g_enabled || g_world != world || g_sourceCount <= 0)
        return 0;

    int_t light = 0;
    for (int_t i = 0; i < g_sourceCount; ++i)
    {
        const Source &source = s_active[i];
        if (source.x == x && source.y == y && source.z == z && source.light > light)
            light = source.light;
    }
    return light;
}

void tick(Minecraft *mc)
{
    if (mc == nullptr)
        return;

    World *world = mc->theWorld;
    if (world == nullptr)
    {
        reset();
        return;
    }

    if (g_world != world)
    {
        clearSources(nullptr, false);
        g_world = world;
        g_enabled = false;
        s_scanCountdown = 0;
    }

    if (!s_modEnabled)
    {
        if (g_sourceCount > 0)
            clearSources(world, true);
        g_enabled = false;
        s_scanCountdown = 0;
        return;
    }

    g_enabled = true;
    if (s_scanCountdown > 0)
    {
        --s_scanCountdown;
        return;
    }
    s_scanCountdown = kScanIntervalTicks;

    const int_t pendingCount = collectSources(mc, world);
    applyPending(world, pendingCount);
}

void reset()
{
    s_activeCount = 0;
    g_sourceCount = 0;
    g_world = nullptr;
    g_enabled = false;
    s_scanCountdown = 0;
}
}
