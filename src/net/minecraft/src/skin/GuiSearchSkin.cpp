#include "GuiSearchSkin.h"
#include "platform/PlatformConfig.h"
#if PLATFORM_XBOX
#include "xbox/system/XboxTls.h"
#endif
#include "GuiSkinSelector.h"
#include "SkinManager.h"

#include "FontRenderer.h"
#include "GameSettings.h"
#include "GuiButton.h"
#include "GuiTextField.h"
#include "Minecraft.h"
#include "ModelBiped.h"
#include "ModelPlayer.h"
#include "ModelRenderer.h"
#include "RenderEngine.h"
#include "RenderHelper.h"
#include "SoundManager.h"
#include "StringTranslate.h"
#include "java/BufferedImage.h"
#include "java/JavaNetwork.h"
#include "legacy/LegacyButtonPrompt.h"
#include "legacy/LegacyPanorama.h"
#include "legacy/LegacySceneState.h"
#include "legacy/LegacyUiTheme.h"
#include "pc/lwjgl/Keyboard.h"
#include "platform/Log.h"
#include "platform/RenderAPI.h"

#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
#include "platform/Input.h"
#include "VirtualKeyboard.h"
#endif
#ifdef XBOX_PLATFORM
#include "xbox/input/XboxPad.h"
#endif

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstring>
#include <mutex>
#include <thread>

namespace
{
constexpr int_t BTN_SEARCH       = 1;
constexpr int_t BTN_SAVE         = 2;
constexpr int_t BTN_USE          = 3;
constexpr int_t BTN_CANCEL       = 4;
constexpr int_t BTN_POPULAR_PREV = 5;
constexpr int_t BTN_POPULAR_USE  = 6;
constexpr int_t BTN_POPULAR_NEXT = 7;

constexpr std::size_t kMaxSkinDownloadBytes = 64 * 1024;
constexpr int kSkinDownloadTimeoutMs = 8000;

static const char *const kPopularSkins[] = {
    "Notch",
    "jeb_",
    "Dinnerbone",
    "Dream",
    "Technoblade",
    "Grian",
    "Alex",
    "MHF_Steve",
    "Herobrine",
    "Vegetta777"
};
constexpr int kPopularSkinCount = static_cast<int>(sizeof(kPopularSkins) / sizeof(kPopularSkins[0]));

std::string trimWhitespace(const std::string &in)
{
    std::size_t s = 0;
    while (s < in.size() && std::isspace(static_cast<unsigned char>(in[s])))
        ++s;
    std::size_t e = in.size();
    while (e > s && std::isspace(static_cast<unsigned char>(in[e - 1])))
        --e;
    return in.substr(s, e - s);
}

bool isHexString(const std::string &s)
{
    if (s.empty())
        return false;
    for (char c : s)
    {
        if (!std::isxdigit(static_cast<unsigned char>(c)))
            return false;
    }
    return true;
}

std::string stripDashes(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
    {
        if (c != '-')
            out.push_back(c);
    }
    return out;
}

bool isValidMinecraftUsername(const std::string &name)
{
    if (name.empty() || name.size() > 16)
        return false;
    for (char c : name)
    {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (!std::isalnum(uc) && c != '_')
            return false;
    }
    return true;
}

std::string buildSkinUrlForQuery(const std::string &query, std::string &outSaveName)
{
    const std::string noDash = stripDashes(query);
    // Fallback 1: 32-char UUID -> crafatar.com
    if (noDash.size() == 32 && isHexString(noDash))
    {
        outSaveName = noDash.substr(0, 12);
        return "http://crafatar.com/skins/" + noDash;
    }
    // Fallback 2: 64-char texture hash -> textures.minecraft.net
    if (query.size() == 64 && isHexString(query))
    {
        outSaveName = query.substr(0, 12);
        return "http://textures.minecraft.net/texture/" + query;
    }
    // Primary: standard Minecraft username -> minotar.net (plain HTTP)
    if (isValidMinecraftUsername(query))
    {
        outSaveName = query;
        return "http://minotar.net/skin/" + query;
    }
    return "";
}
} // namespace

struct GuiSearchSkin::SearchJob
{
    std::mutex mtx;
    std::atomic<bool> done{false};
    std::atomic<bool> cancelled{false};

    std::string query;
    std::string saveName;
    std::string url;

    JavaNetwork::HttpResult httpResult = JavaNetwork::HttpResult::NoConnection;
    int httpStatus = -1;
    std::vector<unsigned char> rawPng;
    std::vector<unsigned char> rgba64x64;
    int srcW = 0;
    int srcH = 0;
    bool wasSlim = false;
    bool decodeOk = false;
    std::string decodeError;
};

