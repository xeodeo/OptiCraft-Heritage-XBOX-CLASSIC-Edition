// Step 2b: the risky runtime features the game relies on. Each check sets one
// bit so a failure can be identified from the screen color.
#include <atomic>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

struct Base {
    virtual ~Base() = default;
    virtual int kind() const { return 1; }
};
struct Derived : Base {
    int kind() const override { return 2; }
};

thread_local int t_counter = 5;

int checkExceptions()
{
    try {
        throw std::runtime_error("boom");
    } catch (const std::exception& e) {
        return std::string(e.what()) == "boom" ? 1 : 0;
    }
    return 0;
}

int checkRtti()
{
    Base* b = new Derived();
    Derived* d = dynamic_cast<Derived*>(b);
    int ok = (d != nullptr && d->kind() == 2 && typeid(*b) == typeid(Derived)) ? 1 : 0;
    delete b;
    return ok;
}

int checkStreams()
{
    std::ostringstream os;
    os << "x=" << 42 << " y=" << 1.5;
    std::istringstream is("7 8");
    int a = 0, b = 0;
    is >> a >> b;
    return (os.str() == "x=42 y=1.5" && a == 7 && b == 8) ? 1 : 0;
}

int checkThreads()
{
    std::atomic<int> value{0};
    std::thread worker([&] { value.store(99); });
    worker.join();
    return value.load() == 99 ? 1 : 0;
}

int checkThreadLocal()
{
    t_counter += 1;
    return t_counter == 6 ? 1 : 0;
}

}  // namespace

// Runs one check by index (0 exceptions, 1 RTTI, 2 streams, 3 threads, 4 thread_local).
extern "C" int GameRiskyStep(int step)
{
    switch (step) {
    case 0: return checkExceptions();
    case 1: return checkRtti();
    case 2: return checkStreams();
    case 3: return checkThreads();
    case 4: return checkThreadLocal();
    }
    return 0;
}