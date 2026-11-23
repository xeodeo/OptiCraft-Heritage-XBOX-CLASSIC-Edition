/* simplewebp (BSD-3-Clause, Google Inc. / Miku AuahDark; licence at the end of
 * simplewebp.h): WebP decoder for the texture pack downloader's Modrinth icons
 * (most of them are .webp). Plain C, no SIMD, no floating point. Only the
 * Xbox downloads icons, so only the Xbox build compiles it. */
#if defined(XBOX_PLATFORM) || defined(TEXDL_HOST_TEST)
#define SIMPLEWEBP_DISABLE_STDIO
#define SIMPLEWEBP_IMPLEMENTATION
#include "simplewebp.h"
#endif
