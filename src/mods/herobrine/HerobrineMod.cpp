#include "HerobrineMod.h"
#include "EntityHerobrine.h"
#include "GuiHerobrineMenu.h"
#include "mods/ModManager.h"

#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/BlockGrass.h"
#include "net/minecraft/src/BlockLeaves.h"
#include "net/minecraft/src/BlockFlower.h"
#include "net/minecraft/src/Item.h"
#include "net/minecraft/src/ItemStack.h"
#include "net/minecraft/src/EntityItem.h"
#include "net/minecraft/src/EntityCreeper.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/EntityLightningBolt.h"
#include "net/minecraft/src/GuiIngame.h"
#include "net/minecraft/src/TileEntitySign.h"
#include "net/minecraft/src/TileEntityChest.h"
#include "net/minecraft/src/ISaveHandler.h"
#include "net/minecraft/src/StringTranslate.h"
#include "java/File.h"
#include "platform/Storage.h"
#include "platform/Log.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>

namespace
{
HerobrineMod *s_herobrineInstance = nullptr;

bool hbIsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);
}

bool isRedstoneTorchBlock(int_t blockId)
{
    return (Block::torchRedstoneActive != nullptr && blockId == Block::torchRedstoneActive->blockID) ||
           (Block::torchRedstoneIdle != nullptr && blockId == Block::torchRedstoneIdle->blockID);
}

bool isPlayerBuiltBlock(int_t id)
{
    if (id <= 0) return false;
    return id == 4 || id == 5 || id == 20 || id == 23 || id == 25 || id == 26 ||
           id == 35 || id == 41 || id == 42 || id == 43 || id == 44 || id == 45 ||
           id == 46 || id == 47 || id == 50 || id == 53 || id == 54 || id == 57 ||
           id == 58 || id == 61 || id == 62 || id == 63 || id == 64 || id == 65 ||
           id == 67 || id == 68 || id == 71 || id == 84 || id == 85 || id == 98 ||
           id == 102 || id == 107 || id == 108 || id == 109 || id == 116 || id == 130;
}

bool isNaturalDiggable(int_t id)
{
    if (id == 0) return true;
    if (id == 7 || id == 8 || id == 9 || id == 10 || id == 11 || id == 56) // bedrock, liquids, diamond ore
        return false;
    return !isPlayerBuiltBlock(id);
}

std::string pickFromTable(World *world, const char *const *enLines, const char *const *esLines, int count)
{
    const int idx = (world != nullptr && count > 0) ? world->rand.nextInt(count) : 0;
    return hbIsSpanish() ? esLines[idx] : enLines[idx];
}
} // namespace

HerobrineMod::HerobrineMod()
    : m_mc(nullptr)
    , m_enabled(false)
    , m_summoned(false)
    , m_altarX(0)
    , m_altarY(0)
    , m_altarZ(0)
    , m_sightings(0)
    , m_mythEvents(0)
    , m_wheelCooldown(600)
    , m_soundCooldown(400)
    , m_extraCooldown(1600)
{
    s_herobrineInstance = this;
}

HerobrineMod *HerobrineMod::getInstance()
{
    return s_herobrineInstance;
}

void HerobrineMod::setEnabled(bool state)
{
    m_enabled = state;
    if (!state)
    {
        m_altarRitual.active = false;
        m_digger.active = false;
        m_diamond.active = false;
    }
}

GuiScreen *HerobrineMod::createConfigScreen(GuiScreen *parent)
{
    return new GuiHerobrineMenu(parent);
}

void HerobrineMod::onInit(Minecraft *mc)
{
    m_mc = mc;
    s_herobrineInstance = this;
    loadConfig();
}

void HerobrineMod::sendHerobrineChat(const std::string &text)
{
    if (m_mc != nullptr && m_mc->ingameGUI != nullptr && !text.empty())
        m_mc->ingameGUI->addChatMessage("<Herobrine> " + text);
}

std::string HerobrineMod::pickAppearanceLine(World *world) const
{
    // Exact AltarEvent.appearance() message set from tkelly's iLoveYou (EN & ES)
    static const char *const EN[] = {
        "I am here.",
        "Why would you do that?",
        "You are so foolish.",
        "Only god can help you now.",
        "This is mine.",
        "ha",
        "Stop."
    };
    static const char *const ES[] = {
        "Yo estoy aqui.",
        "Por que hiciste eso?",
        "Eres tan tonto.",
        "Solo Dios puede ayudarte ahora.",
        "Esto es mio.",
        "ja",
        "Alto."
    };
    return pickFromTable(world, EN, ES, 7);
}

bool HerobrineMod::checkAltarStructure(World *world, int_t fireX, int_t fireY, int_t fireZ)
{
    if (world == nullptr || Block::netherrack == nullptr || Block::blockGold == nullptr || Block::cobblestoneMossy == nullptr)
        return false;

    const int_t netherId = Block::netherrack->blockID;
    const int_t goldId = Block::blockGold->blockID;
    const int_t mossyId = Block::cobblestoneMossy->blockID;

    // Netherrack directly beneath the fire: (nx, ny, nz) = (fireX, fireY - 1, fireZ)
    const int_t nx = fireX;
    const int_t ny = fireY - 1;
    const int_t nz = fireZ;
    if (world->getBlockId(nx, ny, nz) != netherId)
        return false;

    // CANONICAL tkelly iLoveYou AltarEvent.checkForAltar():
    //   y-1 (ny-1): Mossy Cobblestone at center (nx, ny-1, nz), surrounded by 8 Gold Blocks
    //               at (nx+-1, ny-1, nz+-1) (3x3 gold ring around mossy center).
    //   y   (ny)  : Netherrack at (nx, ny, nz) with 4 Redstone Torches on the 4 horizontal
    //               sides (nx+-1, ny, nz) and (nx, ny, nz+-1).
    //   y+1 (ny+1): Fire on top of Netherrack.
    if (world->getBlockId(nx, ny - 1, nz) == mossyId)
    {
        const bool torchesAtNether =
            isRedstoneTorchBlock(world->getBlockId(nx + 1, ny, nz)) &&
            isRedstoneTorchBlock(world->getBlockId(nx - 1, ny, nz)) &&
            isRedstoneTorchBlock(world->getBlockId(nx, ny, nz + 1)) &&
            isRedstoneTorchBlock(world->getBlockId(nx, ny, nz - 1));
        if (torchesAtNether)
        {
            bool goldRing = true;
            for (int_t dx = -1; dx <= 1 && goldRing; ++dx)
            {
                for (int_t dz = -1; dz <= 1 && goldRing; ++dz)
                {
                    if (dx == 0 && dz == 0)
                        continue;
                    if (world->getBlockId(nx + dx, ny - 1, nz + dz) != goldId)
                        goldRing = false;
                }
            }
            if (goldRing)
                return true;
        }
    }

    return false;
}

