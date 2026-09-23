// File_xbox.cpp — Xbox implementation of java/File.h.
//
// Same shape as src/wii/java/File_wii.cpp: every operation goes through
// platform/storage/PosixFileSystem.h, which the Xbox implements with XAPI in
// src/xbox/storage/XboxFileSystem.cpp (paths keep '/' here and are converted
// to '\' at the XAPI boundary).
//
// One difference from the Wii that matters for saving: the install directory
// is the game disc (D:, read-only), so the working directory -- where worlds
// and options are written -- is the title's persistent drive T: instead.
#ifdef XBOX_PLATFORM

#include "java/File.h"
#include "xbox/system/XboxWritableRoot.h"
#include "platform/Storage.h"
#include "platform/storage/PathUtils.h"
#include "platform/storage/PosixFileSystem.h"

#include <fstream>
#include <memory>
#include <string>

#include "net/minecraft/src/GameResources.h"
#include "util/Memory.h"

namespace
{

// Writable root: T: (E:\TDATA\<title id>) or Z: -- see XboxWritableRoot.

class File_Impl : public File
{
private:
	std::string u8path;

public:
	explicit File_Impl(const jstring &p)
	{
		u8path = PlatformStorage::normalizeSlashes(static_cast<const std::string &>(p));
		this->path = u8path;
	}

	bool createNewFile() const override
	{
		return PlatformStorage::createFile(u8path);
	}

	bool remove() const override
	{
		return PlatformStorage::removePath(u8path);
	}

	bool renameTo(const File &dest) const override
	{
		return PlatformStorage::renamePath(u8path, dest.toString());
	}

	bool exists() const override
	{
		return PlatformStorage::exists(u8path);
	}

	bool isDirectory() const override
	{
		return PlatformStorage::isDirectory(u8path);
	}

	bool isFile() const override
	{
		return PlatformStorage::isFile(u8path);
	}

	long_t lastModified() const override
	{
		return static_cast<long_t>(PlatformStorage::lastModifiedMs(u8path));
	}

	long_t length() const override
	{
		const std::int64_t size = PlatformStorage::fileSize(u8path);
		return size >= 0 ? static_cast<long_t>(size) : 0;
	}

	std::vector<std::unique_ptr<File>> listFiles() const override
	{
		std::vector<std::unique_ptr<File>> files;
		if (!isDirectory())
			return files;

		std::vector<std::string> entries;
		if (!PlatformStorage::listEntries(u8path, entries))
			return files;

		for (const std::string &entry : entries)
			files.push_back(Util::make_unique<File_Impl>(jstring(PlatformStorage::join(u8path, entry))));
		return files;
	}

	File *getParentFile() const override
	{
		return new File_Impl(jstring(PlatformStorage::parent(u8path)));
	}

	bool mkdir() const override
	{
		if (PlatformStorage::exists(u8path))
			return false;
		return PlatformStorage::makeDirectory(u8path, 0755);
	}

	std::istream *toStreamIn() const override
	{
		// The UCRT resolves '/' paths through XboxCrtShim.cpp, which converts
		// separators before calling XAPI, so streams can use u8path directly.
		auto is = Util::make_unique<std::ifstream>(u8path, std::ios::binary);
		if (!is->is_open() || !is->good())
			return nullptr;
		return is.release();
	}

	std::ostream *toStreamOut() const override
	{
		auto os = Util::make_unique<std::ofstream>(u8path, std::ios::binary);
		if (!os->is_open() || !os->good())
			return nullptr;
		return os.release();
	}
};

} // namespace

File *File::open(const jstring &path)
{
	return new File_Impl(path);
}

File *File::open(const File &parent, const jstring &child)
{
	return new File_Impl(jstring(PlatformStorage::join(parent.toString(), child)));
}

File *File::openResourceDirectory()
{
	return new File_Impl(jstring(GameResources::getAssetsDir()));
}

File *File::openWorkingDirectory(const jstring &name)
{
	return new File_Impl(jstring(PlatformStorage::join(XboxWritableRoot::get(), name)));
}

#endif // XBOX_PLATFORM
