#include "net/minecraft/src/ControlIcon.h"
#include "net/minecraft/src/UiStrings.h"
#include "GuiSkinSelector.h"
#include "GuiLoadSkinsMenu.h"
#include "SkinManager.h"
#include "GuiButton.h"
#include "Minecraft.h"
#include "FontRenderer.h"
#include "GameSettings.h"
#include "StringTranslate.h"
#include "EntityPlayerSP.h"
#include "RenderEngine.h"
#include "RenderHelper.h"
#include "ModelBiped.h"
#include "ModelRenderer.h"
#include "SoundManager.h"
#include "Tessellator.h"
#include "legacy/LegacyButtonPrompt.h"
#include "legacy/LegacyPanorama.h"
#include "legacy/LegacySceneState.h"
#include "legacy/LegacyUiTheme.h"
#include "platform/RenderAPI.h"
#include "platform/PlatformCompat.h"

#include "pc/lwjgl/Keyboard.h"

#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
#include "platform/Input.h"
#endif

#ifdef PS2_PLATFORM
#include "ps2/input/Ps2PadState.h"
#include "ps2/input/Ps2PadKeyCodes.h"
#endif
#ifdef XBOX_PLATFORM
#include "xbox/input/XboxPad.h"
#endif

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
constexpr int_t BUTTON_ID_TAB_DEFAULT = 1;
constexpr int_t BUTTON_ID_TAB_CUSTOM = 2;
constexpr int_t BUTTON_ID_PREV_SKIN = 3;
constexpr int_t BUTTON_ID_NEXT_SKIN = 4;
constexpr int_t BUTTON_ID_PLAYER2 = 10;
constexpr int_t BUTTON_ID_LOAD_SKINS = 11;
constexpr int_t BUTTON_ID_DELETE_SKIN = 12;
constexpr int_t BUTTON_ID_TAB_BASE = 100;

std::string toUpperString(const std::string &str)
{
    std::string result = str;
    for (char &c : result)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return result;
}
}

GuiSkinSelector::GuiSkinSelector(GuiScreen *parent, bool isPlayer2)
    : parentScreen(parent)
    , isPlayer2Skin(isPlayer2)
    , initializedSelection(false)
    , currentPackIndex(0)
    , currentSkinIndex(0)
    , scrollOffset(0.0f)
    , previewModel32(nullptr)
    , previewModel64(nullptr)
    , previewYaw(0.0f)
    , previewPitch(0.0f)
    , previewAutoRotate(true)
    , previewRotatingLeft(true)
    , previewWalkPos(0.0f)
    , previewWalkSpeed(0.32f)
    , previewPrevWalkSpeed(0.32f)
    , previewBobTicks(0.0f)
    , previewPoseIndex(0)
    , previewSwingTicks(0.0f)
    , previewPoseStickLatch(false)
    , navHoldRepeatTimer(0)
    , dialogLeft(0)
    , dialogTop(0)
    , dialogWidth(0)
    , dialogHeight(0)
    , leftPanelWidth(0)
    , rightPanelX(0)
    , rightPanelWidth(0)
    , carouselCenterX(0)
    , carouselGroundY(0)
    , nameplateY(0)
    , nameplateHeight(0)
    , legacyStageTop(0)
    , legacyStageBottom(0)
    , legacyPackBarY(0)
    , legacyPackBarH(0)
    , legacyCenterTabX(0)
    , legacyCenterTabW(0)
    , legacyLeftTabX(0)
    , legacyRightTabX(0)
    , legacySideTabW(0)
    , legacyNameplateX(0)
    , legacyNameplateW(0)
    , buttonPlayer2Skin(nullptr)
    , buttonLoadSkins(nullptr)
    , buttonDeleteSkin(nullptr)
#if PLATFORM_PS2
    , ps2ActionReleaseLatch(true)
    , stickNavLatched(false)
    , dpadRepeatTimer(0)
    , stickRepeatTimer(0)
    , lastPadHeld(0)
#endif
{
#if PLATFORM_PS2
    lastPadHeld = ps2PadGetSnapshot(platformMenuPad()).held;
#endif
#if PLATFORM_XBOX
    XboxPad::clearLatchedPressed();
#endif
    SkinManager::init();
    currentPackIndex = SkinManager::getSelectedPackIndex();
    currentSkinIndex = SkinManager::getSelectedIndex();
}

void GuiSkinSelector::recordPreviewTexture(const std::string &path)
{
    if (path.empty())
        return;
    if (std::find(loadedTextures.begin(), loadedTextures.end(), path) == loadedTextures.end())
        loadedTextures.push_back(path);
}

void GuiSkinSelector::releaseUnusedPreviewTextures()
{
    if (mc == nullptr || mc->renderEngine == nullptr)
        return;

    std::string p1Active = SkinManager::getActiveSkinTexture();
    std::string p2Active = SkinManager::getPlayer2SkinTexture();
    std::string defTex = SkinManager::getDefaultSkinTexture();

    for (const std::string &path : loadedTextures)
    {
        if (path.empty())
            continue;
        if (path == p1Active || path == p2Active || path == defTex || path == "/mob/char.png")
            continue;
        mc->renderEngine->releaseTexture(path);
    }
    loadedTextures.clear();
    mc->renderEngine->clearDecodedTextureCache();
}

GuiSkinSelector::~GuiSkinSelector()
{
    releaseUnusedPreviewTextures();
    delete previewModel32;
    previewModel32 = nullptr;
    delete previewModel64;
    previewModel64 = nullptr;
}

bool GuiSkinSelector::isLegacyMode() const
{
    return mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
}

void GuiSkinSelector::initGui()
{
#if PLATFORM_XBOX
    XboxPad::clearLatchedPressed();
#endif
    controlList.clear();
    packButtons.clear();
    buttonPlayer2Skin = nullptr;
    buttonLoadSkins = nullptr;
    buttonDeleteSkin = nullptr;

    StringTranslate *tr = StringTranslate::getInstance();
    const bool isEs = (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);

    if (!initializedSelection)
    {
        if (isPlayer2Skin)
        {
            std::string p2Skin = "TennisSteve";
            if (mc != nullptr && mc->gameSettings != nullptr && !mc->gameSettings->selectedSkinP2.empty())
                p2Skin = mc->gameSettings->selectedSkinP2;
            else
                p2Skin = SkinManager::getSelectedSkinIdP2();

            const SkinEntry *entry = SkinManager::getSkinById(p2Skin);
            if (entry != nullptr && entry->isCustom)
            {
                currentPackIndex = 1;
                currentSkinIndex = SkinManager::getIndexById(p2Skin);
            }
            else
            {
                currentPackIndex = 0;
                currentSkinIndex = SkinManager::getIndexById(p2Skin);
            }
        }
        else
        {
            if (mc != nullptr && mc->gameSettings != nullptr && !mc->gameSettings->selectedSkin.empty())
            {
                SkinManager::setSelectedSkinId(mc->gameSettings->selectedSkin);
                currentPackIndex = SkinManager::getSelectedPackIndex();
                currentSkinIndex = SkinManager::getSelectedIndex();
            }
        }
        initializedSelection = true;
    }

    if (currentPackIndex >= SkinManager::getPackCount())
        currentPackIndex = 0;

    const int totalInPack = SkinManager::getSkinCountForPack(currentPackIndex);
    if (totalInPack > 0)
    {
        if (currentSkinIndex < 0 || currentSkinIndex >= totalInPack)
            currentSkinIndex = 0;
    }

    if (isLegacyMode())
    {
        const bool hdLayout = (height >= 300 || width >= 480);
        legacyStageTop = hdLayout ? 40 : 24;
        legacyStageBottom = height - (hdLayout ? 28 : 22);
        legacyPackBarY = legacyStageTop + (hdLayout ? 5 : 3);
        legacyPackBarH = hdLayout ? 22 : 16;

        legacyCenterTabW = std::clamp<int_t>(width * 34 / 100, 108, 220);
        legacySideTabW = std::clamp<int_t>(width * 24 / 100, 72, 156);
        const int_t tabGap = hdLayout ? 6 : 4;
        legacyCenterTabX = (width - legacyCenterTabW) / 2;
        legacyLeftTabX = legacyCenterTabX - tabGap - legacySideTabW;
        legacyRightTabX = legacyCenterTabX + legacyCenterTabW + tabGap;

        nameplateHeight = hdLayout ? 28 : 22;
        nameplateY = legacyStageBottom - nameplateHeight - (hdLayout ? 5 : 3);
        legacyNameplateW = std::clamp<int_t>(width * 40 / 100, 128, 250);
        legacyNameplateX = (width - legacyNameplateW) / 2;

        carouselCenterX = width / 2;
        carouselGroundY = nameplateY - (hdLayout ? 18 : 13);
        return;
    }

    // Dialog layout sizing (non-Legacy UI)
    dialogWidth = std::min<int_t>(width - 16, 384);
    dialogHeight = std::min<int_t>(height - 48, 172);
    dialogLeft = (width - dialogWidth) / 2;
    dialogTop = 12;

    leftPanelWidth = dialogWidth * 30 / 100;
    rightPanelX = dialogLeft + leftPanelWidth + 4;
    rightPanelWidth = dialogWidth - leftPanelWidth - 4;

    carouselCenterX = rightPanelX + rightPanelWidth / 2;
    nameplateHeight = 20;
    nameplateY = dialogTop + dialogHeight - nameplateHeight - 8;
    carouselGroundY = nameplateY - 6;

    // Tabs in left panel (placed below pack cover art)
    const int_t packBtnX = dialogLeft + 6;
    const int_t packBtnW = leftPanelWidth - 12;
    const int numPacks = SkinManager::getPackCount();
    const int_t btnH = (numPacks > 3) ? 15 : 18;
    const int_t btnGap = 2;
    const int_t artSize = (numPacks > 3) ? 40 : std::min<int_t>(leftPanelWidth - 20, 56);
    const int_t tabY = dialogTop + artSize + 12;

    for (int p = 0; p < numPacks; ++p)
    {
        std::string pName = SkinManager::getPackName(p);
        if (p == 0)
            pName = isEs ? "Originales" : "Default";
        else if (pName == "Custom Skins")
            pName = std::string(isEs ? "Personal." : "Custom") + " (" + std::to_string(SkinManager::getSkinCountForPack(p)) + ")";

        std::string label = (currentPackIndex == p) ? ("> " + pName + " <") : pName;
        GuiButton *btn = new GuiButton(BUTTON_ID_TAB_BASE + p, packBtnX, tabY + p * (btnH + btnGap), packBtnW, btnH, label);
        packButtons.push_back(btn);
        controlList.push_back(btn);
    }

    // Bottom action buttons
    const int_t btnY = dialogTop + dialogHeight + 4;
    const int_t p2BtnWidth = 140;
    const int_t p2BtnHeight = 18;
    const int_t p2BtnX = dialogLeft + dialogWidth - p2BtnWidth;

    if (!isPlayer2Skin)
    {
        std::string p2BtnText = isEs ? "Skin 2do Jugador" : "Choose 2nd Player Skin";
        buttonPlayer2Skin = new GuiButton(BUTTON_ID_PLAYER2, p2BtnX, btnY, p2BtnWidth, p2BtnHeight, p2BtnText);
        buttonPlayer2Skin->enabled = true;
        controlList.push_back(buttonPlayer2Skin);
    }
    else
    {
        buttonPlayer2Skin = nullptr;
    }

    const int_t loadBtnWidth = 85;
    const int_t loadBtnX = (buttonPlayer2Skin != nullptr) ? (p2BtnX - loadBtnWidth - 5) : (dialogLeft + dialogWidth - loadBtnWidth);
    std::string loadBtnText = isEs ? "Cargar Skins" : "Load Skins";
    buttonLoadSkins = new GuiButton(BUTTON_ID_LOAD_SKINS, loadBtnX, btnY, loadBtnWidth, p2BtnHeight, loadBtnText);
    controlList.push_back(buttonLoadSkins);

    const bool isCustomPack = (SkinManager::getPackName(currentPackIndex) == "Custom Skins");
    if (isCustomPack && !SkinManager::getCustomSkins().empty())
    {
        const int_t delBtnWidth = 85;
        std::string delBtnText = isEs ? "Eliminar Skin" : "Delete Skin";
        buttonDeleteSkin = new GuiButton(BUTTON_ID_DELETE_SKIN, dialogLeft, btnY, delBtnWidth, p2BtnHeight, delBtnText);
        controlList.push_back(buttonDeleteSkin);
    }
    else
    {
        buttonDeleteSkin = nullptr;
    }
}

