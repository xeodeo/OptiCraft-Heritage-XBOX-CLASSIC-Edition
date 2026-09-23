#pragma once

// -----------------------------------------------------------------------------
// Wii overrides
// -----------------------------------------------------------------------------
// The Wii takes the desktop branch above on purpose -- PLATFORM_CONSOLE_LOW is 0
// for it (see PlatformConfig.h) -- and then overrides the handful of values that
// are desktop *assumptions* rather than deliberate choices. Kept as an override
// file rather than a third full copy of the table: the Wii agrees with the
// desktop baseline on roughly fifty of these knobs, and a copy would drift.
#if PLATFORM_WII
#  include "wii/WiiTuning.h"
#  undef  PLATFORM_LEGACY_GUI_SCALE
#  define PLATFORM_LEGACY_GUI_SCALE WII_LEGACY_GUI_SCALE
#  undef  PLATFORM_LEGACY_CREATE_WORLD_PANEL_WIDTH
#  define PLATFORM_LEGACY_CREATE_WORLD_PANEL_WIDTH WII_LEGACY_CREATE_WORLD_PANEL_WIDTH
#elif PLATFORM_XBOX
#  include "xbox/XboxTuning.h"
#  undef  PLATFORM_LEGACY_GUI_SCALE
#  define PLATFORM_LEGACY_GUI_SCALE XBOX_LEGACY_GUI_SCALE
#  undef  PLATFORM_LEGACY_CREATE_WORLD_PANEL_WIDTH
#  define PLATFORM_LEGACY_CREATE_WORLD_PANEL_WIDTH XBOX_LEGACY_CREATE_WORLD_PANEL_WIDTH
#endif
