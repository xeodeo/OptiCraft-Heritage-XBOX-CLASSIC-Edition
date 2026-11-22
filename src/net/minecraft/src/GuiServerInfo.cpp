#include "GuiServerInfo.h"
#include "legacy/LegacySceneState.h"

#include <algorithm>
#include <string>
#include <vector>

#include "FontRenderer.h"
#include "GuiButton.h"
#include "GameSettings.h"
#include "Minecraft.h"
#include "ServerNBTStorage.h"
#include "SoundManager.h"
#include "StringTranslate.h"
#include "UiStrings.h"
#include "ControlIcon.h"
#include "legacy/LegacyMenuHints.h"
#include "legacy/LegacyMenuWidgets.h"
#include "legacy/LegacyOptionsLayout.h"
#include "legacy/LegacyOptionsPanel.h"
#include "legacy/LegacyPanorama.h"
#include "java/System.h"
#include "platform/PlatformConfig.h"
#include "platform/Input.h"

namespace
{
constexpr int_t BUTTON_DONE = 0;

bool serverInfoIsEs()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);
}

std::string relativeAge(long_t now, long_t timestamp)
{
    long_t s = (now - timestamp) / 1000;
    if (s < 0) s = 0;
    if (s < 5) return serverInfoIsEs() ? "ahora" : "just now";
    if (s < 60) return std::to_string(s) + "s";
    return std::to_string(s / 60) + "m";
}
} // namespace

GuiServerInfo::GuiServerInfo(GuiScreen *parent, const std::shared_ptr<ServerNBTStorage> &serverEntry)
    : parentScreen(parent), server(serverEntry), openLatch(true)
{
}

bool GuiServerInfo::usesSpecializedMenuNavigation() const
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    return true;
#else
    return false;
#endif
}

void GuiServerInfo::initGui()
{
    openLatch = true;
    controlList.clear();
    const bool legacy = (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI);
    if (!legacy)
    {
        controlList.push_back(new GuiButton(BUTTON_DONE, width / 2 - 50, height - 28, 100, 20,
            serverInfoIsEs() ? "Volver" : "Done"));
    }
}

void GuiServerInfo::actionPerformed(GuiButton *button)
{
    if (button != nullptr && button->id == BUTTON_DONE && mc != nullptr)
        mc->displayGuiScreen(parentScreen);
}

void GuiServerInfo::keyTyped(char_t c, int_t key)
{
    if (key == 1 && mc != nullptr)
    {
        mc->displayGuiScreen(parentScreen);
        return;
    }
    GuiScreen::keyTyped(c, key);
}

void GuiServerInfo::handleSpecializedMenuInput()
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    if (!pad.connected)
        return;
    if (openLatch)
    {
        if ((pad.held & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_BACK | PLATFORM_TEXT_SHIFT)) == 0)
            openLatch = false;
        return;
    }
    if (pad.pressed & (PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_TYPE))
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        if (mc != nullptr)
            mc->displayGuiScreen(parentScreen);
    }
#endif
}