GuiSearchSkin::GuiSearchSkin(GuiScreen *parent, bool isPlayer2)
    : parentScreen(parent)
    , isPlayer2Skin(isPlayer2)
    , usernameField(nullptr)
    , btnSearch(nullptr)
    , btnPopularPrev(nullptr)
    , btnPopularUse(nullptr)
    , btnPopularNext(nullptr)
    , btnSave(nullptr)
    , btnUse(nullptr)
    , btnCancel(nullptr)
    , initialInputText("")
    , popularIndex(0)
    , activeJob(nullptr)
    , searchAnimTicks(0)
    , downloadedUsername("")
    , downloadedSrcW(0)
    , downloadedSrcH(0)
    , downloadedWasSlim(false)
    , alreadySaved(false)
    , statusMessage("")
    , statusColor(0xFFB8B8B8)
    , previewModel(nullptr)
    , previewModelWide(nullptr)
    , previewModelSlim(nullptr)
    , previewTexSlim(false)
    , previewTexId(-1)
    , previewYaw(18.0f)
    , previewWalkPos(0.0f)
    , previewBobTicks(0.0f)
    , previewAutoRotate(true)
    , focusIndex(0)
    , panelX(0)
    , panelY(0)
    , panelW(0)
    , panelH(0)
    , previewBoxX(0)
    , previewBoxY(0)
    , previewBoxW(0)
    , previewBoxH(0)
    , fieldX(0)
    , fieldY(0)
    , fieldW(0)
    , fieldH(18)
{
#ifdef XBOX_PLATFORM
    XboxPad::clearLatchedPressed();
#endif
}

GuiSearchSkin::~GuiSearchSkin()
{
    if (activeJob != nullptr)
        activeJob->cancelled.store(true, std::memory_order_release);
    activeJob.reset();

    if (usernameField != nullptr)
        usernameField->setFocused(false);
    delete usernameField;
    usernameField = nullptr;

    releasePreviewTexture();
    delete previewModel;
    previewModel = nullptr;
    delete previewModelWide;
    previewModelWide = nullptr;
    delete previewModelSlim;
    previewModelSlim = nullptr;
}

bool GuiSearchSkin::isLegacyMode() const
{
    return mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
}

bool GuiSearchSkin::isSpanish() const
{
    StringTranslate *tr = StringTranslate::getInstance();
    return tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0;
}

bool GuiSearchSkin::allowsPlatformPointerInput() const
{
    return !isLegacyMode();
}

bool GuiSearchSkin::usesSpecializedMenuNavigation() const
{
    return isLegacyMode();
}

bool GuiSearchSkin::isSearching() const
{
    return activeJob != nullptr && !activeJob->done.load(std::memory_order_acquire);
}

void GuiSearchSkin::releasePreviewTexture()
{
    if (previewTexId >= 0 && mc != nullptr && mc->renderEngine != nullptr)
        mc->renderEngine->deleteTexture(previewTexId);
    previewTexId = -1;
}

void GuiSearchSkin::uploadPreviewTexture(const std::vector<unsigned char> &rgba64x64)
{
    releasePreviewTexture();
    if (mc == nullptr || mc->renderEngine == nullptr || rgba64x64.size() < 64 * 64 * 4)
        return;

    std::unique_ptr<unsigned char[]> pixels(new unsigned char[64 * 64 * 4]);
    std::memcpy(pixels.get(), rgba64x64.data(), 64 * 64 * 4);
    BufferedImage img(64, 64, std::move(pixels));
    previewTexId = mc->renderEngine->allocateAndSetupTexture(&img);
}

void GuiSearchSkin::initGui()
{
#ifdef XBOX_PLATFORM
    XboxPad::clearLatchedPressed();
#endif
    lwjgl::Keyboard::enableRepeatEvents(true);

    if (mc != nullptr && mc->isScreenOwnedByPlayer2())
        isPlayer2Skin = true;

    std::string preservedText = initialInputText;
    if (usernameField != nullptr)
    {
        preservedText = usernameField->getText();
        usernameField->setFocused(false);
        delete usernameField;
        usernameField = nullptr;
    }

    controlList.clear();

    const bool es = isSpanish();
    const bool compact = (height < 180); // Split screen

    panelW = std::min<int_t>(width - 14, compact ? 306 : 368);
    panelH = std::min<int_t>(height - (compact ? 14 : 34), compact ? 114 : 184);
    panelX = (width - panelW) / 2;
    panelY = compact ? ((height - panelH) / 2) : std::max<int_t>(16, (height - panelH) / 2 - 4);

    previewBoxW = compact ? 74 : 104;
    previewBoxH = panelH - (compact ? 14 : 26);
    previewBoxX = panelX + 8;
    previewBoxY = panelY + (compact ? 7 : 18);

    const int_t rightX = previewBoxX + previewBoxW + 8;
    const int_t rightW = panelX + panelW - rightX - 8;

    const int_t searchBtnW = compact ? 52 : 62;
    fieldX = rightX;
    fieldY = previewBoxY + (compact ? 12 : 18);
    fieldW = std::max<int_t>(60, rightW - searchBtnW - 4);
    fieldH = compact ? 16 : 18;

    usernameField = new GuiTextField(this, fontRenderer, fieldX, fieldY, fieldW, fieldH, preservedText);
    usernameField->setMaxStringLength(64);
    usernameField->setFocused(false);

    btnSearch = new GuiButton(BTN_SEARCH, fieldX + fieldW + 4, fieldY - 1, searchBtnW, fieldH + 2,
                              es ? "Buscar" : "Search");
    controlList.push_back(btnSearch);

    // Popular skin quick-picker row
    const int_t popY = fieldY + fieldH + (compact ? 6 : 14);
    const int_t arrowW = 18;
    const int_t popUseW = std::max<int_t>(60, rightW - arrowW * 2 - 4);
    const int_t popH = compact ? 16 : 18;

    btnPopularPrev = new GuiButton(BTN_POPULAR_PREV, rightX, popY, arrowW, popH, "<");
    btnPopularUse  = new GuiButton(BTN_POPULAR_USE, rightX + arrowW + 2, popY, popUseW, popH,
                                   std::string(es ? "Popular: " : "Popular: ") + kPopularSkins[popularIndex]);
    btnPopularNext = new GuiButton(BTN_POPULAR_NEXT, rightX + arrowW + 2 + popUseW + 2, popY, arrowW, popH, ">");
    controlList.push_back(btnPopularPrev);
    controlList.push_back(btnPopularUse);
    controlList.push_back(btnPopularNext);

    // Action buttons: Save / Use / Cancel
    const int_t btnH = compact ? 16 : 20;
    const int_t btnY = panelY + panelH - btnH - (compact ? 6 : 8);
    const int_t gap = 4;
    const int_t actW = std::max<int_t>(42, (rightW - gap * 2) / 3);

    btnSave   = new GuiButton(BTN_SAVE, rightX, btnY, actW, btnH, es ? "Guardar" : "Save");
    btnUse    = new GuiButton(BTN_USE, rightX + actW + gap, btnY, actW, btnH, es ? "Usar" : "Use");
    btnCancel = new GuiButton(BTN_CANCEL, rightX + (actW + gap) * 2, btnY,
                              rightW - (actW + gap) * 2, btnH, es ? "Cancelar" : "Cancel");
    controlList.push_back(btnSave);
    controlList.push_back(btnUse);
    controlList.push_back(btnCancel);

    if (statusMessage.empty())
    {
        statusMessage = es ? "Escribe un nombre de usuario de Minecraft" : "Enter a Minecraft username";
        statusColor = 0xFFB8B8B8;
    }

    updateButtonStates();
}

