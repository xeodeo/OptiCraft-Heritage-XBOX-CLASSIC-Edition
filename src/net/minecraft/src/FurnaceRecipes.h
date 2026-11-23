#pragma once

#include <map>
#include <vector>
#include "java/Type.h"
#include "java/HashSet.h"

class ItemStack;

// net.minecraft.src.FurnaceRecipes
class FurnaceRecipes
{
private:
    FurnaceRecipes();

public:
    static FurnaceRecipes* smelting();

    void addSmelting(int_t itemId, ItemStack *output, float experience);
    void addSmelting(int_t itemId, ItemStack *output) { addSmelting(itemId, output, 0.0f); }
    ItemStack* getSmeltingResult(int_t itemId);
    float getExperience(int_t itemId);
    std::map<int_t, ItemStack*>& getSmeltingList();
    std::vector<ItemStack*> getSmeltingValuesInJavaOrder() const;

private:
    struct JavaIntHash
    {
        uint_t operator()(int_t value) const
        {
            return static_cast<uint_t>(value);
        }
    };

    struct JavaIntEqual
    {
        bool operator()(int_t left, int_t right) const
        {
            return left == right;
        }
    };

    static FurnaceRecipes *smeltingBase;
    std::map<int_t, ItemStack*> smeltingList;
    std::map<int_t, float> experienceList;
    JavaHashSet<int_t, JavaIntHash, JavaIntEqual> smeltingOrder;
};
