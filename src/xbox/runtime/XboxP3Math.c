// XboxP3Math.c — Pentium III replacements for UCRT math routines.
//
// The Xbox CPU has SSE but no SSE2. The VS2022 UCRT is built for SSE2, and a
// few of its routines do double arithmetic in SSE2 registers with no CPU
// check (ldexp, scalbn, ceil, floor, the formatter's log10 and the x87 error
// hook). xemu executes SSE2 anyway; a real console faults on the first one.
// These definitions are linked ahead of libucrt, so the library objects are
// never pulled in. They use integer registers and the x87 unit only; built
// with /arch:IA32 (see xbox.cmake).
//
// Data-movement-only SSE2 in the rest of the CRT (movsd/movq/movlpd copies)
// is rewritten to SSE1 after linking: scripts/xbox/patch_sse2_moves.ps1.
#ifdef XBOX_PLATFORM

#include <errno.h>
#include <string.h>

typedef unsigned __int64 u64;

static u64 bitsOf(double x)
{
    u64 bits;
    memcpy(&bits, &x, sizeof(bits));
    return bits;
}

static double fromBits(u64 bits)
{
    double x;
    memcpy(&x, &bits, sizeof(x));
    return x;
}

static const u64 kSignMask = 0x8000000000000000ull;
static const u64 kExpMask = 0x7FF0000000000000ull;
static const u64 kFracMask = 0x000FFFFFFFFFFFFFull;

// x * 2^n with IEEE rounding for results that stay normal; results that
// underflow into the subnormal range are rounded to nearest-even by hand.
double __cdecl scalbn(double x, int n)
{
    u64 bits = bitsOf(x);
    int exp = (int)((bits & kExpMask) >> 52);
    if (exp == 0x7FF) return x;                         // inf or nan
    if ((bits & ~kSignMask) == 0) return x;             // +-0
    const u64 sign = bits & kSignMask;
    u64 frac = bits & kFracMask;
    if (exp == 0)
    {
        // Subnormal input: normalize.
        while ((frac & (1ull << 52)) == 0)
        {
            frac <<= 1;
            --exp;
        }
        frac &= kFracMask;
        exp += 1;
    }
    if (n > 5000) n = 5000;
    if (n < -5000) n = -5000;
    exp += n;
    if (exp >= 0x7FF)
    {
        errno = ERANGE;
        return fromBits(sign | kExpMask);                // overflow -> inf
    }
    if (exp > 0) return fromBits(sign | ((u64)exp << 52) | frac);

    // Result is subnormal or zero.
    const int shift = 1 - exp;                           // >= 1
    if (shift > 53)
    {
        errno = ERANGE;
        return fromBits(sign);
    }
    const u64 mant = frac | (1ull << 52);
    u64 result = mant >> shift;
    const u64 rem = mant & ((1ull << shift) - 1);
    const u64 half = 1ull << (shift - 1);
    if (rem > half || (rem == half && (result & 1))) ++result;
    if (result == 0) errno = ERANGE;
    return fromBits(sign | result);                      // carry into exp 1 is correct
}

double __cdecl ldexp(double x, int n)
{
    return scalbn(x, n);
}

// ceil/floor: without SSE2 the UCRT dispatches to __ceil_default and
// __floor_default, which are themselves SSE2 code. x87 frndint under a
// temporarily switched rounding mode gives the same results, including
// -0.0, infinities and NaN passing through.
#pragma function(ceil, floor)
static double roundWithMode(double x, unsigned short mode)
{
    unsigned short saved, rounding;
    double result;
    __asm fnstcw saved
    rounding = (unsigned short)((saved & ~0x0C00) | mode);
    __asm
    {
        fldcw   rounding
        fld     x
        frndint
        fstp    result
        fldcw   saved
    }
    return result;
}

double __cdecl ceil(double x)
{
    return roundWithMode(x, 0x0800);   // round toward +inf
}

double __cdecl floor(double x)
{
    return roundWithMode(x, 0x0400);   // round toward -inf
}

// frexp is left to the UCRT: its object also carries helpers other library
// code links against, so a replacement here collides. Its only callers are
// iostream floating-point formatting (num_put), which the game does not use.

// Error hook the x87 transcendental dispatch calls for domain/range errors.
// The UCRT version runs its classification in SSE2. Report the error through
// errno, restore the caller's control word as the original does, and let the
// default IEEE result stand.
struct _exception;
void __cdecl _87except(int opcode, struct _exception* exc, unsigned short* controlWord)
{
    (void)opcode;
    (void)exc;
    errno = EDOM;
    if (controlWord)
    {
        unsigned short cw = *controlWord;
        __asm fldcw cw
    }
}

#endif // XBOX_PLATFORM

#ifdef XBOX_PLATFORM
// log10 as the UCRT float formatter (__acrt_fltout) calls it: argument and
// result in the low double of xmm0, no CPU check. The library body is SSE2
// throughout; this one moves the value with SSE1 movlps and computes on the
// x87 unit (log10(x) = log10(2) * log2(x)).
__declspec(naked) void __cdecl _libm_sse2_log10_precise(void)
{
    __asm
    {
        sub     esp, 8
        movlps  qword ptr [esp], xmm0
        fldlg2
        fld     qword ptr [esp]
        fyl2x
        fstp    qword ptr [esp]
        movlps  xmm0, qword ptr [esp]
        add     esp, 8
        ret
    }
}
#endif // XBOX_PLATFORM

#ifdef XBOX_PLATFORM
// Code cave for scripts/xbox/patch_sse2_moves.ps1. SSE2 instructions that have
// no same-length SSE1 twin (cvttsd2si r32,m64 in the UCRT's float formatter)
// are replaced by a call into x87 stubs the script writes here. 512 bytes of
// int3; the linker keeps it through the /INCLUDE directive.
#pragma comment(linker, "/INCLUDE:_XboxSse2Cave")
#define CAVE16 __asm _emit 0xCC __asm _emit 0xCC __asm _emit 0xCC __asm _emit 0xCC \
               __asm _emit 0xCC __asm _emit 0xCC __asm _emit 0xCC __asm _emit 0xCC \
               __asm _emit 0xCC __asm _emit 0xCC __asm _emit 0xCC __asm _emit 0xCC \
               __asm _emit 0xCC __asm _emit 0xCC __asm _emit 0xCC __asm _emit 0xCC
#define CAVE128 CAVE16 CAVE16 CAVE16 CAVE16 CAVE16 CAVE16 CAVE16 CAVE16
__declspec(naked) void XboxSse2Cave(void)
{
    CAVE128 CAVE128 CAVE128 CAVE128
}
#endif // XBOX_PLATFORM