void GuiSearchSkin::onGuiClosed()
{
#if PLATFORM_XBOX
    // Free the shared TLS state so HTTPS costs nothing once back in the game.
    XboxTls::shutdown();
#endif
    lwjgl::Keyboard::enableRepeatEvents(false);
    if (usernameField != nullptr)
        usernameField->setFocused(false);
    if (activeJob != nullptr)
        activeJob->cancelled.store(true, std::memory_order_release);
}

void GuiSearchSkin::updateButtonStates()
{
    const bool busy = isSearching();
    const bool ready = !downloadedPng.empty() && !busy;
    if (btnSearch != nullptr)
        btnSearch->enabled = !busy;
    if (btnPopularPrev != nullptr)
        btnPopularPrev->enabled = !busy;
    if (btnPopularUse != nullptr)
    {
        btnPopularUse->enabled = !busy;
        btnPopularUse->displayString = std::string(isSpanish() ? "Popular: " : "Popular: ") + kPopularSkins[popularIndex];
    }
    if (btnPopularNext != nullptr)
        btnPopularNext->enabled = !busy;
    if (btnSave != nullptr)
        btnSave->enabled = ready;
    if (btnUse != nullptr)
        btnUse->enabled = ready;
}

void GuiSearchSkin::startSearchFor(const std::string &username)
{
    if (usernameField != nullptr)
    {
        usernameField->setFocused(false);
        usernameField->setText(username);
    }
    initialInputText = username;
    startSearch();
}

void GuiSearchSkin::startSearch()
{
    const bool es = isSpanish();
    const std::string query = trimWhitespace(usernameField != nullptr ? usernameField->getText() : initialInputText);
    if (query.empty())
    {
        statusMessage = es ? "Escribe un nombre de usuario" : "Enter a username first";
        statusColor = 0xFFFFAA55;
        return;
    }

    std::string saveName;
    const std::string url = buildSkinUrlForQuery(query, saveName);
    if (url.empty())
    {
        statusMessage = es ? "Usuario no encontrado" : "User not found";
        statusColor = 0xFFFF6666;
        return;
    }

    if (usernameField != nullptr)
        usernameField->setFocused(false);

    if (activeJob != nullptr)
        activeJob->cancelled.store(true, std::memory_order_release);

    auto job = std::make_shared<SearchJob>();
    job->query = query;
    job->saveName = saveName;
    job->url = url;
    activeJob = job;

    searchAnimTicks = 0;
    alreadySaved = false;
    statusMessage = es ? "Buscando..." : "Searching...";
    statusColor = 0xFFFFE066;
    updateButtonStates();

    MC_LOG_INFO("skins", "Searching online skin: user='%s' url='%s'\n", query.c_str(), url.c_str());

    std::thread([job]() {
        std::vector<unsigned char> png;
        int httpCode = -1;
        JavaNetwork::HttpResult res = JavaNetwork::httpGetUrl(job->url, png, &httpCode,
                                                              kMaxSkinDownloadBytes, kSkinDownloadTimeoutMs);
        // Plain HTTP failed (blocked, or the site now forces HTTPS): try the same
        // address over the relaxed-TLS client.
        if ((res == JavaNetwork::HttpResult::NoConnection || res == JavaNetwork::HttpResult::InvalidResponse) &&
            job->url.rfind("http://", 0) == 0 && !job->cancelled.load(std::memory_order_acquire))
        {
            png.clear();
            httpCode = -1;
            res = JavaNetwork::httpGetUrl("https://" + job->url.substr(7), png, &httpCode,
                                          kMaxSkinDownloadBytes, kSkinDownloadTimeoutMs);
        }
        if (job->cancelled.load(std::memory_order_acquire))
            return;

        std::vector<unsigned char> rgba;
        int w = 0, h = 0;
        bool slim = false;
        bool decodeOk = false;
        std::string decodeErr;
        if (res == JavaNetwork::HttpResult::Ok)
        {
            decodeOk = SkinManager::convertSkinTo64(png.data(), png.size(), rgba, w, h, slim, decodeErr);
        }

        {
            std::lock_guard<std::mutex> lock(job->mtx);
            job->httpResult = res;
            job->httpStatus = httpCode;
            if (decodeOk)
            {
                job->rawPng = std::move(png);
                job->rgba64x64 = std::move(rgba);
                job->srcW = w;
                job->srcH = h;
                job->wasSlim = slim;
                job->decodeOk = true;
            }
            else
            {
                job->decodeOk = false;
                job->decodeError = decodeErr;
            }
        }
        job->done.store(true, std::memory_order_release);
    }).detach();
}

