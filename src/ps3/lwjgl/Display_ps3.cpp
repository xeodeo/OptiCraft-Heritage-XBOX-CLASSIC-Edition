// Display_ps3.cpp -- PS3 implementation of lwjgl::Display.
//
// Same contract as src/xbox/lwjgl/Display_xbox.cpp: create() brings the RSX up
// (Ps3Rsx), update() presents the frame and polls input. The XMB's own events
// (the PS button menu, "Quit Game") arrive through sysutil callbacks, which must
// be pumped every frame or the console marks the game as unresponsive.
#ifdef PS3_PLATFORM
#include "lwjgl/Display.h"
#include "client/Minecraft.h"
#include "net/minecraft/src/GuiScreen.h"
#include "platform/Log.h"
#include "ps3/input/Ps3Input.h"
#include "ps3/render/Ps3Rsx.h"
#include "ps3/system/Ps3System.h"

void ps3RenderBeginFrame();

namespace lwjgl {
namespace Display {

namespace {
    DisplayMode g_mode(1280, 720);
}

void setDisplayMode(const DisplayMode &display_mode) {
    g_mode = display_mode;
}

DisplayMode getDisplayMode() {
    return g_mode;
}

void setTitle(const jstring &string) {}
void setFullscreen(bool fullscreen) {}
bool isCloseRequested() { return Ps3System::exitRequested(); }
bool isVisible() { return true; }
bool isActive() { return true; }

void processMessages()
{
    Ps3System::pumpEvents();

    // The game says whether a screen is open (menu cursor mapping) or not
    // (gameplay mapping), and the camera is recaptured whenever there is a
    // world and no screen -- as on the Xbox and the Wii.
    Minecraft* mc = Minecraft::getMinecraft();
    const bool inMenu = (mc != nullptr && mc->currentScreen != nullptr);
    const bool specializedMenuNavigation = inMenu &&
        mc->currentScreen->usesSpecializedMenuNavigationForPlatform();

    if (!inMenu && mc != nullptr && mc->theWorld != nullptr && !mc->inGameHasFocus)
        mc->setIngameFocus();

    Ps3Input::poll(inMenu, specializedMenuNavigation);
}

void swapBuffers() {
    Ps3Rsx::endFrame();
    ps3RenderBeginFrame();
}

void update(bool doProcessMessages) {
    swapBuffers();
    if (doProcessMessages)
        processMessages();
}

void create() {
    if (Ps3Rsx::ready()) return;
    if (!Ps3Rsx::initialize())
    {
        MC_LOG_ERROR("ps3", "RSX initialisation failed\n");
        return;
    }
    g_mode = DisplayMode(Ps3Rsx::width(), Ps3Rsx::height());
    Ps3Rsx::beginFrame();
    ps3RenderBeginFrame();
}

int_t getX() { return 0; }
int_t getY() { return 0; }
int_t getWidth() { return g_mode.getWidth(); }
int_t getHeight() { return g_mode.getHeight(); }

} // namespace Display
} // namespace lwjgl

#endif
