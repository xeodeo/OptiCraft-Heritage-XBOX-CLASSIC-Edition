#pragma once

#include "LegacyLook.h"

#include "platform/PlatformConfig.h"

#include <algorithm>
#include <cmath>
#if PLATFORM_XBOX
#include "java/Math.h"
#endif

// Desktop uses the real framebuffer gamma shader. Console backends keep the
// low-cost lightmap/fog/sky approximation because they do not share the PC
// GLSL pipeline.
inline bool legacyLookGradeFramebufferPassEnabled()
{
    return true;
}

inline float legacyLookGradePreview(float color)
{
    color = std::max(0.0f, std::min(1.0f, color));
#if PLATFORM_XBOX
    // The UCRT pow helper uses SSE2, which the Xbox CPU lacks.
    return static_cast<float>(JavaMath::pow(color, LEGACY_LOOK_GAMMA_EXPONENT));
#else
    return std::pow(color, LEGACY_LOOK_GAMMA_EXPONENT);
#endif
}
