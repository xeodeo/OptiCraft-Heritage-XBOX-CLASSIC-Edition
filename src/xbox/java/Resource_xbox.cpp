// Resource_xbox.cpp — Xbox implementation of Resource::getResource().
//
// Same contract as src/wii/Resource_wii.cpp: callers ask for paths rooted at
// the resource directory ("/terrain.png", "/gui/logo.png") and the stream is
// owned by the caller. On the Xbox the resources are the data/ tree on the game
// disc (D:\data\assets), which XAPI has mounted before any static initialiser
// runs, so there is no storage bring-up to wait for.
#ifdef XBOX_PLATFORM

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
			"\n\nThe disc must contain data/assets and data/resources\n"
			"(cmake --build --preset xbox-release --target xbox-data).");
	}

	return input.release();
}

} // namespace Resource

#endif // XBOX_PLATFORM
