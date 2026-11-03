#include "ItemEnchantedBook.h"

#include "Enchantment.h"
#include "EnchantmentData.h"
#include "EnumRarity.h"
#include "ItemStack.h"
#include "MathHelper.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "java/Random.h"

ItemEnchantedBook::ItemEnchantedBook(int id)
    : Item(id)
{
    setMaxStackSize(1);
    Enchantment::initEnchantments();
}

bool ItemEnchantedBook::hasEffect(ItemStack *stack)
{
    (void)stack;
    return true;
}

bool ItemEnchantedBook::isItemTool(ItemStack *stack)
{
    (void)stack;
    return false;
}

const EnumRarity &ItemEnchantedBook::getRarity(ItemStack *stack)
{
    NBTTagList *tags = func_92110_g(stack);
    if (tags != nullptr && tags->tagCount() > 0)
    {
        return EnumRarity::uncommon;
    }
    return Item::getRarity(stack);
}

NBTTagList *ItemEnchantedBook::func_92110_g(ItemStack *stack)
{
    if (stack != nullptr && stack->hasTagCompound() && stack->getTagCompound()->hasKey("StoredEnchantments"))
    {
        return stack->getTagCompound()->getTagList("StoredEnchantments");
    }
    return nullptr;
}

void ItemEnchantedBook::addInformation(ItemStack *stack, std::vector<std::string> &information)
{
    Enchantment::initEnchantments();
    Item::addInformation(stack, information);
    NBTTagList *stored = func_92110_g(stack);
    if (stored != nullptr)
    {
        for (int_t i = 0; i < stored->tagCount(); ++i)
        {
            NBTTagCompound *tag = dynamic_cast<NBTTagCompound *>(stored->tagAt(i));
            if (tag != nullptr)
            {
                short_t id = tag->getShort("id");
                short_t lvl = tag->getShort("lvl");
                if (id >= 0 && id < 256 && Enchantment::enchantmentsList[id] != nullptr)
                {
                    information.push_back(Enchantment::enchantmentsList[id]->getTranslatedName(lvl));
                }
            }
        }
    }
}

void ItemEnchantedBook::func_92115_a(ItemStack *stack, const EnchantmentData &data)
{
    if (stack == nullptr || data.enchantmentobj == nullptr)
        return;

    NBTTagList *tags = func_92110_g(stack);
    bool isNew = true;

    if (tags == nullptr)
    {
        tags = new NBTTagList();
        if (!stack->hasTagCompound())
        {
            stack->setTagCompound(new NBTTagCompound("tag"));
        }
        stack->getTagCompound()->setTag("StoredEnchantments", tags);
    }

    for (int_t i = 0; i < tags->tagCount(); ++i)
    {
        NBTTagCompound *entry = dynamic_cast<NBTTagCompound *>(tags->tagAt(i));
        if (entry != nullptr && entry->getShort("id") == data.enchantmentobj->effectId)
        {
            if (entry->getShort("lvl") < data.enchantmentLevel)
            {
                entry->setShort("lvl", static_cast<short_t>(data.enchantmentLevel));
            }
            isNew = false;
            break;
        }
    }

    if (isNew)
    {
        NBTTagCompound *entry = new NBTTagCompound();
        entry->setShort("id", static_cast<short_t>(data.enchantmentobj->effectId));
        entry->setShort("lvl", static_cast<short_t>(data.enchantmentLevel));
        tags->appendTag(entry);
    }
}

ItemStack *ItemEnchantedBook::func_92111_a(const EnchantmentData &data)
{
    ItemStack *stack = new ItemStack(this);
    func_92115_a(stack, data);
    return stack;
}

void ItemEnchantedBook::func_92113_a(Enchantment *enchantment, std::vector<ItemStack *> &list)
{
    Enchantment::initEnchantments();
    if (enchantment == nullptr)
        return;

    for (int_t lvl = enchantment->getMinLevel(); lvl <= enchantment->getMaxLevel(); ++lvl)
    {
        list.push_back(func_92111_a(EnchantmentData(enchantment, lvl)));
    }
}

ItemStack *ItemEnchantedBook::func_92109_a(Random &random)
{
    Enchantment::initEnchantments();
    if (Enchantment::field_92090_c.empty())
        return new ItemStack(this->itemID, 1, 0);

    Enchantment *ench = Enchantment::field_92090_c[static_cast<size_t>(random.nextInt(static_cast<int_t>(Enchantment::field_92090_c.size())))];
    ItemStack *stack = new ItemStack(this->itemID, 1, 0);
    int_t lvl = MathHelper::getRandomIntegerInRange(random, ench->getMinLevel(), ench->getMaxLevel());
    func_92115_a(stack, EnchantmentData(ench, lvl));
    return stack;
}

WeightedRandomChestContent ItemEnchantedBook::func_92114_b(Random &random)
{
    return func_92112_a(random, 1, 1, 1);
}

WeightedRandomChestContent ItemEnchantedBook::func_92112_a(Random &random, int minCount, int maxCount, int weight)
{
    Enchantment::initEnchantments();
    if (Enchantment::field_92090_c.empty())
    {
        ItemStack fallback(this->itemID, 1, 0);
        return WeightedRandomChestContent(fallback, minCount, maxCount, weight);
    }

    Enchantment *ench = Enchantment::field_92090_c[static_cast<size_t>(random.nextInt(static_cast<int_t>(Enchantment::field_92090_c.size())))];
    ItemStack stack(this->itemID, 1, 0);
    int_t lvl = MathHelper::getRandomIntegerInRange(random, ench->getMinLevel(), ench->getMaxLevel());
    func_92115_a(&stack, EnchantmentData(ench, lvl));
    return WeightedRandomChestContent(stack, minCount, maxCount, weight);
}