void GuiSkinSelector::switchPack(int newPackIndex)
{
    const int packCount = SkinManager::getPackCount();
    if (packCount <= 0)
        return;
    newPackIndex = ((newPackIndex % packCount) + packCount) % packCount;
    if (newPackIndex == currentPackIndex)
        return;

    releaseUnusedPreviewTextures();

    currentPackIndex = newPackIndex;
    if (!isPlayer2Skin)
        SkinManager::setSelectedPackIndex(currentPackIndex);
    currentSkinIndex = 0;
    scrollOffset = 0.0f;

    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.1f);

    initGui();
}

void GuiSkinSelector::togglePlayer2Mode()
{
    isPlayer2Skin = !isPlayer2Skin;
    initializedSelection = false;
    scrollOffset = 0.0f;

    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);

    initGui();
}

void GuiSkinSelector::deleteCurrentCustomSkin()
{
    if (SkinManager::getPackName(currentPackIndex) != "Custom Skins")
        return;

    const auto &customs = SkinManager::getCustomSkins();
    if (customs.empty() || currentSkinIndex < 0 || currentSkinIndex >= static_cast<int>(customs.size()))
        return;

    std::string skinId = customs[currentSkinIndex].id;
    bool deleted = SkinManager::deleteCustomSkin(skinId);
    if (!deleted)
        return;

    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.break", 1.0f, 1.0f);

    // If no custom skins are left, return immediately to the main menu as requested
    if (SkinManager::getCustomSkins().empty())
    {
        cancelAndReturn();
        return;
    }

    if (currentSkinIndex >= static_cast<int>(SkinManager::getCustomSkins().size()))
        currentSkinIndex = static_cast<int>(SkinManager::getCustomSkins().size()) - 1;

    initGui();
}

void GuiSkinSelector::handleSpecializedMenuInput()
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    if (!pad.connected)
        return;

#if PLATFORM_PS2
    const Ps2PadSnapshot &ps2Pad = ps2PadGetSnapshot(platformMenuPad());
    const unsigned short held = ps2Pad.held;
    unsigned short pressed = held & ~lastPadHeld;
    lastPadHeld = held;

    if (ps2ActionReleaseLatch)
    {
        pressed &= ~PS2_PAD_CROSS;
        if ((held & PS2_PAD_CROSS) == 0)
            ps2ActionReleaseLatch = false;
    }

    // Circle: Cancel / Return
    if ((pressed & PS2_PAD_CIRCLE) != 0)
    {
        cancelAndReturn();
        return;
    }

    // Triangle: Open Load Skins screen
    if ((pressed & PS2_PAD_TRIANGLE) != 0)
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        mc->displayGuiScreen(new GuiLoadSkinsMenu(this));
        return;
    }

    // Square: Delete current custom skin (when on custom pack), or toggle P2 skin in Legacy mode
    if ((pressed & PS2_PAD_SQUARE) != 0)
    {
        if (SkinManager::getPackName(currentPackIndex) == "Custom Skins")
        {
            deleteCurrentCustomSkin();
            return;
        }
        else if (isLegacyMode())
        {
            togglePlayer2Mode();
            return;
        }
    }

    // Select: Toggle Player 2 skin in Legacy mode
    if (isLegacyMode() && (pressed & PS2_PAD_SELECT) != 0)
    {
        togglePlayer2Mode();
        return;
    }

    // R3: Reset 3D rotation in Legacy mode
    if (isLegacyMode() && (pressed & PS2_PAD_R3) != 0)
    {
        previewYaw = 0.0f;
        previewPitch = 0.0f;
        previewAutoRotate = true;
        previewPoseIndex = 0;
    }

    // L2 / R2 (and D-Pad Up/Down in Legacy mode): Switch between skin packs (tabs)
    if ((pressed & PS2_PAD_L2) != 0 || (isLegacyMode() && (pad.pressed & PLATFORM_TEXT_UP) != 0))
    {
        if (SkinManager::getPackCount() > 1)
        {
            switchPack(currentPackIndex - 1);
            return;
        }
    }
    if ((pressed & PS2_PAD_R2) != 0 || (isLegacyMode() && (pad.pressed & PLATFORM_TEXT_DOWN) != 0))
    {
        if (SkinManager::getPackCount() > 1)
        {
            switchPack(currentPackIndex + 1);
            return;
        }
    }

    if (isLegacyMode())
    {
        if ((pad.pressed & (PLATFORM_TEXT_LEFT | PLATFORM_TEXT_TAB_LEFT)) != 0 || (pressed & PS2_PAD_L1) != 0)
            prevSkin();
        else if ((pad.pressed & (PLATFORM_TEXT_RIGHT | PLATFORM_TEXT_TAB_RIGHT)) != 0 || (pressed & PS2_PAD_R1) != 0)
            nextSkin();

        if ((pressed & PS2_PAD_CROSS) != 0)
        {
            selectAndConfirm();
            return;
        }
        return;
    }

    // L1 / R1: Navigate carousel skins (D-Pad disabled for carousel in non-Legacy mode to avoid conflict with pointer)
    bool movedLeft = false;
    bool movedRight = false;

    if ((pressed & PS2_PAD_L1) != 0)
    {
        movedLeft = true;
        dpadRepeatTimer = 0;
    }
    else if ((held & PS2_PAD_L1) != 0)
    {
        dpadRepeatTimer++;
        if (dpadRepeatTimer > 15 && (dpadRepeatTimer % 5) == 0)
            movedLeft = true;
    }

    if ((pressed & PS2_PAD_R1) != 0)
    {
        movedRight = true;
        dpadRepeatTimer = 0;
    }
    else if ((held & PS2_PAD_R1) != 0)
    {
        dpadRepeatTimer++;
        if (dpadRepeatTimer > 15 && (dpadRepeatTimer % 5) == 0)
            movedRight = true;
    }

    if (movedLeft) prevSkin();
    if (movedRight) nextSkin();

    // Confirm selection (Cross) - ONLY when pointer is NOT hovering over an active GUI button
    if ((pressed & PS2_PAD_CROSS) != 0)
    {
        int mx = 0, my = 0;
        PlatformCompat::getMouseState(&mx, &my);
        int mouseX = (mc != nullptr && mc->displayWidth > 0) ? (mx * width) / mc->displayWidth : 0;
        int mouseY = (mc != nullptr && mc->displayHeight > 0) ? (my * height) / mc->displayHeight : 0;
        bool overButton = false;
        for (GuiButton *btn : controlList)
        {
            if (btn != nullptr && btn->enabled && btn->mousePressed(mc, mouseX, mouseY))
            {
                overButton = true;
                break;
            }
        }
        if (!overButton)
        {
            while (lwjgl::Mouse::next()) {}
            selectAndConfirm();
            return;
        }
    }

