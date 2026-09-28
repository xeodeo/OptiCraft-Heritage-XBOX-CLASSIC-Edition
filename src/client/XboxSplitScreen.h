#pragma once

class Minecraft;

// Two-player split screen on the Xbox (see XboxSplitScreen.cpp).
namespace XboxSplitScreen
{
#ifdef XBOX_PLATFORM
    void joinPlayer2(Minecraft *mc);
    void leavePlayer2(Minecraft *mc);
    void tick(Minecraft *mc);
    void postTick(Minecraft *mc);
    // Once per rendered frame: player 2's right stick turns its camera.
    void turnCamera(Minecraft *mc);
    // Player 2 chose "Leave Game" in its pause menu; it leaves on the next tick,
    // outside its own screen context.
    void requestLeave();
#else
    inline void joinPlayer2(Minecraft *mc) { (void)mc; }
    inline void leavePlayer2(Minecraft *mc) { (void)mc; }
    inline void tick(Minecraft *mc) { (void)mc; }
    inline void postTick(Minecraft *mc) { (void)mc; }
    inline void turnCamera(Minecraft *mc) { (void)mc; }
    inline void requestLeave() {}
#endif
}
