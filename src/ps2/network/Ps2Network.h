#pragma once

#ifdef PS2_PLATFORM

#include <cstdint>

namespace Ps2Network
{
bool initialize();
bool isReady();
bool localAddressNetworkOrder(std::uint32_t &address);
}

#endif