#elif PLATFORM_WII || PLATFORM_XBOX
    if (isLegacyMode())
    {
#if PLATFORM_XBOX
        const XboxPadSnapshot &xpad = XboxPad::playerSnapshot(XboxPad::menuPlayer());
        if ((xpad.pressed & XBOX_PAD_LT) != 0 && SkinManager::getPackCount() > 1)
        {
            switchPack(currentPackIndex - 1);
            return;
        }
        if ((xpad.pressed & XBOX_PAD_RT) != 0 && SkinManager::getPackCount() > 1)
        {
            switchPack(currentPackIndex + 1);
            return;
        }
        if ((xpad.pressed & XBOX_PAD_RIGHT_THUMB) != 0)
        {
            previewYaw = 0.0f;
            previewPitch = 0.0f;
            previewAutoRotate = true;
            previewPoseIndex = 0;
        }
#endif
        // B / Cancel: Return to parent screen
        if ((pad.pressed & PLATFORM_TEXT_CLOSE) != 0
#if PLATFORM_WII
            || (pad.pressed & PLATFORM_TEXT_BACK) != 0
#endif
        )
        {
            cancelAndReturn();
            return;
        }

        // Y (PLATFORM_TEXT_SHIFT): Open Load Skins menu
        if ((pad.pressed & PLATFORM_TEXT_SHIFT) != 0)
        {
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
            mc->displayGuiScreen(new GuiLoadSkinsMenu(this));
            return;
        }

#if PLATFORM_XBOX
        // X (PLATFORM_TEXT_BACK on Xbox): Delete custom skin on custom pack, or toggle Player 2 mode
        if ((pad.pressed & PLATFORM_TEXT_BACK) != 0)
        {
            if (SkinManager::getPackName(currentPackIndex) == "Custom Skins" && !SkinManager::getCustomSkins().empty())
                deleteCurrentCustomSkin();
            else
                togglePlayer2Mode();
            return;
        }

        // Back button (PLATFORM_TEXT_SPACE on Xbox): Toggle Player 2 mode
        if ((pad.pressed & PLATFORM_TEXT_SPACE) != 0)
        {
            togglePlayer2Mode();
            return;
        }
#endif

        // D-Pad / Left Stick Left/Right or White/Black (L1/R1): Prev / Next skin
        const std::uint32_t leftMask = PLATFORM_TEXT_LEFT | PLATFORM_TEXT_TAB_LEFT;
        const std::uint32_t rightMask = PLATFORM_TEXT_RIGHT | PLATFORM_TEXT_TAB_RIGHT;
        if ((pad.pressed & leftMask) != 0)
        {
            prevSkin();
            navHoldRepeatTimer = 0;
        }
        else if ((pad.pressed & rightMask) != 0)
        {
            nextSkin();
            navHoldRepeatTimer = 0;
        }
        else if ((pad.held & (leftMask | rightMask)) != 0)
        {
            ++navHoldRepeatTimer;
            if (navHoldRepeatTimer >= 16 && (navHoldRepeatTimer % 5) == 0)
            {
                if ((pad.held & leftMask) != 0)
                    prevSkin();
                else if ((pad.held & rightMask) != 0)
                    nextSkin();
            }
        }
        else
        {
            navHoldRepeatTimer = 0;
        }

        // D-Pad / Left Stick Up/Down: Switch skin pack
        if ((pad.pressed & PLATFORM_TEXT_UP) != 0 && SkinManager::getPackCount() > 1)
        {
            switchPack(currentPackIndex - 1);
            return;
        }
        if ((pad.pressed & PLATFORM_TEXT_DOWN) != 0 && SkinManager::getPackCount() > 1)
        {
            switchPack(currentPackIndex + 1);
            return;
        }

        // A / Confirm: Select and save skin
        if ((pad.pressed & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_ENTER)) != 0)
        {
            selectAndConfirm();
            return;
        }
        return;
    }

    // Non-Legacy mode: Wii Classic Controller / GameCube / Wiimote D-Pad; Xbox pad
    if ((pad.pressed & (PLATFORM_TEXT_BACK | PLATFORM_TEXT_CLOSE)) != 0)
    {
        cancelAndReturn();
        return;
    }

    if ((pad.pressed & PLATFORM_TEXT_LEFT) != 0)
        prevSkin();
    else if ((pad.pressed & PLATFORM_TEXT_RIGHT) != 0)
        nextSkin();

    if ((pad.pressed & PLATFORM_TEXT_UP) != 0 && SkinManager::getPackCount() > 1)
    {
        switchPack(currentPackIndex - 1);
        return;
    }
    if ((pad.pressed & PLATFORM_TEXT_DOWN) != 0 && SkinManager::getPackCount() > 1)
    {
        switchPack(currentPackIndex + 1);
        return;
    }

    if ((pad.pressed & PLATFORM_TEXT_TYPE) != 0)
    {
        selectAndConfirm();
        return;
    }
#endif
#endif
}

void GuiSkinSelector::updateScreen()
{
    GuiScreen::updateScreen();

    // Smoothly dampen carousel scrolling transition
    if (std::fabs(scrollOffset) > 0.001f)
    {
        scrollOffset *= 0.65f;
        if (std::fabs(scrollOffset) < 0.002f)
            scrollOffset = 0.0f;
    }

    if (isLegacyMode())
    {
        previewPrevWalkSpeed = previewWalkSpeed;
        const float targetSpeed = (previewPoseIndex == 1) ? 0.14f : 0.34f;
        previewWalkSpeed += (targetSpeed - previewWalkSpeed) * 0.25f;
        previewWalkPos += previewWalkSpeed * 1.15f;
        previewBobTicks += 1.0f;

        if (previewPoseIndex == 2)
        {
            previewSwingTicks += 0.12f;
            if (previewSwingTicks >= 1.0f)
                previewSwingTicks = 0.0f;
        }
        else
        {
            previewSwingTicks = 0.0f;
        }
    }
}

void GuiSkinSelector::keyTyped(char_t c, int_t key)
{
    if (key == 1 || key == lwjgl::Keyboard::KEY_ESCAPE) // ESC
    {
        cancelAndReturn();
        return;
    }
    if (key == lwjgl::Keyboard::KEY_RETURN || key == lwjgl::Keyboard::KEY_NUMPADENTER)
    {
        selectAndConfirm();
        return;
    }

    // Left arrow or A: Previous skin
    if (key == lwjgl::Keyboard::KEY_LEFT || key == lwjgl::Keyboard::KEY_A)
    {
        prevSkin();
        return;
    }

    // Right arrow or D: Next skin
    if (key == lwjgl::Keyboard::KEY_RIGHT || key == lwjgl::Keyboard::KEY_D)
    {
        nextSkin();
        return;
    }

    // Up / Down arrows in Legacy mode: Switch pack
    if (isLegacyMode() && (key == lwjgl::Keyboard::KEY_UP || key == lwjgl::Keyboard::KEY_W))
    {
        if (SkinManager::getPackCount() > 1)
            switchPack(currentPackIndex - 1);
        return;
    }
    if (isLegacyMode() && (key == lwjgl::Keyboard::KEY_DOWN || key == lwjgl::Keyboard::KEY_S))
    {
        if (SkinManager::getPackCount() > 1)
            switchPack(currentPackIndex + 1);
        return;
    }

    // Q / E in Legacy mode: Rotate 3D model manually
    if (isLegacyMode() && (key == lwjgl::Keyboard::KEY_Q || c == 'q' || c == 'Q'))
    {
        previewAutoRotate = false;
        previewYaw -= 12.0f;
        return;
    }
    if (isLegacyMode() && (key == lwjgl::Keyboard::KEY_E || c == 'e' || c == 'E'))
    {
        previewAutoRotate = false;
        previewYaw += 12.0f;
        return;
    }

    // P in Legacy mode: Toggle Player 2 skin selector
    if (isLegacyMode() && (c == 'p' || c == 'P' || c == 'x' || c == 'X'))
    {
        togglePlayer2Mode();
        return;
    }

    // L or Y in Legacy mode: Open Load Skins menu
    if (isLegacyMode() && (c == 'l' || c == 'L' || c == 'y' || c == 'Y'))
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        mc->displayGuiScreen(new GuiLoadSkinsMenu(this));
        return;
    }

    // Tab: Switch pack/tab
    if (key == lwjgl::Keyboard::KEY_TAB)
    {
        if (SkinManager::getPackCount() > 1)
            switchPack(currentPackIndex + 1);
        return;
    }

    // Delete: Delete current custom skin
    if (key == lwjgl::Keyboard::KEY_DELETE)
    {
        deleteCurrentCustomSkin();
        return;
    }

    GuiScreen::keyTyped(c, key);
}

