#include "ItemFirework.h"

#include "EntityFireworkRocket.h"
#include "EntityPlayer.h"
#include "ItemFireworkCharge.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "StatCollector.h"
#include "World.h"

ItemFirework::ItemFirework(int id)
    : Item(id)
{
}

bool ItemFirework::onItemUse(ItemStack *itemstack, EntityPlayer *entityplayer, World *world,
                             int i, int j, int k, int l)
{
    if (world == nullptr || itemstack == nullptr)
        return false;

    if (!world->multiplayerWorld)
    {
        double spawnX = static_cast<double>(i) + 0.5;
        double spawnY = static_cast<double>(j) + 0.5;
        double spawnZ = static_cast<double>(k) + 0.5;

        if (l == 0)      spawnY = static_cast<double>(j) - 0.1;
        else if (l == 1) spawnY = static_cast<double>(j) + 1.0;
        else if (l == 2) spawnZ = static_cast<double>(k);
        else if (l == 3) spawnZ = static_cast<double>(k) + 1.0;
        else if (l == 4) spawnX = static_cast<double>(i);
        else if (l == 5) spawnX = static_cast<double>(i) + 1.0;

        EntityFireworkRocket *rocket = new EntityFireworkRocket(world, spawnX, spawnY, spawnZ, itemstack);
        if (!world->entityJoinedWorld(rocket))
            delete rocket;

        if (entityplayer == nullptr || !entityplayer->capabilities.isCreativeMode)
        {
            --itemstack->stackSize;
        }
        return true;
    }

    return false;
}

void ItemFirework::addInformation(ItemStack *itemstack, std::vector<std::string> &information)
{
    if (itemstack == nullptr || !itemstack->hasTagCompound())
        return;

    NBTTagCompound *root = itemstack->getTagCompound();
    if (root == nullptr || !root->hasKey("Fireworks"))
        return;

    NBTTagCompound *fireworks = root->getCompoundTag("Fireworks");
    if (fireworks == nullptr)
        return;

    if (fireworks->hasKey("Flight"))
    {
        information.push_back(StatCollector::translateToLocal("item.fireworks.flight") + " " +
                              std::to_string(static_cast<int>(fireworks->getByte("Flight"))));
    }

    if (fireworks->hasKey("Explosions"))
    {
        NBTTagList *explosions = fireworks->getTagList("Explosions");
        if (explosions != nullptr && explosions->tagCount() > 0)
        {
            for (int_t idx = 0; idx < explosions->tagCount(); ++idx)
            {
                NBTTagCompound *explosion = dynamic_cast<NBTTagCompound *>(explosions->tagAt(idx));
                if (explosion == nullptr)
                    continue;

                std::vector<std::string> lines;
                ItemFireworkCharge::addExplosionInfo(explosion, lines);
                if (!lines.empty())
                {
                    for (size_t lineIdx = 1; lineIdx < lines.size(); ++lineIdx)
                    {
                        lines[lineIdx] = "  " + lines[lineIdx];
                    }
                    information.insert(information.end(), lines.begin(), lines.end());
                }
            }
        }
    }
}