bool HerobrineMod::onFirePlaced(World *world, int_t x, int_t y, int_t z)
{
    HerobrineMod *mod = getInstance();
    if (mod == nullptr || !mod->isEnabled() || world == nullptr || world->multiplayerWorld)
        return false;

    if (!checkAltarStructure(world, x, y, z))
        return false;

    if (mod->m_config.legendExtras)
        world->addWeatherEffect(new EntityLightningBolt(world, x + 0.5, y, z + 0.5));
    world->playSoundEffect(x + 0.5, y + 0.5, z + 0.5, "ambient.cave.cave", 1.0f, 0.7f);

    mod->m_summoned = true;
    mod->m_altarX = x;
    mod->m_altarY = y - 1; // netherrack block Y
    mod->m_altarZ = z;
    mod->m_wheelCooldown = 600;
    mod->m_soundCooldown = 260;
    mod->m_extraCooldown = 1400;
    mod->saveState();

    // Start faithful tkelly iLoveYou AltarEvent sequence
    if (mod->m_config.altarRitual)
    {
        mod->m_altarRitual.active = true;
        mod->m_altarRitual.x = x;
        mod->m_altarRitual.y = y - 1;
        mod->m_altarRitual.z = z;
        mod->m_altarRitual.ticks = 0;
        mod->m_altarRitual.entityId = -1;
    }

    MC_LOG_INFO("mods", "Herobrine (iLoveYou) altar ignited at (%d, %d, %d)\n",
                static_cast<int>(x), static_cast<int>(y - 1), static_cast<int>(z));
    return true;
}

void HerobrineMod::breakTorchesInRadius(World *world, int_t cx, int_t cy, int_t cz, int_t radius)
{
    if (world == nullptr || Block::torchWood == nullptr)
        return;
    const int_t torchId = Block::torchWood->blockID;
    const int_t yMin = std::max<int_t>(1, cy - radius);
    const int_t yMax = std::min<int_t>(126, cy + radius);
    int_t broken = 0;
    for (int_t x = cx - radius; x <= cx + radius; ++x)
    {
        for (int_t z = cz - radius; z <= cz + radius; ++z)
        {
            if (!world->blockExists(x, cy, z))
                continue;
            for (int_t y = yMin; y <= yMax; ++y)
            {
                if (world->getBlockId(x, y, z) == torchId)
                {
                    world->setBlockWithNotify(x, y, z, 0);
                    ItemStack *drop = new ItemStack(Block::torchWood, 1);
                    EntityItem *ei = new EntityItem(world, x + 0.5, y + 0.25, z + 0.5, drop);
                    world->entityJoinedWorld(ei);
                    if (++broken >= 64)
                        return;
                }
            }
        }
    }
}

void HerobrineMod::deleteAltarBlocks(World *world, int_t nx, int_t ny, int_t nz)
{
    if (world == nullptr)
        return;
    // Exact AltarEvent.deleteAltar() cleanup: fire, netherrack, 4 redstone torches, 3x3 base
    world->setBlockWithNotify(nx, ny + 1, nz, 0);
    world->setBlockWithNotify(nx, ny, nz, 0);
    world->setBlockWithNotify(nx + 1, ny, nz, 0);
    world->setBlockWithNotify(nx - 1, ny, nz, 0);
    world->setBlockWithNotify(nx, ny, nz + 1, 0);
    world->setBlockWithNotify(nx, ny, nz - 1, 0);
    for (int_t dx = -1; dx <= 1; ++dx)
        for (int_t dz = -1; dz <= 1; ++dz)
            world->setBlockWithNotify(nx + dx, ny - 1, nz + dz, 0);
}

void HerobrineMod::tickAltarRitual(World *world, EntityPlayer *player)
{
    if (!m_altarRitual.active || world == nullptr || player == nullptr)
        return;

    m_altarRitual.ticks++;
    const int_t t = m_altarRitual.ticks;
    const int_t ax = m_altarRitual.x;
    const int_t ay = m_altarRitual.y;
    const int_t az = m_altarRitual.z;

    // Exact tkelly AltarEvent.AltarEvents timeline (20 ticks/sec):
    //   t=100 (5s):  set time to night (14000)
    //   t=140 (+2s): break torches radius 5
    //   t=160 (+1s): break torches radius 10
    //   t=180 (+1s): break torches radius 20
    //   t=240 (+3s): spawn Herobrine atop altar with Redstone Torch + HeroLanguage appearance chat
    //   t=340 (+5s): curse: fire 200 ticks, re-lit while the player moves (option, ON)
    //   t=540 (+10s): vanish Herobrine & deleteAltar() (with the curse option)
    if (t == 100)
    {
        world->setWorldTime(14000);
        world->playSoundEffect(ax + 0.5, ay + 1.0, az + 0.5, "ambient.cave.cave", 0.9f, 0.65f);
    }
    else if (t == 140)
    {
        breakTorchesInRadius(world, ax, ay, az, 5);
    }
    else if (t == 160)
    {
        breakTorchesInRadius(world, ax, ay, az, 10);
    }
    else if (t == 180)
    {
        breakTorchesInRadius(world, ax, ay, az, 20);
    }
    else if (t == 240)
    {
        // Extinguish fire on top of netherrack right as Herobrine steps onto the altar
        world->setBlockWithNotify(ax, ay + 1, az, 0);
        EntityHerobrine *hb = new EntityHerobrine(world);
        hb->setEventMode(EntityHerobrine::EventMode::Altar);
        hb->setHeldTool(EntityHerobrine::HeldTool::RedstoneTorch);
        hb->setLocationAndAngles(ax + 0.5, static_cast<double>(ay + 1), az + 0.5, player->rotationYaw + 180.0f, 0.0f);
        if (world->entityJoinedWorld(hb))
        {
            m_altarRitual.entityId = hb->entityId;
            m_sightings++;
            saveState();
            sendHerobrineChat(pickAppearanceLine(world));
        }
        else
        {
            delete hb;
        }
    }
    else if (t >= 340 && t < 540)
    {
        // Original AltarEvent: setFireTicks(200) when the curse starts, and again every
        // time the cursed player moves (stalkPlayer) until the altar is deleted.
        if (m_config.altarCurse)
        {
            const bool moved = t == 340 ||
                std::fabs(player->posX - player->prevPosX) > 0.01 ||
                std::fabs(player->posZ - player->prevPosZ) > 0.01;
            if (moved && player->fire < 200)
                player->fire = 200;
        }
    }
    else if (t >= 540)
    {
        if (EntityHerobrine *hb = findHerobrineById(world, m_altarRitual.entityId))
            hb->vanishWithEffects();
        if (m_config.altarCurse)
            deleteAltarBlocks(world, ax, ay, az);
        m_altarRitual.active = false;
        m_altarRitual.entityId = -1;
    }
}