void GuiSkinSelector::nextSkin()
{
    const int total = SkinManager::getSkinCountForPack(currentPackIndex);
    if (total <= 0)
        return;

    currentSkinIndex = (currentSkinIndex + 1) % total;
    scrollOffset -= 1.0f;
    if (scrollOffset < -1.5f)
        scrollOffset = -1.5f;

    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.2f);
}

void GuiSkinSelector::prevSkin()
{
    const int total = SkinManager::getSkinCountForPack(currentPackIndex);
    if (total <= 0)
        return;

    currentSkinIndex = ((currentSkinIndex - 1) % total + total) % total;
    scrollOffset += 1.0f;
    if (scrollOffset > 1.5f)
        scrollOffset = 1.5f;

    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.2f);
}

void GuiSkinSelector::selectAndConfirm()
{
    const SkinEntry *skin = SkinManager::getSkin(currentPackIndex, currentSkinIndex);
    if (skin != nullptr)
    {
        if (isPlayer2Skin)
        {
            SkinManager::setSelectedSkinIdP2(skin->id);
            if (mc != nullptr && mc->gameSettings != nullptr)
            {
                mc->gameSettings->selectedSkinP2 = skin->id;
                mc->gameSettings->saveOptions();
            }
        }
        else
        {
            SkinManager::setSelectedPackIndex(currentPackIndex);
            SkinManager::setSelectedSkinId(skin->id);
            if (mc != nullptr && mc->gameSettings != nullptr)
            {
                mc->gameSettings->selectedSkin = skin->id;
                mc->gameSettings->saveOptions();
            }
            if (mc != nullptr && mc->thePlayer != nullptr)
            {
                mc->thePlayer->setEntityTexture(skin->modelPath);
                mc->thePlayer->skinUrl = "";
            }
        }
    }

    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.action", 1.0f, 1.0f);

    mc->displayGuiScreen(parentScreen);
}

void GuiSkinSelector::cancelAndReturn()
{
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);

    mc->displayGuiScreen(parentScreen);
}

void GuiSkinSelector::mouseClicked(int_t mouseX, int_t mouseY, int_t button)
{
    GuiScreen::mouseClicked(mouseX, mouseY, button);

    if (button != 0)
        return;

    if (isLegacyMode())
    {
        // Click on Left or Right pack tabs switches packs
        if (mouseY >= legacyPackBarY && mouseY <= legacyPackBarY + legacyPackBarH)
        {
            const int pc = SkinManager::getPackCount();
            if (mouseX >= legacyLeftTabX && mouseX <= legacyLeftTabX + legacySideTabW)
            {
                if (pc > 1)
                    switchPack(currentPackIndex - 1);
                return;
            }
            if (mouseX >= legacyRightTabX && mouseX <= legacyRightTabX + legacySideTabW)
            {
                if (pc > 1)
                    switchPack(currentPackIndex + 1);
                return;
            }
            if (mouseX >= legacyCenterTabX && mouseX <= legacyCenterTabX + legacyCenterTabW)
            {
                if (pc > 1)
                    switchPack(currentPackIndex + 1);
                return;
            }
        }

        // Click on nameplate confirms
        if (mouseY >= nameplateY && mouseY <= nameplateY + nameplateHeight &&
            mouseX >= legacyNameplateX && mouseX <= legacyNameplateX + legacyNameplateW)
        {
            selectAndConfirm();
            return;
        }

        // Click inside character stage area
        const int_t stageTopY = legacyPackBarY + legacyPackBarH + 4;
        const int_t stageBottomY = nameplateY - 2;
        if (mouseY >= stageTopY && mouseY <= stageBottomY)
        {
            const int_t centerHalfW = std::max<int_t>(26, width / 12);
            if (mouseX >= carouselCenterX - centerHalfW && mouseX <= carouselCenterX + centerHalfW)
            {
                selectAndConfirm();
                return;
            }
            if (mouseX < carouselCenterX - centerHalfW)
            {
                prevSkin();
                return;
            }
            if (mouseX > carouselCenterX + centerHalfW)
            {
                nextSkin();
                return;
            }
        }
        return;
    }

    // If an active button was clicked, don't trigger carousel actions
    for (GuiButton *btn : controlList)
    {
        if (btn != nullptr && btn->enabled && btn->mousePressed(mc, mouseX, mouseY))
            return;
    }

    // Click on nameplate confirms
    if (mouseY >= nameplateY && mouseY <= nameplateY + nameplateHeight &&
        mouseX >= rightPanelX && mouseX <= rightPanelX + rightPanelWidth)
    {
        selectAndConfirm();
        return;
    }

    // Click on center character confirms
    if (mouseX >= carouselCenterX - 25 && mouseX <= carouselCenterX + 25 &&
        mouseY >= carouselGroundY - 75 && mouseY <= carouselGroundY)
    {
        selectAndConfirm();
        return;
    }

    // Click on left skin in carousel advances to left
    if (mouseX >= carouselCenterX - 85 && mouseX < carouselCenterX - 25 &&
        mouseY >= carouselGroundY - 65 && mouseY <= carouselGroundY)
    {
        prevSkin();
        return;
    }

    // Click on right skin in carousel advances to right
    if (mouseX > carouselCenterX + 25 && mouseX <= carouselCenterX + 85 &&
        mouseY >= carouselGroundY - 65 && mouseY <= carouselGroundY)
    {
        nextSkin();
        return;
    }
}

void GuiSkinSelector::actionPerformed(GuiButton *button)
{
    if (!button->enabled)
        return;

    if (button->id >= BUTTON_ID_TAB_BASE && button->id < BUTTON_ID_TAB_BASE + SkinManager::getPackCount())
    {
        switchPack(button->id - BUTTON_ID_TAB_BASE);
    }
    else if (button->id == BUTTON_ID_TAB_DEFAULT)
    {
        switchPack(0);
    }
    else if (button->id == BUTTON_ID_TAB_CUSTOM)
    {
        switchPack(1);
    }
    else if (button->id == BUTTON_ID_PLAYER2)
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        mc->displayGuiScreen(new GuiSkinSelector(this, true));
    }
    else if (button->id == BUTTON_ID_LOAD_SKINS)
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        mc->displayGuiScreen(new GuiLoadSkinsMenu(this));
    }
    else if (button->id == BUTTON_ID_DELETE_SKIN)
    {
        deleteCurrentCustomSkin();
    }
}

bool GuiSkinSelector::doesGuiPauseGame()
{
    return true;
}

// ----------------------------------------------------------------------------
// Custom Visual Drawing Helpers
// ----------------------------------------------------------------------------

void GuiSkinSelector::drawBeveledPanel(int_t left, int_t top, int_t right, int_t bottom, int_t fillColor)
{
    drawRect(left, top, right, bottom, fillColor);
    drawRect(left, top, right, top + 1, 0x40FFFFFF);
    drawRect(left, top, left + 1, bottom, 0x40FFFFFF);
    drawRect(left, bottom - 1, right, bottom, 0xFF000000);
    drawRect(right - 1, top, right, bottom, 0xFF000000);
}

void GuiSkinSelector::drawInsetPanel(int_t left, int_t top, int_t right, int_t bottom, int_t fillColor)
{
    drawRect(left, top, right, bottom, fillColor);
    drawRect(left, top, right, top + 1, 0xFF000000);
    drawRect(left, top, left + 1, bottom, 0xFF000000);
    drawRect(left, bottom - 1, right, bottom, 0x30FFFFFF);
    drawRect(right - 1, top, right, bottom, 0x30FFFFFF);
}

void GuiSkinSelector::drawFeetShadow(float centerX, float groundY, float radiusX, float radiusY, float alpha)
{
    renderEnable(RenderCapability::Blend);
    renderDisable(RenderCapability::Texture2D);
    renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);

    int a = static_cast<int>(alpha * 90.0f);
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    int color = (a << 24) | 0x000000;

    int_t x1 = static_cast<int_t>(centerX - radiusX);
    int_t x2 = static_cast<int_t>(centerX + radiusX);
    int_t y1 = static_cast<int_t>(groundY - radiusY * 0.5f);
    int_t y2 = static_cast<int_t>(groundY + radiusY * 0.5f);

    drawRect(x1 + 2, y1, x2 - 2, y2, color);
    renderDisable(RenderCapability::Blend);
    renderEnable(RenderCapability::Texture2D);
}