void GuiSearchSkin::applyPopularIndex(int delta, bool autoSearch)
{
    popularIndex = ((popularIndex + delta) % kPopularSkinCount + kPopularSkinCount) % kPopularSkinCount;
    const std::string chosen = kPopularSkins[popularIndex];
    if (usernameField != nullptr)
        usernameField->setText(chosen);
    initialInputText = chosen;
    updateButtonStates();
    if (autoSearch)
        startSearch();
}

bool GuiSearchSkin::saveCurrentSkin(bool applyAndClose)
{
    const bool es = isSpanish();
    if (downloadedPng.empty() || downloadedUsername.empty())
        return false;

    std::string skinId;
    std::string err;
    if (!SkinManager::saveDownloadedSkin(downloadedUsername, downloadedPng, applyAndClose, isPlayer2Skin, skinId, err))
    {
        statusMessage = err.empty() ? (es ? "Error al guardar la skin" : "Failed to save skin") : err;
        statusColor = 0xFFFF6666;
        return false;
    }

    alreadySaved = true;
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.levelup", 0.7f, 1.2f);

    if (applyAndClose)
    {
        if (GuiSkinSelector *sel = dynamic_cast<GuiSkinSelector *>(parentScreen))
            sel->focusCustomSkin(skinId);
        mc->displayGuiScreen(parentScreen);
        return true;
    }

    if (GuiSkinSelector *sel = dynamic_cast<GuiSkinSelector *>(parentScreen))
        sel->focusCustomSkin(skinId);

    statusMessage = es ? ("Guardada en Skins personalizadas (" + downloadedUsername + ")")
                       : ("Saved to Custom Skins (" + downloadedUsername + ")");
    statusColor = 0xFF77FF77;
    return true;
}

void GuiSearchSkin::updateScreen()
{
    GuiScreen::updateScreen();
    if (usernameField != nullptr)
        usernameField->updateCursorCounter();

    previewBobTicks += 1.0f;
    previewWalkPos += 0.28f;
    if (previewAutoRotate)
    {
        previewYaw += 0.6f;
        if (previewYaw > 180.0f)
            previewYaw -= 360.0f;
    }

    if (activeJob != nullptr)
    {
        if (!activeJob->done.load(std::memory_order_acquire))
        {
            ++searchAnimTicks;
            const bool es = isSpanish();
            const int dots = (searchAnimTicks / 6) % 4;
            statusMessage = (es ? "Buscando" : "Searching") + std::string(static_cast<std::size_t>(dots), '.');
            statusColor = 0xFFFFE066;
        }
        else
        {
            const bool es = isSpanish();
            std::shared_ptr<SearchJob> job = activeJob;
            activeJob.reset();

            std::lock_guard<std::mutex> lock(job->mtx);
            if (job->httpResult == JavaNetwork::HttpResult::Ok && job->decodeOk)
            {
                downloadedUsername = job->saveName;
                downloadedPng = std::move(job->rawPng);
                downloadedSrcW = job->srcW;
                downloadedSrcH = job->srcH;
                downloadedWasSlim = job->wasSlim;
                alreadySaved = false;

                previewTexSlim = job->wasSlim;
                uploadPreviewTexture(job->rgba64x64);

                if (downloadedWasSlim)
                {
                    statusMessage = es ? "Skin lista (slim/Alex, brazos finos)"
                                       : "Skin ready (slim/Alex arms)";
                    statusColor = 0xFF77FF77;
                }
                else if (downloadedSrcH == 64)
                {
                    statusMessage = es ? "Skin lista (64x64, segunda capa)"
                                       : "Skin ready (64x64, second layer)";
                    statusColor = 0xFF77FF77;
                }
                else
                {
                    statusMessage = es ? "Skin lista (64x32)" : "Skin ready (64x32)";
                    statusColor = 0xFF77FF77;
                }
                if (mc != nullptr && mc->sndManager != nullptr)
                    mc->sndManager->playSoundFX("random.orb", 0.6f, 1.1f);
                MC_LOG_INFO("skins", "Online skin downloaded OK: user='%s' %dx%d slim=%d\n",
                            downloadedUsername.c_str(), downloadedSrcW, downloadedSrcH, downloadedWasSlim ? 1 : 0);
            }
            else if (job->httpResult == JavaNetwork::HttpResult::NotFound)
            {
                statusMessage = es ? "Usuario no encontrado" : "User not found";
                statusColor = 0xFFFF6666;
                MC_LOG_INFO("skins", "Online skin not found: user='%s' http=%d\n",
                            job->query.c_str(), job->httpStatus);
            }
            else if (job->httpResult == JavaNetwork::HttpResult::NoConnection)
            {
                statusMessage = es ? "Sin conexion" : "No connection";
                statusColor = 0xFFFF6666;
                MC_LOG_INFO("skins", "Online skin no connection: user='%s'\n", job->query.c_str());
            }
            else if (job->httpResult == JavaNetwork::HttpResult::TooLarge)
            {
                statusMessage = es ? "Archivo demasiado grande (>64 KB)" : "File too large (>64 KB)";
                statusColor = 0xFFFF6666;
            }
            else
            {
                statusMessage = es ? "Imagen PNG no valida" : "Invalid PNG image";
                statusColor = 0xFFFF6666;
                MC_LOG_INFO("skins", "Online skin invalid response: user='%s' http=%d err='%s'\n",
                            job->query.c_str(), job->httpStatus, job->decodeError.c_str());
            }

            updateButtonStates();
        }
    }
}