bool HerobrineMod::hasActiveHerobrine(World *world) const
{
    if (world == nullptr)
        return false;
    for (Entity *ent : world->loadedEntityList)
    {
        if (ent != nullptr && !ent->isDead && dynamic_cast<EntityHerobrine *>(ent) != nullptr)
            return true;
    }
    return false;
}

EntityHerobrine *HerobrineMod::findHerobrineById(World *world, int_t entityId) const
{
    if (world == nullptr || entityId < 0)
        return nullptr;
    for (Entity *ent : world->loadedEntityList)
    {
        if (ent != nullptr && !ent->isDead && ent->entityId == entityId)
            return dynamic_cast<EntityHerobrine *>(ent);
    }
    return nullptr;
}

EntityHerobrine *HerobrineMod::spawnGhostNear(World *world, EntityPlayer *player, bool behind)
{
    if (world == nullptr || player == nullptr || world->multiplayerWorld)
        return nullptr;

    const float yawRad = player->rotationYaw * 3.14159265f / 180.0f;
    const float fwdX = -std::sin(yawRad);
    const float fwdZ = std::cos(yawRad);

    for (int attempt = 0; attempt < 16; ++attempt)
    {
        float angleOffset = 0.0f;
        float dist = 17.0f; // Exact iLoveYou GhostEvent ~17 block distance
        if (behind)
        {
            angleOffset = 3.14159265f + (world->rand.nextFloat() - 0.5f) * 0.7f;
            dist = 7.5f + world->rand.nextFloat() * 4.0f;
        }
        else
        {
            angleOffset = (world->rand.nextFloat() - 0.5f) * 1.1f;
            dist = 15.0f + world->rand.nextFloat() * 6.0f;
        }

        const float cosA = std::cos(angleOffset);
        const float sinA = std::sin(angleOffset);
        const float dirX = fwdX * cosA - fwdZ * sinA;
        const float dirZ = fwdX * sinA + fwdZ * cosA;

        const int_t sx = static_cast<int_t>(std::floor(player->posX + dirX * dist));
        const int_t sz = static_cast<int_t>(std::floor(player->posZ + dirZ * dist));
        if (!world->blockExists(sx, 64, sz))
            continue;

        int_t sy = world->getTopSolidOrLiquidBlock(sx, sz);
        if (sy < 4 || sy > 122)
            continue;

        const int_t groundId = world->getBlockId(sx, sy - 1, sz);
        if (groundId <= 0 || groundId == 8 || groundId == 9 || groundId == 10 || groundId == 11)
            continue;
        if (!world->isAirBlock(sx, sy, sz) || !world->isAirBlock(sx, sy + 1, sz))
            continue;

        EntityHerobrine *hb = new EntityHerobrine(world);
        hb->setEventMode(behind ? EntityHerobrine::EventMode::Behind : EntityHerobrine::EventMode::Ghost);
        hb->setHeldTool(EntityHerobrine::HeldTool::RedstoneTorch);
        hb->setLocationAndAngles(sx + 0.5, static_cast<double>(sy), sz + 0.5, player->rotationYaw + 180.0f, 0.0f);
        if (world->entityJoinedWorld(hb))
        {
            m_sightings++;
            saveState();
            MC_LOG_INFO("mods", "Herobrine GhostEvent (%s) at (%d,%d,%d)\n",
                        behind ? "behind" : "ghost", static_cast<int>(sx), static_cast<int>(sy), static_cast<int>(sz));
            return hb;
        }
        delete hb;
    }
    return nullptr;
}

bool HerobrineMod::startDiggerEvent(World *world, EntityPlayer *player)
{
    if (!m_config.diggerEvent || m_digger.active || world == nullptr || player == nullptr)
        return false;

    const float yawRad = player->rotationYaw * 3.14159265f / 180.0f;
    const float fwdX = -std::sin(yawRad);
    const float fwdZ = std::cos(yawRad);

    const int_t sx = static_cast<int_t>(std::floor(player->posX + fwdX * 17.0f));
    const int_t sz = static_cast<int_t>(std::floor(player->posZ + fwdZ * 17.0f));
    if (!world->blockExists(sx, 64, sz))
        return false;
    const int_t sy = world->getTopSolidOrLiquidBlock(sx, sz);
    if (sy < 18 || sy > 110 || !isAreaSafeFromBuilds(world, sx, sy, sz))
        return false;

    EntityHerobrine *hb = new EntityHerobrine(world);
    hb->setEventMode(EntityHerobrine::EventMode::Digger);
    hb->setHeldTool(EntityHerobrine::HeldTool::DiamondShovel);
    hb->setLocationAndAngles(sx + 0.5, static_cast<double>(sy), sz + 0.5, player->rotationYaw + 180.0f, 0.0f);
    if (!world->entityJoinedWorld(hb))
    {
        delete hb;
        return false;
    }

    m_digger.active = true;
    m_digger.hx = sx;
    m_digger.hy = sy;
    m_digger.hz = sz;
    m_digger.dirX = (std::fabs(fwdX) >= std::fabs(fwdZ)) ? (fwdX >= 0 ? 1 : -1) : 0;
    m_digger.dirZ = (m_digger.dirX == 0) ? (fwdZ >= 0 ? 1 : -1) : 0;
    m_digger.steps = 0;
    m_digger.ticks = 0;
    m_digger.entityId = hb->entityId;
    m_digger.warned = false;

    static const char *const EN[] = { "Stop.", "You should not be here.", "Help me!", "...", "What are you doing?", "You can not do that." };
    static const char *const ES[] = { "Alto.", "No deberias estar aqui.", "Ayudame!", "...", "Que estas haciendo?", "No puedes hacer eso." };
    sendHerobrineChat(pickFromTable(world, EN, ES, 6));
    m_sightings++;
    saveState();
    return true;
}

