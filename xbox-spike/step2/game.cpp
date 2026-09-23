// Step 2: representative C++17 translation unit, no Xbox headers.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace {

struct Block {
    int id;
    std::string name;
};

std::vector<std::unique_ptr<Block>> g_blocks;
std::mutex g_lock;

template <typename T>
constexpr T sq(T v)
{
    if constexpr (sizeof(T) >= 8) {
        return v * v;
    } else {
        return static_cast<T>(v * v);
    }
}

std::optional<int> findId(std::string_view name)
{
    for (const auto& b : g_blocks) {
        if (b->name == name) {
            return b->id;
        }
    }
    return std::nullopt;
}

}  // namespace

extern "C" int GameSelfTest(char* msg, int msgLen)
{
    std::lock_guard<std::mutex> guard(g_lock);
    const char* names[] = {"stone", "grass", "dirt", "cobblestone", "planks"};
    for (int i = 0; i < 5; ++i) {
        g_blocks.push_back(std::make_unique<Block>(Block{i + 1, names[i]}));
    }

    std::unordered_map<std::string, int> counts;
    for (const auto& b : g_blocks) {
        counts[b->name] += b->id;
    }

    std::vector<int> ids;
    for (const auto& [name, id] : counts) {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end(), std::greater<int>());

    std::variant<int, float> v = 2.5f;
    float f = std::get<float>(v);

    std::function<int(int)> fn = [&](int x) { return x + sq(3); };

    int sum = 0;
    for (int id : ids) {
        sum += id;
    }
    int dirt = findId("dirt").value_or(-1);

    std::snprintf(msg, static_cast<size_t>(msgLen), "sum=%d dirt=%d fn=%d f=%.1f", sum, dirt, fn(1), f);
    // Expected: sum=15 dirt=3 fn=10 f=2.5
    return (sum == 15 && dirt == 3 && fn(1) == 10 && f == 2.5f) ? 1 : 0;
}
