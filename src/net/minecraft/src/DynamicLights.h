#pragma once

#include "java/Type.h"

class World;
class Minecraft;

// Native port of Dynamic Lights (from erluza/main, adapted as an opt-in mod on Xbox).
namespace DynamicLights
{
extern bool g_enabled;
extern World *g_world;
extern int_t g_sourceCount;

inline bool activeFor(const World *world)
{
    return g_enabled && world != nullptr && g_world == world && g_sourceCount > 0;
}

int_t getLightValue(const World *world, int_t x, int_t y, int_t z);
void setModEnabled(bool enabled);
bool isModEnabled();
void tick(Minecraft *mc);
void reset();
}
