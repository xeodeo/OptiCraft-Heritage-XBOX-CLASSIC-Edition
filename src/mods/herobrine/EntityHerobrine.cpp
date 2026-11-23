#include "EntityHerobrine.h"
#include "HerobrineMod.h"
#include "mods/ModManager.h"

#include "net/minecraft/src/World.h"
#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/Item.h"
#include "net/minecraft/src/EntityPlayer.h"
#include "net/minecraft/src/NBTTagCompound.h"
#include "net/minecraft/src/Vec3D.h"
#include "net/minecraft/src/MathHelper.h"

#include <cmath>

EntityHerobrine::EntityHerobrine(World *world)
    : EntityCreature(world)
    , m_eventMode(EventMode::Ghost)
    , m_heldTool(HeldTool::RedstoneTorch)
    , m_heldStack(nullptr)
    , m_lifeTicks(0)
    , m_stareTicks(0)
    , m_spotted(false)
    , m_lastPlayerDist(-1.0)
{
    ensureEntityInit();
    texture = "/mob/char.png";
    setSize(0.6f, 1.8f);
    moveSpeed = 0.0f;
    setHealth(getMaxHealth());
    setHeldTool(HeldTool::RedstoneTorch);
}

EntityHerobrine::~EntityHerobrine()
{
    delete m_heldStack;
    m_heldStack = nullptr;
}

void EntityHerobrine::setHeldTool(HeldTool tool)
{
    m_heldTool = tool;
    delete m_heldStack;
    m_heldStack = nullptr;
    if (tool == HeldTool::RedstoneTorch && Block::torchRedstoneActive != nullptr)
        m_heldStack = new ItemStack(Block::torchRedstoneActive, 1);
    else if (tool == HeldTool::DiamondShovel && Item::shovelDiamond != nullptr)
        m_heldStack = new ItemStack(Item::shovelDiamond, 1);
    else if (tool == HeldTool::DiamondPickaxe && Item::pickaxeDiamond != nullptr)
        m_heldStack = new ItemStack(Item::pickaxeDiamond, 1);
}

void EntityHerobrine::vanishWithEffects()
{
    if (isDead)
        return;
    if (worldObj != nullptr)
    {
        for (int i = 0; i < 14; ++i)
        {
            double ox = (rand.nextDouble() - 0.5) * 0.9;
            double oy = rand.nextDouble() * 1.8;
            double oz = (rand.nextDouble() - 0.5) * 0.9;
            worldObj->spawnParticle("largesmoke", posX + ox, posY + oy, posZ + oz, 0.0, 0.04, 0.0);
        }
        worldObj->playSoundAtEntity(this, "ambient.cave.cave", 0.55f, 0.8f + rand.nextFloat() * 0.25f);
    }
    setEntityDead();
}