void HerobrineMod::buildDiggerRoom(World *world, int_t cx, int_t cy, int_t cz)
{
    if (world == nullptr || Block::cobblestoneMossy == nullptr || Block::gravel == nullptr)
        return;

    // Iconic iLoveYou HeroDigger underground room: mossy cobble walls, gravel floor,
    // workbench, loot chest, redstone torches, and a creeper.
    const int_t mossy = Block::cobblestoneMossy->blockID;
    const int_t gravel = Block::gravel->blockID;
    for (int_t dx = -3; dx <= 3; ++dx)
    {
        for (int_t dz = -3; dz <= 3; ++dz)
        {
            for (int_t dy = 0; dy <= 4; ++dy)
            {
                const int_t bx = cx + dx, by = cy + dy, bz = cz + dz;
                if (!isNaturalDiggable(world->getBlockId(bx, by, bz)))
                    continue;
                if (dy == 0)
                    world->setBlockWithNotify(bx, by, bz, gravel);
                else if (dy == 4 || dx == -3 || dx == 3 || dz == -3 || dz == 3)
                    world->setBlockWithNotify(bx, by, bz, mossy);
                else
                    world->setBlockWithNotify(bx, by, bz, 0);
            }
        }
    }
    if (Block::workbench != nullptr)
        world->setBlockWithNotify(cx - 2, cy + 1, cz - 2, Block::workbench->blockID);
    if (Block::torchRedstoneActive != nullptr)
    {
        world->setBlockAndMetadataWithNotify(cx + 2, cy + 1, cz - 2, Block::torchRedstoneActive->blockID, 5);
        world->setBlockAndMetadataWithNotify(cx - 2, cy + 1, cz + 2, Block::torchRedstoneActive->blockID, 5);
    }
    if (Block::chest != nullptr)
    {
        world->setBlockWithNotify(cx, cy + 1, cz + 2, Block::chest->blockID);
        TileEntityChest *chest = dynamic_cast<TileEntityChest *>(world->getBlockTileEntity(cx, cy + 1, cz + 2));
        if (chest != nullptr)
        {
            if (Item::fishRaw) chest->setInventorySlotContents(0, new ItemStack(Item::fishRaw, 2));
            if (Item::ingotIron) chest->setInventorySlotContents(1, new ItemStack(Item::ingotIron, 2));
            if (Item::diamond) chest->setInventorySlotContents(2, new ItemStack(Item::diamond, 1));
            if (Item::gunpowder) chest->setInventorySlotContents(3, new ItemStack(Item::gunpowder, 3));
            if (Item::stick) chest->setInventorySlotContents(4, new ItemStack(Item::stick, 4));
            if (Block::torchRedstoneActive) chest->setInventorySlotContents(5, new ItemStack(Block::torchRedstoneActive, 2));
            if (Item::coal) chest->setInventorySlotContents(6, new ItemStack(Item::coal, 4));
            if (Block::plantRed) chest->setInventorySlotContents(7, new ItemStack(Block::plantRed, 1));
        }
    }
    EntityCreeper *creeper = new EntityCreeper(world);
    creeper->setLocationAndAngles(cx + 1.5, static_cast<double>(cy + 1), cz + 0.5, 0.0f, 0.0f);
    world->entityJoinedWorld(creeper);
}

void HerobrineMod::tickDiggerEvent(World *world, EntityPlayer *player)
{
    if (!m_digger.active || world == nullptr || player == nullptr)
        return;

    EntityHerobrine *hb = findHerobrineById(world, m_digger.entityId);
    if (hb == nullptr)
    {
        m_digger.active = false;
        return;
    }

    m_digger.ticks++;
    if (m_digger.ticks > 1600)
    {
        hb->vanishWithEffects();
        m_digger.active = false;
        return;
    }

    const double dx = player->posX - hb->posX;
    const double dy = player->posY - hb->posY;
    const double dz = player->posZ - hb->posZ;
    const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist > 48.0)
    {
        hb->vanishWithEffects();
        m_digger.active = false;
        return;
    }

    // Advance tunnel one step every 8 ticks when player chases within 14 blocks
    if (dist < 14.0 && (m_digger.ticks % 8) == 0)
    {
        if (!m_digger.warned && m_digger.steps >= 3)
        {
            m_digger.warned = true;
            static const char *const EN[] = { "Stop following me!", "I know you are following me.", "No.", "Stop it.", "You can not catch me.", "haha" };
            static const char *const ES[] = { "Deja de seguirme!", "Se que me estas siguiendo.", "No.", "Basta.", "No puedes atraparme.", "jaja" };
            sendHerobrineChat(pickFromTable(world, EN, ES, 6));
        }

        const int_t nx = m_digger.hx + m_digger.dirX;
        const int_t ny = std::max<int_t>(8, m_digger.hy - 1);
        const int_t nz = m_digger.hz + m_digger.dirZ;

        if (!isNaturalDiggable(world->getBlockId(nx, ny, nz)) ||
            !isNaturalDiggable(world->getBlockId(nx, ny + 1, nz)) ||
            !isNaturalDiggable(world->getBlockId(nx, ny + 2, nz)))
        {
            hb->vanishWithEffects();
            m_digger.active = false;
            return;
        }

        world->setBlockWithNotify(nx, ny, nz, 0);
        world->setBlockWithNotify(nx, ny + 1, nz, 0);
        world->setBlockWithNotify(nx, ny + 2, nz, 0);

        if (Block::torchRedstoneActive != nullptr && world->rand.nextInt(10) == 0)
            world->setBlockAndMetadataWithNotify(m_digger.hx, m_digger.hy, m_digger.hz, Block::torchRedstoneActive->blockID, 5);

        m_digger.hx = nx;
        m_digger.hy = ny;
        m_digger.hz = nz;
        m_digger.steps++;
        hb->setPosition(nx + 0.5, static_cast<double>(ny), nz + 0.5);

        if (m_digger.hy <= 10 || m_digger.steps >= 24)
        {
            buildDiggerRoom(world, m_digger.hx, m_digger.hy, m_digger.hz);
            hb->vanishWithEffects();
            m_digger.active = false;
            m_mythEvents++;
            saveState();
        }
    }
}

