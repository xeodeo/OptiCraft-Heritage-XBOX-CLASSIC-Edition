#pragma once

#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)

#include "java/Type.h"

class GuiContainer;
class Slot;

class ContainerSlotNavigator
{
public:
    struct Layout
    {
        int_t guiLeft = 0;
        int_t guiTop = 0;
        int_t screenWidth = 0;
        int_t screenHeight = 0;
        int_t displayWidth = 0;
        int_t displayHeight = 0;
    };

    static ContainerSlotNavigator& instance();

    void notifyOpen(GuiContainer *screen, const Layout &layout);
    void notifyClosed(const GuiContainer *screen);
    bool isActive() const { return screen != nullptr; }

    void tick();

    Slot *selectedSlot() const { return selected; }
    bool controllerSelectionActive() const { return controllerActive && selected != nullptr; }
    void notePointerActivity();
    // Slot under the platform pointer this frame (null when none or when the
    // pointer is not an active input). Seeds the D-pad selection.
    void notePointerSlot(Slot *slot);
    void activateControllerSelection();
    bool consumePrimaryClick();
    bool consumeSecondaryClick();

private:
    ContainerSlotNavigator() = default;

    void repairSelection();
    void clearControllerSelection();
    void moveMenuCursorToSelection();
    Slot *pickSlot(int_t originX, int_t originY, int_t dirX, int_t dirY) const;

    GuiContainer *screen = nullptr;
    Layout layout;
    Slot *selected = nullptr;
    Slot *pointerSlot = nullptr;
    bool controllerActive = false;
    bool ignorePointerMotionOnce = false;
    bool pendingPrimary = false;
    bool pendingSecondary = false;
    int nextRepeatMs = 0;
};

#endif // PS2_PLATFORM || WII_PLATFORM
