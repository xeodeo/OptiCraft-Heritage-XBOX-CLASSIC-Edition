#include "ModManager.h"
#include "Minecraft.h"
#include "java/File.h"
#include "platform/Storage.h"
#include "platform/Log.h"
#include "DynamicMod.h"

// Built-in compiled mods (all OFF by default until activated in Mods Catalog)
#include "reiminimap/ReiMinimapMod.h"
#include "herobrine/HerobrineMod.h"
#include "chococraft/ChocoCraftMod.h"
#include "toomanyitems/TooManyItemsMod.h"
#include "toomanyitems/TooManyItems.h"
#include "chococraft/ChocoCraftItems.h"
#include "chococraft/EntityChocobo.h"
#include "herobrine/EntityHerobrine.h"
#include "xray/XRayMod.h"
#include "dynamiclights/DynamicLightsMod.h"
#include "strongholdlocator/StrongholdLocatorMod.h"
#include "villagelocator/VillageLocatorMod.h"
#include "serverutils/ServerUtilsMod.h"
#include "adminmode/AdminModeMod.h"

#include "net/minecraft/src/WorldInfo.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/Entity.h"
#include "net/minecraft/src/IRecipe.h"
#include "net/minecraft/src/ShapedRecipes.h"
#include "net/minecraft/src/ShapelessRecipes.h"
#include "net/minecraft/src/ItemStack.h"

#include <algorithm>
#include <cstdio>

#if __has_include("net/minecraft/src/BlockHopper.h")
#define OPTICRAFT_GAME_VERSION "1.5.2"
#else
#define OPTICRAFT_GAME_VERSION "1.2.5"
#endif

namespace
{
std::string modsSignatureForWorld()
{
    return ModManager::getInstance().getEnabledModsSignature();
}
} // namespace

const char *ModManager::gameVersion()
{
    return OPTICRAFT_GAME_VERSION;
}

bool ModManager::isWorldContentMod(const std::string &id)
{
    // Utility mods (minimap, X-Ray, TooManyItems, locators, lights) leave nothing in the world.
    return id == "chococraft" || id == "herobrine";
}

const char *ModManager::getOwnerModForBlock(int_t blockId)
{
    if (blockId == ChocoCraftItems::BLOCK_GYSAHL_ID ||
        blockId == ChocoCraftItems::BLOCK_STRAW_ID ||
        blockId == ChocoCraftItems::BLOCK_GYSAHL_GREEN_ID)
    {
        return "chococraft";
    }
    return nullptr;
}

const char *ModManager::getOwnerModForItem(int_t itemId)
{
    if (itemId >= 0 && itemId < 256)
        return getOwnerModForBlock(itemId);

    const int_t firstChocoItem = 256 + ChocoCraftItems::ITEM_FIRST_RAW;
    const int_t lastChocoItem  = 256 + ChocoCraftItems::ITEM_LAST_RAW;
    if (itemId >= firstChocoItem && itemId <= lastChocoItem)
        return "chococraft";

    return nullptr;
}

const char *ModManager::getOwnerModForEntityId(const std::string &entityId)
{
    if (entityId == "Chocobo")
        return "chococraft";
    if (entityId == "Herobrine")
        return "herobrine";
    return nullptr;
}

const char *ModManager::getOwnerModForEntity(const Entity *entity)
{
    if (entity == nullptr)
        return nullptr;
    if (dynamic_cast<const EntityChocobo *>(entity) != nullptr)
        return "chococraft";
    if (dynamic_cast<const EntityHerobrine *>(entity) != nullptr)
        return "herobrine";
    return nullptr;
}

bool ModManager::isBlockAllowed(int_t blockId)
{
    const char *owner = getOwnerModForBlock(blockId);
    return owner == nullptr || getInstance().isModEnabled(owner);
}

bool ModManager::isItemAllowed(int_t itemId)
{
    const char *owner = getOwnerModForItem(itemId);
    return owner == nullptr || getInstance().isModEnabled(owner);
}