bool HerobrineMod::startDiamondEvent(World *world, EntityPlayer *player)
{
    if (!m_config.diamondEvent || m_diamond.active || world == nullptr || player == nullptr || Block::oreDiamond == nullptr)
        return false;

    const int_t px = static_cast<int_t>(std::floor(player->posX));
    const int_t pz = static_cast<int_t>(std::floor(player->posZ));
    const int_t diaId = Block::oreDiamond->blockID;

    // Sample candidate columns within radius 20 for natural diamond ore (Y 4..16)
    bool found = false;
    int_t tx = 0, ty = 0, tz = 0;
    for (int attempt = 0; attempt < 48 && !found; ++attempt)
    {
        const int_t cx = px + world->rand.nextInt(41) - 20;
        const int_t cz = pz + world->rand.nextInt(41) - 20;
        if (!world->blockExists(cx, 12, cz))
            continue;
        for (int_t cy = 4; cy <= 16; ++cy)
        {
            if (world->getBlockId(cx, cy, cz) == diaId)
            {
                tx = cx;
                ty = cy;
                tz = cz;
                found = true;
                break;
            }
        }
    }
    if (!found)
        return false;

    const int_t topY = world->getTopSolidOrLiquidBlock(tx, tz);
    if (topY < ty + 8 || !isAreaSafeFromBuilds(world, tx, topY, tz))
        return false;

    EntityHerobrine *hb = new EntityHerobrine(world);
    hb->setEventMode(EntityHerobrine::EventMode::Diamond);
    hb->setHeldTool(EntityHerobrine::HeldTool::DiamondPickaxe);
    hb->setLocationAndAngles(tx + 0.5, static_cast<double>(topY), tz + 0.5, player->rotationYaw + 180.0f, 0.0f);
    if (!world->entityJoinedWorld(hb))
    {
        delete hb;
        return false;
    }

    m_diamond.active = true;
    m_diamond.targetX = tx;
    m_diamond.targetY = ty;
    m_diamond.targetZ = tz;
    m_diamond.curY = topY;
    m_diamond.ticks = 0;
    m_diamond.phase = 0;
    m_diamond.entityId = hb->entityId;
    m_diamond.dugBlocks.clear();

    static const char *const EN[] = { "Follow me.", "I have found something!", "I have something for you.", "Diamonds", "This is mine.", "Help me!" };
    static const char *const ES[] = { "Sigueme.", "He encontrado algo!", "Tengo algo para ti.", "Diamantes", "Esto es mio.", "Ayudame!" };
    sendHerobrineChat(pickFromTable(world, EN, ES, 6));
    m_sightings++;
    saveState();
    return true;
}

void HerobrineMod::tickDiamondEvent(World *world, EntityPlayer *player)
{
    if (!m_diamond.active || world == nullptr || player == nullptr)
        return;

    m_diamond.ticks++;

    // Phase 2: BlockRestorer (restores dug shaft blocks one-by-one every 10 ticks)
    if (m_diamond.phase == 2)
    {
        if ((m_diamond.ticks % 10) == 0)
        {
            if (m_diamond.dugBlocks.empty())
            {
                m_diamond.active = false;
                return;
            }
            const SavedBlock sb = m_diamond.dugBlocks.back();
            m_diamond.dugBlocks.pop_back();
            world->setBlockAndMetadataWithNotify(sb.x, sb.y, sb.z, sb.blockId, sb.meta);
        }
        return;
    }

    EntityHerobrine *hb = findHerobrineById(world, m_diamond.entityId);
    if (hb == nullptr || m_diamond.ticks > 1800)
    {
        if (hb != nullptr) hb->vanishWithEffects();
        m_diamond.phase = 2;
        return;
    }

    const double dx = player->posX - hb->posX;
    const double dy = player->posY - hb->posY;
    const double dz = player->posZ - hb->posZ;
    const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (m_diamond.phase == 0)
    {
        if (dist < 14.0 && (m_diamond.ticks % 6) == 0)
        {
            const int_t ny = m_diamond.curY - 1;
            if (ny <= m_diamond.targetY + 1)
            {
                m_diamond.phase = 1;
                return;
            }
            const int_t id = world->getBlockId(m_diamond.targetX, ny, m_diamond.targetZ);
            const int_t meta = world->getBlockMetadata(m_diamond.targetX, ny, m_diamond.targetZ);
            if (!isNaturalDiggable(id))
            {
                hb->vanishWithEffects();
                m_diamond.phase = 2;
                return;
            }
            m_diamond.dugBlocks.push_back({ m_diamond.targetX, ny, m_diamond.targetZ, id, meta });
            if (Block::ladder != nullptr)
                world->setBlockAndMetadataWithNotify(m_diamond.targetX, ny, m_diamond.targetZ, Block::ladder->blockID, 2);
            else
                world->setBlockWithNotify(m_diamond.targetX, ny, m_diamond.targetZ, 0);

            m_diamond.curY = ny;
            hb->setPosition(m_diamond.targetX + 0.5, static_cast<double>(ny), m_diamond.targetZ + 0.5);
        }
    }
    else if (m_diamond.phase == 1)
    {
        // Wait at the diamond ore until player gets within 3.5 blocks, then say farewell, vanish & seal shaft!
        if (dist < 3.5)
        {
            static const char *const EN[] = { "haha", "Your greed has betrayed you.", "Did you find what you wanted?", "Diamonds", "Looking for these?", "Only god can help you now." };
            static const char *const ES[] = { "jaja", "Tu avaricia te ha traicionado.", "Encontraste lo que buscabas?", "Diamantes", "Buscas esto?", "Solo Dios puede ayudarte ahora." };
            sendHerobrineChat(pickFromTable(world, EN, ES, 6));
            hb->vanishWithEffects();
            m_diamond.phase = 2;
            m_mythEvents++;
            saveState();
        }
    }
}

