#include "TexturePackBase.h"
#include "GameResources.h"

#include <fstream>
#include <algorithm>
#ifdef XBOX_PLATFORM
#include "legacy/XboxButtonAtlasPng.h"
#include "legacy/LegacyCraftingIconsPng.h"
#include <sstream>
#endif
#if !defined(PS2_PLATFORM) && !defined(WII_PLATFORM) && !defined(XBOX_PLATFORM)
#include <filesystem>
#endif

TexturePackBase::TexturePackBase()
{
}

void TexturePackBase::loadTexturePack()
{
}

void TexturePackBase::closeTexturePackFile()
{
}

void TexturePackBase::getTexturePackFolder(Minecraft *minecraft)
{
}

void TexturePackBase::getResourceAsStream(Minecraft *minecraft)
{
}

void TexturePackBase::bindThumbnailTexture(Minecraft *minecraft)
{
}

std::istream* TexturePackBase::getResourceAsStream(const std::string &s)
{
#ifdef XBOX_PLATFORM
	// Serve the compiled-in Xbox button atlas and crafting icons ahead of
	// data/ and assets.pak so the texture always matches XboxButtonAtlasData.h
	// even on consoles with an older buttons_xbox.png on disk.
	if (s.find("buttons_xbox.png") != std::string::npos)
		return new std::istringstream(std::string(reinterpret_cast<const char*>(s_xboxButtonAtlasPngData), XBOX_BUTTON_ATLAS_PNG_SIZE));
	if (s.find("crafting_icons.png") != std::string::npos)
		return new std::istringstream(std::string(reinterpret_cast<const char*>(s_legacyCraftingIconsPngData), LEGACY_CRAFTING_ICONS_PNG_SIZE));
#endif
	// Java getResourceAsStream fallback: resolve against the vanilla asset root.
	auto stream = GameResources::open(s);
	return stream.release();
}

std::vector<std::string> TexturePackBase::listResources(const std::string &prefix, const std::string &suffix)
{
	std::vector<std::string> result;
#if !defined(PS2_PLATFORM) && !defined(WII_PLATFORM) && !defined(XBOX_PLATFORM)
	namespace fs = std::filesystem;
	std::string normalizedPrefix = prefix;
	while (!normalizedPrefix.empty() && normalizedPrefix.front() == '/')
		normalizedPrefix.erase(normalizedPrefix.begin());
	const fs::path root = fs::path(GameResources::getAssetsDir()) / normalizedPrefix;
	if (!fs::exists(root) || !fs::is_directory(root))
		return result;
	for (const auto &entry : fs::recursive_directory_iterator(root))
	{
		if (!entry.is_regular_file())
			continue;
		const std::string name = entry.path().filename().string();
		if (!suffix.empty() && (name.size() < suffix.size() || name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0))
			continue;
		std::string relative = fs::relative(entry.path(), GameResources::getAssetsDir()).generic_string();
		result.push_back('/' + relative);
	}
	std::sort(result.begin(), result.end());
#else
	(void)prefix;
	(void)suffix;
#endif
	return result;
}
