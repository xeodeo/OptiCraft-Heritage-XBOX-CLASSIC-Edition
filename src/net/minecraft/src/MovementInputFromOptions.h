#pragma once

#include "MovementInput.h"

class GameSettings;

// net.minecraft.src.MovementInputFromOptions
class MovementInputFromOptions : public MovementInput
{
public:
	MovementInputFromOptions(GameSettings *gamesettings, int port = 0);

	void checkKeyForMovementInput(int i, bool flag) override;
	void resetKeyState() override;
	void updatePlayerMoveState(EntityPlayer *entityplayer) override;

	int getPadPort() const { return padPort; }
	void setPadPort(int port) { padPort = port; }
	// Mounts call this so a sneak toggled on before mounting is not a dismount press.
	void clearSneakToggle() { sneakToggled = false; }

private:
	GameSettings *gameSettings;
	int padPort;
	// Toggle-sneak state; a new MovementInputFromOptions on respawn/world change resets it.
	bool sneakToggled = false;
	bool sneakHoldOnly = false; // flying or riding this tick
	bool sneakKeyWasDown = false;
	bool applySneak(bool rawDown, bool inScreen);
};