bool HerobrineMod::triggerThiefEvent(World *world, EntityPlayer *player)
{
    if (!m_config.thiefEvent || world == nullptr || player == nullptr)
        return false;

    for (TileEntity *te : world->loadedTileEntityList)
    {
        TileEntityChest *chest = dynamic_cast<TileEntityChest *>(te);
        if (chest == nullptr)
            continue;
        const double dx = (chest->xCoord + 0.5) - player->posX;
        const double dy = (chest->yCoord + 0.5) - player->posY;
        const double dz = (chest->zCoord + 0.5) - player->posZ;
        const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        // iLoveYou ThiefEvent: player must be >20 blocks away from chest
        if (dist < 20.0 || dist > 55.0)
            continue;
        if (!world->isAirBlock(chest->xCoord, chest->yCoord + 1, chest->zCoord) ||
            !world->isAirBlock(chest->xCoord, chest->yCoord + 2, chest->zCoord))
            continue;

        bool hasAnyItem = false;
        for (int_t i = 0; i < chest->getSizeInventory(); ++i)
        {
            if (chest->getStackInSlot(i) != nullptr)
            {
                hasAnyItem = true;
                break;
            }
        }
        if (!hasAnyItem)
            continue;

        // Spawn Herobrine atop chest with Redstone Torch (vanishes if player approaches <10 blocks)
        EntityHerobrine *hb = new EntityHerobrine(world);
        hb->setEventMode(EntityHerobrine::EventMode::Thief);
        hb->setHeldTool(EntityHerobrine::HeldTool::RedstoneTorch);
        hb->setLocationAndAngles(chest->xCoord + 0.5, static_cast<double>(chest->yCoord + 1), chest->zCoord + 0.5, 0.0f, 0.0f);
        if (!world->entityJoinedWorld(hb))
        {
            delete hb;
            return false;
        }

        // Exact iLoveYou ThiefEvent probabilities: 20% drop item, 30% steal item
        for (int_t i = 0; i < chest->getSizeInventory(); ++i)
        {
            ItemStack *st = chest->getStackInSlot(i);
            if (st == nullptr)
                continue;
            const int roll = world->rand.nextInt(100);
            if (roll < 20)
            {
                ItemStack *copy = st->copy();
                chest->setInventorySlotContents(i, nullptr);
                EntityItem *ei = new EntityItem(world, chest->xCoord + 0.5, chest->yCoord + 1.1, chest->zCoord + 0.5, copy);
                world->entityJoinedWorld(ei);
            }
            else if (roll < 50)
            {
                chest->setInventorySlotContents(i, nullptr);
            }
        }
        m_sightings++;
        m_mythEvents++;
        saveState();
        MC_LOG_INFO("mods", "Herobrine ThiefEvent at chest (%d,%d,%d)\n",
                    static_cast<int>(chest->xCoord), static_cast<int>(chest->yCoord), static_cast<int>(chest->zCoord));
        return true;
    }
    return false;
}

void HerobrineMod::spinEventWheel(World *world, EntityPlayer *player)
{
    if (world == nullptr || player == nullptr || hasActiveHerobrine(world))
        return;

    // Exact iLoveYou WheelSpinner weights:
    //   GhostEvent=2.0, DiamondEvent=2.0, HeroDigger=1.0, ThiefEvent=1.0
    float totalWeight = 0.0f;
    if (m_config.ghostEvent) totalWeight += 2.0f;
    if (m_config.diamondEvent) totalWeight += 2.0f;
    if (m_config.diggerEvent) totalWeight += 1.0f;
    if (m_config.thiefEvent) totalWeight += 1.0f;
    if (totalWeight <= 0.0f)
        return;

    float r = world->rand.nextFloat() * totalWeight;
    if (m_config.ghostEvent)
    {
        if (r < 2.0f)
        {
            const bool behind = (world->rand.nextInt(5) == 0);
            spawnGhostNear(world, player, behind);
            return;
        }
        r -= 2.0f;
    }
    if (m_config.diamondEvent)
    {
        if (r < 2.0f)
        {
            if (startDiamondEvent(world, player))
                return;
        }
        r -= 2.0f;
    }
    if (m_config.diggerEvent)
    {
        if (r < 1.0f)
        {
            if (startDiggerEvent(world, player))
                return;
        }
        r -= 1.0f;
    }
    if (m_config.thiefEvent)
    {
        if (triggerThiefEvent(world, player))
            return;
    }
    // Fallback to GhostEvent if the chosen contextual event had no valid target
    if (m_config.ghostEvent)
        spawnGhostNear(world, player, false);
}

void HerobrineMod::playEerieSound(World *world, EntityPlayer *player)
{
    if (!m_config.legendExtras || world == nullptr || player == nullptr)
        return;
    const float angle = world->rand.nextFloat() * 6.283185f;
    const float dist = 10.0f + world->rand.nextFloat() * 10.0f;
    const double sx = player->posX + std::cos(angle) * dist;
    const double sy = player->posY + (world->rand.nextFloat() - 0.5f) * 4.0f;
    const double sz = player->posZ + std::sin(angle) * dist;

    static const char *const SOUNDS[] = {
        "ambient.cave.cave",
        "step.stone",
        "step.grass",
        "random.door_open"
    };
    const int pick = world->rand.nextInt(4);
    world->playSoundEffect(sx, sy, sz, SOUNDS[pick], pick == 0 ? 0.65f : 0.4f, 0.7f + world->rand.nextFloat() * 0.25f);
}

bool HerobrineMod::isAreaSafeFromBuilds(World *world, int_t cx, int_t cy, int_t cz) const
{
    if (world == nullptr)
        return false;
    if (std::abs(cx - m_altarX) < 12 && std::abs(cz - m_altarZ) < 12)
        return false;
    for (int_t dx = -12; dx <= 12; dx += 2)
    {
        for (int_t dz = -12; dz <= 12; dz += 2)
        {
            for (int_t dy = -4; dy <= 6; dy += 2)
            {
                if (isPlayerBuiltBlock(world->getBlockId(cx + dx, cy + dy, cz + dz)))
                    return false;
            }
        }
    }
    return true;
}

