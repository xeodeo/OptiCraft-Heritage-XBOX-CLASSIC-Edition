#include "LegacyLook.h"

#include "platform/PlatformConfig.h"

#include <algorithm>
#include <cmath>
#if PLATFORM_XBOX
#include "java/Math.h"
#endif

namespace
{
constexpr int LEGACY_GAMMA_LUT_SIZE = 256;

struct LegacyGammaLut
{
    float_t values[LEGACY_GAMMA_LUT_SIZE];
    unsigned char bytes[LEGACY_GAMMA_LUT_SIZE];

    LegacyGammaLut()
    {
        for (int i = 0; i < LEGACY_GAMMA_LUT_SIZE; ++i)
        {
            const float_t input = static_cast<float_t>(i) /
                static_cast<float_t>(LEGACY_GAMMA_LUT_SIZE - 1);
#if PLATFORM_XBOX
            // The UCRT pow helper uses SSE2, which the Xbox CPU lacks.
            values[i] = static_cast<float_t>(JavaMath::pow(input, LEGACY_LOOK_GAMMA_EXPONENT));
#else
            values[i] = std::pow(input, LEGACY_LOOK_GAMMA_EXPONENT);
#endif
            bytes[i] = static_cast<unsigned char>(values[i] * 255.0f + 0.5f);
        }
    }
};

const LegacyGammaLut &legacyGammaLut()
{
    // Built once. This keeps pow() out of the lightmap/fog hot paths on the
    // Atom N270, PS2 and Wii while preserving the Legacy4J gamma curve.
    static const LegacyGammaLut lut;
    return lut;
}
}

bool legacyLookDefaultEnabled()
{
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
    return true;
#else
    return false;
#endif
}

float_t legacyLookChannel(float_t value)
{
    const float_t c = std::max<float_t>(0.0f, std::min<float_t>(1.0f, value));

    // Legacy4J reference:
    //   effectiveGamma = legacyGamma * 1.5 + 0.5
    //   output = pow(input, 1 / effectiveGamma)
    //
    // OptiCraft uses a 256-entry LUT because this function is reached by the
    // lightmap and also by fog/sky grading. Linear interpolation avoids visible
    // 8-bit steps while still avoiding pow() during rendering.
    const float_t scaled = c * static_cast<float_t>(LEGACY_GAMMA_LUT_SIZE - 1);
    int index = static_cast<int>(scaled);
    if (index >= LEGACY_GAMMA_LUT_SIZE - 1)
        return 1.0f;

    const float_t fraction = scaled - static_cast<float_t>(index);
    const LegacyGammaLut &lut = legacyGammaLut();
    return lut.values[index] +
        (lut.values[index + 1] - lut.values[index]) * fraction;
}

unsigned char legacyLookByte(unsigned char value)
{
    return legacyGammaLut().bytes[value];
}

void legacyLookRgb(float_t &r, float_t &g, float_t &b)
{
    // Do not add contrast, saturation, a black floor or a warm/cold tint here.
    // Legacy4J's gamma pass applies the same power curve independently to RGB.
    r = legacyLookChannel(r);
    g = legacyLookChannel(g);
    b = legacyLookChannel(b);
}
