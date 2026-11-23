#include "GuiWorldModsWarning.h"

#include "ModManager.h"
#include "net/minecraft/src/GuiButton.h"
#include "net/minecraft/src/GuiSelectWorld.h"
#include "net/minecraft/src/StringTranslate.h"
#include "client/Minecraft.h"

#include <algorithm>

namespace
{
bool warnIsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0;
}

// "id@ver;id@ver" -> ids
std::vector<std::string> parseIds(const std::string &mods)
{
    std::vector<std::string> ids;
    size_t start = 0;
    while (start < mods.size())
    {
        size_t end = mods.find(';', start);
        if (end == std::string::npos)
            end = mods.size();
        std::string entry = mods.substr(start, end - start);
        const size_t at = entry.find('@');
        if (at != std::string::npos)
            entry.resize(at);
        if (!entry.empty())
            ids.push_back(entry);
        start = end + 1;
    }
    return ids;
}

// Compares "1.2.5" style versions numerically. <0 if a<b.
int compareVersions(const std::string &a, const std::string &b)
{
    size_t i = 0, j = 0;
    while (i < a.size() || j < b.size())
    {
        long x = 0, y = 0;
        while (i < a.size() && a[i] != '.') { if (a[i] >= '0' && a[i] <= '9') x = x * 10 + (a[i] - '0'); ++i; }
        while (j < b.size() && b[j] != '.') { if (b[j] >= '0' && b[j] <= '9') y = y * 10 + (b[j] - '0'); ++j; }
        if (x != y)
            return x < y ? -1 : 1;
        ++i; ++j;
    }
    return 0;
}

std::string modDisplayName(const std::string &id)
{
    IMod *mod = ModManager::getInstance().getMod(id);
    return mod != nullptr ? mod->getName() : id;
}

std::string joinNames(const std::vector<std::string> &ids)
{
    std::string out;
    for (const std::string &id : ids)
    {
        if (!out.empty())
            out += ", ";
        out += modDisplayName(id);
    }
    return out;
}
} // namespace

WorldModsCheck checkWorldMods(const std::string &storedMods, const std::string &storedVersion)
{
    WorldModsCheck check;
    check.worldVersion = storedVersion;
    if (storedVersion.empty())
        return check; // world from before the detector: nothing to compare with

    const std::vector<std::string> worldIds = parseIds(storedMods);
    ModManager &mm = ModManager::getInstance();
    for (const std::string &id : worldIds)
    {
        if (ModManager::isWorldContentMod(id) && !mm.isModEnabled(id))
            check.missing.push_back(id);
    }
    for (const auto &mod : mm.getMods())
    {
        const std::string id = mod->getId();
        if (ModManager::isWorldContentMod(id) && mod->isEnabled()
            && std::find(worldIds.begin(), worldIds.end(), id) == worldIds.end())
            check.added.push_back(id);
    }
    const std::string current = ModManager::gameVersion();
    const int cmp = compareVersions(storedVersion, current);
    check.otherVersion = cmp != 0;
    check.newerWorld = cmp > 0;
    return check;
}

GuiWorldModsWarning::GuiWorldModsWarning(GuiSelectWorld *parent, int_t worldIndex, const WorldModsCheck &check)
    : m_parent(parent)
    , m_worldIndex(worldIndex)
    , m_check(check)
{
}

void GuiWorldModsWarning::initGui()
{
    const bool es = warnIsSpanish();
    const bool modsDiffer = !m_check.missing.empty() || !m_check.added.empty();
    const int_t y = height / 2 + 36;
    if (modsDiffer)
        controlList.push_back(new GuiButton(0, width / 2 - 100, y, es ? "Usar los mods del mundo y entrar" : "Use the world's mods and play"));
    controlList.push_back(new GuiButton(1, width / 2 - 100, y + 24,
                                        modsDiffer ? (es ? "Entrar con mis mods actuales" : "Play with my current mods")
                                                   : (es ? "Entrar igualmente" : "Play anyway")));
    controlList.push_back(new GuiButton(2, width / 2 - 100, y + 48, es ? "Volver" : "Back"));
}

void GuiWorldModsWarning::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;
    if (button->id == 2)
    {
        mc->displayGuiScreen(m_parent);
        return;
    }
    if (button->id == 0)
    {
        // Match the world: turn its mods back on and the ones it never used off. Nothing is
        // deleted either way; entities/items of a mod that is off stay saved in the world.
        ModManager &mm = ModManager::getInstance();
        for (const std::string &id : m_check.missing)
            mm.setModEnabled(id, true);
        for (const std::string &id : m_check.added)
            mm.setModEnabled(id, false);
    }
    m_parent->selectWorldChecked(m_worldIndex);
}

void GuiWorldModsWarning::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    drawDefaultBackground();
    const bool es = warnIsSpanish();
    int_t y = height / 2 - 70;
    drawCenteredString(fontRenderer, es ? "Este mundo se jugó con otros mods" : "This world was played with other mods",
                       width / 2, y, 0xFFFF55);
    if (m_check.otherVersion)
    {
        y += 18;
        const std::string current = ModManager::gameVersion();
        std::string line = (es ? "Versión del mundo: " : "World version: ") + m_check.worldVersion
                           + (es ? "  (este juego: " : "  (this game: ") + current + ")";
        drawCenteredString(fontRenderer, line, width / 2, y, m_check.newerWorld ? 0xFF5555 : 0xFFFFFF);
        if (m_check.newerWorld)
        {
            y += 12;
            drawCenteredString(fontRenderer,
                               es ? "¡Es de una versión más nueva! Puedes perder bloques y objetos nuevos."
                                  : "It is from a newer version! New blocks and items may be lost.",
                               width / 2, y, 0xFF5555);
        }
    }
    if (!m_check.missing.empty())
    {
        y += 18;
        drawCenteredString(fontRenderer, (es ? "Usaba y ahora están apagados: " : "Used, now turned off: ") + joinNames(m_check.missing),
                           width / 2, y, 0xFFFFFF);
    }
    if (!m_check.added.empty())
    {
        y += 14;
        drawCenteredString(fontRenderer, (es ? "Nuevos (no los usaba): " : "New (never used here): ") + joinNames(m_check.added),
                           width / 2, y, 0xFFFFFF);
    }
    if (!m_check.missing.empty())
    {
        y += 14;
        drawCenteredString(fontRenderer,
                           es ? "Sin ellos sus criaturas y objetos se guardan, no se borran."
                              : "Without them their creatures and items are kept, not deleted.",
                           width / 2, y, 0xA0A0A0);
    }
    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}
