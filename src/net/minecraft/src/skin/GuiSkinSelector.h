#pragma once

#include "platform/PlatformConfig.h"
#include "net/minecraft/src/GuiScreen.h"
#include "SkinManager.h"
#include <string>

class GuiButton;
class ModelBiped;

class GuiSkinSelector : public GuiScreen
{
public:
    explicit GuiSkinSelector(GuiScreen *parent, bool isPlayer2 = false);
    ~GuiSkinSelector() override;

    void initGui() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void updateScreen() override;
    void keyTyped(char_t c, int_t key) override;
    void mouseClicked(int_t mouseX, int_t mouseY, int_t button) override;
    void actionPerformed(GuiButton *button) override;
    bool doesGuiPauseGame() override;

protected:
    bool allowsPlatformPointerInput() const override { return !isLegacyMode(); }
    bool usesSpecializedMenuNavigation() const override { return isLegacyMode(); }
    void handleSpecializedMenuInput() override;

private:
    bool isLegacyMode() const;
    void nextSkin();
    void prevSkin();
    void switchPack(int newPackIndex);
    void togglePlayer2Mode();
    void selectAndConfirm();
    void cancelAndReturn();
    void deleteCurrentCustomSkin();

    void drawBeveledPanel(int_t left, int_t top, int_t right, int_t bottom, int_t fillColor);
    void drawInsetPanel(int_t left, int_t top, int_t right, int_t bottom, int_t fillColor);
    void drawFrontPreview(const SkinEntry *skin, float x, float y, float w, float h, float alpha);
    void drawFeetShadow(float centerX, float groundY, float radiusX, float radiusY, float alpha);

    void drawLegacyScreen(int_t mouseX, int_t mouseY, float_t partialTick);
    void drawLegacyCharacter3D(const SkinEntry *skin, float centerX, float feetY, float scale,
                               float yawDeg, float pitchDeg, bool isCenterSlot,
                               float brightness, float partialTick);

    GuiScreen *parentScreen;
    bool isPlayer2Skin;
    bool initializedSelection;
    int currentPackIndex;
    int currentSkinIndex;
    float scrollOffset; // smooth transition offset: -1.0 (moving right) to +1.0 (moving left), dampens to 0.0

    // 3D character preview state for LCE mode
    ModelBiped *previewModel32;
    ModelBiped *previewModel64;
    float previewYaw;
    float previewPitch;
    bool previewAutoRotate;
    bool previewRotatingLeft;
    float previewWalkPos;
    float previewWalkSpeed;
    float previewPrevWalkSpeed;
    float previewBobTicks;
    int previewPoseIndex;
    float previewSwingTicks;
    bool previewPoseStickLatch;
    int navHoldRepeatTimer;

    // Stored dimensions for click hit-testing
    int_t dialogLeft;
    int_t dialogTop;
    int_t dialogWidth;
    int_t dialogHeight;
    int_t leftPanelWidth;
    int_t rightPanelX;
    int_t rightPanelWidth;
    int_t carouselCenterX;
    int_t carouselGroundY;
    int_t nameplateY;
    int_t nameplateHeight;

    // LCE mode hit-test regions
    int_t legacyStageTop;
    int_t legacyStageBottom;
    int_t legacyPackBarY;
    int_t legacyPackBarH;
    int_t legacyCenterTabX;
    int_t legacyCenterTabW;
    int_t legacyLeftTabX;
    int_t legacyRightTabX;
    int_t legacySideTabW;
    int_t legacyNameplateX;
    int_t legacyNameplateW;

    // Interactive GUI buttons
    GuiButton *buttonTabDefault;
    GuiButton *buttonTabCustom;
    GuiButton *buttonPlayer2Skin;
    GuiButton *buttonLoadSkins;
    GuiButton *buttonDeleteSkin;

#if PLATFORM_PS2
    bool ps2ActionReleaseLatch;
    bool stickNavLatched;
    int dpadRepeatTimer;
    int stickRepeatTimer;
    unsigned short lastPadHeld;
#endif
};