void GuiSkinSelector::drawFrontPreview(const SkinEntry *skin, float x, float y, float w, float h, float alpha)
{
    if (skin == nullptr || mc == nullptr || mc->renderEngine == nullptr)
        return;

    if (!skin->frontPath.empty()) recordPreviewTexture(skin->frontPath);
    if (!skin->skinPath.empty()) recordPreviewTexture(skin->skinPath);
    if (!skin->modelPath.empty()) recordPreviewTexture(skin->modelPath);

    // If pre-rendered 16x32 front preview is present, draw single quad
    if (!skin->isCustom && !skin->frontPath.empty())
    {
        int texId = mc->renderEngine->getTexture(skin->frontPath);
        if (texId >= 0)
        {
            mc->renderEngine->bindTexture(texId);
            renderEnable(RenderCapability::Texture2D);
            renderEnable(RenderCapability::Blend);
            renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
            renderColor4f(alpha, alpha, alpha, alpha);

            Tessellator &tess = Tessellator::instance;
            tess.startDrawingQuads();
            tess.addVertexWithUV(x,     y + h, zLevel, 0.0f, 1.0f);
            tess.addVertexWithUV(x + w, y + h, zLevel, 1.0f, 1.0f);
            tess.addVertexWithUV(x + w, y,     zLevel, 1.0f, 0.0f);
            tess.addVertexWithUV(x,     y,     zLevel, 0.0f, 0.0f);
            tess.draw();
            renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            return;
        }
    }

    // Custom skin (or fallback): Assemble the 2D front character dynamically
    int texId = -1;
    if (!skin->frontPath.empty())
        texId = mc->renderEngine->getTexture(skin->frontPath);

    if (texId < 0 && !skin->skinPath.empty())
        texId = mc->renderEngine->getTexture(skin->skinPath);

    if (texId < 0 && !skin->modelPath.empty())
        texId = mc->renderEngine->getTexture(skin->modelPath);

    if (texId < 0)
        return;

    mc->renderEngine->bindTexture(texId);
    renderEnable(RenderCapability::Texture2D);
    renderEnable(RenderCapability::Blend);
    renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
    renderColor4f(alpha, alpha, alpha, alpha);

    int_t tw = 64, th = 32;
    mc->renderEngine->getTextureDimensions(texId, &tw, &th);
    if (th <= 0) th = 32;

    // If the texture loaded is already a 16x32 front preview, draw single quad
    if (tw == 16 && th == 32)
    {
        Tessellator &tess = Tessellator::instance;
        tess.startDrawingQuads();
        tess.addVertexWithUV(x,     y + h, zLevel, 0.0f, 1.0f);
        tess.addVertexWithUV(x + w, y + h, zLevel, 1.0f, 1.0f);
        tess.addVertexWithUV(x + w, y,     zLevel, 1.0f, 0.0f);
        tess.addVertexWithUV(x,     y,     zLevel, 0.0f, 0.0f);
        tess.draw();
        renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        return;
    }

    // Assemble standing body parts from standard skin sheet
    const float uScale = 1.0f / 64.0f;
    const float vScale = 1.0f / static_cast<float>(th);
    const float unitW = w / 16.0f;
    const float unitH = h / 32.0f;

    Tessellator &tess = Tessellator::instance;
    tess.startDrawingQuads();

    auto drawQuad = [&](float qx, float qy, float qw, float qh, float su0, float sv0, float su1, float sv1) {
        tess.addVertexWithUV(qx,      qy + qh, zLevel, su0, sv1);
        tess.addVertexWithUV(qx + qw, qy + qh, zLevel, su1, sv1);
        tess.addVertexWithUV(qx + qw, qy,      zLevel, su1, sv0);
        tess.addVertexWithUV(qx,      qy,      zLevel, su0, sv0);
    };

    // 1. Head front (8,8 to 16,16)
    drawQuad(x + 4.0f * unitW, y, 8.0f * unitW, 8.0f * unitH, 8.0f * uScale, 8.0f * vScale, 16.0f * uScale, 16.0f * vScale);

    // 2. Hat overlay (40,8 to 48,16)
    drawQuad(x + 3.5f * unitW, y - 0.5f * unitH, 9.0f * unitW, 9.0f * unitH, 40.0f * uScale, 8.0f * vScale, 48.0f * uScale, 16.0f * vScale);

    // 3. Torso front (20,20 to 28,32)
    drawQuad(x + 4.0f * unitW, y + 8.0f * unitH, 8.0f * unitW, 12.0f * unitH, 20.0f * uScale, 20.0f * vScale, 28.0f * uScale, 32.0f * vScale);

    // 4. Right Arm front (44,20 to 48,32)
    drawQuad(x, y + 8.0f * unitH, 4.0f * unitW, 12.0f * unitH, 44.0f * uScale, 20.0f * vScale, 48.0f * uScale, 32.0f * vScale);

    // 5. Left Arm front
    if (th == 64)
        drawQuad(x + 12.0f * unitW, y + 8.0f * unitH, 4.0f * unitW, 12.0f * unitH, 36.0f * uScale, 52.0f * vScale, 40.0f * uScale, 64.0f * vScale);
    else
        drawQuad(x + 12.0f * unitW, y + 8.0f * unitH, 4.0f * unitW, 12.0f * unitH, 48.0f * uScale, 20.0f * vScale, 44.0f * uScale, 32.0f * vScale);

    // 6. Right Leg front (4,20 to 8,32)
    drawQuad(x + 4.0f * unitW, y + 20.0f * unitH, 4.0f * unitW, 12.0f * unitH, 4.0f * uScale, 20.0f * vScale, 8.0f * uScale, 32.0f * vScale);

    // 7. Left Leg front
    if (th == 64)
        drawQuad(x + 8.0f * unitW, y + 20.0f * unitH, 4.0f * unitW, 12.0f * unitH, 20.0f * uScale, 52.0f * vScale, 24.0f * uScale, 64.0f * vScale);
    else
        drawQuad(x + 8.0f * unitW, y + 20.0f * unitH, 4.0f * unitW, 12.0f * unitH, 8.0f * uScale, 20.0f * vScale, 4.0f * uScale, 32.0f * vScale);

    tess.draw();
    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// ----------------------------------------------------------------------------
// LCE (Legacy Console Edition) 3D Skin Carousel & Scene
// ----------------------------------------------------------------------------

void GuiSkinSelector::drawLegacyCharacter3D(const SkinEntry *skin, float centerX, float feetY, float scale,
                                            float yawDeg, float pitchDeg, bool isCenterSlot,
                                            float brightness, float partialTick)
{
    if (skin == nullptr || mc == nullptr || mc->renderEngine == nullptr)
        return;

    std::string texPath = !skin->modelPath.empty() ? skin->modelPath : skin->skinPath;
    if (texPath.empty())
        texPath = SkinManager::getDefaultSkinTexture();
    recordPreviewTexture(texPath);

    int_t texId = mc->renderEngine->getTexture(texPath);
    if (texId < 0 || mc->renderEngine->isTextureFailed(texPath))
    {
        texPath = SkinManager::getDefaultSkinTexture();
        texId = mc->renderEngine->getTexture(texPath);
    }
    if (texId < 0)
        return;

    int_t tw = 64, th = 32;
    mc->renderEngine->getTextureDimensions(texId, &tw, &th);

    ModelBiped *model = nullptr;
    if (tw == 64 && th == 64)
    {
        if (previewModel64 == nullptr)
            previewModel64 = new ModelBiped(0.0f, 0.0f, 64, 64);
        model = previewModel64;
    }
    else
    {
        if (previewModel32 == nullptr)
            previewModel32 = new ModelBiped(0.0f, 0.0f, 64, 32);
        model = previewModel32;
    }

    mc->renderEngine->bindTexture(texId);
    renderEnable(RenderCapability::Texture2D);

    float walkSpeed = 0.0f;
    float walkPos = 0.0f;
    float bob = 0.0f;
    float headYaw = 0.0f;
    float headPitch = 0.0f;

    if (isCenterSlot)
    {
        walkSpeed = previewPrevWalkSpeed + (previewWalkSpeed - previewPrevWalkSpeed) * partialTick;
        if (walkSpeed > 1.0f)
            walkSpeed = 1.0f;
        walkPos = previewWalkPos - previewWalkSpeed * (1.0f - partialTick);
        bob = previewBobTicks + partialTick;
        headYaw = yawDeg * 0.25f;
        headPitch = pitchDeg * 0.35f;
    }

    model->isChild = false;
    model->isRiding = false;
    model->aimedBow = false;
    model->heldItemLeft = 0;
    model->heldItemRight = 0;
    model->isSneak = (isCenterSlot && previewPoseIndex == 1);
    model->onGround = (isCenterSlot && previewPoseIndex == 2) ? previewSwingTicks : 0.0f;
    if (model->bipedHeadwear != nullptr)
        model->bipedHeadwear->rotationPointY = model->isSneak ? 1.0f : 0.0f;

    renderClear(RenderClearMask::Depth);
    renderPushMatrix();
    renderTranslate(centerX, feetY, 50.0f);
    renderScale(-scale, scale, scale);
    renderRotate(180.0f, 0.0f, 0.0f, 1.0f);
    renderRotate(135.0f, 0.0f, 1.0f, 0.0f);
    RenderHelper::enableStandardItemLighting();
    renderRotate(-135.0f, 0.0f, 1.0f, 0.0f);
    renderRotate(-pitchDeg, 1.0f, 0.0f, 0.0f);
    renderRotate(180.0f - yawDeg, 0.0f, 1.0f, 0.0f);
    renderScale(-1.0f, -1.0f, 1.0f);
    renderScale(0.9375f, 0.9375f, 0.9375f);
    renderTranslate(0.0f, -24.0f * 0.0625f - 0.0078125f, 0.0f);

    renderColor4f(brightness, brightness, brightness, 1.0f);
    model->render(walkPos, walkSpeed, bob, headYaw, headPitch, 0.0625f);
    renderPopMatrix();
}

void GuiSkinSelector::drawLegacyScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    (void)mouseX;
    (void)mouseY;

    StringTranslate *tr = StringTranslate::getInstance();
    const bool isEs = (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);
    const bool hdLayout = (height >= 300 || width >= 480);

    // 1. Update Right Stick rotation & pose cycling for the center 3D model
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    const PlatformGamepadSnapshot gpad = platformGamepadSnapshot(platformMenuPad());
    if (gpad.connected)
    {
        if (std::fabs(gpad.rightX) > 0.08f)
        {
            previewAutoRotate = false;
            previewYaw += gpad.rightX * 5.0f;
            while (previewYaw > 180.0f) previewYaw -= 360.0f;
            while (previewYaw < -180.0f) previewYaw += 360.0f;
        }
        if (std::fabs(gpad.rightY) > 0.65f)
        {
            if (!previewPoseStickLatch)
            {
                previewPoseStickLatch = true;
                if (gpad.rightY < 0.0f)
                    previewPoseIndex = (previewPoseIndex + 1) % 3;
                else
                    previewPoseIndex = (previewPoseIndex + 2) % 3;
            }
        }
        else if (std::fabs(gpad.rightY) < 0.30f)
        {
            previewPoseStickLatch = false;
        }
    }
#endif

    if (previewAutoRotate)
    {
        if (previewRotatingLeft)
        {
            previewYaw += 0.45f;
            if (previewYaw > 42.0f)
                previewRotatingLeft = false;
        }
        else
        {
            previewYaw -= 0.45f;
            if (previewYaw < -42.0f)
                previewRotatingLeft = true;
        }
    }

    // 2. Background: LCE Panorama (from main menu) or Pause dark gradient (in-game)
    if (mc != nullptr && mc->theWorld != nullptr)
    {
        drawGradientRect(0, 0, width, height, 0xC0101010, 0xD0101010);
    }
    else
    {
        if (!legacyDrawPanorama(mc, width, height, legacyScenePanoramaTimer(), partialTick, zLevel))
            drawDefaultBackground();
        else
            drawGradientRect(0, 0, width, height, 0x18000000, 0x50000000);
    }

    auto drawLceBevelBox = [&](int_t x, int_t y, int_t w, int_t h) {
        const LegacyUiTheme &theme = legacyUiTheme();
        drawRect(x + 2, y + 2, x + w + 2, y + h + 2, theme.panelDropShadowColor);
        drawRect(x, y, x + w, y + h, theme.panelBorderColor);
        drawRect(x + 1, y + 1, x + w - 1, y + h - 1, theme.panelFillColor);
        drawRect(x + 1, y + 1, x + w - 2, y + 2, theme.panelHighlightColor);
        drawRect(x + 1, y + 1, x + 2, y + h - 2, theme.panelHighlightColor);
        drawRect(x + 2, y + h - 2, x + w - 1, y + h - 1, theme.panelShadowColor);
        drawRect(x + w - 2, y + 2, x + w - 1, y + h - 1, theme.panelShadowColor);
    };

    // 3. Top Screen Title & Player 1 / Player 2 Mode Indicator
    const int_t titleY = std::max<int_t>(4, (legacyStageTop - 8) / 2);
    if (isPlayer2Skin)
    {
        std::string p2Title = isEs ? "Cambiar Skin - Jugador 2" : "Change Skin - Player 2";
        drawCenteredString(fontRenderer, p2Title, width / 2, titleY, 0xFF55FFFF);
    }
    else
    {
        std::string p1Title = isEs ? "Cambiar Skin" : "Change Skin";
        drawCenteredString(fontRenderer, p1Title, width / 2, titleY, 0xFFFFFFFF);
    }

    // 4. LCE Stage Backdrop (BackgroundTint + Top/Bottom TabBar metallic trim)
    drawGradientRect(0, legacyStageTop, width, legacyStageBottom, 0x900C0C0C, 0xB8060606);

    // Top trim bar
    drawRect(0, legacyStageTop - 2, width, legacyStageTop - 1, 0xFF080808);
    drawRect(0, legacyStageTop - 1, width, legacyStageTop,     0xFF707070);
    drawRect(0, legacyStageTop,     width, legacyStageTop + 1, 0xFF282828);

    // Bottom trim bar
    drawRect(0, legacyStageBottom - 1, width, legacyStageBottom,     0xFF707070);
    drawRect(0, legacyStageBottom,     width, legacyStageBottom + 1, 0xFF282828);
    drawRect(0, legacyStageBottom + 1, width, legacyStageBottom + 2, 0xFF080808);

    // 5. Top Pack Switcher (PackGroup: Left, Center, Right LCE Pack Boxes)
    const int packCount = SkinManager::getPackCount();
    const int totalSkins = SkinManager::getSkinCountForPack(currentPackIndex);

    auto getPackDisplayTitle = [&](int packIdx) -> std::string {
        if (packIdx < 0 || packIdx >= packCount)
            return "";
        std::string pName = SkinManager::getPackName(packIdx);
        if (packIdx == 0)
            return isEs ? "Skins Originales" : "Default Skins";
        if (pName == "Custom Skins")
        {
            const int customCount = SkinManager::getSkinCountForPack(packIdx);
            return (isEs ? "Personalizado (" : "Custom Skins (") + std::to_string(customCount) + ")";
        }
        return pName;
    };

    const std::string activePackTitle = getPackDisplayTitle(currentPackIndex);

    auto drawLegacyPackTab = [&](int_t x, int_t y, int_t w, int_t h, const std::string &text, bool isCenterActive, bool enabled) {
        if (isCenterActive)
        {
            drawLceBevelBox(x, y, w, h);
            drawRect(x + 3, y + 3, x + w - 3, y + h - 3, 0xC81C1C1C);
            drawRect(x + 3, y + 3, x + w - 3, y + 4, 0x60FFE080);
        }
        else
        {
            drawInsetPanel(x, y, x + w, y + h, enabled ? 0xA8121212 : 0x70101010);
        }

        std::string clipped = fontRenderer->trimStringToWidth(text, std::max<int_t>(16, w - 10));
        const int_t textY = y + (h - 8) / 2;
        const int_t color = isCenterActive ? 0xFFFFE888 : (enabled ? 0xFFB0B0B0 : 0xFF686868);
        drawCenteredString(fontRenderer, clipped, x + w / 2, textY, color);
    };

    if (packCount > 1)
    {
        int prevIdx = (currentPackIndex - 1 + packCount) % packCount;
        int nextIdx = (currentPackIndex + 1) % packCount;
        drawLegacyPackTab(legacyLeftTabX, legacyPackBarY, legacySideTabW, legacyPackBarH,
                          "< " + getPackDisplayTitle(prevIdx), false,
                          SkinManager::getSkinCountForPack(prevIdx) > 0);
        drawLegacyPackTab(legacyCenterTabX, legacyPackBarY, legacyCenterTabW, legacyPackBarH, activePackTitle, true, true);
        drawLegacyPackTab(legacyRightTabX, legacyPackBarY, legacySideTabW, legacyPackBarH,
                          getPackDisplayTitle(nextIdx) + " >", false,
                          SkinManager::getSkinCountForPack(nextIdx) > 0);
    }
    else
    {
        drawLegacyPackTab(legacyCenterTabX, legacyPackBarY, legacyCenterTabW, legacyPackBarH, activePackTitle, true, true);
    }

    // 6. 3D Character Carousel (Center + 4 Left + 4 Right)
    if (totalSkins > 0)
    {
        const int_t stageTopY = legacyPackBarY + legacyPackBarH + 4;
        const int_t availStageH = std::max<int_t>(70, nameplateY - stageTopY);
        const float centerScale = std::clamp(static_cast<float>(availStageH) * 0.60f / 1.875f, 32.0f, 74.0f);

        const int slotOrder[] = { -4, 4, -3, 3, -2, 2, -1, 1, 0 };

        struct CarouselSlotGeom
        {
            int k;
            const SkinEntry *skin;
            float x;
            float feetY;
            float scale;
            float yaw;
            float pitch;
            float brightness;
            float shadowAlpha;
        };

        CarouselSlotGeom slots[9];
        int validSlotCount = 0;

        for (int k : slotOrder)
        {
            float t = static_cast<float>(k) - scrollOffset;
            float dist = std::fabs(t);
            if (dist > 4.35f)
                continue;

            int skinIndex = ((currentSkinIndex + k) % totalSkins + totalSkins) % totalSkins;
            const SkinEntry *skin = SkinManager::getSkin(currentPackIndex, skinIndex);
            if (skin == nullptr)
                continue;

            const float sign = (t < 0.0f) ? -1.0f : 1.0f;
            const float normX = sign * (0.295f * dist / (1.0f + 0.145f * dist));
            const float slotX = static_cast<float>(carouselCenterX) + normX * static_cast<float>(width);
            const float slotScale = centerScale * std::pow(0.79f, dist);
            // The console's GPU does not clip triangles far past the screen edge; a model cut by
            // the edge streaks across the screen (xemu clips them, so only real hardware shows it).
            const float slotHalfWidth = slotScale * 0.8f;
            if (slotX - slotHalfWidth < 0.0f || slotX + slotHalfWidth > static_cast<float>(width))
                continue;
            const float slotFeetY = static_cast<float>(carouselGroundY) - std::min(dist, 3.5f) * (centerScale * 0.085f);

            // Facing: left slots (-t) face -45 deg (right, towards center); right slots (+t) face +45 deg (left, towards center);
            // center slot (t ~ 0) smoothly blends into previewYaw.
            const float sideYaw = (t < 0.0f) ? -45.0f : 45.0f;
            const float centerBlend = std::clamp(dist, 0.0f, 1.0f);
            const float slotYaw = previewYaw * (1.0f - centerBlend) + sideYaw * centerBlend;
            const float slotPitch = (k == 0) ? previewPitch : 0.0f;
            const float brightness = (k == 0) ? 1.0f : std::max(0.58f, 0.94f - 0.09f * dist);
            const float shadowAlpha = std::max(0.30f, 1.0f - 0.17f * dist);

            slots[validSlotCount++] = { k, skin, slotX, slotFeetY, slotScale, slotYaw, slotPitch, brightness, shadowAlpha };
        }

        // 6a. Draw all 2D ground shadows first before enabling 3D DepthTest/Lighting
        for (int i = 0; i < validSlotCount; ++i)
        {
            const CarouselSlotGeom &s = slots[i];
            drawFeetShadow(s.x, s.feetY + 1.0f, s.scale * 0.28f, std::max(2.5f, s.scale * 0.07f), s.shadowAlpha);
        }

        // 6b. 3D ModelBiped pass (outer slots to center slot, restoring 2D state strictly afterwards)
        renderClear(RenderClearMask::Depth);
        renderEnable(RenderCapability::DepthTest);
        renderDepthFunc(RenderCompare::LessEqual);
        renderDepthMask(true);
        renderEnable(RenderCapability::AlphaTest);
        renderAlphaFunc(RenderCompare::Greater, 0.1f);
        renderEnable(RenderCapability::Texture2D);
        renderEnable(RenderCapability::RescaleNormal);
        renderEnable(RenderCapability::ColorMaterial);

        for (int i = 0; i < validSlotCount; ++i)
        {
            const CarouselSlotGeom &s = slots[i];
            drawLegacyCharacter3D(s.skin, s.x, s.feetY, s.scale, s.yaw, s.pitch, s.k == 0, s.brightness, partialTick);
        }

        RenderHelper::disableStandardItemLighting();
        renderDisable(RenderCapability::RescaleNormal);
        renderDisable(RenderCapability::ColorMaterial);
        renderDisable(RenderCapability::DepthTest);
        renderDepthMask(true);
        renderDisable(RenderCapability::Blend);
        renderEnable(RenderCapability::Texture2D);
        renderEnable(RenderCapability::AlphaTest);
        renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // 7. LCE "Selected" Badge (SelectedGroup) above the SkinDetails Nameplate
    const SkinEntry *selectedSkin = SkinManager::getSkin(currentPackIndex, currentSkinIndex);
    if (selectedSkin != nullptr)
    {
        std::string equippedId = isPlayer2Skin ? SkinManager::getSelectedSkinIdP2() : SkinManager::getSelectedSkinId();
        if (mc != nullptr && mc->gameSettings != nullptr)
        {
            if (isPlayer2Skin && !mc->gameSettings->selectedSkinP2.empty())
                equippedId = mc->gameSettings->selectedSkinP2;
            else if (!isPlayer2Skin && !mc->gameSettings->selectedSkin.empty())
                equippedId = mc->gameSettings->selectedSkin;
        }

        if (selectedSkin->id == equippedId)
        {
            const std::string badgeText = isEs ? "Seleccionado" : "Selected";
            const int_t badgeW = std::max<int_t>(68, fontRenderer->getStringWidth(badgeText) + 18);
            const int_t badgeH = hdLayout ? 14 : 12;
            const int_t badgeX = width / 2 - badgeW / 2;
            const int_t badgeY = nameplateY - badgeH - 2;

            drawBeveledPanel(badgeX, badgeY, badgeX + badgeW, badgeY + badgeH, 0xD81E2B18);
            drawRect(badgeX + 1, badgeY + 1, badgeX + badgeW - 1, badgeY + 2, 0x8060FF60);
            drawCenteredString(fontRenderer, badgeText, width / 2, badgeY + (badgeH - 8) / 2, 0xFF88FF88);
        }
    }

    // 8. LCE SkinDetails Nameplate (SkinName + OriginName + Index Counter)
    drawLceBevelBox(legacyNameplateX, nameplateY, legacyNameplateW, nameplateHeight);
    drawRect(legacyNameplateX + 3, nameplateY + 3,
             legacyNameplateX + legacyNameplateW - 3, nameplateY + nameplateHeight - 3, 0xD8141414);

    if (selectedSkin != nullptr)
    {
        const std::string skinTitle = selectedSkin->name;
        std::string originTitle = activePackTitle;
        if (totalSkins > 0)
            originTitle += "  (" + std::to_string(currentSkinIndex + 1) + "/" + std::to_string(totalSkins) + ")";
        if (isPlayer2Skin)
            originTitle += isEs ? " [J2]" : " [P2]";

        if (hdLayout)
        {
            drawCenteredString(fontRenderer, skinTitle, width / 2, nameplateY + 5, 0xFFFFFFFF);
            drawCenteredString(fontRenderer, originTitle, width / 2, nameplateY + 16, isPlayer2Skin ? 0xFF55FFFF : 0xFFB0B0B0);
        }
        else
        {
            drawCenteredString(fontRenderer, skinTitle, width / 2, nameplateY + 3, 0xFFFFFFFF);
            drawCenteredString(fontRenderer, originTitle, width / 2, nameplateY + 12, isPlayer2Skin ? 0xFF55FFFF : 0xFFB0B0B0);
        }
    }

    // 9. Bottom LCE Controller Prompts Bar
    const int_t footerY = height - (hdLayout ? 16 : 13);
    const std::string lblSelect = isEs ? "Seleccionar" : "Select";
    const std::string lblBack   = isEs ? "Volver" : "Back";
    const std::string lblLoad   = isEs ? "Cargar" : "Load Skins";
    const std::string lblDelete = isEs ? "Eliminar" : "Delete";
    const std::string lblP2     = isPlayer2Skin ? (isEs ? "Jugador 1" : "Player 1") : (isEs ? "Jugador 2" : "Player 2");
    const std::string lblNav    = isEs ? "Skin / Pack" : "Skin / Pack";

#if PLATFORM_XBOX || PLATFORM_PS2
    RenderEngine *renderEngine = mc ? mc->renderEngine : nullptr;
    if (renderEngine != nullptr)
    {
        const int_t iconH = hdLayout ? 11 : 10;
        const int_t gap = hdLayout ? 10 : 6;
        const bool showDeleteOnX = (SkinManager::getPackName(currentPackIndex) == "Custom Skins" && !SkinManager::getCustomSkins().empty());
        const std::string &xLabel = showDeleteOnX ? lblDelete : lblP2;

        int_t totalPromptW = 0;
        totalPromptW += LegacyButtonPrompt::getPromptWidth(fontRenderer, Ps2ButtonIcon::Cross, lblSelect, iconH) + gap;
        totalPromptW += LegacyButtonPrompt::getPromptWidth(fontRenderer, Ps2ButtonIcon::Circle, lblBack, iconH) + gap;
        totalPromptW += LegacyButtonPrompt::getPromptWidth(fontRenderer, Ps2ButtonIcon::Triangle, lblLoad, iconH) + gap;
        totalPromptW += LegacyButtonPrompt::getPromptWidth(fontRenderer, Ps2ButtonIcon::Square, xLabel, iconH) + gap;
        totalPromptW += LegacyButtonPrompt::getPromptWidth(fontRenderer, Ps2ButtonIcon::DPad, lblNav, iconH);
        if (showDeleteOnX && hdLayout)
            totalPromptW += gap + LegacyButtonPrompt::getPromptWidth(fontRenderer, Ps2ButtonIcon::Select, lblP2, iconH);

        int_t curX = std::max<int_t>(6, (width - totalPromptW) / 2);
        curX += LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::Cross, lblSelect, curX, footerY, iconH, 0xFFE8E8E8) + gap;
        curX += LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::Circle, lblBack, curX, footerY, iconH, 0xFFE8E8E8) + gap;
        curX += LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::Triangle, lblLoad, curX, footerY, iconH, 0xFFE8E8E8) + gap;
        curX += LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::Square, xLabel, curX, footerY, iconH, 0xFFE8E8E8) + gap;
        curX += LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::DPad, lblNav, curX, footerY, iconH, 0xFFE8E8E8);
        if (showDeleteOnX && hdLayout)
        {
            curX += gap;
            LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::Select, lblP2, curX, footerY, iconH, 0xFFE8E8E8);
        }
    }