// LCE look: grey bevelled panel with a dark title and the details in a recessed
// list well (the Legacy flat-world list style), hint row at the bottom.
void GuiServerInfo::drawLegacyScreen(float_t partialTick)
{
    const bool es = serverInfoIsEs();
    legacyDrawMenuBackdrop(mc, width, height, partialTick);

    struct Line { std::string text; int_t color; int_t indent; };
    std::vector<Line> lines;
    if (server != nullptr)
    {
        std::string name, host, motd, playerCount;
        long_t lag = -2;
        bool polled = false;
        std::vector<ServerActivityEntry> act;
        std::vector<jstring> names;
        {
            std::lock_guard<std::mutex> guard(server->stateMutex);
            name = server->name;
            host = server->host;
            motd = server->motd;
            playerCount = server->playerCount;
            lag = server->lag;
            polled = server->polled;
            act = server->activity;
            names = server->playerNames;
        }

        lines.push_back({(es ? "Nombre: " : "Name: ") + name, 0xFFFFFF, 0});
        lines.push_back({(es ? "Direccion: " : "Address: ") + host, 0xCCCCCC, 0});
        const std::string status = !polled ? (es ? "Esperando..." : "Waiting...")
                                 : (lag == -2 ? (es ? "Comprobando..." : "Checking...")
                                 : (lag < 0 ? "Offline" : ("Online (" + std::to_string(lag) + " ms)")));
        const int_t statusCol = (!polled || lag == -2) ? 0xAAAAAA : (lag < 0 ? 0xFF5555 : 0x55FF55);
        lines.push_back({(es ? "Estado: " : "Status: ") + status, statusCol, 0});
        if (!playerCount.empty())
            lines.push_back({(es ? "Jugadores: " : "Players: ") + playerCount, 0xDDDDDD, 0});
        if (!motd.empty())
            lines.push_back({"MOTD: " + motd, 0xAAAAAA, 0});
        if (!names.empty())
        {
            std::string plist;
            for (size_t i = 0; i < names.size() && i < 6; ++i)
            {
                if (i > 0) plist += ", ";
                plist += std::string(names[i]);
            }
            lines.push_back({plist, 0x88DDFF, 0});
        }
        lines.push_back({es ? "Actividad reciente:" : "Recent activity:", 0xFFFF88, 0});
        const long_t now = System::currentTimeMillis();
        int_t shown = 0;
        for (int i = static_cast<int>(act.size()) - 1; i >= 0 && shown < 4; --i)
        {
            std::string line = relativeAge(now, act[i].timestamp) + " - " +
                (act[i].latency < 0 ? "Offline" : (std::to_string(act[i].latency) + " ms"));
            if (!act[i].playerCount.empty())
                line += " (" + std::string(act[i].playerCount) + ")";
            lines.push_back({line, act[i].latency < 0 ? 0xFF8888 : 0xAAFFAA, 4});
            ++shown;
        }
        if (shown == 0)
            lines.push_back({es ? "(Sin registros todavia)" : "(No polls recorded yet)", 0x888888, 4});
    }

    const int_t lineCount = std::max<int_t>(1, static_cast<int_t>(lines.size()));
    // One layout row per text line; 10 px rows + the Legacy gap give an 12-13 px pitch.
    const LegacyMenuLayout l = legacyMenuLayout(mc, width, height, 292, lineCount, 24, 4, 10);
    legacyDrawMenuPanel(l.panelX, l.panelY, l.panelW, l.panelH);
    legacyDrawPanelTitle(fontRenderer, es ? "Informacion del servidor" : "Server Information",
        l.panelX + l.panelW / 2, l.titleY);

    const int_t boxY = l.firstRowY - 5;
    const int_t boxH = l.footerY + 4 - boxY;
    legacyDrawInsetBox(l.contentX, boxY, l.contentW, boxH, true);
    for (int_t i = 0; i < l.visibleRows && i < static_cast<int_t>(lines.size()); ++i)
    {
        const Line &ln = lines[static_cast<size_t>(i)];
        drawString(fontRenderer, legacyFitText(fontRenderer, ln.text, l.contentW - 10 - ln.indent),
            l.contentX + 5 + ln.indent, l.rowY(i) + 1, ln.color);
    }

#if PLATFORM_XBOX
    const std::string btns[] = {"B"};
    const std::string acts[] = { es ? "Volver" : "Back" };
    drawControlHintRow(mc, width, l.hintY, btns, acts, 1);
#endif
}

