#pragma once

#include "GuiScreen.h"
#include <set>

class Container;
class Slot;
class RenderItem;
class InventoryPlayer;
class EntityPlayer;
class ItemStack;

// net.minecraft.src.GuiContainer
class GuiContainer : public GuiScreen
{
	friend class TooManyItems;
public:
	GuiContainer(Container *container, bool ownsContainer = false, EntityPlayer *player = nullptr);
	virtual ~GuiContainer();

	EntityPlayer *getContainerPlayer() const;
	void setContainerPlayer(EntityPlayer *player);
	int getOwnerPlayerIndex() const override;

	void initGui() override;
	void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;

protected:
	virtual void drawGuiContainerForegroundLayer();
	virtual void drawGuiContainerBackgroundLayer(float_t partialTick) = 0;

private:
	void drawSlotInventory(Slot *slot);

public:
	// Public for the console D-pad slot navigation (ContainerSlotNavigator),
	// which has to ask which slot the simulated cursor is over before it can
	// step to the next one. Sharing the click path's own hit test is what keeps
	// the two from disagreeing about where a slot ends.
	Slot *getSlotAtPosition(int_t mouseX, int_t mouseY);
	// A container can keep controller navigation inside a specialized slot grid.
	// Returning nullptr uses the normal geometric navigation.
	virtual Slot *getControllerNavigationTarget(Slot *selected, int_t dirX, int_t dirY);
	virtual void handleMouseClick(Slot *slot, int_t slotId, int_t button, bool shift);
	virtual void handleMouseClickMode(Slot *slot, int_t slotId, int_t button, int_t mode);
	// Holding A on a slot: one more item per repeat (creative adds one, survival takes one).
	// Returns false when nothing more can be taken, which stops the repeat until A is released.
	virtual bool holdRepeatTakeOne(Slot *slot);
	virtual void returnInitialStack(Slot *slot);

protected:
	bool holdInitialPickedUp = false;
	std::set<Slot *> field_94077_p;
	bool field_94076_q = false;

private:
	bool getIsMouseOverSlot(Slot *slot, int_t mouseX, int_t mouseY);
	void updateDragSplitting();
	void finishDragSplitting(int_t button);
	void tickHoldRepeat(Slot *target);
	bool holdSawRelease = false;   // a press that opened this screen does not count
	bool holdArmed = false;
	bool holdStopped = false;
	Slot *holdSlot = nullptr;       // compared only, never dereferenced
	int holdNextMs = 0;
	int holdIntervalMs = 80;
	int_t field_94071_C = 0;
	int_t field_94067_D = -1;
	bool field_94068_E = false;
	int_t field_94069_F = 0;
	long_t field_94070_G = 0;
	Slot *field_94072_H = nullptr;
	bool field_94073_N = false;

protected:
	void mouseClicked(int_t x, int_t y, int_t button) override;
	void mouseMovedOrUp(int_t x, int_t y, int_t button) override;
	void keyTyped(char_t c, int_t key) override;

public:
	void onGuiClosed() override;
	bool doesGuiPauseGame() override;
	void updateScreen() override;
	bool allowsPlatformPointerInput() const override { return true; }

protected:
	static RenderItem *itemRenderer;
	int_t xSize;
	int_t ySize;
	int_t guiLeft;
	int_t guiTop;
	EntityPlayer *m_containerPlayer;

public:
	Container *inventorySlots;

private:
	bool ownsInventorySlots;
};
