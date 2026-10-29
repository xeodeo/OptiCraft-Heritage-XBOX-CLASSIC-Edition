// Placeholder until the DualShock 3 port of src/xbox/input lands (agy encargo):
// the game boots and renders but cannot be driven yet.
#ifdef PS3_PLATFORM

#include "ps3/input/Ps3Input.h"

namespace Ps3Input
{
void poll(bool, bool)
{
}
}

#endif // PS3_PLATFORM
