#pragma once

#include "GuiCompiledMods.h"

// GuiMods opens the unified Store-style Mod Catalog & Search screen (1b + 1c)
// in both Java UI and Legacy UI with full Xbox controller support.
class GuiMods : public GuiCompiledMods
{
public:
    explicit GuiMods(GuiScreen *parent)
        : GuiCompiledMods(parent)
    {
    }
    ~GuiMods() override = default;
};