void GuiSearchSkin::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;

    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);

    switch (button->id)
    {
    case BTN_SEARCH:
        startSearch();
        break;
    case BTN_POPULAR_PREV:
        applyPopularIndex(-1, false);
        break;
    case BTN_POPULAR_NEXT:
        applyPopularIndex(1, false);
        break;
    case BTN_POPULAR_USE:
        applyPopularIndex(0, true);
        break;
    case BTN_SAVE:
        saveCurrentSkin(false);
        break;
    case BTN_USE:
        saveCurrentSkin(true);
        break;
    case BTN_CANCEL:
        mc->displayGuiScreen(parentScreen);
        break;
    default:
        break;
    }
}

void GuiSearchSkin::keyTyped(char_t c, int_t key)
{
    if (usernameField != nullptr && usernameField->getFocused())
    {
        if (key == lwjgl::Keyboard::KEY_RETURN || key == lwjgl::Keyboard::KEY_NUMPADENTER)
        {
            usernameField->setFocused(false);
            startSearch();
            return;
        }
        if (key == 1 || key == lwjgl::Keyboard::KEY_ESCAPE)
        {
            usernameField->setFocused(false);
            return;
        }
        if (usernameField->textboxKeyTyped(c, key))
            return;
    }

    if (key == 1 || key == lwjgl::Keyboard::KEY_ESCAPE)
    {
        mc->displayGuiScreen(parentScreen);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_RETURN || key == lwjgl::Keyboard::KEY_NUMPADENTER)
    {
        startSearch();
        return;
    }
    if (c == 'q' || c == 'Q')
    {
        previewAutoRotate = false;
        previewYaw -= 14.0f;
        return;
    }
    if (c == 'e' || c == 'E')
    {
        previewAutoRotate = false;
        previewYaw += 14.0f;
        return;
    }
    GuiScreen::keyTyped(c, key);
}

void GuiSearchSkin::mouseClicked(int_t mouseX, int_t mouseY, int_t button)
{
    if (usernameField != nullptr)
        usernameField->mouseClicked(mouseX, mouseY, button);
    GuiScreen::mouseClicked(mouseX, mouseY, button);
}

void GuiSearchSkin::handleSpecializedMenuInput()
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    // When VirtualKeyboard is open, it owns the pad exclusively.
    if (VirtualKeyboard::instance().isActive())
        return;

    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    if (!pad.connected)
        return;

    // Right stick rotates the 3D skin preview
    const PlatformGamepadSnapshot gpad = platformGamepadSnapshot(platformMenuPad());
    if (gpad.connected && std::fabs(gpad.rightX) > 0.12f)
    {
        previewAutoRotate = false;
        previewYaw += gpad.rightX * 5.0f;
        while (previewYaw > 180.0f) previewYaw -= 360.0f;
        while (previewYaw < -180.0f) previewYaw += 360.0f;
    }

    // B: Cancel / Back
    if ((pad.pressed & PLATFORM_TEXT_CLOSE) != 0)
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        mc->displayGuiScreen(parentScreen);
        return;
    }

    // Y: Open on-screen keyboard to type username immediately
    if ((pad.pressed & PLATFORM_TEXT_SHIFT) != 0)
    {
        focusIndex = 0;
        if (usernameField != nullptr)
            usernameField->setFocused(true);
        return;
    }

    // X: Quick-cycle popular skin
    if ((pad.pressed & PLATFORM_TEXT_BACK) != 0 && !isSearching())
    {
        applyPopularIndex(1, false);
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.15f);
        return;
    }

    // Focus order: 0=TextField, 1=Search, 2=Popular, 3=Save, 4=Use, 5=Cancel
    if ((pad.pressed & PLATFORM_TEXT_UP) != 0)
    {
        if (focusIndex >= 3) focusIndex = 2;
        else if (focusIndex == 2) focusIndex = 0;
        else focusIndex = 5;
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.15f);
    }
    else if ((pad.pressed & PLATFORM_TEXT_DOWN) != 0)
    {
        if (focusIndex == 0 || focusIndex == 1) focusIndex = 2;
        else if (focusIndex == 2) focusIndex = hasDownloadedSkin() ? 4 : 5;
        else focusIndex = 0;
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.15f);
    }
    else if ((pad.pressed & PLATFORM_TEXT_LEFT) != 0)
    {
        if (focusIndex == 2 && !isSearching())
            applyPopularIndex(-1, false);
        else if (focusIndex == 1) focusIndex = 0;
        else if (focusIndex == 4) focusIndex = 3;
        else if (focusIndex == 5) focusIndex = 4;
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.15f);
    }
    else if ((pad.pressed & PLATFORM_TEXT_RIGHT) != 0)
    {
        if (focusIndex == 2 && !isSearching())
            applyPopularIndex(1, false);
        else if (focusIndex == 0) focusIndex = 1;
        else if (focusIndex == 3) focusIndex = 4;
        else if (focusIndex == 4) focusIndex = 5;
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.15f);
    }

    // A / Confirm
    if ((pad.pressed & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_ENTER)) != 0)
    {
        switch (focusIndex)
        {
        case 0:
            if (usernameField != nullptr)
                usernameField->setFocused(true);
            break;
        case 1:
            if (btnSearch != nullptr && btnSearch->enabled)
                actionPerformed(btnSearch);
            break;
        case 2:
            if (btnPopularUse != nullptr && btnPopularUse->enabled)
                actionPerformed(btnPopularUse);
            break;
        case 3:
            if (btnSave != nullptr && btnSave->enabled)
                actionPerformed(btnSave);
            break;
        case 4:
            if (btnUse != nullptr && btnUse->enabled)
                actionPerformed(btnUse);
            break;
        case 5:
            if (btnCancel != nullptr)
                actionPerformed(btnCancel);
            break;
        default:
            break;
        }
    }