bool HerobrineMod::buildSandPyramid(World *world, int_t sx, int_t sy, int_t sz)
{
    for (int_t dx = -2; dx <= 2; ++dx)
        for (int_t dz = -2; dz <= 2; ++dz)
            if (world->getBlockId(sx + dx, sy - 1, sz + dz) != Block::sand->blockID || !world->isAirBlock(sx + dx, sy, sz + dz))
                return false;
    for (int_t layer = 0; layer < 3; ++layer)
    {
        const int_t r = 2 - layer;
        for (int_t dx = -r; dx <= r; ++dx)
            for (int_t dz = -r; dz <= r; ++dz)
                if (world->isAirBlock(sx + dx, sy + layer, sz + dz))
                    world->setBlockWithNotify(sx + dx, sy + layer, sz + dz, Block::sand->blockID);
    }
    return true;
}

bool HerobrineMod::stripLeavesFromNaturalTree(World *world, int_t sx, int_t sy, int_t sz)
{
    int_t tx = 0, ty = 0, tz = 0;
    bool found = false;
    for (int_t dx = -6; dx <= 6 && !found; ++dx)
    {
        for (int_t dz = -6; dz <= 6 && !found; ++dz)
        {
            const int_t y = world->getTopSolidOrLiquidBlock(sx + dx, sz + dz);
            for (int_t cy = y; cy >= std::max<int_t>(4, y - 8); --cy)
            {
                if (world->getBlockId(sx + dx, cy, sz + dz) == Block::wood->blockID)
                {
                    tx = sx + dx; ty = cy; tz = sz + dz;
                    found = true;
                    break;
                }
            }
        }
    }
    if (!found) return false;
    int_t removed = 0;
    for (int_t dx = -3; dx <= 3; ++dx)
        for (int_t dz = -3; dz <= 3; ++dz)
            for (int_t dy = -1; dy <= 5; ++dy)
                if (world->getBlockId(tx + dx, ty + dy, tz + dz) == Block::leaves->blockID)
                {
                    world->setBlockWithNotify(tx + dx, ty + dy, tz + dz, 0);
                    removed++;
                }
    return removed >= 4;
}

bool HerobrineMod::placeCreepySign(World *world, int_t sx, int_t sy, int_t sz)
{
    if (Block::signPost == nullptr) return false;
    const int_t below = world->getBlockId(sx, sy - 1, sz);
    if (below != Block::grass->blockID && below != Block::dirt->blockID && below != Block::sand->blockID)
        return false;
    if (!world->isAirBlock(sx, sy, sz))
        return false;
    world->setBlockAndMetadataWithNotify(sx, sy, sz, Block::signPost->blockID, world->rand.nextInt(16));
    TileEntitySign *sign = dynamic_cast<TileEntitySign *>(world->getBlockTileEntity(sx, sy, sz));
    if (sign != nullptr)
    {
        static const char *const LINES[][4] = {
            { "", "Stop.", "", "" },
            { "", "I am here.", "", "" },
            { "", "This is mine.", "", "" },
            { "", "Follow me.", "", "" }
        };
        const int pick = world->rand.nextInt(4);
        for (int i = 0; i < 4; ++i)
            sign->signText[i] = LINES[pick][i];
        sign->setEditable(false);
    }
    return true;
}

bool HerobrineMod::placeLoneRedstoneTorch(World *world, int_t sx, int_t sy, int_t sz)
{
    if (Block::torchRedstoneActive == nullptr) return false;
    const int_t below = world->getBlockId(sx, sy - 1, sz);
    if (below != Block::grass->blockID && below != Block::dirt->blockID && below != Block::stone->blockID)
        return false;
    if (!world->isAirBlock(sx, sy, sz))
        return false;
    world->setBlockAndMetadataWithNotify(sx, sy, sz, Block::torchRedstoneActive->blockID, 5);
    return true;
}

bool HerobrineMod::triggerSafeLegendExtra(World *world, EntityPlayer *player)
{
    if (!m_config.legendExtras || world == nullptr || player == nullptr || world->multiplayerWorld)
        return false;

    const float angle = world->rand.nextFloat() * 6.283185f;
    const float dist = 26.0f + world->rand.nextFloat() * 16.0f;
    const int_t sx = static_cast<int_t>(std::floor(player->posX + std::cos(angle) * dist));
    const int_t sz = static_cast<int_t>(std::floor(player->posZ + std::sin(angle) * dist));
    if (!world->blockExists(sx, 64, sz))
        return false;

    const int_t sy = world->getTopSolidOrLiquidBlock(sx, sz);
    if (sy < 4 || sy > 118 || !isAreaSafeFromBuilds(world, sx, sy, sz))
        return false;

    const int pick = world->rand.nextInt(4);
    bool ok = false;
    if (pick == 0) ok = stripLeavesFromNaturalTree(world, sx, sy, sz);
    else if (pick == 1) ok = placeCreepySign(world, sx, sy, sz);
    else if (pick == 2) ok = buildSandPyramid(world, sx, sy, sz);
    else ok = placeLoneRedstoneTorch(world, sx, sy, sz);

    if (ok)
    {
        m_mythEvents++;
        saveState();
    }
    return ok;
}

void HerobrineMod::onTick()
{
    if (m_mc == nullptr || m_mc->theWorld == nullptr)
        return;

    std::string curDir;
    if (m_mc->theWorld->getSaveHandler() != nullptr)
        curDir = m_mc->theWorld->getSaveHandler()->getSaveDirectory();
    if (curDir != m_currentWorldDir)
    {
        m_currentWorldDir = curDir;
        m_altarRitual.active = false;
        m_digger.active = false;
        m_diamond.active = false;
        loadState();
    }

    if (!m_enabled || !m_summoned || m_mc->theWorld->multiplayerWorld)
        return;

    EntityPlayer *player = m_mc->thePlayerOne ? m_mc->thePlayerOne : m_mc->thePlayer;
    if (player == nullptr)
        return;

    World *world = m_mc->theWorld;

    if (m_altarRitual.active)
    {
        tickAltarRitual(world, player);
        return;
    }

    if (m_digger.active)
        tickDiggerEvent(world, player);
    if (m_diamond.active)
        tickDiamondEvent(world, player);

    if (--m_wheelCooldown <= 0)
    {
        if (!m_digger.active && !m_diamond.active)
            spinEventWheel(world, player);
        m_wheelCooldown = 900 + world->rand.nextInt(900); // 45-90s
    }

    if (--m_soundCooldown <= 0)
    {
        playEerieSound(world, player);
        m_soundCooldown = 700 + world->rand.nextInt(800);
    }

    if (--m_extraCooldown <= 0)
    {
        triggerSafeLegendExtra(world, player);
        m_extraCooldown = 1800 + world->rand.nextInt(1400);
    }
}

