#include "PotionEffect.h"

#include <cstdio>

#include "EntityLiving.h"
#include "Potion.h"
#include "NBTTagCompound.h"

PotionEffect::PotionEffect(int id, int effectDuration, int effectAmplifier) :
	potionID(id), duration(effectDuration), amplifier(effectAmplifier)
{
}

PotionEffect::PotionEffect(int id, int effectDuration, int effectAmplifier, bool effectAmbient) :
	potionID(id), duration(effectDuration), amplifier(effectAmplifier), ambient(effectAmbient)
{
}

PotionEffect::PotionEffect(const PotionEffect &other) :
	potionID(other.potionID), duration(other.duration), amplifier(other.amplifier)
{
}

void PotionEffect::combine(const PotionEffect &other)
{
	if (potionID != other.potionID)
		std::fprintf(stderr, "This method should only be called for matching effects!\n");

	if (other.amplifier > amplifier)
	{
		amplifier = other.amplifier;
		duration = other.duration;
	}
	else if (other.amplifier == amplifier && duration < other.duration)
	{
		duration = other.duration;
	}
	else if (!other.ambient && ambient)
	{
		ambient = false;
	}
}

int PotionEffect::getPotionID() const
{
	return potionID;
}

int PotionEffect::getDuration() const
{
	return duration;
}

int PotionEffect::getAmplifier() const
{
	return amplifier;
}

bool PotionEffect::isSplashPotionEffect() const { return splashPotion; }
void PotionEffect::setSplashPotion(bool splash) { splashPotion = splash; }
bool PotionEffect::getIsAmbient() const { return ambient; }
bool PotionEffect::getIsPotionDurationMax() const { return potionDurationMax; }
void PotionEffect::setPotionDurationMax(bool value) { potionDurationMax = value; }

NBTTagCompound *PotionEffect::writeCustomPotionEffectToNBT(NBTTagCompound *tag) const
{
	if (tag == nullptr) return nullptr;
	tag->setByte("Id", static_cast<byte_t>(potionID));
	tag->setByte("Amplifier", static_cast<byte_t>(amplifier));
	tag->setInteger("Duration", duration);
	tag->setBoolean("Ambient", ambient);
	return tag;
}

PotionEffect *PotionEffect::readCustomPotionEffectFromNBT(NBTTagCompound *tag)
{
	if (tag == nullptr) return nullptr;
	return new PotionEffect(static_cast<int>(tag->getByte("Id")), tag->getInteger("Duration"),
	                        static_cast<int>(tag->getByte("Amplifier")), tag->getBoolean("Ambient"));
}

bool PotionEffect::onUpdate(EntityLiving *entity)
{
	if (duration > 0)
	{
		Potion *potion = Potion::getPotion(potionID);
		if (potion != nullptr && potion->isReady(duration, amplifier))
			performEffect(entity);
		deincrementDuration();
	}
	return duration > 0;
}

int PotionEffect::deincrementDuration()
{
	return --duration;
}

void PotionEffect::performEffect(EntityLiving *entity)
{
	if (duration <= 0)
		return;
	Potion *potion = Potion::getPotion(potionID);
	if (potion != nullptr)
		potion->performEffect(entity, amplifier);
}

jstring PotionEffect::getEffectName() const
{
	Potion *potion = Potion::getPotion(potionID);
	return potion != nullptr ? potion->getName() : jstring();
}

bool PotionEffect::equals(const PotionEffect &other) const
{
	return potionID == other.potionID && amplifier == other.amplifier && duration == other.duration &&
	       splashPotion == other.splashPotion && ambient == other.ambient;
}
