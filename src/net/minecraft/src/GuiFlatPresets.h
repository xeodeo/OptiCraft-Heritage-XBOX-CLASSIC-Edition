#pragma once

#include <string>
#include <vector>

#include "GuiScreen.h"
#include "legacy/LegacyOptionsLayout.h"
#include "legacy/LegacyOptionsPanel.h"

class GuiButton;
class GuiCreateFlatWorld;
class GuiTextField;

struct FlatPresetItem
{
	int_t iconId;
	int_t iconDamage;
	std::string name;
	std::string presetString;
};

// net.minecraft.src.GuiFlatPresets (Java 1.5.2, Java + Legacy UI)
class GuiFlatPresets : public GuiScreen
{
public:
	explicit GuiFlatPresets(GuiCreateFlatWorld *parent);
	~GuiFlatPresets() override;

	void initGui() override;
	void onGuiClosed() override;
	void updateScreen() override;
	void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;

	static const std::vector<FlatPresetItem> &getPresets();

protected:
	bool usesSpecializedMenuNavigation() const override { return true; }
	void actionPerformed(GuiButton *button) override;
	void keyTyped(char_t c, int_t key) override;
	void mouseClicked(int_t mouseX, int_t mouseY, int_t button) override;

private:
	void updateButtonStates();
	void syncFocus();
	void selectPresetIndex(int_t index);
	void ensureSelectedPresetVisible();
	bool canUsePreset() const;
	void computeRects(int_t &fieldX, int_t &fieldY, int_t &fieldW,
	                  int_t &listLeft, int_t &listTop, int_t &listRight, int_t &listBottom) const;
	void drawPresetIcon(int_t x, int_t y, int_t iconId, int_t iconDamage);

	GuiCreateFlatWorld *createFlatWorldGui;
	std::string titleText;
	std::string shareText;
	std::string listText;

	GuiTextField *presetTextField;
	GuiButton *buttonUsePreset;
	GuiButton *buttonCancel;

	int_t selectedPreset;
	int_t scrollOffset;
	// 0 = text field, 1 = presets list, 2 = Use Preset, 3 = Cancel
	int_t focusZone;

	LegacyOptionsLayout legacyLayout;
	LegacyOptionsPanel panelRenderer;
	bool panoramaAvailable;
};
