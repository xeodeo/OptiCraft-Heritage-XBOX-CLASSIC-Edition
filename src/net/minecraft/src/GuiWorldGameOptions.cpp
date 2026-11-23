#include "GuiWorldGameOptions.h"

#include "GameSettings.h"
#include "GuiButton.h"
#include "Minecraft.h"
#include "StringTranslate.h"
#include "legacy/LegacyGuiButton.h"
#include "legacy/LegacyMainMenuLayout.h"
#include "legacy/LegacyMenuHints.h"

namespace
{
bool optionsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0;
}

struct RuleText
{
    const char *rule;
    const char *en;
    const char *es;
};

const RuleText kRuleText[] = {
    { "doFireTick", "Fire Spreads", "El fuego se propaga" },
    { "tntExplodes", "TNT Explodes", "La dinamita explota" },
    { "pvp", "Player vs Player", "Jugador contra jugador" },
    { "keepInventory", "Keep Inventory", "Conservar inventario" },
    { "doMobSpawning", "Mob Spawning", "Aparecen criaturas" },
    { "doMobLoot", "Mob Loot", "Las criaturas sueltan objetos" },
    { "mobGriefing", "Mob Griefing", "Las criaturas rompen bloques" },
    { "doTileDrops", "Tile Drops", "Los bloques sueltan objetos" },
};

const int_t BUTTON_DONE = 100;
} // namespace

GuiWorldGameOptions::RuleList GuiWorldGameOptions::defaultRules()
{
    RuleList rules;
    for (const RuleText &t : kRuleText)
        rules.emplace_back(t.rule, std::string(t.rule) != "keepInventory");
    return rules;
}

GuiWorldGameOptions::GuiWorldGameOptions(GuiScreen *parent, RuleList *rules)
    : m_parent(parent)
    , m_rules(rules)
{
}

std::string GuiWorldGameOptions::labelFor(std::size_t index) const
{
    const bool es = optionsSpanish();
    const std::pair<std::string, bool> &rule = (*m_rules)[index];
    std::string text = rule.first;
    for (const RuleText &t : kRuleText)
        if (rule.first == t.rule)
            text = es ? t.es : t.en;
    return text + ": " + (rule.second ? (es ? "S\xC3\xAD" : "On") : (es ? "No" : "Off"));
}

void GuiWorldGameOptions::initGui()
{
    controlList.clear();
    const bool legacy = mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
    const int_t count = static_cast<int_t>(m_rules->size()) + 1;
    const std::string done = StringTranslate::getInstance()->translateKey("gui.done");
    if (legacy)
    {
        LegacyMainMenuLayout layout = legacyMainMenuLayout(width, height, count);
        const int_t stride = layout.buttonHeight + layout.buttonSpacing;
        const int_t column = count * stride - layout.buttonSpacing;
        const int_t top = std::max<int_t>(24, height / 8);
        const int_t bottom = legacyHintRowY(height) - std::max<int_t>(6, height / 30);
        const int_t firstY = std::max<int_t>(4, top + std::max<int_t>(0, (bottom - top - column) / 2));
        for (std::size_t i = 0; i < m_rules->size(); ++i)
            controlList.push_back(new LegacyGuiButton(static_cast<int_t>(i), layout.buttonX,
                firstY + static_cast<int_t>(i) * stride, layout.buttonWidth, layout.buttonHeight, labelFor(i)));
        controlList.push_back(new LegacyGuiButton(BUTTON_DONE, layout.buttonX,
            firstY + static_cast<int_t>(m_rules->size()) * stride, layout.buttonWidth, layout.buttonHeight, done));
    }
    else
    {
        // Java style: two columns like the vanilla options screens.
        for (std::size_t i = 0; i < m_rules->size(); ++i)
        {
            const int_t x = width / 2 - 155 + static_cast<int_t>(i % 2) * 160;
            const int_t y = height / 6 + 24 * static_cast<int_t>(i / 2);
            controlList.push_back(new GuiButton(static_cast<int_t>(i), x, y, 150, 20, labelFor(i)));
        }
        controlList.push_back(new GuiButton(BUTTON_DONE, width / 2 - 100, height / 6 + 120, done));
    }
}

void GuiWorldGameOptions::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;
    if (button->id == BUTTON_DONE)
    {
        mc->displayGuiScreen(m_parent);
        return;
    }
    const std::size_t i = static_cast<std::size_t>(button->id);
    if (i < m_rules->size())
    {
        (*m_rules)[i].second = !(*m_rules)[i].second;
        button->displayString = labelFor(i);
    }
}

void GuiWorldGameOptions::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    drawDefaultBackground();
    const bool es = optionsSpanish();
    drawCenteredString(fontRenderer, es ? "Opciones de juego" : "Game Options", width / 2,
                       std::max<int_t>(8, height / 8 - 14), 0xFFFFFF);
    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
    if (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI)
        drawLegacyMenuHints(mc, width, height, true);
}