void EntityHerobrine::onLivingUpdate()
{
    if (!ModManager::getInstance().isModEnabled("herobrine"))
        return;

    // Keep Herobrine standing still and immune to fire (e.g. when standing atop the lit altar)
    fire = 0;
    moveForward = 0.0f;
    moveStrafing = 0.0f;
    motionX = 0.0;
    motionZ = 0.0;
    EntityCreature::onLivingUpdate();

    m_lifeTicks++;
    if (worldObj == nullptr)
        return;

    EntityPlayer *player = worldObj->getClosestPlayerToEntity(this, 64.0);
    if (player == nullptr)
    {
        if (m_lifeTicks > 200)
            setEntityDead();
        return;
    }

    // Always turn head and body to stare straight at the player (matching iLoveYou face-tracking)
    const double dx = player->posX - posX;
    const double dy = (player->posY + player->getEyeHeight()) - (posY + getEyeHeight());
    const double dz = player->posZ - posZ;
    const double horizDist = std::sqrt(dx * dx + dz * dz);
    const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float targetYaw = static_cast<float>(std::atan2(dz, dx) * 180.0 / 3.141592653589793) - 90.0f;
    const float targetPitch = static_cast<float>(-(std::atan2(dy, horizDist) * 180.0 / 3.141592653589793));

    rotationYaw = targetYaw;
    rotationYawHead = targetYaw;
    renderYawOffset = targetYaw;
    rotationPitch = targetPitch;

    // Scripted modes (Altar, Digger, Diamond, Thief) have their lifecycle managed by HerobrineMod
    if (m_eventMode == EventMode::Altar ||
        m_eventMode == EventMode::Digger ||
        m_eventMode == EventMode::Diamond)
    {
        if (m_lifeTicks > 2400)
            vanishWithEffects();
        return;
    }

    if (m_eventMode == EventMode::Thief)
    {
        // iLoveYou ThiefEvent: vanishes when player approaches within 10 blocks or after timeout
        if (dist < 10.0 || m_lifeTicks > 1200)
            vanishWithEffects();
        return;
    }

    // Check if the player is looking directly at Herobrine
    bool playerLooking = false;
    Vec3D *look = player->getLook(1.0f);
    if (look != nullptr && dist > 0.01)
    {
        const double toHx = -dx / dist;
        const double toHy = -dy / dist;
        const double toHz = -dz / dist;
        const double dot = look->xCoord * toHx + look->yCoord * toHy + look->zCoord * toHz;
        playerLooking = (dot > 0.92);
    }

    if (m_eventMode == EventMode::Behind)
    {
        if (playerLooking)
        {
            if (!m_spotted)
                m_spotted = true;
            m_stareTicks++;
            if (m_stareTicks >= 16)
            {
                vanishWithEffects();
                return;
            }
        }
        else if (m_spotted || m_lifeTicks > 180 || dist < 3.5 || dist > 36.0)
        {
            vanishWithEffects();
            return;
        }
    }
    else
    {
        // iLoveYou GhostEvent: stands ~17 blocks away with redstone torch; vanishes if player
        // moves meaningfully closer, approaches within 9 blocks, or leaves >50 blocks away.
        if (m_lastPlayerDist < 0.0)
            m_lastPlayerDist = dist;

        if (dist < 9.0 || dist > 50.0)
        {
            vanishWithEffects();
            return;
        }
        if (m_lifeTicks > 25 && dist < m_lastPlayerDist - 1.75)
        {
            vanishWithEffects();
            return;
        }
        if (playerLooking)
        {
            m_stareTicks++;
            if (m_stareTicks >= 50)
            {
                vanishWithEffects();
                return;
            }
        }
        if (m_lifeTicks > 360)
        {
            vanishWithEffects();
            return;
        }
    }
}

bool EntityHerobrine::attackEntityFrom(Entity *entity, int_t damage)
{
    (void)entity;
    (void)damage;
    vanishWithEffects();
    return false;
}

bool EntityHerobrine::attackEntityFrom(const DamageSource &source, int_t damage)
{
    (void)source;
    (void)damage;
    vanishWithEffects();
    return false;
}

void EntityHerobrine::writeEntityToNBT(NBTTagCompound *nbt)
{
    EntityCreature::writeEntityToNBT(nbt);
    nbt->setInteger("EventMode", static_cast<int_t>(m_eventMode));
    nbt->setInteger("HeldTool", static_cast<int_t>(m_heldTool));
    nbt->setInteger("LifeTicks", m_lifeTicks);
}

void EntityHerobrine::readEntityFromNBT(NBTTagCompound *nbt)
{
    EntityCreature::readEntityFromNBT(nbt);
    const int_t mode = nbt->getInteger("EventMode");
    if (mode >= 0 && mode <= 5)
        m_eventMode = static_cast<EventMode>(mode);
    const int_t tool = nbt->getInteger("HeldTool");
    if (tool >= 0 && tool <= 3)
        setHeldTool(static_cast<HeldTool>(tool));
    m_lifeTicks = nbt->getInteger("LifeTicks");
}
