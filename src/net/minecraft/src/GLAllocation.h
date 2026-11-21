#pragma once

#include <vector>
#include "java/Type.h"
#include "platform/PlatformConfig.h"

// net.minecraft.src.GLAllocation
class GLAllocation
{
public:
#if PLATFORM_PC || defined(XBOX_PLATFORM)
    static int_t generateDisplayLists(int_t count);
    static void deleteDisplayLists(int_t first);
    // Index of the [base, count] pair whose base is `first` in the flat pair
    // table, or -1. Counts share slots with list names, so a plain value search
    // can match a count by mistake (see deleteDisplayLists).
    static int_t findDisplayListPair(const std::vector<int_t> &table, int_t first)
    {
        for (int_t i = 0; i + 1 < static_cast<int_t>(table.size()); i += 2)
        {
            if (table[i] == first)
                return i;
        }
        return -1;
    }
#endif
    static void generateTextureNames(std::vector<int_t> &names);
    static void deleteTexturesAndDisplayLists();

private:
#if PLATFORM_PC || defined(XBOX_PLATFORM)
    static std::vector<int_t> displayLists;
#endif
    static std::vector<int_t> textureNames;
};
