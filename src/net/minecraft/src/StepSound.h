#pragma once

#include "java/Type.h"
#include "java/String.h"

// net.minecraft.src.StepSound
class StepSound
{
public:
	StepSound(const jstring &s, float f, float f1);
	virtual ~StepSound() = default;

	virtual float getVolume();
	virtual float getPitch();
	virtual jstring getStepSound();
	virtual jstring getBreakSound();
	virtual jstring getPlaceSound();

	// Compatibility alias for older OptiCraft callers.
	virtual jstring stepSoundDir();

	const jstring motionZ;
	const float motionY;
	const float rotationYaw;
};

class StepSoundAnvil : public StepSound
{
public:
	StepSoundAnvil(const jstring &s, float f, float f1);
	jstring getBreakSound() override;
	jstring getPlaceSound() override;
};
