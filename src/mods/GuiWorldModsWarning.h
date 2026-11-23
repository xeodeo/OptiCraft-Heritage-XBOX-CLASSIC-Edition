#pragma once

#include "net/minecraft/src/GuiScreen.h"
#include <string>
#include <vector>

class GuiSelectWorld;

// Shown when a world is opened with a different set of content mods (ChocoCraft, Herobrine...)
// or with another game version than the one it was last played with (encargo 115).
struct WorldModsCheck
{
    std::vector<std::string> missing;   // ids the world used that are now off
    std::vector<std::string> added;     // ids now on that the world never used
    std::string worldVersion;           // "" when the world predates the detector
    bool otherVersion = false;
    bool newerWorld = false;            // world saved by a newer game version

    bool needsWarning() const { return !missing.empty() || !added.empty() || otherVersion; }
};

// Reads the OptiCraftMods / OptiCraftVersion keys of the world's level.dat.
WorldModsCheck checkWorldMods(const std::string &storedMods, const std::string &storedVersion);

class GuiWorldModsWarning : public GuiScreen
{
public:
    GuiWorldModsWarning(GuiSelectWorld *parent, int_t worldIndex, const WorldModsCheck &check);

    void initGui() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;

protected:
    void actionPerformed(GuiButton *button) override;

private:
    GuiSelectWorld *m_parent;
    int_t m_worldIndex;
    WorldModsCheck m_check;
};
