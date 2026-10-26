#include "ContainerRepair.h"

#include <algorithm>
#include <map>
#include "Block.h"
#include "Enchantment.h"
#include "EnchantmentHelper.h"
#include "EntityPlayer.h"
#include "ICrafting.h"
#include "InventoryCraftResult.h"
#include "InventoryPlayer.h"
#include "InventoryRepair.h"
#include "Item.h"
#include "ItemEnchantedBook.h"
#include "ItemStack.h"
#include "NBTTagList.h"
#include "Slot.h"
#include "SlotRepair.h"
#include "World.h"
#include "StringTranslate.h"
#include "java/String.h"

ContainerRepair::ContainerRepair(InventoryPlayer *playerInv, World *world, int x, int y, int z, EntityPlayer *player)
    : outputSlot(new InventoryCraftResult()),
      inputSlots(new InventoryRepair(this, "Repair", true, 2)),
      theWorld(world),
      field_82861_i(x),
      field_82858_j(y),
      field_82859_k(z),
      thePlayer(player)
{
    addSlot(new Slot(inputSlots, 0, 27, 47));
    addSlot(new Slot(inputSlots, 1, 76, 47));
    addSlot(new SlotRepair(this, outputSlot, 2, 134, 47, world, x, y, z));

    for (int_t row = 0; row < 3; ++row)
    {
        for (int_t col = 0; col < 9; ++col)
        {
            addSlot(new Slot(playerInv, col + row * 9 + 9, 8 + col * 18, 84 + row * 18));
        }
    }

    for (int_t col = 0; col < 9; ++col)
    {
        addSlot(new Slot(playerInv, col, 8 + col * 18, 142));
    }
}

ContainerRepair::~ContainerRepair()
{
    delete outputSlot;
    delete inputSlots;
}

void ContainerRepair::onCraftMatrixChanged(IInventory *inv)
{
    Container::onCraftMatrixChanged(inv);
    if (inv == inputSlots)
    {
        updateRepairOutput();
    }
}