bool ModManager::isEntityIdAllowed(const std::string &entityId)
{
    const char *owner = getOwnerModForEntityId(entityId);
    return owner == nullptr || getInstance().isModEnabled(owner);
}

bool ModManager::isEntityAllowed(const Entity *entity)
{
    const char *owner = getOwnerModForEntity(entity);
    return owner == nullptr || getInstance().isModEnabled(owner);
}

bool ModManager::isRecipeAllowed(IRecipe *recipe)
{
    if (recipe == nullptr)
        return false;
    ItemStack *output = recipe->getRecipeOutput();
    if (output != nullptr && !isItemAllowed(output->itemID))
        return false;
    if (ShapedRecipes *shaped = dynamic_cast<ShapedRecipes *>(recipe))
    {
        const int_t count = shaped->getWidth() * shaped->getHeight();
        for (int_t i = 0; i < count; ++i)
        {
            ItemStack *ing = shaped->getPatternItem(i);
            if (ing != nullptr && !isItemAllowed(ing->itemID))
                return false;
        }
    }
    else if (ShapelessRecipes *shapeless = dynamic_cast<ShapelessRecipes *>(recipe))
    {
        for (ItemStack *ing : shapeless->getIngredients())
        {
            if (ing != nullptr && !isItemAllowed(ing->itemID))
                return false;
        }
    }
    return true;
}

std::string ModManager::getEnabledModsSignature() const
{
    std::vector<std::string> entries;
    for (const auto &mod : mods)
    {
        if (mod->isEnabled())
            entries.push_back(mod->getId() + "@" + mod->getVersion());
    }
    std::sort(entries.begin(), entries.end());
    std::string out;
    for (const std::string &e : entries)
    {
        if (!out.empty())
            out += ";";
        out += e;
    }
    return out;
}

ModManager::ModManager()
    : mc(nullptr)
    , initialized(false)
{
}

ModManager::~ModManager()
{
    shutdown();
}

ModManager &ModManager::getInstance()
{
    static ModManager instance;
    return instance;
}

void ModManager::init(Minecraft *mcInstance)
{
    if (initialized)
        return;

    mc = mcInstance;
    initialized = true;
    WorldInfo::modsSignatureProvider = &modsSignatureForWorld;
    WorldInfo::gameVersionProvider = &ModManager::gameVersion;

    // Register all built-in compiled mods so players can search and enable them
    // with one click from the Mods store/catalog on Xbox without FTP transfers.
    if (getMod("reiminimap") == nullptr)
        registerMod(std::make_unique<ReiMinimapMod>());
    if (getMod("herobrine") == nullptr)
        registerMod(std::make_unique<HerobrineMod>());
    if (getMod("chococraft") == nullptr)
        registerMod(std::make_unique<ChocoCraftMod>());
    if (getMod("toomanyitems") == nullptr)
        registerMod(std::make_unique<TooManyItemsMod>());
    if (getMod("xray") == nullptr)
        registerMod(std::make_unique<XRayMod>());
    if (getMod("dynamiclights") == nullptr)
        registerMod(std::make_unique<DynamicLightsMod>());
    if (getMod("strongholdlocator") == nullptr)
        registerMod(std::make_unique<StrongholdLocatorMod>());
    if (getMod("villagelocator") == nullptr)
        registerMod(std::make_unique<VillageLocatorMod>());
    if (getMod("serverutils") == nullptr)
        registerMod(std::make_unique<ServerUtilsMod>());
    if (getMod("adminmode") == nullptr)
        registerMod(std::make_unique<AdminModeMod>());

    // Scan and load installed .ochpack packages from the persistent game mods directory
    scanAndLoadPacks();

    // Load persisted enabled/disabled state from storage
    load();

    // Initialize all registered mods
    for (auto &mod : mods)
    {
        try
        {
            mod->onInit(mc);
        }
        catch (...)
        {
            MC_LOG_WARN("mods", "Failed to initialize mod: %s\n", mod->getId().c_str());
        }
    }

    MC_LOG_INFO("mods", "ModManager initialized with %u mods.\n", static_cast<unsigned>(mods.size()));
}