#elif PLATFORM_WII
    const std::string buttons[] = {"A", "B", "L/R", "ZL/ZR"};
    const std::string actions[] = {lblSelect, lblBack, uiText("Skin"), uiText("Tab")};
    drawControlHintRow(mc, width, footerY, buttons, actions, 4);
#else
    const bool showDelete = (SkinManager::getPackName(currentPackIndex) == "Custom Skins" && !SkinManager::getCustomSkins().empty());
    const std::string buttons[] = {"Enter", "Esc", "Left/Right", "Tab", "L", showDelete ? "Del" : "P"};
    const std::string actions[] = {lblSelect, lblBack, uiText("Skin"), uiText("Tab"), lblLoad, showDelete ? lblDelete : lblP2};
    drawControlHintRow(mc, width, footerY, buttons, actions, 6);
#endif
}

void GuiSkinSelector::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    if (isLegacyMode())
    {
        drawLegacyScreen(mouseX, mouseY, partialTick);
        return;
    }

    (void)partialTick;

    // 1. Darkened background
    drawDefaultBackground();

    // 2. Left Panel
    const int_t leftX1 = dialogLeft;
    const int_t leftY1 = dialogTop;
    const int_t leftX2 = dialogLeft + leftPanelWidth;
    const int_t leftY2 = dialogTop + dialogHeight;
    drawInsetPanel(leftX1, leftY1, leftX2, leftY2, 0xD0101010);

    // Pack artwork container (square)
    const int numPacks = SkinManager::getPackCount();
    const int_t artSize = (numPacks > 3) ? 40 : std::min<int_t>(leftPanelWidth - 20, 56);
    const int_t artX = leftX1 + (leftPanelWidth - artSize) / 2;
    const int_t artY = leftY1 + 6;
    drawInsetPanel(artX - 1, artY - 1, artX + artSize + 1, artY + artSize + 1, 0xFF000000);

    // Draw pack cover art
    if (mc != nullptr && mc->renderEngine != nullptr)
    {
        recordPreviewTexture("/skins/default_pack.png");
        int coverTex = mc->renderEngine->getTexture("/skins/default_pack.png");
        if (coverTex >= 0)
        {
            mc->renderEngine->bindTexture(coverTex);
            renderEnable(RenderCapability::Texture2D);
            renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            Tessellator &tess = Tessellator::instance;
            tess.startDrawingQuads();
            tess.addVertexWithUV(artX,           artY + artSize, zLevel, 0.0f, 1.0f);
            tess.addVertexWithUV(artX + artSize, artY + artSize, zLevel, 1.0f, 1.0f);
            tess.addVertexWithUV(artX + artSize, artY,           zLevel, 1.0f, 0.0f);
            tess.addVertexWithUV(artX,           artY,           zLevel, 0.0f, 0.0f);
            tess.draw();
        }
    }

    // 3. Right Panel (Skin Carousel and Details)
    const int_t rightX1 = rightPanelX;
    const int_t rightY1 = dialogTop;
    const int_t rightX2 = rightPanelX + rightPanelWidth;
    const int_t rightY2 = dialogTop + dialogHeight;
    drawInsetPanel(rightX1, rightY1, rightX2, rightY2, 0xD0101010);

    // Header bar inside right panel
    drawRect(rightX1 + 1, rightY1 + 1, rightX2 - 1, rightY1 + 18, 0x80282828);
    StringTranslate *tr = StringTranslate::getInstance();
    const bool isEs = (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);

    std::string headerName = SkinManager::getPackName(currentPackIndex);
    if (isPlayer2Skin)
        headerName += isEs ? " (Jugador 2)" : " (Player 2)";
    fontRenderer->drawStringWithShadow(headerName, rightX1 + 10, rightY1 + 5, isPlayer2Skin ? 0x55FFFF : 0xFFFFFF);

    if (isPlayer2Skin)
    {
        std::string p2Title = isEs ? "[ SELECTOR DE SKIN: JUGADOR 2 ]" : "[ PLAYER 2 SKIN SELECTOR ]";
        drawCenteredString(fontRenderer, p2Title, width / 2, dialogTop - 11, 0x55FFFF);
    }

    const int totalSkins = SkinManager::getSkinCountForPack(currentPackIndex);
    if (totalSkins > 0)
    {
        std::string countStr = std::to_string(currentSkinIndex + 1) + " / " + std::to_string(totalSkins);
        int_t countW = fontRenderer->getStringWidth(countStr);
        fontRenderer->drawStringWithShadow(countStr, rightX2 - countW - 10, rightY1 + 5, 0xAAAAAA);
    }

    // 4. Infinite Carousel Area
    if (totalSkins > 0)
    {
        const float spacing = static_cast<float>(rightPanelWidth) * 0.22f;

        // Render slots from k = 3 down to -3 (so center skin renders on top of flanking ones)
        const int slotOrder[] = { -3, 3, -2, 2, -1, 1, 0 };
        for (int k : slotOrder)
        {
            float t = static_cast<float>(k) - scrollOffset;
            float dist = std::fabs(t);
            if (dist > 2.6f)
                continue;

            int skinIndex = ((currentSkinIndex + k) % totalSkins + totalSkins) % totalSkins;
            const SkinEntry *skin = SkinManager::getSkin(currentPackIndex, skinIndex);
            if (skin == nullptr)
                continue;

            // Scale factor based on distance from center
            float scale = 3.4f - 1.05f * std::min(dist, 2.0f);
            if (scale < 1.0f) scale = 1.0f;

            float alpha = 1.0f - 0.20f * std::min(dist, 2.0f);
            if (alpha < 0.35f) alpha = 0.35f;

            float skinW = 16.0f * scale;
            float skinH = 32.0f * scale;
            float skinX = static_cast<float>(carouselCenterX) + t * spacing - skinW * 0.5f;
            float skinY = static_cast<float>(carouselGroundY) - skinH;

            // Draw soft ground shadow
            drawFeetShadow(static_cast<float>(carouselCenterX) + t * spacing, static_cast<float>(carouselGroundY),
                           skinW * 0.45f, 4.0f, alpha);

            // Draw front preview
            drawFrontPreview(skin, skinX, skinY, skinW, skinH, alpha);
        }
    }

    // 5. Nameplate banner
    const int_t npPadding = 8;
    const int_t npW = rightPanelWidth - npPadding * 2;
    const int_t npX = rightX1 + npPadding;
    const int_t npY = nameplateY;
    drawInsetPanel(npX, npY, npX + npW, npY + nameplateHeight, 0xFF181818);

    const SkinEntry *selectedSkin = SkinManager::getSkin(currentPackIndex, currentSkinIndex);
    if (selectedSkin != nullptr)
    {
        std::string displayName = toUpperString(selectedSkin->name);
        if (isPlayer2Skin)
            displayName += " [P2]";
        drawCenteredString(fontRenderer, displayName, npX + npW / 2, npY + 6, isPlayer2Skin ? 0xAAFFFF : 0xFFFFAA);
    }

    // Keep the upstream skin actions while sharing the platform icon/fallback renderer.
    const int_t footerY = height - 13;