void ContainerRepair::updateRepairOutput()
{
    ItemStack *left = inputSlots->getStackInSlot(0);
    maximumCost = 0;
    int costExtra = 0;
    int costBase = 0;
    int renameCost = 0;

    if (left == nullptr)
    {
        outputSlot->setInventorySlotContents(0, nullptr);
        maximumCost = 0;
        return;
    }

    ItemStack *outputStack = left->copy();
    ItemStack *right = inputSlots->getStackInSlot(1);
    std::map<int, int> leftEnchantments = EnchantmentHelper::getEnchantments(outputStack);
    bool isEnchantedBook = false;

    costBase += left->getRepairCost() + (right == nullptr ? 0 : right->getRepairCost());
    stackSizeToBeUsedInRepair = 0;

    if (right != nullptr)
    {
        ItemEnchantedBook *enchantedBookItem = (Item::enchantedBook != nullptr)
            ? dynamic_cast<ItemEnchantedBook *>(Item::enchantedBook) : nullptr;
        isEnchantedBook = (right->itemID == (Item::enchantedBook != nullptr ? Item::enchantedBook->itemID : -1)) &&
            enchantedBookItem != nullptr && enchantedBookItem->func_92110_g(right) != nullptr &&
            enchantedBookItem->func_92110_g(right)->tagCount() > 0;

        if (outputStack->isItemStackDamageable() && Item::itemsList[outputStack->itemID] != nullptr &&
            Item::itemsList[outputStack->itemID]->getIsRepairable(left, right))
        {
            int damageRecoverPerUnit = std::min(outputStack->getItemDamageForDisplay(), outputStack->getMaxDamage() / 4);
            if (damageRecoverPerUnit <= 0)
            {
                delete outputStack;
                outputSlot->setInventorySlotContents(0, nullptr);
                maximumCost = 0;
                return;
            }

            int unitsUsed = 0;
            for (unitsUsed = 0; damageRecoverPerUnit > 0 && unitsUsed < right->stackSize; ++unitsUsed)
            {
                int newDamage = outputStack->getItemDamageForDisplay() - damageRecoverPerUnit;
                outputStack->setItemDamage(newDamage);
                costExtra += std::max(1, damageRecoverPerUnit / 100) + static_cast<int>(leftEnchantments.size());
                damageRecoverPerUnit = std::min(outputStack->getItemDamageForDisplay(), outputStack->getMaxDamage() / 4);
            }
            stackSizeToBeUsedInRepair = unitsUsed;
        }
        else
        {
            if (!isEnchantedBook && (outputStack->itemID != right->itemID || !outputStack->isItemStackDamageable()))
            {
                delete outputStack;
                outputSlot->setInventorySlotContents(0, nullptr);
                maximumCost = 0;
                return;
            }

            if (outputStack->isItemStackDamageable() && !isEnchantedBook)
            {
                int leftDurability = left->getMaxDamage() - left->getItemDamageForDisplay();
                int rightDurability = right->getMaxDamage() - right->getItemDamageForDisplay();
                int bonusDurability = rightDurability + outputStack->getMaxDamage() * 12 / 100;
                int combinedDurability = leftDurability + bonusDurability;
                int newDamage = outputStack->getMaxDamage() - combinedDurability;
                if (newDamage < 0)
                    newDamage = 0;
                if (newDamage < outputStack->getItemDamage())
                {
                    outputStack->setItemDamage(newDamage);
                    costExtra += std::max(1, bonusDurability / 100);
                }
            }

            std::map<int, int> rightEnchantments = EnchantmentHelper::getEnchantments(right);
            for (const auto &pair : rightEnchantments)
            {
                int enchId = pair.first;
                if (enchId < 0 || enchId >= 256 || Enchantment::enchantmentsList[enchId] == nullptr)
                    continue;

                Enchantment *ench = Enchantment::enchantmentsList[enchId];
                int leftLevel = (leftEnchantments.find(enchId) != leftEnchantments.end()) ? leftEnchantments[enchId] : 0;
                int rightLevel = pair.second;
                rightLevel = (leftLevel == rightLevel) ? rightLevel + 1 : std::max(rightLevel, leftLevel);
                int levelDiff = rightLevel - leftLevel;
                bool canApply = ench->canApply(left);

                if (thePlayer != nullptr && (thePlayer->capabilities.isCreativeMode ||
                    (Item::enchantedBook != nullptr && left->itemID == Item::enchantedBook->itemID)))
                {
                    canApply = true;
                }

                for (const auto &leftPair : leftEnchantments)
                {
                    int existingId = leftPair.first;
                    if (existingId != enchId && existingId >= 0 && existingId < 256 &&
                        Enchantment::enchantmentsList[existingId] != nullptr &&
                        !ench->canApplyTogether(Enchantment::enchantmentsList[existingId]))
                    {
                        canApply = false;
                        costExtra += levelDiff;
                    }
                }

                if (canApply)
                {
                    if (rightLevel > ench->getMaxLevel())
                        rightLevel = ench->getMaxLevel();

                    leftEnchantments[enchId] = rightLevel;
                    int weightMultiplier = 0;
                    switch (ench->getWeight())
                    {
                    case 1: weightMultiplier = 8; break;
                    case 2: weightMultiplier = 4; break;
                    case 5: weightMultiplier = 2; break;
                    case 10: weightMultiplier = 1; break;
                    default: weightMultiplier = 0; break;
                    }

                    if (isEnchantedBook)
                    {
                        weightMultiplier = std::max(1, weightMultiplier / 2);
                    }
                    costExtra += weightMultiplier * levelDiff;
                }
            }
        }
    }

    if (!repairedItemName.empty() &&
        !String::equalsIgnoreCaseJava(repairedItemName, StringTranslate::getInstance()->translateNamedKey(left->getItemName())) &&
        repairedItemName != left->getDisplayName())
    {
        renameCost = left->isItemStackDamageable() ? 7 : left->stackSize * 5;
        costExtra += renameCost;
        if (left->hasDisplayName())
        {
            costBase += renameCost / 2;
        }
        outputStack->setItemName(repairedItemName);
    }

    int enchIndex = 0;
    for (const auto &pair : leftEnchantments)
    {
        int enchId = pair.first;
        if (enchId < 0 || enchId >= 256 || Enchantment::enchantmentsList[enchId] == nullptr)
            continue;
        Enchantment *ench = Enchantment::enchantmentsList[enchId];
        int level = pair.second;
        int mult = 0;
        enchIndex++;
        switch (ench->getWeight())
        {
        case 1: mult = 8; break;
        case 2: mult = 4; break;
        case 5: mult = 2; break;
        case 10: mult = 1; break;
        default: mult = 0; break;
        }

        if (isEnchantedBook)
        {
            mult = std::max(1, mult / 2);
        }
        costBase += enchIndex + level * mult;
    }

    if (isEnchantedBook)
    {
        costBase = std::max(1, costBase / 2);
    }

    maximumCost = costBase + costExtra;
    if (costExtra <= 0)
    {
        delete outputStack;
        outputStack = nullptr;
    }

    if (renameCost == costExtra && renameCost > 0 && maximumCost >= 40)
    {
        maximumCost = 39;
    }

    if (maximumCost >= 40 && (thePlayer == nullptr || !thePlayer->capabilities.isCreativeMode))
    {
        delete outputStack;
        outputStack = nullptr;
    }

    if (outputStack != nullptr)
    {
        int finalRepairCost = outputStack->getRepairCost();
        if (right != nullptr && finalRepairCost < right->getRepairCost())
        {
            finalRepairCost = right->getRepairCost();
        }
        if (outputStack->hasDisplayName())
        {
            finalRepairCost -= 9;
        }
        if (finalRepairCost < 0)
        {
            finalRepairCost = 0;
        }
        finalRepairCost += 2;
        outputStack->setRepairCost(finalRepairCost);
        EnchantmentHelper::setEnchantments(leftEnchantments, outputStack);
    }

    outputSlot->setInventorySlotContents(0, outputStack);
    detectAndSendChanges();
}

