#pragma once

// Public include shim. See Keyboard.h for why this exists.
#if defined(PS3_PLATFORM)
#include <string>
#include <set>

namespace lwjgl
{
namespace GLContext
{
namespace detail
{
struct GLCapabilities
{
private:
	std::set<std::string> caps;

public:
	void add(const std::string &cap)
	{
		caps.insert(cap);
	}

	bool operator[](const std::string &cap) const
	{
		return caps.find(cap) != caps.end();
	}
};
}

void setRequestedSamples(int samples);
int getRequestedSamples();
void instantiate();
const detail::GLCapabilities &getCapabilities();

}
}
#else
#include "../pc/lwjgl/GLContext.h"
#endif
