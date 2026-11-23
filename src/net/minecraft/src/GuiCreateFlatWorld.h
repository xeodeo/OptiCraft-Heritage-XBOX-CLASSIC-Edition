#pragma once

#include <string>

#include "FlatGeneratorInfo.h"
#include "GuiScreen.h"
#include "legacy/LegacyOptionsLayout.h"
#include "legacy/LegacyOptionsPanel.h"

class GuiButton;
class GuiCreateWorld;

// net.minecraft.src.GuiCreateFlatWorld (Java 1.5.2, Java + Legacy UI)
class GuiCreateFlatWorld : public GuiScreen
{
public:
	GuiCreateFlatWorld(GuiCreateWorld *parent, const std::string &generatorOptions);
	~GuiCreateFlatWorld() override = default;

	std::string getFlatGeneratorInfo() const;
	void setFlatGeneratorInfo(const std::string &options);

	void initGui() override;
	void updateScreen() override;
	void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;

protected:
	bool usesSpecializedMenuNavigation() const override { return true; }
	void actionPerformed(GuiButton *button) override;
	void keyTyped(char_t c, int_t key) override;
	void mouseClicked(int_t mouseX, int_t mouseY, int_t button) override;

private:
	void updateButtonStates();
	void syncButtonFocus();
	void ensureSelectedLayerVisible();
	bool hasValidSelectedLayer() const;
	void computeListRect(int_t &left, int_t &top, int_t &right, int_t &bottom) const;
	void drawLayerIcon(int_t x, int_t y, int_t blockId, int_t blockMeta);

	GuiCreateWorld *createWorldGui;
	FlatGeneratorInfo theFlatGeneratorInfo;
	std::string customizationTitle;
	std::string layerMaterialLabel;
	std::string heightLabel;

	GuiButton *buttonRemoveLayer;
	GuiButton *buttonPresets;
	GuiButton *buttonDone;
	GuiButton *buttonCancel;

	int_t selectedLayer;   // display index (0 = top layer)
	int_t scrollOffset;
	bool focusOnList;
	int_t selectedButton;  // 0=remove, 1=presets, 2=done, 3=cancel

	LegacyOptionsLayout legacyLayout;
	LegacyOptionsPanel panelRenderer;
	bool panoramaAvailable;
};
