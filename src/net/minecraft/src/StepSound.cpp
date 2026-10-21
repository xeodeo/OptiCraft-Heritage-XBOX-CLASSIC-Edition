#include "StepSound.h"

StepSound::StepSound(const jstring &s, float f, float f1) :
	motionZ(s),
	motionY(f),
	rotationYaw(f1)
{
}

float StepSound::getVolume()
{
	return motionY;
}

float StepSound::getPitch()
{
	return rotationYaw;
}

jstring StepSound::getStepSound()
{
	return "step." + motionZ;
}

jstring StepSound::getBreakSound()
{
	return "step." + motionZ;
}

jstring StepSound::stepSoundDir()
{
	return getStepSound();
}

jstring StepSound::getPlaceSound()
{
	return getStepSound();
}

StepSoundAnvil::StepSoundAnvil(const jstring &s, float f, float f1) :
	StepSound(s, f, f1)
{
}

jstring StepSoundAnvil::getBreakSound()
{
	return "dig.stone";
}

jstring StepSoundAnvil::getPlaceSound()
{
	return "random.anvil_land";
}
