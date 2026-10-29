#pragma once

// Ps3StdCompat.h -- forced into every C++ file of the PS3 build (-include).
//
// PSL1GHT's newlib 1.20 declares the C99 math functions, but libstdc++ was
// configured against it without _GLIBCXX_USE_C99_MATH_TR1 (the long double
// variants are missing), so std::round, std::lround, std::trunc... do not
// exist. Bring the double/float versions newlib has into std, the way
// <cmath> would. std::stoi/strtof/llabs come from _GLIBCXX11_USE_C99_STDLIB,
// set on the command line by cmake/ps3.cmake.

#ifdef __cplusplus
#include <cmath>
#include <cstdlib>

namespace std
{
using ::acosh;
inline float acosh(float x) { return ::acoshf(x); }
using ::asinh;
inline float asinh(float x) { return ::asinhf(x); }
using ::atanh;
inline float atanh(float x) { return ::atanhf(x); }
using ::cbrt;
inline float cbrt(float x) { return ::cbrtf(x); }
using ::copysign;
inline float copysign(float x, float y) { return ::copysignf(x, y); }
using ::erf;
inline float erf(float x) { return ::erff(x); }
using ::erfc;
inline float erfc(float x) { return ::erfcf(x); }
using ::exp2;
inline float exp2(float x) { return ::exp2f(x); }
using ::expm1;
inline float expm1(float x) { return ::expm1f(x); }
using ::fdim;
inline float fdim(float x, float y) { return ::fdimf(x, y); }
using ::fma;
inline float fma(float x, float y, float z) { return ::fmaf(x, y, z); }
using ::fmax;
inline float fmax(float x, float y) { return ::fmaxf(x, y); }
using ::fmin;
inline float fmin(float x, float y) { return ::fminf(x, y); }
using ::hypot;
inline float hypot(float x, float y) { return ::hypotf(x, y); }
using ::ilogb;
inline int ilogb(float x) { return ::ilogbf(x); }
using ::lgamma;
inline float lgamma(float x) { return ::lgammaf(x); }
using ::llrint;
inline long long llrint(float x) { return ::llrintf(x); }
using ::llround;
inline long long llround(float x) { return ::llroundf(x); }
using ::log1p;
inline float log1p(float x) { return ::log1pf(x); }
using ::log2;
inline float log2(float x) { return ::log2f(x); }
using ::logb;
inline float logb(float x) { return ::logbf(x); }
using ::lrint;
inline long lrint(float x) { return ::lrintf(x); }
using ::lround;
inline long lround(float x) { return ::lroundf(x); }
using ::nearbyint;
inline float nearbyint(float x) { return ::nearbyintf(x); }
using ::nextafter;
inline float nextafter(float x, float y) { return ::nextafterf(x, y); }
using ::remainder;
inline float remainder(float x, float y) { return ::remainderf(x, y); }
using ::rint;
inline float rint(float x) { return ::rintf(x); }
using ::round;
inline float round(float x) { return ::roundf(x); }
using ::scalbln;
inline float scalbln(float x, long n) { return ::scalblnf(x, n); }
using ::scalbn;
inline float scalbn(float x, int n) { return ::scalbnf(x, n); }
using ::tgamma;
inline float tgamma(float x) { return ::tgammaf(x); }
using ::trunc;
inline float trunc(float x) { return ::truncf(x); }
}
#endif // __cplusplus
