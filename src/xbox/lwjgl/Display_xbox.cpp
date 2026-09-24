#ifdef XBOX_PLATFORM
#include "lwjgl/Display.h"
#include "xbox/render/XboxD3D.h"
#include "xbox/input/XboxInput.h"
#include "client/Minecraft.h"
#include "net/minecraft/src/GuiScreen.h"
#include "net/minecraft/src/Entity.h"
#include "net/minecraft/src/GameSettings.h"
#include <cstdio>
#include <float.h>
#include <intrin.h>
#include <mmintrin.h>
#include "platform/Log.h"
#include "platform/Diagnostics.h"

void xboxRenderMemoryStats(long* listKB, long* lists, long* textureKB, long* textures);
void xboxProfileReport(unsigned int frames, unsigned long elapsedMs, double presentMs);
void xboxRenderEndFrame();
long xboxVertexPoolCount();

namespace lwjgl {
namespace Display {

namespace {
    DisplayMode g_mode(640, 480);
}

void setDisplayMode(const DisplayMode &display_mode) {
    g_mode = display_mode;
}

DisplayMode getDisplayMode() {
    return g_mode;
}

void setTitle(const jstring &string) {}
void setFullscreen(bool fullscreen) {}
bool isCloseRequested() { return false; }
bool isVisible() { return true; }
bool isActive() { return true; }

void processMessages()
{
    // Same contract as Display_wii.cpp: the game says whether a screen is
    // open (menu cursor mapping) or not (gameplay mapping), and the camera is
    // recaptured whenever there is a world and no screen.
    Minecraft* mc = Minecraft::getMinecraft();
    const bool inMenu = (mc != nullptr && mc->currentScreen != nullptr);
    const bool specializedMenuNavigation = inMenu &&
        mc->currentScreen->usesSpecializedMenuNavigationForPlatform();

    if (!inMenu && mc != nullptr && mc->theWorld != nullptr && !mc->inGameHasFocus)
        mc->setIngameFocus();

    XboxInput::poll(inMenu, specializedMenuNavigation);

#if MC_LOG_LEVEL > 0
    // Diagnostics: player state twice a second while in a world.
    static unsigned int s_pollCount = 0;
    if (mc != nullptr && mc->theWorld != nullptr && mc->thePlayer != nullptr && (++s_pollCount % 30) == 0)
    {
        const Entity* p = reinterpret_cast<const Entity*>(mc->thePlayer);
        MC_LOG_INFO("xbox.player", "pos=%.3f %.3f %.3f motionY=%.4f onGround=%d menu=%d\n",
                    p->posX, p->posY, p->posZ, p->motionY, p->onGround ? 1 : 0, inMenu ? 1 : 0);
    }
#endif
}

void swapBuffers() {
#if MC_LOG_LEVEL > 0
    // Frame-time report (Profiler_XBOX.cpp): Present() time says how long the
    // CPU waited for the GPU / vsync; the rest of the frame is CPU work.
    static unsigned int s_perfFrames = 0;
    static DWORD s_perfStart = 0;
    static unsigned long long s_presentCycles = 0;
    const unsigned long long presentStart = __rdtsc();
#endif
    if (g_pD3DDevice) {
        // 30 FPS option (limitFramerate 2): at least two vblanks between
        // presents. Waiting here, instead of switching the presentation
        // interval, takes effect (and reverts) immediately.
        static UINT s_lastPresentVBlank = 0;
        Minecraft* mc = Minecraft::getMinecraft();
        const bool cap30 = mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->limitFramerate == 2;
        D3DFIELD_STATUS field;
        g_pD3DDevice->GetDisplayFieldStatus(&field);
        if (cap30)
        {
            for (int guard = 0; guard < 3 && field.VBlankCount - s_lastPresentVBlank < 2; ++guard)
            {
                g_pD3DDevice->BlockUntilVerticalBlank();
                g_pD3DDevice->GetDisplayFieldStatus(&field);
            }
        }
        s_lastPresentVBlank = field.VBlankCount;
        g_pD3DDevice->Present(NULL, NULL, NULL, NULL);
        xboxRenderEndFrame();
    }
#if MC_LOG_LEVEL > 0
    s_presentCycles += __rdtsc() - presentStart;
    ++s_perfFrames;
    const DWORD now = GetTickCount();
    if (s_perfStart == 0)
        s_perfStart = now;
    if (now - s_perfStart >= 5000)
    {
        xboxProfileReport(s_perfFrames, now - s_perfStart, static_cast<double>(s_presentCycles) / 733333.0);
        s_perfFrames = 0;
        s_perfStart = now;
        s_presentCycles = 0;
    }
#endif
    // Keep the main thread's x87 unit in the state the game's double math
    // needs, whatever the XDK libraries did during the frame: empty MMX state
    // and 53-bit precision.
    static unsigned int s_frames = 0;
    if ((++s_frames % 300) == 1)
    {
        // Diagnostics: the x87 control/tag words as the frame left them.
        unsigned short cw = 0, sw = 0, tag = 0;
        unsigned char env[28];
        __asm { fnstcw cw }
        __asm { fnstsw sw }
        __asm { fnstenv env }
        __asm { fldcw cw }  // fnstenv masks exceptions; restore the control word
        tag = static_cast<unsigned short>(env[8] | (env[9] << 8));
        volatile double one = 1.0, tiny = 1e-12;
        const bool precise = (one + tiny) != one;
        MC_LOG_INFO("xbox.fpu", "frame=%u cw=%04x sw=%04x tag=%04x double53=%d freeKB=%ld\n",
                    s_frames, cw, sw, tag, precise ? 1 : 0, platformHeapFreeKb());
        long listKB = 0, lists = 0, textureKB = 0, textures = 0;
        xboxRenderMemoryStats(&listKB, &lists, &textureKB, &textures);
        MC_LOG_INFO("xbox.mem", "lists=%ld (%ldKB) vbPools=%ld textures=%ld (%ldKB)\n", lists, listKB, xboxVertexPoolCount(), textures, textureKB);
    }
    _mm_empty();
    unsigned int control = 0;
    _controlfp_s(&control, _PC_53, _MCW_PC);
}

void update(bool doProcessMessages) {
    swapBuffers();
    if (doProcessMessages)
        processMessages();
}

void create() {
    if (g_pD3DDevice) return;

    IDirect3D8* pD3D = Direct3DCreate8(D3D_SDK_VERSION);
    if (!pD3D) {
        MC_LOG_ERROR("xbox", "Direct3DCreate8 failed\n");
        return;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.BackBufferWidth        = 640;
    d3dpp.BackBufferHeight       = 480;
    d3dpp.BackBufferFormat       = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferCount        = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    const HRESULT hr = pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL,
                       D3DCREATE_HARDWARE_VERTEXPROCESSING,
                       &d3dpp, &g_pD3DDevice);
    MC_LOG_INFO("xbox", "CreateDevice hr=%08lx device=%p video=%08lx\n", (unsigned long)hr,
                (void*)g_pD3DDevice, (unsigned long)XGetVideoFlags());
    pD3D->Release();

    // Like desktop D3D, device creation can leave the x87 unit in 24-bit
    // (single) precision. The game is Java-faithful double math -- collision,
    // movement, world generation -- so put the main thread back to 53-bit
    // double precision.
    unsigned int control = 0;
    _controlfp_s(&control, _PC_53, _MCW_PC);
}

int_t getX() { return 0; }
int_t getY() { return 0; }
int_t getWidth() { return g_mode.getWidth(); }
int_t getHeight() { return g_mode.getHeight(); }

} // namespace Display
} // namespace lwjgl

#endif