#endif
}

void GuiSearchSkin::drawBevelPanel(int_t x, int_t y, int_t w, int_t h, bool legacyTheme)
{
    if (legacyTheme)
    {
        const LegacyUiTheme &theme = legacyUiTheme();
        drawRect(x + 2, y + 2, x + w + 2, y + h + 2, theme.panelDropShadowColor);
        drawRect(x, y, x + w, y + h, theme.panelBorderColor);
        drawRect(x + 1, y + 1, x + w - 1, y + h - 1, theme.panelFillColor);
        drawRect(x + 1, y + 1, x + w - 2, y + 2, theme.panelHighlightColor);
        drawRect(x + 1, y + 1, x + 2, y + h - 2, theme.panelHighlightColor);
        drawRect(x + 2, y + h - 2, x + w - 1, y + h - 1, theme.panelShadowColor);
        drawRect(x + w - 2, y + 2, x + w - 1, y + h - 1, theme.panelShadowColor);
    }
    else
    {
        drawRect(x, y, x + w, y + h, 0xD8141414);
        drawRect(x, y, x + w, y + 1, 0xFF606060);
        drawRect(x, y, x + 1, y + h, 0xFF606060);
        drawRect(x, y + h - 1, x + w, y + h, 0xFF000000);
        drawRect(x + w - 1, y, x + w, y + h, 0xFF000000);
    }
}