#if PLATFORM_PS2
    const std::string buttons[] = {"Cross", "Circle", "Triangle", "Square"};
    const std::string actions[] = {uiText("Select"), uiText("Back"), uiText("Load"), uiText("Delete")};
    drawControlHintRow(mc, width, footerY - 14, buttons, actions, currentPackIndex == 1 ? 4 : 3);
    const std::string navigation[] = {"L1/R1", "L2/R2"};
    const std::string navigationActions[] = {uiText("Skin"), uiText("Tab")};
    drawControlHintRow(mc, width, footerY, navigation, navigationActions, 2);
#elif PLATFORM_XBOX
    RenderEngine *renderEngine = mc ? mc->renderEngine : nullptr;
    if (renderEngine != nullptr)
    {
        const int_t gap = 10;
        int_t curX = dialogLeft;

        curX += LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::Cross, "Select", curX, footerY, 11, 0xE0E0E0) + gap;
        curX += LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::Circle, "Back", curX, footerY, 11, 0xE0E0E0) + gap;
        curX += LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::DPad, "Skin / Tab", curX, footerY, 11, 0xE0E0E0);
    }
    else
    {
        std::string hint = "[A] Select   [B] Back   [D-Pad </>] Skin   [D-Pad ^/v] Tab";
        fontRenderer->drawStringWithShadow(hint, dialogLeft, footerY, 0xC0C0C0);
    }
#elif PLATFORM_WII
    const std::string buttons[] = {"A", "B", "L/R", "ZL/ZR"};
    const std::string actions[] = {uiText("Select"), uiText("Back"), uiText("Skin"), uiText("Tab")};
    drawControlHintRow(mc, width, footerY, buttons, actions, 4);
#else
    const std::string buttons[] = {"Enter", "Esc", "Left/Right", "Tab"};
    const std::string actions[] = {uiText("Select"), uiText("Back"), uiText("Skin"), uiText("Tab")};
    drawControlHintRow(mc, width, footerY, buttons, actions, 4);
#endif

    // 7. Draw standard GUI controls (buttons, tabs, arrows) AND the software cursor
    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}
