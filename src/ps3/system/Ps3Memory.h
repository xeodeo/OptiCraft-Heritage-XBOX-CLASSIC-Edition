#pragma once

#include <cstdint>

// Main-memory accounting for the game process (Runtime, Diagnostics).
namespace Ps3Memory
{
struct Info
{
    std::uint32_t totalBytes = 0;      // user memory granted to the process
    std::uint32_t availableBytes = 0;  // of that, not yet committed by lv2
};

Info query();
}