void GuiSearchSkin::drawPreviewCharacter3D(float centerX, float feetY, float scale, float partialTick)
{
    if (mc == nullptr || mc->renderEngine == nullptr)
        return;

    int_t texId = previewTexId;
    bool slim = previewTexSlim;
    if (texId < 0)
    {
        const std::string fallback = isPlayer2Skin ? SkinManager::getPlayer2SkinTexture() : SkinManager::getActiveSkinTexture();
        texId = mc->renderEngine->getTexture(fallback);
        slim = SkinManager::isSlimSkinTexture(fallback);
    }
    if (texId < 0)
        return;

    // 64x64 textures get the in-game ModelPlayer (second layer, slim arms).
    int_t tw = 0, th = 0;
    mc->renderEngine->getTextureDimensions(texId, &tw, &th);
    ModelBiped *model = nullptr;
    if (tw > 0 && tw == th)
    {
        ModelPlayer *&slot = slim ? previewModelSlim : previewModelWide;
        if (slot == nullptr)
            slot = new ModelPlayer(0.0f, slim);
        model = slot;
    }
    else
    {
        if (previewModel == nullptr)
            previewModel = new ModelBiped(0.0f, 0.0f);
        model = previewModel;
    }

    // Ground shadow
    renderEnable(RenderCapability::Blend);
    renderDisable(RenderCapability::Texture2D);
    renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
    const int_t sx1 = static_cast<int_t>(centerX - scale * 0.30f);
    const int_t sx2 = static_cast<int_t>(centerX + scale * 0.30f);
    const int_t sy1 = static_cast<int_t>(feetY - 2.0f);
    const int_t sy2 = static_cast<int_t>(feetY + 3.0f);
    drawRect(sx1, sy1, sx2, sy2, 0x60000000);
    renderDisable(RenderCapability::Blend);

    // 3D ModelBiped pass (same lighting & transform as GuiSkinSelector / inventory paper doll)
    renderClear(RenderClearMask::Depth);
    renderEnable(RenderCapability::DepthTest);
    renderDepthFunc(RenderCompare::LessEqual);
    renderDepthMask(true);
    renderEnable(RenderCapability::AlphaTest);
    renderAlphaFunc(RenderCompare::Greater, 0.1f);
    renderEnable(RenderCapability::Texture2D);
    renderEnable(RenderCapability::RescaleNormal);
    renderEnable(RenderCapability::ColorMaterial);

    mc->renderEngine->bindTexture(texId);

    model->isChild = false;
    model->isRiding = false;
    model->aimedBow = false;
    model->heldItemLeft = 0;
    model->heldItemRight = 0;
    model->isSneak = false;
    model->onGround = 0.0f;

    renderPushMatrix();
    renderTranslate(centerX, feetY, 50.0f);
    renderScale(-scale, scale, scale);
    renderRotate(180.0f, 0.0f, 0.0f, 1.0f);
    renderRotate(135.0f, 0.0f, 1.0f, 0.0f);
    RenderHelper::enableStandardItemLighting();
    renderRotate(-135.0f, 0.0f, 1.0f, 0.0f);
    renderRotate(180.0f - previewYaw, 0.0f, 1.0f, 0.0f);
    renderScale(-1.0f, -1.0f, 1.0f);
    renderScale(0.9375f, 0.9375f, 0.9375f);
    renderTranslate(0.0f, -24.0f * 0.0625f - 0.0078125f, 0.0f);

    const float brightness = hasDownloadedSkin() ? 1.0f : 0.72f;
    renderColor4f(brightness, brightness, brightness, 1.0f);
    model->render(previewWalkPos, 0.25f, previewBobTicks + partialTick, 0.0f, 0.0f, 0.0625f);
    renderPopMatrix();

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

void GuiSearchSkin::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    const bool legacy = isLegacyMode();
    const bool es = isSpanish();
    const bool compact = (height < 180);

    // 1. Background
    if (legacy)
    {
        if (mc != nullptr && mc->theWorld != nullptr)
        {
            drawGradientRect(0, 0, width, height, 0xC0101010, 0xD0101010);
        }
        else if (!legacyDrawPanorama(mc, width, height, legacyScenePanoramaTimer(), partialTick, zLevel))
        {
            drawDefaultBackground();
        }
        else
        {
            drawGradientRect(0, 0, width, height, 0x20000000, 0x58000000);
        }
    }
    else
    {
        drawDefaultBackground();
    }

    // 2. Main container panel
    drawBevelPanel(panelX, panelY, panelW, panelH, legacy);

    // Title
    std::string title = es ? "Buscar skin por usuario" : "Search Skin by Username";
    if (isPlayer2Skin)
        title += es ? " [J2]" : " [P2]";
    const int_t titleColor = legacy ? 0xFF3A3A3A : 0xFFFFFFFF;
    if (legacy)
        fontRenderer->drawString(title, panelX + (panelW - fontRenderer->getStringWidth(title)) / 2,
                                 panelY + (compact ? 2 : 6), titleColor);
    else
        drawCenteredString(fontRenderer, title, panelX + panelW / 2, panelY + (compact ? 2 : 6), titleColor);

    // 3. Left 3D Preview Stage
    drawRect(previewBoxX - 1, previewBoxY - 1, previewBoxX + previewBoxW + 1, previewBoxY + previewBoxH + 1, 0xFF000000);
    drawGradientRect(previewBoxX, previewBoxY, previewBoxX + previewBoxW, previewBoxY + previewBoxH, 0xFF1B2430, 0xFF0E1218);

    const float previewCenterX = static_cast<float>(previewBoxX + previewBoxW / 2);
    const float previewFeetY = static_cast<float>(previewBoxY + previewBoxH - (compact ? 16 : 24));
    const float previewScale = std::clamp(static_cast<float>(previewBoxH - (compact ? 24 : 36)) * 0.48f, 20.0f, 52.0f);
    drawPreviewCharacter3D(previewCenterX, previewFeetY, previewScale, partialTick);

    // Preview caption
    const std::string caption = hasDownloadedSkin() ? downloadedUsername : (es ? "Vista previa" : "Preview");
    const std::string clippedCaption = fontRenderer->trimStringToWidth(caption, previewBoxW - 6);
    drawCenteredString(fontRenderer, clippedCaption, previewBoxX + previewBoxW / 2,
                       previewBoxY + previewBoxH - (compact ? 11 : 13),
                       hasDownloadedSkin() ? 0xFFFFEE88 : 0xFFA0A0A0);

    // 4. Right side labels + text box + status
    const int_t rightX = fieldX;
    const int_t rightW = panelX + panelW - rightX - 8;
    const std::string inputLbl = es ? "Usuario (o UUID):" : "Username (or UUID):";
    if (legacy)
        fontRenderer->drawString(inputLbl, rightX, fieldY - 10, 0xFF404040);
    else
        drawString(fontRenderer, inputLbl, rightX, fieldY - 10, 0xFFC0C0C0);

    if (usernameField != nullptr)
        usernameField->drawTextBox();

    // Status message area (between Popular row and Save/Use/Cancel buttons)
    const int_t statusY = (btnPopularUse != nullptr ? (btnPopularUse->yPosition + btnPopularUse->getButtonHeight()) : (fieldY + fieldH)) + (compact ? 4 : 10);
    const int_t statusBoxH = std::max<int_t>(14, (btnSave != nullptr ? btnSave->yPosition : (panelY + panelH - 24)) - statusY - 4);
    drawRect(rightX, statusY, rightX + rightW, statusY + statusBoxH, 0xB0101010);
    drawRect(rightX, statusY, rightX + rightW, statusY + 1, 0xFF383838);

    if (!statusMessage.empty())
    {
        const int_t maxLineW = std::max<int_t>(40, rightW - 8);
        std::string line1 = statusMessage;
        std::string line2;
        if (fontRenderer->getStringWidth(statusMessage) > maxLineW && statusBoxH >= 22)
        {
            std::size_t bestSplit = std::string::npos;
            for (std::size_t i = 0; i < statusMessage.size(); ++i)
            {
                if (statusMessage[i] == ' ')
                {
                    if (fontRenderer->getStringWidth(statusMessage.substr(0, i)) <= maxLineW)
                        bestSplit = i;
                    else
                        break;
                }
            }
            if (bestSplit != std::string::npos && bestSplit > 0 && bestSplit + 1 < statusMessage.size())
            {
                line1 = statusMessage.substr(0, bestSplit);
                line2 = fontRenderer->trimStringToWidth(statusMessage.substr(bestSplit + 1), maxLineW);
            }
            else
            {
                line1 = fontRenderer->trimStringToWidth(statusMessage, maxLineW);
            }
        }
        else
        {
            line1 = fontRenderer->trimStringToWidth(statusMessage, maxLineW);
        }

        if (!line2.empty())
        {
            const int_t textTop = statusY + std::max<int_t>(2, (statusBoxH - 18) / 2);
            fontRenderer->drawStringWithShadow(line1, rightX + 4, textTop, statusColor);
            fontRenderer->drawStringWithShadow(line2, rightX + 4, textTop + 10, statusColor);
        }
        else
        {
            fontRenderer->drawStringWithShadow(line1, rightX + 4, statusY + (statusBoxH - 8) / 2, statusColor);
        }
    }

    // 5. Highlight focused element in Legacy mode when VirtualKeyboard is closed
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    const bool kbOpen = VirtualKeyboard::instance().isActive();
#else
    const bool kbOpen = false;
#endif
    auto drawFocusFrame = [&](int_t fx, int_t fy, int_t fw, int_t fh) {
        drawRect(fx - 2, fy - 2, fx + fw + 2, fy, 0xFFFFE855);
        drawRect(fx - 2, fy + fh, fx + fw + 2, fy + fh + 2, 0xFFFFE855);
        drawRect(fx - 2, fy, fx, fy + fh, 0xFFFFE855);
        drawRect(fx + fw, fy, fx + fw + 2, fy + fh, 0xFFFFE855);
    };

    if (legacy && !kbOpen)
    {
        if (focusIndex == 0) drawFocusFrame(fieldX, fieldY, fieldW, fieldH);
        else if (focusIndex == 1 && btnSearch) drawFocusFrame(btnSearch->xPosition, btnSearch->yPosition, btnSearch->getButtonWidth(), btnSearch->getButtonHeight());
        else if (focusIndex == 2 && btnPopularUse) drawFocusFrame(btnPopularUse->xPosition, btnPopularUse->yPosition, btnPopularUse->getButtonWidth(), btnPopularUse->getButtonHeight());
        else if (focusIndex == 3 && btnSave) drawFocusFrame(btnSave->xPosition, btnSave->yPosition, btnSave->getButtonWidth(), btnSave->getButtonHeight());
        else if (focusIndex == 4 && btnUse) drawFocusFrame(btnUse->xPosition, btnUse->yPosition, btnUse->getButtonWidth(), btnUse->getButtonHeight());
        else if (focusIndex == 5 && btnCancel) drawFocusFrame(btnCancel->xPosition, btnCancel->yPosition, btnCancel->getButtonWidth(), btnCancel->getButtonHeight());
    }

    // 6. Standard buttons
    GuiScreen::drawScreen(mouseX, mouseY, partialTick);

    // 7. Bottom Xbox prompts in Legacy mode (when not split-screen compact)
    if (legacy && !compact && !kbOpen)
    {
#if PLATFORM_XBOX || PLATFORM_PS2
        RenderEngine *renderEngine = mc ? mc->renderEngine : nullptr;
        if (renderEngine != nullptr)
        {
            const int_t footerY = height - 14;
            const int_t iconH = 10;
            const int_t gap = 8;
            const std::string lblOk  = es ? "Elegir" : "Select";
            const std::string lblKb  = es ? "Teclado" : "Keyboard";
            const std::string lblPop = es ? "Popular" : "Popular";
            const std::string lblBack = es ? "Volver" : "Back";

            int_t totalW = LegacyButtonPrompt::getPromptWidth(fontRenderer, Ps2ButtonIcon::Cross, lblOk, iconH) + gap
                         + LegacyButtonPrompt::getPromptWidth(fontRenderer, Ps2ButtonIcon::Triangle, lblKb, iconH) + gap
                         + LegacyButtonPrompt::getPromptWidth(fontRenderer, Ps2ButtonIcon::Square, lblPop, iconH) + gap
                         + LegacyButtonPrompt::getPromptWidth(fontRenderer, Ps2ButtonIcon::Circle, lblBack, iconH);
            int_t curX = std::max<int_t>(6, (width - totalW) / 2);
            curX += LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::Cross, lblOk, curX, footerY, iconH, 0xFFE8E8E8) + gap;
            curX += LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::Triangle, lblKb, curX, footerY, iconH, 0xFFE8E8E8) + gap;
            curX += LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::Square, lblPop, curX, footerY, iconH, 0xFFE8E8E8) + gap;
            LegacyButtonPrompt::drawPrompt(renderEngine, fontRenderer, Ps2ButtonIcon::Circle, lblBack, curX, footerY, iconH, 0xFFE8E8E8);
        }
#endif
    }
}
