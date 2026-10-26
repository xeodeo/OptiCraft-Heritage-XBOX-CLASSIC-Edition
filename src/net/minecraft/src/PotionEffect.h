#pragma once

#include "java/String.h"

class EntityLiving;
class NBTTagCompound;

// net.minecraft.src.PotionEffect
class PotionEffect
{
public:
	PotionEffect(int id, int duration, int amplifier);
	PotionEffect(int id, int duration, int amplifier, bool ambient);
	PotionEffect(const PotionEffect &other);

	void combine(const PotionEffect &other);
	int getPotionID() const;
	int getDuration() const;
	int getAmplifier() const;
	bool isSplashPotionEffect() const;
	void setSplashPotion(bool splash);
	bool getIsAmbient() const;
	bool getIsPotionDurationMax() const;
	void setPotionDurationMax(bool value);
	NBTTagCompound *writeCustomPotionEffectToNBT(NBTTagCompound *tag) const;
	static PotionEffect *readCustomPotionEffectFromNBT(NBTTagCompound *tag);
	bool onUpdate(EntityLiving *entity);
	void performEffect(EntityLiving *entity);
	jstring getEffectName() const;
	bool equals(const PotionEffect &other) const;

private:
	int deincrementDuration();

	int potionID;
	int duration;
	int amplifier;
	bool splashPotion = false;
	bool ambient = false;
	bool potionDurationMax = false;
};
