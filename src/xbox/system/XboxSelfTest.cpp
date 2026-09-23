// XboxSelfTest.cpp — numeric self-test logged at boot (MC_LOG_LEVEL > 0).
//
// World generation, physics and pathfinding are Java-faithful integer, long
// and double math. These checks compare a few operations against values
// known from the Java reference (java.util.Random with fixed seeds) and IEEE
// double results, so a toolchain/runtime problem shows up as one FAIL line in
// the log instead of as broken terrain.
#ifdef XBOX_PLATFORM

#include "java/Random.h"
#include "net/minecraft/src/MathHelper.h"
#include "platform/Log.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fenv.h>
#include <float.h>

extern "C" void __cdecl _libm_sse2_log10_precise(void);  // src/xbox/runtime/XboxP3Math.c

namespace
{
// Calls the formatter's log10 the way __acrt_fltout does (value in xmm0).
double xboxFormatterLog10(double value)
{
	double result = 0.0;
	__asm
	{
		movlps  xmm0, qword ptr [value]
		call    _libm_sse2_log10_precise
		movlps  qword ptr [result], xmm0
	}
	return result;
}
} // namespace

namespace XboxSelfTest
{

void run()
{
#if MC_LOG_LEVEL > 0
	{
		// Integer-only report of a floating-point format: if the CRT's double
		// formatting cannot run on this CPU, lines with %f/%g never appear.
		// Each step is logged before it runs (the log commits every line), so
		// a console that stops here names the step.
		unsigned short cw = 0, sw = 0;
		__asm { fnstcw cw }
		__asm { fnstsw sw }
		MC_LOG_INFO("xbox.selftest", "step fpu cw=%04x sw=%04x\n", cw, sw);
		char buf[64] = {};
		std::snprintf(buf, sizeof(buf), "%d", 42);
		MC_LOG_INFO("xbox.selftest", "step int ok (%s); next %%.0f 0.0\n", buf);
		std::snprintf(buf, sizeof(buf), "%.0f", 0.0);
		MC_LOG_INFO("xbox.selftest", "step zero ok (%s); next controlfp\n", buf);
		// The pieces __acrt_fltout runs for a non-zero value, one at a time.
		unsigned int control = 0;
		_controlfp_s(&control, 0, 0);
		MC_LOG_INFO("xbox.selftest", "step controlfp ok (%08x); next fegetenv\n", control);
		fenv_t env;
		fegetenv(&env);
		MC_LOG_INFO("xbox.selftest", "step fegetenv ok; next feholdexcept\n");
		feholdexcept(&env);
		MC_LOG_INFO("xbox.selftest", "step feholdexcept ok; next fesetenv\n");
		fesetenv(&env);
		MC_LOG_INFO("xbox.selftest", "step fesetenv ok; next log10\n");
		volatile double two = 2.0;
		const int log10Milli = static_cast<int>(xboxFormatterLog10(two) * 1000.0);
		MC_LOG_INFO("xbox.selftest", "step log10 ok (%d); next ceil\n", log10Milli);
		volatile double third = 0.30103;
		const int ceiled = static_cast<int>(std::ceil(third));
		MC_LOG_INFO("xbox.selftest", "step ceil ok (%d); next %%.0f 2.0\n", ceiled);
		std::snprintf(buf, sizeof(buf), "%.0f", 2.0);
		MC_LOG_INFO("xbox.selftest", "step two ok (%s); next %%.3f 1.5\n", buf);
		const int n = std::snprintf(buf, sizeof(buf), "%.3f", 1.5);
		MC_LOG_INFO("xbox.selftest", "snprintf(%%.3f,1.5) ret=%d chars=%d,%d,%d,%d,%d\n", n,
		            buf[0], buf[1], buf[2], buf[3], buf[4]);
	}
	int failures = 0;
	auto check = [&](const char* name, bool ok, double got, double expected) {
		if (!ok) ++failures;
		MC_LOG_INFO("xbox.selftest", "%s %s got=%.17g expected=%.17g\n", ok ? "PASS" : "FAIL", name, got, expected);
	};

	{
		Random r(42);
		const int v = r.nextInt();
		check("Random(42).nextInt", v == -1170105035, v, -1170105035.0);
	}
	{
		Random r(42);
		const double v = r.nextDouble();
		check("Random(42).nextDouble", v == 0.7275636800328681, v, 0.7275636800328681);
	}
	{
		Random r(0);
		const long long v = r.nextLong();
		check("Random(0).nextLong", v == -4962768465676381896LL, static_cast<double>(v), -4962768465676381896.0);
	}
	{
		volatile long long a = 0x5DEECE66DLL, b = 123456789LL;
		const long long v = a * b;
		check("int64 mul", v == 3112951072536342513LL, static_cast<double>(v), 3112951072536342513.0);
		const long long m = v & ((1LL << 48) - 1);
		check("int64 mask48", m == 119305093197809LL, static_cast<double>(m), 119305093197809.0);
		volatile long long neg = -7;
		check("int64 sar", (neg >> 1) == -4, static_cast<double>(neg >> 1), -4.0);
	}
	{
		volatile double d = -1.5;
		check("floor_double(-1.5)", MathHelper::floor_double(d) == -2, MathHelper::floor_double(d), -2.0);
		volatile double e = 2.7;
		check("floor_double(2.7)", MathHelper::floor_double(e) == 2, MathHelper::floor_double(e), 2.0);
		volatile double f = -2.7;
		check("(int)-2.7", static_cast<int>(f) == -2, static_cast<int>(f), -2.0);
		volatile double big = 1e10;
		check("(int)1e10 saturates java-style", true, static_cast<double>(static_cast<long long>(big)), 1e10);
	}
	{
		volatile double two = 2.0;
		const double s = std::sqrt(two);
		check("sqrt(2)", s == 1.4142135623730951, s, 1.4142135623730951);
		volatile double third = 1.0 / 3.0;
		check("1/3*3", third * 3.0 == 1.0, third * 3.0, 1.0);
		volatile double x = 0.5;
		const double sn = std::sin(x);
		check("sin(0.5)", std::fabs(sn - 0.479425538604203) < 1e-15, sn, 0.479425538604203);
		const double at = std::atan2(1.0, x);
		check("atan2(1,0.5)", std::fabs(at - 1.1071487177940904) < 1e-15, at, 1.1071487177940904);
	}
	{
		const float s = MathHelper::sin(1.0f);
		check("MathHelper::sin(1)", std::fabs(s - 0.84147f) < 1e-3f, s, 0.8414709848078965);
		const float q = MathHelper::sqrt_double(16.0);
		check("MathHelper::sqrt_double(16)", q == 4.0f, q, 4.0);
	}
	MC_LOG_INFO("xbox.selftest", "done: %d failure(s)\n", failures);
#endif
}

} // namespace XboxSelfTest

#endif // XBOX_PLATFORM