void ModManager::shutdown()
{
    if (!initialized)
        return;

    save();
    mods.clear();
    mc = nullptr;
    initialized = false;
}

void ModManager::registerMod(std::unique_ptr<IMod> mod)
{
    if (!mod)
        return;
    mods.push_back(std::move(mod));
}

IMod *ModManager::getMod(const std::string &id)
{
    for (auto &mod : mods)
    {
        if (mod->getId() == id)
            return mod.get();
    }
    return nullptr;
}

bool ModManager::isModEnabled(const std::string &id)
{
    IMod *mod = getMod(id);
    return mod != nullptr && mod->isEnabled();
}

void ModManager::setModEnabled(const std::string &id, bool enabled)
{
    IMod *mod = getMod(id);
    if (mod == nullptr)
        return;

    const bool changed = (mod->isEnabled() != enabled);
    mod->setEnabled(enabled);

    if (changed)
    {
        TooManyItems::refreshItems();
        if (mc != nullptr && mc->theWorld != nullptr && isWorldContentMod(id))
        {
            if (enabled)
                mc->theWorld->reviveModEntities(id);
            else
                mc->theWorld->suspendModEntities(id);
        }
    }

    save();
}

std::string ModManager::getGameModsDir() const
{
    File *dataDir = Minecraft::getMinecraftDir();
    if (dataDir != nullptr)
        return PlatformStorage::join(dataDir->toString(), "mods");
    return "mods";
}

std::string ModManager::getExtraCatalogPath() const
{
    return PlatformStorage::join(getGameModsDir(), "catalog.txt");
}

std::vector<CompiledModInfo> ModManager::getCatalogEntries() const
{
    std::vector<CompiledModInfo> out;
    out.reserve(mods.size() + 4);
    for (const auto &mod : mods)
    {
        if (!mod) continue;
        CompiledModInfo info;
        info.id = mod->getId();
        info.name = mod->getName();
        info.version = mod->getVersion();
        info.author = mod->getAuthor();
        info.description = mod->getDescription();
        info.builtIn = true;
        info.installed = true;
        info.enabled = mod->isEnabled();
        info.removable = mod->isRemovable();
        info.hasConfig = mod->hasConfigScreen();
        out.push_back(std::move(info));
    }

    // Optional local extra catalog (mods/catalog.txt): id|name|version|author|description
    // Prepared for future local catalog expansion without requiring network access.
    std::vector<unsigned char> bytes;
    if (PlatformStorage::readFile(getExtraCatalogPath(), bytes) && !bytes.empty())
    {
        std::string content(bytes.begin(), bytes.end());
        size_t pos = 0;
        while (pos < content.size())
        {
            size_t nl = content.find('\n', pos);
            std::string line = (nl == std::string::npos) ? content.substr(pos) : content.substr(pos, nl - pos);
            pos = (nl == std::string::npos) ? content.size() : nl + 1;
            while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
                line.pop_back();
            if (line.empty() || line[0] == '#')
                continue;

            std::vector<std::string> fields;
            size_t fStart = 0;
            while (fStart <= line.size())
            {
                size_t bar = line.find('|', fStart);
                if (bar == std::string::npos)
                {
                    fields.push_back(line.substr(fStart));
                    break;
                }
                fields.push_back(line.substr(fStart, bar - fStart));
                fStart = bar + 1;
            }
            if (fields.size() < 2 || fields[0].empty())
                continue;
            bool exists = false;
            for (const auto &e : out)
            {
                if (e.id == fields[0]) { exists = true; break; }
            }
            if (exists)
                continue;
            CompiledModInfo extra;
            extra.id = fields[0];
            extra.name = fields[1];
            extra.version = fields.size() > 2 ? fields[2] : "1.0";
            extra.author = fields.size() > 3 ? fields[3] : "Local";
            extra.description = fields.size() > 4 ? fields[4] : "External catalog entry (.ochpack)";
            extra.builtIn = false;
            extra.installed = false;
            extra.enabled = false;
            out.push_back(std::move(extra));
        }
    }

    return out;
}