void ContainerRepair::addCraftingToCrafters(ICrafting *crafter)
{
    Container::addCraftingToCrafters(crafter);
    if (crafter != nullptr)
    {
        crafter->sendProgressBarUpdate(this, 0, maximumCost);
    }
}

void ContainerRepair::updateProgressBar(int id, int value)
{
    if (id == 0)
    {
        maximumCost = value;
    }
}

void ContainerRepair::onCraftGuiClosed(EntityPlayer *player)
{
    Container::onCraftGuiClosed(player);
    if (theWorld != nullptr && !theWorld->multiplayerWorld && player != nullptr)
    {
        for (int_t i = 0; i < inputSlots->getSizeInventory(); ++i)
        {
            ItemStack *stack = inputSlots->getStackInSlotOnClosing(i);
            if (stack != nullptr)
            {
                player->dropPlayerItem(stack);
            }
        }
    }
}

bool ContainerRepair::isUsableByPlayer(EntityPlayer *player)
{
    if (theWorld == nullptr || player == nullptr)
        return false;
    if (Block::anvil == nullptr || theWorld->getBlockId(field_82861_i, field_82858_j, field_82859_k) != Block::anvil->blockID)
        return false;
    return player->getDistanceSq(static_cast<double>(field_82861_i) + 0.5,
                                 static_cast<double>(field_82858_j) + 0.5,
                                 static_cast<double>(field_82859_k) + 0.5) <= 64.0;
}

ItemStack *ContainerRepair::transferStackInSlot(EntityPlayer *player, int slotIndex)
{
    ItemStack *result = nullptr;
    Slot *slot = getSlot(slotIndex);
    if (slot != nullptr && slot->getHasStack())
    {
        ItemStack *stack = slot->getStack();
        result = stack->copy();
        if (slotIndex == 2)
        {
            if (!mergeItemStack(stack, 3, 39, true))
            {
                delete result;
                return nullptr;
            }
            slot->onSlotChange(stack, result);
        }
        else if (slotIndex != 0 && slotIndex != 1)
        {
            if (slotIndex >= 3 && slotIndex < 39 && !mergeItemStack(stack, 0, 2, false))
            {
                delete result;
                return nullptr;
            }
        }
        else if (!mergeItemStack(stack, 3, 39, false))
        {
            delete result;
            return nullptr;
        }

        if (stack->stackSize == 0)
        {
            slot->putStack(nullptr);
        }
        else
        {
            slot->onSlotChanged();
        }

        if (stack->stackSize == result->stackSize)
        {
            delete result;
            return nullptr;
        }

        slot->onPickupFromSlot(player, stack);
    }
    return result;
}

void ContainerRepair::updateItemName(const std::string &name)
{
    repairedItemName = name;
    Slot *output = getSlot(2);
    if (output != nullptr && output->getHasStack())
    {
        output->getStack()->setItemName(repairedItemName);
    }
    updateRepairOutput();
}
