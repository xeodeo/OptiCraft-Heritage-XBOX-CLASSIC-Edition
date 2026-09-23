// Step 2: Xbox entry. Blue = main() reached (CRT init OK), then
// green = C++17 self-test passed, red = failed. Black = died before main().
#include <xtl.h>

extern "C" int GameSelfTest(char* msg, int msgLen);
extern "C" int GameRiskyStep(int step);
extern "C" int ProbeSeh(void);
extern "C" int ProbeSeh71(void);
extern "C" unsigned long ProbeCppThrowCode(void);

static LPDIRECT3D8 g_d3d;
static LPDIRECT3DDEVICE8 g_dev;

static void Show(D3DCOLOR color, int frames)
{
    for (int i = 0; i < frames; ++i) {
        g_dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.0f, 0);
        g_dev->Present(NULL, NULL, NULL, NULL);
    }
}

void __cdecl main()
{
    g_d3d = Direct3DCreate8(D3D_SDK_VERSION);

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;

    g_d3d->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &g_dev);

    // ~5 s of blue at 60 Hz so the stage is visible before the test runs.
    Show(D3DCOLOR_XRGB(0, 0, 200), 300);

    char msg[128];
    int ok = GameSelfTest(msg, sizeof(msg));
    OutputDebugStringA(msg);
    if (!ok) {
        for (;;) {
            Show(D3DCOLOR_XRGB(200, 0, 0), 1);
        }
    }

    // SEH probes. Purple while running plain SEH; then olive while C++ throw
    // runs under __except. Results: red+blue=seh fail, orange-red=wrong code.
    // Teal while the VC7.1-compiled (_except_handler3) SEH probe runs.
    Show(D3DCOLOR_XRGB(0, 128, 128), 90);
    if (!ProbeSeh71()) {
        for (;;) {
            Show(D3DCOLOR_XRGB(0, 60, 60), 1);
        }
    }
    Show(D3DCOLOR_XRGB(128, 0, 255), 90);
    if (!ProbeSeh()) {
        for (;;) {
            Show(D3DCOLOR_XRGB(200, 0, 200), 1);
        }
    }
    Show(D3DCOLOR_XRGB(128, 128, 0), 90);
    if (ProbeCppThrowCode() != 0xE06D7363) {
        for (;;) {
            Show(D3DCOLOR_XRGB(255, 60, 0), 1);
        }
    }

    // One stage color per risky check; the screen freezes on the color of a
    // check that hangs or crashes. Failed (returned 0) checks turn red.
    static const D3DCOLOR kStage[5] = {
        D3DCOLOR_XRGB(255, 128, 0),    // 0 exceptions: orange
        D3DCOLOR_XRGB(255, 0, 255),    // 1 RTTI: magenta
        D3DCOLOR_XRGB(0, 255, 255),    // 2 streams: cyan
        D3DCOLOR_XRGB(255, 255, 255),  // 3 threads: white
        D3DCOLOR_XRGB(128, 64, 0),     // 4 thread_local: brown
    };
    for (int step = 0; step < 5; ++step) {
        Show(kStage[step], 90);
        if (!GameRiskyStep(step)) {
            for (;;) {
                Show(D3DCOLOR_XRGB(200, 0, (step + 1) * 40), 1);
            }
        }
    }
    for (;;) {
        Show(D3DCOLOR_XRGB(0, 160, 0), 1);
    }
}