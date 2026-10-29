// Resource_ps3.cpp -- PS3 implementation of Resource::getResource().
//
// Same contract as src/xbox/java/Resource_xbox.cpp: callers ask for paths rooted
// at the resource directory ("/terrain.png", "/gui/logo.png") and own the
// returned stream. On the PS3 the resources are assets.pak (or a loose data/
// tree) in the title's USRDIR, see Resources_PS3.cpp.
#ifdef PS3_PLATFORM

#include "java/Resource.h"
#include "java/String.h"
#include "net/minecraft/src/GameResources.h"
#include "platform/storage/PathUtils.h"

#include <stdexcept>
#include <string>

namespace Resource
{

std::istream *getResource(const jstring &name)
{
	auto input = GameResources::open(static_cast<const std::string &>(name));
	if (!input)
	{
		const std::string path = PlatformStorage::join(
			GameResources::getAssetsDir(), static_cast<const std::string &>(name));
		throw std::runtime_error(
			"Missing game resource:\n" + path +
			"\n\nUSRDIR must contain assets.pak (or data/assets and data/resources).");
	}

	return input.release();
}

} // namespace Resource

#endif // PS3_PLATFORM
