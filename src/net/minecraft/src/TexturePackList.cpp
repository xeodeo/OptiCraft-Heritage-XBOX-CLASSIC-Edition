#include "platform/Log.h"
#include "TexturePackList.h"

#include <algorithm>
#include <cctype>
#if !defined(PS2_PLATFORM) && !defined(WII_PLATFORM) && !defined(XBOX_PLATFORM) && !defined(PS3_PLATFORM)
#include <filesystem>
#endif
#include <iostream>
#include "TexturePackDefault.h"
#include "TexturePackCustom.h"
#if defined(XBOX_PLATFORM)
#include "TexturePackFolder.h"
#include "TexturePackPak.h"
#include "java/File.h"
#include <memory>
#endif
#include "Minecraft.h"
#include "GameSettings.h"
#include "java/String.h"

#if !defined(PS2_PLATFORM) && !defined(WII_PLATFORM) && !defined(XBOX_PLATFORM) && !defined(PS3_PLATFORM)
namespace fs = std::filesystem;
#endif

TexturePackList::TexturePackList(Minecraft *minecraft, const std::string &file) :
	mc(minecraft),
	defaultTexturePack(new TexturePackDefault()),
	selectedTexturePack(nullptr)
{
	texturePackDir = file + "/texturepacks";
#if !defined(PS2_PLATFORM) && !defined(WII_PLATFORM) && !defined(XBOX_PLATFORM) && !defined(PS3_PLATFORM)
	if (!fs::exists(texturePackDir))
	{
		fs::create_directories(texturePackDir);
	}
#endif
	currentTexturePack = minecraft->gameSettings->skin;
	updateAvailableTexturePacks();
	selectedTexturePack->loadTexturePack();
}

TexturePackList::~TexturePackList()
{
	if (selectedTexturePack != nullptr)
		selectedTexturePack->closeTexturePackFile();
	for (auto &entry : soundPool)
		delete entry.second;
	soundPool.clear();
	availableTexturePacks.clear();
	delete defaultTexturePack;
	defaultTexturePack = nullptr;
	selectedTexturePack = nullptr;
}

bool TexturePackList::setTexturePack(TexturePackBase *texturepackbase)
{
	if (texturepackbase == selectedTexturePack)
	{
		return false;
	}
	else
	{
		selectedTexturePack->closeTexturePackFile();
		currentTexturePack = texturepackbase->texturePackFileName;
		selectedTexturePack = texturepackbase;
		mc->gameSettings->skin = currentTexturePack;
		mc->gameSettings->saveOptions();
		selectedTexturePack->loadTexturePack();
		return true;
	}
}

void TexturePackList::updateAvailableTexturePacks()
{
	std::vector<TexturePackBase*> arraylist;
	selectedTexturePack = nullptr;
	arraylist.push_back(defaultTexturePack);

#if !defined(PS2_PLATFORM) && !defined(WII_PLATFORM) && !defined(XBOX_PLATFORM) && !defined(PS3_PLATFORM)
	if (fs::exists(texturePackDir) && fs::is_directory(texturePackDir))
	{
		for (const auto &entry : fs::directory_iterator(texturePackDir))
		{
			if (!entry.is_regular_file()) continue;
			std::string filename = entry.path().filename().string();
			const jstring lowerName = String::toLowerCaseJava(jstring(filename));
			if (lowerName.size() < 4 || lowerName.compare(lowerName.size() - 4, 4, ".zip") != 0) continue;

			auto file = entry.path();
			// libc++ represents file_time_type with rep __int128 and has not std::to_string for him (ambiguous call).
			const auto mtime = static_cast<long long>(fs::last_write_time(file).time_since_epoch().count());
			std::string s = filename + ":" + std::to_string(fs::file_size(file)) + ":" + std::to_string(mtime);

			try
			{
				auto it = soundPool.find(s);
				if (it == soundPool.end())
				{
					TexturePackCustom *texturepackcustom = new TexturePackCustom(file.string());
					texturepackcustom->texturePackFolder = s;
					soundPool[s] = texturepackcustom;
					texturepackcustom->getTexturePackFolder(mc);
				}
				TexturePackBase *texturepackbase1 = soundPool[s];
				if (texturepackbase1->texturePackFileName == currentTexturePack)
				{
					selectedTexturePack = texturepackbase1;
				}
				arraylist.push_back(texturepackbase1);
			}
			catch (std::exception &ioexception)
			{
				MC_LOG_ERROR("game", "%s\n", ioexception.what());
			}
		}
	}
#elif defined(XBOX_PLATFORM)
	// Xbox: packs in texturepacks\ next to the game's XBE, either one .pak
	// file each (scripts/make_pak.py on the pack's folder; one file to copy
	// and open) or an unzipped folder (D:\texturepacks\<name>\terrain.png,
	// gui\items.png, ...). What a pack lacks comes from the game's own data.
	std::unique_ptr<File> packRoot(File::open(jstring("D:\\texturepacks")));
	if (packRoot != nullptr && packRoot->isDirectory())
	{
		for (std::unique_ptr<File> &entry : packRoot->listFiles())
		{
			if (entry == nullptr)
				continue;
			const std::string name = entry->getName();
			const jstring lowerName = String::toLowerCaseJava(jstring(name));
			const bool isPak = lowerName.size() > 4 && lowerName.compare(lowerName.size() - 4, 4, ".pak") == 0;
			if (!entry->isDirectory() && !isPak)
				continue;
			const std::string key = (isPak ? "pak:" : "folder:") + name;
			auto it = soundPool.find(key);
			if (it == soundPool.end())
			{
				TexturePackBase *pack = nullptr;
				if (isPak)
				{
					TexturePackPak *pakPack = new TexturePackPak(entry->toString());
					if (!pakPack->isValid())
					{
						delete pakPack;
						continue;
					}
					pack = pakPack;
				}
				else
				{
					pack = new TexturePackFolder(entry.release());
				}
				pack->texturePackFolder = key;
				pack->getTexturePackFolder(mc);
				it = soundPool.emplace(key, pack).first;
			}
			if (it->second->texturePackFileName == currentTexturePack)
				selectedTexturePack = it->second;
			arraylist.push_back(it->second);
		}
	}
#endif

	if (selectedTexturePack == nullptr)
	{
		selectedTexturePack = defaultTexturePack;
	}

	for (auto *tp : availableTexturePacks)
	{
		bool found = false;
		for (auto *tp2 : arraylist)
		{
			if (tp == tp2) { found = true; break; }
		}
		if (!found)
		{
			soundPool.erase(tp->texturePackFolder);
			delete tp;
		}
	}

	availableTexturePacks = arraylist;
}

const std::vector<TexturePackBase*> &TexturePackList::getAvailableTexturePacks() const
{
	return availableTexturePacks;
}

TexturePackBase *TexturePackList::getSelectedTexturePack() const
{
	return selectedTexturePack;
}
