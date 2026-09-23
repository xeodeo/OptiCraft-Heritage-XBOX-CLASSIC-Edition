// Xbox toolchain smoke test (XBOX_BRINGUP=ON). Proves that the hybrid
// VS2022-compiler / XDK-linker image boots and that the runtime features the
// game depends on work, reporting progress as full-screen colors:
//
//   blue      main() reached (XboxEntry + modern CRT startup OK)
//   orange    C++ exceptions         magenta  RTTI
//   cyan      string streams         white    std::thread
//   brown     thread_local           purple   plain SEH (__try/__except)
//   green     everything passed
//   red       a check returned a wrong result (blue channel = 40 * (check + 1))
//
// A check that hangs or crashes freezes the screen on its stage color.
//
// The C++ checks live in an anonymous namespace below the D3D code; they only
// use the standard library, so this file is the one place the XDK and the STL
// meet in the bring-up image.

#include "XboxXtl.h"

#include <atomic>
#include <excpt.h>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <typeinfo>
#include <unordered_map>
#include <variant>
#include <vector>

namespace {

LPDIRECT3D8 g_d3d;
LPDIRECT3DDEVICE8 g_dev;

void show(D3DCOLOR color, int frames)
{
    for (int i = 0; i < frames; ++i) {
        g_dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.0f, 0);
        g_dev->Present(NULL, NULL, NULL, NULL);
    }
}

void initVideo()
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
}

// --- checks -------------------------------------------------------------------

struct Base {
    virtual ~Base() = default;
    virtual int kind() const { return 1; }
};
struct Derived : Base {
    int kind() const override { return 2; }
};

thread_local int t_counter = 5;
std::mutex g_lock;

bool checkStl()
{
    std::lock_guard<std::mutex> guard(g_lock);
    std::vector<std::unique_ptr<std::string>> names;
    for (const char* n : {"stone", "grass", "dirt"}) {
        names.push_back(std::make_unique<std::string>(n));
    }
    std::unordered_map<std::string, int> ids;
    for (size_t i = 0; i < names.size(); ++i) {
        ids[*names[i]] = static_cast<int>(i) + 1;
    }
    std::optional<int> dirt = ids.count("dirt") ? std::optional<int>(ids["dirt"]) : std::nullopt;
    std::variant<int, float> v = 2.5f;
    std::function<int(int)> fn = [](int x) { return x * 3; };
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", fn(dirt.value_or(0)));
    return std::string(buf) == "9" && std::get<float>(v) == 2.5f;
}

bool checkExceptions()
{
    try {
        throw std::runtime_error("boom");
    } catch (const std::exception& e) {
        return std::string(e.what()) == "boom";
    }
    return false;
}

bool checkRtti()
{
    std::unique_ptr<Base> b = std::make_unique<Derived>();
    return dynamic_cast<Derived*>(b.get()) != nullptr && typeid(*b) == typeid(Derived);
}

bool checkStreams()
{
    std::ostringstream os;
    os << "x=" << 42 << " y=" << 1.5;
    std::istringstream is("7 8");
    int a = 0, b = 0;
    is >> a >> b;
    return os.str() == "x=42 y=1.5" && a == 7 && b == 8;
}

bool checkThreads()
{
    std::atomic<int> value{0};
    std::thread worker([&] { value.store(99); });
    worker.join();
    return value.load() == 99;
}

bool checkThreadLocal()
{
    t_counter += 1;
    return t_counter == 6;
}

int sehFilter(unsigned long code, unsigned long* seen)
{
    *seen = code;
    return EXCEPTION_EXECUTE_HANDLER;
}

bool checkSeh()
{
    unsigned long seen = 0;
    __try {
        RaiseException(0xE0001234, 0, 0, NULL);
    } __except (sehFilter(GetExceptionCode(), &seen)) {
    }
    return seen == 0xE0001234;
}

struct Stage {
    D3DCOLOR color;
    bool (*run)();
};

}  // namespace

void __cdecl main()
{
    initVideo();
    show(D3DCOLOR_XRGB(0, 0, 200), 120);

    const Stage stages[] = {
        {D3DCOLOR_XRGB(0, 120, 200), checkStl},
        {D3DCOLOR_XRGB(128, 0, 255), checkSeh},
        {D3DCOLOR_XRGB(255, 128, 0), checkExceptions},
        {D3DCOLOR_XRGB(255, 0, 255), checkRtti},
        {D3DCOLOR_XRGB(0, 255, 255), checkStreams},
        {D3DCOLOR_XRGB(255, 255, 255), checkThreads},
        {D3DCOLOR_XRGB(128, 64, 0), checkThreadLocal},
    };
    for (int i = 0; i < static_cast<int>(sizeof(stages) / sizeof(stages[0])); ++i) {
        show(stages[i].color, 60);
        if (!stages[i].run()) {
            for (;;) {
                show(D3DCOLOR_XRGB(200, 0, (i + 1) * 40), 1);
            }
        }
    }
    for (;;) {
        show(D3DCOLOR_XRGB(0, 160, 0), 1);
    }
}