std::string HerobrineMod::getStateFilePath() const
{
    if (!m_currentWorldDir.empty())
        return PlatformStorage::join(m_currentWorldDir, "herobrine.txt");
    if (m_mc != nullptr && m_mc->theWorld != nullptr && m_mc->theWorld->getSaveHandler() != nullptr)
    {
        std::string saveDir = m_mc->theWorld->getSaveHandler()->getSaveDirectory();
        if (!saveDir.empty())
            return PlatformStorage::join(saveDir, "herobrine.txt");
    }
    return "";
}

void HerobrineMod::saveState()
{
    const std::string path = getStateFilePath();
    if (path.empty())
        return;
    std::string content;
    content.reserve(160);
    content += "# Herobrine Classic (iLoveYou) World State\n";
    content += "summoned:" + std::string(m_summoned ? "1" : "0") + "\n";
    content += "altarX:" + std::to_string(m_altarX) + "\n";
    content += "altarY:" + std::to_string(m_altarY) + "\n";
    content += "altarZ:" + std::to_string(m_altarZ) + "\n";
    content += "sightings:" + std::to_string(m_sightings) + "\n";
    content += "mythEvents:" + std::to_string(m_mythEvents) + "\n";
    PlatformStorage::writeFile(path, content.data(), content.size());
}

void HerobrineMod::loadState()
{
    m_summoned = false;
    m_altarX = m_altarY = m_altarZ = 0;
    m_sightings = 0;
    m_mythEvents = 0;
    m_wheelCooldown = 500;
    m_soundCooldown = 350;
    m_extraCooldown = 1400;

    const std::string path = getStateFilePath();
    if (path.empty())
        return;
    std::vector<unsigned char> bytes;
    if (!PlatformStorage::readFile(path, bytes) || bytes.empty())
        return;

    std::string content(bytes.begin(), bytes.end());
    size_t start = 0;
    while (start < content.size())
    {
        size_t end = content.find('\n', start);
        std::string line = (end == std::string::npos) ? content.substr(start) : content.substr(start, end - start);
        start = (end == std::string::npos) ? content.size() : end + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;
        size_t sep = line.find(':');
        if (sep == std::string::npos)
            continue;
        std::string key = line.substr(0, sep);
        int val = std::atoi(line.substr(sep + 1).c_str());
        if (key == "summoned") m_summoned = (val != 0);
        else if (key == "altarX") m_altarX = val;
        else if (key == "altarY") m_altarY = val;
        else if (key == "altarZ") m_altarZ = val;
        else if (key == "sightings") m_sightings = val;
        else if (key == "mythEvents") m_mythEvents = val;
    }
}

std::string HerobrineMod::getConfigFilePath() const
{
    File *dir = Minecraft::getMinecraftDir();
    if (dir != nullptr)
        return PlatformStorage::join(dir->toString(), "herobrine_config.txt");
    return "herobrine_config.txt";
}

void HerobrineMod::saveConfig()
{
    const std::string path = getConfigFilePath();
    if (path.empty())
        return;
    std::string out;
    out += "# Herobrine Classic (iLoveYou) Config\n";
    out += "altarRitual:" + std::string(m_config.altarRitual ? "1" : "0") + "\n";
    out += "ghostEvent:" + std::string(m_config.ghostEvent ? "1" : "0") + "\n";
    out += "altarCurse:" + std::string(m_config.altarCurse ? "1" : "0") + "\n";
    out += "altarCurseDefault2:1\n";
    out += "diggerEvent:" + std::string(m_config.diggerEvent ? "1" : "0") + "\n";
    out += "diamondEvent:" + std::string(m_config.diamondEvent ? "1" : "0") + "\n";
    out += "thiefEvent:" + std::string(m_config.thiefEvent ? "1" : "0") + "\n";
    out += "legendExtras:" + std::string(m_config.legendExtras ? "1" : "0") + "\n";
    PlatformStorage::writeFile(path, out.data(), out.size());
}

void HerobrineMod::loadConfig()
{
    const std::string path = getConfigFilePath();
    if (path.empty())
        return;
    std::vector<unsigned char> bytes;
    if (!PlatformStorage::readFile(path, bytes) || bytes.empty())
        return;
    std::string content(bytes.begin(), bytes.end());
    bool sawCurseDefault2 = false;
    size_t start = 0;
    while (start < content.size())
    {
        size_t end = content.find('\n', start);
        std::string line = (end == std::string::npos) ? content.substr(start) : content.substr(start, end - start);
        start = (end == std::string::npos) ? content.size() : end + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;
        size_t sep = line.find(':');
        if (sep == std::string::npos)
            continue;
        std::string key = line.substr(0, sep);
        bool val = (std::atoi(line.substr(sep + 1).c_str()) != 0);
        if (key == "altarRitual") m_config.altarRitual = val;
        else if (key == "ghostEvent") m_config.ghostEvent = val;
        else if (key == "altarCurse") m_config.altarCurse = val;
        else if (key == "altarCurseDefault2") sawCurseDefault2 = true;
        else if (key == "diggerEvent") m_config.diggerEvent = val;
        else if (key == "diamondEvent") m_config.diamondEvent = val;
        else if (key == "thiefEvent") m_config.thiefEvent = val;
        else if (key == "legendExtras") m_config.legendExtras = val;
    }
    // Worlds saved before the curse became ON by default (22-11) stored it as 0
    // without the user choosing it: switch them to the original behaviour once.
    if (!sawCurseDefault2)
        m_config.altarCurse = true;
}
