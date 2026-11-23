#pragma once

#include "GuiScreen.h"
#include <string>
#include <utility>
#include <vector>

// LCE "More Options > Game Options" when creating a world: the game rules a
// console host could set (fire spreads, TNT explodes, PVP, keep inventory, mob
// spawning/loot/griefing, tile drops). Edits the creator's list in place.
class GuiWorldGameOptions : public GuiScreen
{
public:
    using RuleList = std::vector<std::pair<std::string, bool>>;

    GuiWorldGameOptions(GuiScreen *parent, RuleList *rules);

    // Default list, in LCE order.
    static RuleList defaultRules();

    void initGui() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;

protected:
    void actionPerformed(GuiButton *button) override;

private:
    std::string labelFor(std::size_t index) const;

    GuiScreen *m_parent;
    RuleList *m_rules;
};