void GuiServerInfo::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    const bool es = serverInfoIsEs();
    const bool legacy = (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI);
    if (legacy)
    {
        drawLegacyScreen(partialTick);
        return;
    }

    if (legacy && mc != nullptr && legacyDrawPanorama(mc, width, height, legacyScenePanoramaTimer(), partialTick, zLevel))
    {
    }
    else
    {
        drawDefaultBackground();
    }

    const int_t hintY = legacyHintRowY(height, false);
    const int_t panelW = std::min<int_t>(292, width - 16);
    const int_t panelH = std::min<int_t>(174, (legacy ? hintY : height - 34) - 14);
    const int_t panelX = (width - panelW) / 2;
    const int_t panelY = std::max<int_t>(8, ((legacy ? hintY : height - 32) - panelH) / 2);

    if (legacy)
    {
        LegacyOptionsLayout layout{};
        layout.panelX = panelX;
        layout.panelY = panelY;
        layout.panelWidth = panelW;
        layout.panelHeight = panelH;
        LegacyOptionsPanel panel;
        panel.draw(layout);
    }
    else
    {
        drawRect(panelX, panelY, panelX + panelW, panelY + panelH, static_cast<int_t>(0xD8121212u));
        drawRect(panelX, panelY, panelX + panelW, panelY + 1, static_cast<int_t>(0xFF666666u));
        drawRect(panelX, panelY + panelH - 1, panelX + panelW, panelY + panelH, static_cast<int_t>(0xFF666666u));
    }

    drawCenteredString(fontRenderer, es ? "Informacion del Servidor" : "Server Information",
        panelX + panelW / 2, panelY + 7, 0xFFFFFF);

    if (server != nullptr)
    {
        std::string name, host, motd, playerCount;
        long_t lag = -2;
        bool polled = false;
        std::vector<ServerActivityEntry> act;
        std::vector<jstring> names;
        {
            std::lock_guard<std::mutex> guard(server->stateMutex);
            name = server->name;
            host = server->host;
            motd = server->motd;
            playerCount = server->playerCount;
            lag = server->lag;
            polled = server->polled;
            act = server->activity;
            names = server->playerNames;
        }

        int_t y = panelY + 24;
        const int_t lx = panelX + 10;
        drawString(fontRenderer, (es ? "Nombre: " : "Name: ") + name, lx, y, 0xFFFFFF); y += 12;
        drawString(fontRenderer, (es ? "Direccion: " : "Address: ") + host, lx, y, 0xCCCCCC); y += 12;

        std::string status = !polled ? (es ? "Esperando..." : "Waiting...")
                           : (lag == -2 ? (es ? "Comprobando..." : "Checking...")
                           : (lag < 0 ? "Offline" : ("Online (" + std::to_string(lag) + " ms)")));
        int_t statusCol = (!polled || lag == -2) ? 0xAAAAAA : (lag < 0 ? 0xFF5555 : 0x55FF55);
        drawString(fontRenderer, (es ? "Estado: " : "Status: ") + status, lx, y, statusCol); y += 12;

        if (!playerCount.empty())
        {
            drawString(fontRenderer, (es ? "Jugadores: " : "Players: ") + playerCount, lx, y, 0xDDDDDD);
            y += 12;
        }
        if (!motd.empty())
        {
            drawString(fontRenderer, fontRenderer->trimStringToWidth("MOTD: " + motd, panelW - 20), lx, y, 0xAAAAAA);
            y += 14;
        }

        // Player names if available
        if (!names.empty())
        {
            std::string plist;
            for (size_t i = 0; i < names.size() && i < 6; ++i)
            {
                if (i > 0) plist += ", ";
                plist += std::string(names[i]);
            }
            drawString(fontRenderer, fontRenderer->trimStringToWidth(plist, panelW - 20), lx, y, 0x88DDFF);
            y += 14;
        }

        // Recent poll activity
        drawString(fontRenderer, es ? "Actividad reciente:" : "Recent activity:", lx, y, 0xFFFF88);
        y += 11;
        const long_t now = System::currentTimeMillis();
        int_t shown = 0;
        for (int i = static_cast<int>(act.size()) - 1; i >= 0 && shown < 4 && y < panelY + panelH - 12; --i)
        {
            std::string line = relativeAge(now, act[i].timestamp) + " - " +
                (act[i].latency < 0 ? "Offline" : (std::to_string(act[i].latency) + " ms"));
            if (!act[i].playerCount.empty())
                line += " (" + std::string(act[i].playerCount) + ")";
            drawString(fontRenderer, line, lx + 4, y, act[i].latency < 0 ? 0xFF8888 : 0xAAFFAA);
            y += 11;
            ++shown;
        }
        if (shown == 0)
            drawString(fontRenderer, es ? "(Sin registros todavia)" : "(No polls recorded yet)", lx + 4, y, 0x888888);
    }

    if (legacy)
    {
#if PLATFORM_XBOX
        const std::string btns[] = {"B"};
        const std::string acts[] = { es ? "Volver" : "Back" };
        drawControlHintRow(mc, width, hintY, btns, acts, 1);
#endif
    }
    else
    {
        GuiScreen::drawScreen(mouseX, mouseY, partialTick);
    }
}
