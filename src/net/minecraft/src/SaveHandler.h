#pragma once

#include "ISaveHandler.h"
#include <string>
#include "java/Type.h"

class WorldProvider;
class WorldInfo;
class IChunkLoader;
class NBTTagCompound;

// net.minecraft.src.SaveHandler
class SaveHandler : public ISaveHandler
{
public:
    SaveHandler(const std::string &baseDir, const std::string &name, bool createPlayers, bool readOnly = false);
    ~SaveHandler() override = default;

    WorldInfo* loadWorldInfo() override;
    void checkSessionLock() override;
    IChunkLoader* getChunkLoader(WorldProvider *worldprovider) override;
    void saveWorldInfoAndPlayer(WorldInfo *worldinfo, const std::vector<EntityPlayer *> &players) override;
    void saveWorldInfo(WorldInfo *worldinfo) override;
    std::string getMapFile(const jstring &s) override;
    std::string getSaveDirectoryName() const override;
    std::string getSaveDirectory() const override { return saveDirectory; }
    bool isReadOnly() const override { return readOnly; }

private:
    void writeSessionLock();

    std::string saveDirectory;
    std::string saveDirectoryName;
    std::string playersDirectory;
    std::string dataDirectory;
    long_t sessionTime;
    bool readOnly;
};