std::string ModManager::getInstalledModVersion(const std::string &id) const
{
    for (const auto &mod : mods)
    {
        if (mod->getId() == id)
            return mod->getVersion();
    }
    return "";
}

void ModManager::scanAndLoadPacks()
{
    std::string modsDir = getGameModsDir();
    PlatformStorage::mkdirs(modsDir);

    auto packs = OchPackReader::scanDirectory(modsDir);
    for (const auto &pack : packs)
    {
        IMod *existing = getMod(pack.id);
        if (existing != nullptr)
        {
            existing->setPackPath(pack.filePath);
            continue;
        }

        std::unique_ptr<IMod> newMod;
        if (pack.id == "toomanyitems")            newMod = std::make_unique<TooManyItemsMod>();
        else if (pack.id == "reiminimap")         newMod = std::make_unique<ReiMinimapMod>();
        else if (pack.id == "strongholdlocator")  newMod = std::make_unique<StrongholdLocatorMod>();
        else if (pack.id == "herobrine")          newMod = std::make_unique<HerobrineMod>();
        else if (pack.id == "chococraft")         newMod = std::make_unique<ChocoCraftMod>();
        else if (pack.id == "xray")               newMod = std::make_unique<XRayMod>();
        else if (pack.id == "villagelocator")     newMod = std::make_unique<VillageLocatorMod>();
        else if (pack.id == "serverutils")        newMod = std::make_unique<ServerUtilsMod>();
        else if (pack.id == "dynamiclights")      newMod = std::make_unique<DynamicLightsMod>();
        else                                      newMod = std::make_unique<DynamicMod>(pack);

        if (newMod)
        {
            newMod->setPackPath(pack.filePath);
            if (initialized && mc != nullptr)
            {
                try { newMod->onInit(mc); } catch (...) {}
            }
            registerMod(std::move(newMod));
        }
    }
}

bool ModManager::installModPack(const std::string &sourcePath, std::string &outError)
{
    OchPackInfo info;
    if (!OchPackReader::readInfo(sourcePath, info))
    {
        outError = "Failed to read .ochpack archive or missing mod.info.";
        return false;
    }

    std::string modsDir = getGameModsDir();
    if (!PlatformStorage::mkdirs(modsDir))
    {
        outError = "Failed to access/create game mods directory.";
        return false;
    }

    std::string destPath = PlatformStorage::join(modsDir, info.fileName);

    std::vector<unsigned char> data;
    if (!OchPackReader::readFileBytes(sourcePath, data))
    {
        if (!info.filePath.empty() && info.filePath != sourcePath)
            OchPackReader::readFileBytes(info.filePath, data);
    }

    if (data.empty())
    {
        outError = "Failed to read package file.";
        return false;
    }

    if (!PlatformStorage::writeFile(destPath, data.data(), data.size()))
    {
        outError = "Failed to write package to destination.";
        return false;
    }

    info.filePath = destPath;
    IMod *existing = getMod(info.id);
    if (existing != nullptr)
    {
        existing->setPackPath(destPath);
        existing->setEnabled(true);
    }
    else
    {
        std::unique_ptr<IMod> newMod;
        if (info.id == "toomanyitems")           newMod = std::make_unique<TooManyItemsMod>();
        else if (info.id == "reiminimap")        newMod = std::make_unique<ReiMinimapMod>();
        else if (info.id == "strongholdlocator") newMod = std::make_unique<StrongholdLocatorMod>();
        else if (info.id == "herobrine")         newMod = std::make_unique<HerobrineMod>();
        else if (info.id == "chococraft")        newMod = std::make_unique<ChocoCraftMod>();
        else if (info.id == "xray")              newMod = std::make_unique<XRayMod>();
        else if (info.id == "villagelocator")    newMod = std::make_unique<VillageLocatorMod>();
        else if (info.id == "serverutils")       newMod = std::make_unique<ServerUtilsMod>();
        else if (info.id == "dynamiclights")     newMod = std::make_unique<DynamicLightsMod>();
        else                                     newMod = std::make_unique<DynamicMod>(info);

        if (newMod)
        {
            newMod->setPackPath(destPath);
            newMod->setEnabled(true);
            if (initialized && mc != nullptr)
            {
                try { newMod->onInit(mc); } catch (...) {}
            }
            registerMod(std::move(newMod));
        }
    }

    save();
    return true;
}

