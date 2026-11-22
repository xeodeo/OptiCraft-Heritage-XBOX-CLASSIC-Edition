#pragma once

#include "platform/PlatformConfig.h"
#include "net/minecraft/src/GuiScreen.h"

#include <memory>
#include <string>
#include <vector>

class GuiButton;
class GuiTextField;
class ModelBiped;
class ModelPlayer;

class GuiSearchSkin : public GuiScreen
{
public:
    explicit GuiSearchSkin(GuiScreen *parent, bool isPlayer2 = false);
    ~GuiSearchSkin() override;

    void initGui() override;
    void onGuiClosed() override;
    void updateScreen() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void keyTyped(char_t c, int_t key) override;
    void mouseClicked(int_t mouseX, int_t mouseY, int_t button) override;
    void actionPerformed(GuiButton *button) override;
    bool doesGuiPauseGame() override { return true; }

    // Programmatic helper for tests / external triggers.
    void startSearchFor(const std::string &username);
    bool isSearching() const;
    bool hasDownloadedSkin() const { return !downloadedPng.empty(); }
    bool lastSkinWasSlim() const { return downloadedWasSlim; }
    const std::string &getStatusMessage() const { return statusMessage; }
    bool saveCurrentSkin(bool applyAndClose);

protected:
    bool allowsPlatformPointerInput() const override;
    bool usesSpecializedMenuNavigation() const override;
    void handleSpecializedMenuInput() override;

private:
    struct SearchJob;

    bool isLegacyMode() const;
    bool isSpanish() const;
    void startSearch();
    void releasePreviewTexture();
    void uploadPreviewTexture(const std::vector<unsigned char> &rgba64x64);
    void updateButtonStates();
    void applyPopularIndex(int delta, bool autoSearch);
    void drawPreviewCharacter3D(float centerX, float feetY, float scale, float partialTick);
    void drawBevelPanel(int_t x, int_t y, int_t w, int_t h, bool legacyTheme);

    GuiScreen *parentScreen;
    bool isPlayer2Skin;

    GuiTextField *usernameField;
    GuiButton *btnSearch;
    GuiButton *btnPopularPrev;
    GuiButton *btnPopularUse;
    GuiButton *btnPopularNext;
    GuiButton *btnSave;
    GuiButton *btnUse;
    GuiButton *btnCancel;

    std::string initialInputText;
    int popularIndex;

    // Async download state
    std::shared_ptr<SearchJob> activeJob;
    int searchAnimTicks;

    // Last downloaded skin (kept in memory until Save / Use / close)
    std::string downloadedUsername;
    std::vector<unsigned char> downloadedPng;
    int downloadedSrcW;
    int downloadedSrcH;
    bool downloadedWasSlim;
    bool alreadySaved;

    // Status message
    std::string statusMessage;
    int_t statusColor;

    // 3D preview state
    ModelBiped *previewModel;          // 64x32 fallback texture
    ModelPlayer *previewModelWide;     // 64x64 textures (the download preview)
    ModelPlayer *previewModelSlim;
    bool previewTexSlim;
    int_t previewTexId;
    float previewYaw;
    float previewWalkPos;
    float previewBobTicks;
    bool previewAutoRotate;

    // Legacy D-pad focus (0=input, 1=search, 2=popular, 3=save, 4=use, 5=cancel)
    int focusIndex;

    // Layout geometry
    int_t panelX;
    int_t panelY;
    int_t panelW;
    int_t panelH;
    int_t previewBoxX;
    int_t previewBoxY;
    int_t previewBoxW;
    int_t previewBoxH;
    int_t fieldX;
    int_t fieldY;
    int_t fieldW;
    int_t fieldH;
};
