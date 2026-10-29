#pragma once

// Public include shim. See Keyboard.h for why this exists.
#if defined(PS3_PLATFORM)
#include "java/Type.h"

namespace lwjgl
{
namespace Mouse
{
namespace detail
{
void pushMotion(int x, int y, int xrel, int yrel);
void pushButton(int button, bool down, int x, int y);
void pushWheel(int delta, int x, int y);
}

void setCursorPosition(int_t x, int_t y);

bool next();

int_t getEventButton();
bool getEventButtonState();

int_t getEventDX();
int_t getEventDY();

int_t getEventX();
int_t getEventY();

int_t getEventDWheel();

int_t getX();
int_t getY();

int_t getDX();
int_t getDY();

int_t getDWheel();

void clearDeltas();

bool isButtonDown(int_t button);

bool isGrabbed();
void setGrabbed(bool grabbed);

}
}
#else
#include "../pc/lwjgl/Mouse.h"
#endif