bool ModManager::deleteMod(const std::string &id)
{
    for (auto it = mods.begin(); it != mods.end(); ++it)
    {
        if ((*it)->getId() == id)
        {
            std::string pack = (*it)->getPackPath();
            if (!pack.empty())
            {
                PlatformStorage::removeFile(pack);
                std::remove(pack.c_str());
            }

            mods.erase(it);
            save();
            return true;
        }
    }
    return false;
}

std::string ModManager::getConfigPath() const
{
    File *dataDir = Minecraft::getMinecraftDir();
    if (dataDir != nullptr)
        return PlatformStorage::join(dataDir->toString(), "mods.txt");
    return "mods.txt";
}

void ModManager::load()
{
    std::string path = getConfigPath();
    std::vector<unsigned char> bytes;
    if (!PlatformStorage::readFile(path, bytes) || bytes.empty())
        return;

    std::string content(bytes.begin(), bytes.end());
    size_t start = 0;
    while (start < content.size())
    {
        size_t end = content.find('\n', start);
        std::string line;
        if (end == std::string::npos)
        {
            line = content.substr(start);
            start = content.size();
        }
        else
        {
            line = content.substr(start, end - start);
            start = end + 1;
        }

        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();

        if (line.empty() || line[0] == '#')
            continue;

        size_t colon = line.find(':');
        if (colon != std::string::npos)
        {
            std::string id = line.substr(0, colon);
            std::string val = line.substr(colon + 1);
            bool enabled = (val == "true" || val == "1" || val == "ON" || val == "on");

            for (auto &mod : mods)
            {
                if (mod->getId() == id)
                {
                    mod->setEnabled(enabled);
                    break;
                }
            }
        }
    }
}

void ModManager::save()
{
    std::string path = getConfigPath();
    std::string content;
    content.reserve(384);
    content += "# OptiCraft Mods Configuration\n";
    for (const auto &mod : mods)
    {
        content += mod->getId();
        content += ":";
        content += (mod->isEnabled() ? "true" : "false");
        content += "\n";
    }

    PlatformStorage::writeFile(path, content.data(), content.size());
}

void ModManager::onTick()
{
    for (auto &mod : mods)
    {
        if (mod->isEnabled())
            mod->onTick();
    }
}

void ModManager::onRenderGameOverlay(GuiIngame *gui, int_t screenWidth, int_t screenHeight, float_t partialTick)
{
    for (auto &mod : mods)
    {
        if (mod->isEnabled())
            mod->onRenderGameOverlay(gui, screenWidth, screenHeight, partialTick);
    }
}

void ModManager::onDrawContainer(GuiContainer *container, int_t mouseX, int_t mouseY)
{
    for (auto &mod : mods)
    {
        if (mod->isEnabled())
            mod->onDrawContainer(container, mouseX, mouseY);
    }
}

bool ModManager::onContainerMouseClicked(GuiContainer *container, int_t x, int_t y, int_t button)
{
    for (auto &mod : mods)
    {
        if (mod->isEnabled())
        {
            if (mod->onContainerMouseClicked(container, x, y, button))
                return true;
        }
    }
    return false;
}

bool ModManager::onContainerKeyTyped(char_t c, int_t key)
{
    for (auto &mod : mods)
    {
        if (mod->isEnabled())
        {
            if (mod->onContainerKeyTyped(c, key))
                return true;
        }
    }
    return false;
}
