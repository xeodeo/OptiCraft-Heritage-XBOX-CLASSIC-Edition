#include "GuiContainer.h"
#include "mods/ModManager.h"
#include "java/String.h"
#include "EntityPlayerSP.h"
#include "World.h"
#include "Container.h"
#include "Slot.h"
#include "RenderItem.h"
#include "RenderHelper.h"
#include "InventoryPlayer.h"
#include "StringTranslate.h"
#include "ItemStack.h"
#include "EnumRarity.h"
#include "FontRenderer.h"
#include "RenderEngine.h"
#include "OpenGlHelper.h"
#include "Minecraft.h"
#include "GameSettings.h"
#include "KeyBinding.h"
#include "legacy/LegacySlotCursor.h"
#include "PlayerController.h"
#include "platform/RenderAPI.h"
#include "platform/PlatformConfig.h"
#include "pc/lwjgl/Keyboard.h"
#include "pc/lwjgl/Mouse.h"
#include <algorithm>

#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM) || defined(XBOX_PLATFORM)
#include "ContainerSlotNavigator.h"
#include "platform/Input.h"
#include "SlotCrafting.h"
#include "SlotFurnace.h"
#include "SlotMerchantResult.h"
#include "platform/ConsoleInputClock.h"
#if defined(XBOX_PLATFORM)
#include "xbox/input/XboxPad.h"
#endif
#endif

RenderItem *GuiContainer::itemRenderer = new RenderItem();

GuiContainer::GuiContainer(Container *container, bool ownsContainer, EntityPlayer *player)
	: xSize(176)
	, ySize(166)
	, guiLeft(0)
	, guiTop(0)
	, m_containerPlayer(player)
	, inventorySlots(container)
	, ownsInventorySlots(ownsContainer)
{
	if (player != nullptr)
	{
		Minecraft *m = Minecraft::getMinecraft();
		if (m != nullptr && player == m->thePlayer2)
			m_ownerPlayerIndex = 1;
		else
			m_ownerPlayerIndex = 0;
	}
}

EntityPlayer *GuiContainer::getContainerPlayer() const
{
	if (m_containerPlayer != nullptr)
		return m_containerPlayer;
	return mc ? mc->thePlayer : nullptr;
}

void GuiContainer::setContainerPlayer(EntityPlayer *player)
{
	m_containerPlayer = player;
	if (player != nullptr)
	{
		Minecraft *m = Minecraft::getMinecraft();
		if (m != nullptr && player == m->thePlayer2)
			m_ownerPlayerIndex = 1;
		else
			m_ownerPlayerIndex = 0;
	}
}

int GuiContainer::getOwnerPlayerIndex() const
{
	if (m_ownerPlayerIndex >= 0)
		return m_ownerPlayerIndex;
	if (m_containerPlayer != nullptr && mc != nullptr && m_containerPlayer == mc->thePlayer2)
		return 1;
	if (mc != nullptr && mc->isScreenOwnedByPlayer2())
		return 1;
	return 0;
}

GuiContainer::~GuiContainer()
{
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM) || defined(XBOX_PLATFORM)
	// onGuiClosed() is the normal exit, but a container screen can also be
	// destroyed while it is still the one the navigator points at (world change,
	// shutdown), and that pointer is read from the pad poll rather than from a
	// tick -- so it has to stop being live here too.
	ContainerSlotNavigator::instance(getOwnerPlayerIndex()).notifyClosed(this);
#endif

	if (ownsInventorySlots)
	{
		delete inventorySlots;
		inventorySlots = nullptr;
	}
}

void GuiContainer::initGui()
{
	GuiScreen::initGui();
	guiLeft = (width - xSize) / 2;
	guiTop = (height - ySize) / 2;
	EntityPlayer *p = getContainerPlayer();
	if (p != nullptr)
		p->craftingInventory = inventorySlots;
	holdSawRelease = false;
	holdArmed = false;
	holdStopped = false;
	holdSlot = nullptr;
	holdInitialPickedUp = false;
	field_94077_p.clear();
	field_94076_q = false;
	field_94067_D = -1;
	field_94069_F = 0;
	field_94072_H = nullptr;
	field_94073_N = false;
}

void GuiContainer::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
	drawDefaultBackground();
	int_t guiX = guiLeft;
	int_t guiY = guiTop;

#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
	ContainerSlotNavigator &navigator = ContainerSlotNavigator::instance(getOwnerPlayerIndex());
	Slot *controllerSlot = nullptr;
	if (mc->gameSettings != nullptr && mc->gameSettings->legacyUI)
	{
		ContainerSlotNavigator::Layout navigation;
		navigation.guiLeft = guiX;
		navigation.guiTop = guiY;
		navigation.screenWidth = width;
		navigation.screenHeight = height;
		navigation.displayWidth = mc->displayWidth;
		navigation.displayHeight = mc->displayHeight;
		navigator.notifyOpen(this, navigation);
		navigator.notePointerSlot(platformMenuPointerActive() ? getSlotAtPosition(mouseX, mouseY) : nullptr);
		if (navigator.controllerSelectionActive())
			controllerSlot = navigator.selectedSlot();
	}
	else
	{
		navigator.notifyClosed(this);
	}

	Slot *selectedSlot = controllerSlot;
	Slot *activeSlot = controllerSlot != nullptr ? controllerSlot : getSlotAtPosition(mouseX, mouseY);
	EntityPlayer *cp = getContainerPlayer();
	InventoryPlayer *inv = (cp != nullptr) ? cp->inventory : mc->thePlayer->inventory;
#if defined(XBOX_PLATFORM)
	if (!field_94076_q && field_94067_D < 0)
		tickHoldRepeat(activeSlot);
#endif
	if (mc == nullptr || !mc->isSplitScreenActive())
	{
		const bool navPrimary = navigator.consumePrimaryClick();
		const bool navSecondary = navigator.consumeSecondaryClick();
		const bool navQuick = navigator.consumeQuickMove();
		const bool javaUiQuick = (mc->gameSettings == nullptr || !mc->gameSettings->legacyUI) &&
			(platformTextInputSnapshot(getOwnerPlayerIndex()).pressed & PLATFORM_TEXT_SHIFT) != 0;
		if ((navQuick || javaUiQuick) && activeSlot != nullptr)
		{
			handleMouseClick(activeSlot, activeSlot->slotNumber, 0, true);
		}
		if (selectedSlot != nullptr && (navPrimary || navSecondary))
		{
			const int_t btn = navPrimary ? 0 : 1;
			ItemStack *carried = inv != nullptr ? inv->getItemStack() : nullptr;
			if (carried != nullptr && Container::func_94527_a(selectedSlot, carried, true) &&
			    selectedSlot->isItemValid(carried) && inventorySlots->canDragIntoSlot(selectedSlot))
			{
				field_94077_p.clear();
				field_94077_p.insert(selectedSlot);
				field_94067_D = btn;
				field_94076_q = true;
				updateDragSplitting();
			}
			else
			{
				handleMouseClick(selectedSlot, selectedSlot->slotNumber, btn, false);
			}
		}
	}
	else
	{
		navigator.consumePrimaryClick();
		navigator.consumeSecondaryClick();
		navigator.consumeQuickMove();
	}

	if (field_94067_D == 0 || field_94067_D == 1)
	{
		ItemStack *carried = inv != nullptr ? inv->getItemStack() : nullptr;
		if (carried == nullptr)
		{
			field_94076_q = false;
			field_94077_p.clear();
			field_94067_D = -1;
		}
		else
		{
			if (activeSlot != nullptr &&
			    Container::func_94527_a(activeSlot, carried, true) &&
			    activeSlot->isItemValid(carried) &&
			    carried->stackSize > static_cast<int_t>(field_94077_p.size()) &&
			    inventorySlots->canDragIntoSlot(activeSlot))
			{
				field_94077_p.insert(activeSlot);
				field_94076_q = true;
				updateDragSplitting();
			}
			const PlatformTextInputSnapshot padSnap = platformTextInputSnapshot(getOwnerPlayerIndex());
			const unsigned int requiredPadMask = (field_94067_D == 0) ? PLATFORM_TEXT_TYPE : PLATFORM_TEXT_BACK;
			const bool padHeld = (padSnap.held & requiredPadMask) != 0;
			const bool mouseHeld = (getOwnerPlayerIndex() == 0) && lwjgl::Mouse::isButtonDown(field_94067_D);
			if (!padHeld && !mouseHeld)
			{
				finishDragSplitting(field_94067_D);
			}
		}
	}
#endif

	drawGuiContainerBackgroundLayer(partialTick);

	// Match Java 1.5.2 GuiContainer.drawScreen(): draw controlList buttons
	// (e.g. GuiBeacon power/confirm/cancel buttons, GuiMerchant arrows) after
	// the background and BEFORE slots/items/foreground so button hover states
	// are fresh and foreground tooltips render on top of the buttons.
	renderDisable(RenderCapability::RescaleNormal);
	RenderHelper::disableStandardItemLighting();
	renderDisable(RenderCapability::Lighting);
	renderDisable(RenderCapability::DepthTest);
	GuiScreen::drawScreen(mouseX, mouseY, partialTick);

#if PLATFORM_GUI_FORCE_DEPTH_DISABLED
	// The console GUI pass starts with depth disabled, but 3D item icons need
	// their own faces depth-sorted before the 2D overlay/foreground pass.
	renderEnable(RenderCapability::DepthTest);
	renderDepthMask(true);
	renderDepthFunc(RenderCompare::LessEqual);
#endif

	RenderHelper::enableGUIStandardItemLighting();

	renderPushMatrix();
	renderTranslate((float_t)guiX, (float_t)guiY, 0.0f);
	renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	renderEnable(RenderCapability::RescaleNormal);
	OpenGlHelper::setLightmapTextureCoords(OpenGlHelper::lightmapTexUnit, 240.0f, 240.0f);
	renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	Slot *hoveredSlot = nullptr;
	for (int_t i = 0; i < (int_t)inventorySlots->slots.size(); i++)
	{
		Slot *slot = inventorySlots->slots[i];
		drawSlotInventory(slot);
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
		const bool selectedByController = slot == controllerSlot;
		const bool selectedByPointer = controllerSlot == nullptr && getIsMouseOverSlot(slot, mouseX, mouseY);
		if (selectedByController || selectedByPointer)
#else
		if (getIsMouseOverSlot(slot, mouseX, mouseY))
#endif
		{
			hoveredSlot = slot;
			renderDisable(RenderCapability::Lighting);
			renderDisable(RenderCapability::DepthTest);
			int_t sx = slot->xDisplayPosition;
			int_t sy = slot->yDisplayPosition;
			drawGradientRect(sx, sy, sx + 16, sy + 16, 0x80ffffff, 0x80ffffff);
			renderEnable(RenderCapability::Lighting);
			renderEnable(RenderCapability::DepthTest);
		}
	}

	if (inv->getItemStack() != nullptr)
	{
		int_t carriedX = mouseX - guiX - 8;
		int_t carriedY = mouseY - guiY - 8;
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
		if (controllerSlot != nullptr)
		{
			carriedX = controllerSlot->xDisplayPosition;
			carriedY = controllerSlot->yDisplayPosition;
		}
#endif
		ItemStack *carriedStack = inv->getItemStack();
		const int_t savedCount = carriedStack->stackSize;
		std::string customText;
		if (field_94076_q && field_94077_p.size() > 1)
		{
			carriedStack->stackSize = field_94069_F;
			if (carriedStack->stackSize == 0)
				customText = std::string("\xc2\xa7" "e0");
		}
		renderTranslate(0.0f, 0.0f, 32.0f);
		itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, carriedStack, carriedX, carriedY);
		itemRenderer->renderItemOverlayIntoGUI(fontRenderer, mc->renderEngine, carriedStack, carriedX, carriedY, customText);
		carriedStack->stackSize = savedCount;
	}

#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
	if (controllerSlot != nullptr)
	{
		renderDisable(RenderCapability::Lighting);
		renderDisable(RenderCapability::DepthTest);
		legacyDrawSlotCursor(mc, controllerSlot, zLevel + 128.0f);
		renderEnable(RenderCapability::DepthTest);
		renderEnable(RenderCapability::Lighting);
	}
#endif

	renderDisable(RenderCapability::RescaleNormal);
	RenderHelper::disableStandardItemLighting();
	renderDisable(RenderCapability::Lighting);
	renderDisable(RenderCapability::DepthTest);
	drawGuiContainerForegroundLayer();

	if (inv->getItemStack() == nullptr && hoveredSlot != nullptr && hoveredSlot->getHasStack())
	{
		ItemStack *hoveredStack = hoveredSlot->getStack();
		std::vector<std::string> information = hoveredStack->getItemNameandInformation();
		if (!information.empty())
		{
			int_t tooltipWidth = 0;
			for (const std::string& line : information)
				tooltipWidth = std::max(tooltipWidth, fontRenderer->getStringWidth(line));

			int_t tooltipX = mouseX - guiX + 12;
			int_t tooltipY = mouseY - guiY - 12;
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
			if (controllerSlot != nullptr)
			{
				tooltipX = controllerSlot->xDisplayPosition + 22;
				tooltipY = controllerSlot->yDisplayPosition - 10;
			}
			if (guiY + tooltipY + 24 > height)
				tooltipY = height - guiY - 24;
			if (guiY + tooltipY < 0)
				tooltipY = -guiY;
#endif
			int_t tooltipHeight = 8;
			if (information.size() > 1)
				tooltipHeight += 2 + (static_cast<int_t>(information.size()) - 1) * 10;

			const int_t background = static_cast<int_t>(0xf0100010u);
			drawGradientRect(tooltipX - 3, tooltipY - 4, tooltipX + tooltipWidth + 3, tooltipY - 3, background, background);
			drawGradientRect(tooltipX - 3, tooltipY + tooltipHeight + 3, tooltipX + tooltipWidth + 3, tooltipY + tooltipHeight + 4, background, background);
			drawGradientRect(tooltipX - 3, tooltipY - 3, tooltipX + tooltipWidth + 3, tooltipY + tooltipHeight + 3, background, background);
			drawGradientRect(tooltipX - 4, tooltipY - 3, tooltipX - 3, tooltipY + tooltipHeight + 3, background, background);
			drawGradientRect(tooltipX + tooltipWidth + 3, tooltipY - 3, tooltipX + tooltipWidth + 4, tooltipY + tooltipHeight + 3, background, background);

			const int_t borderTop = 0x505000ff;
			const int_t borderBottom = (borderTop & 0x00fefefe) >> 1 | (borderTop & static_cast<int_t>(0xff000000u));
			drawGradientRect(tooltipX - 3, tooltipY - 2, tooltipX - 2, tooltipY + tooltipHeight + 2, borderTop, borderBottom);
			drawGradientRect(tooltipX + tooltipWidth + 2, tooltipY - 2, tooltipX + tooltipWidth + 3, tooltipY + tooltipHeight + 2, borderTop, borderBottom);
			drawGradientRect(tooltipX - 3, tooltipY - 3, tooltipX + tooltipWidth + 3, tooltipY - 2, borderTop, borderTop);
			drawGradientRect(tooltipX - 3, tooltipY + tooltipHeight + 2, tooltipX + tooltipWidth + 3, tooltipY + tooltipHeight + 3, borderBottom, borderBottom);

			const char *hex = "0123456789abcdef";
			for (std::size_t i = 0; i < information.size(); ++i)
			{
				std::string line = information[i];
				if (i == 0)
				{
					int_t color = hoveredStack->getRarity().nameColor & 15;
					line = std::string(u8"§") + hex[color] + line;
				}
				else
				{
					line = std::string(u8"§7") + line;
				}

				fontRenderer->drawStringWithShadow(line, tooltipX, tooltipY, -1);
				if (i == 0)
					tooltipY += 2;
				tooltipY += 10;
			}
		}
	}

	renderPopMatrix();
	ModManager::getInstance().onDrawContainer(this, mouseX, mouseY);

	renderEnable(RenderCapability::Lighting);
	renderEnable(RenderCapability::DepthTest);
}

void GuiContainer::drawGuiContainerForegroundLayer()
{
}

void GuiContainer::drawSlotInventory(Slot *slot)
{
	int_t x = slot->xDisplayPosition;
	int_t y = slot->yDisplayPosition;
	ItemStack *stack = slot->getStack();
	bool isDragSlot = false;
	std::string customCountText;
	EntityPlayer *cp = getContainerPlayer();
	InventoryPlayer *inv = (cp != nullptr) ? cp->inventory : (mc != nullptr && mc->thePlayer != nullptr ? mc->thePlayer->inventory : nullptr);
	ItemStack *carried = inv != nullptr ? inv->getItemStack() : nullptr;
	const int_t savedCarriedCount = carried != nullptr ? carried->stackSize : 0;

	if (field_94076_q && field_94077_p.size() > 1 && carried != nullptr && field_94077_p.count(slot) > 0)
	{
		stack = carried;
		isDragSlot = true;
		const int_t baseSize = slot->getHasStack() ? slot->getStack()->stackSize : 0;
		Container::func_94525_a(field_94077_p, field_94067_D, stack, baseSize);
		if (stack->stackSize > stack->getMaxStackSize())
		{
			customCountText = std::string("\xc2\xa7" "e") + std::to_string(stack->getMaxStackSize());
			stack->stackSize = stack->getMaxStackSize();
		}
		if (stack->stackSize > slot->getSlotStackLimit())
		{
			customCountText = std::string("\xc2\xa7" "e") + std::to_string(slot->getSlotStackLimit());
			stack->stackSize = slot->getSlotStackLimit();
		}
	}

	if (stack == nullptr)
	{
		int_t icon = slot->getBackgroundIconIndex();
		if (icon >= 0)
		{
			renderDisable(RenderCapability::Lighting);
			mc->renderEngine->bindTexture(mc->renderEngine->getTexture("/gui/items.png"));
			drawTexturedModalRect(x, y, (icon % 16) * 16, (icon / 16) * 16, 16, 16);
			renderEnable(RenderCapability::Lighting);
			return;
		}
	}
	if (isDragSlot)
	{
		renderDisable(RenderCapability::Lighting);
		renderDisable(RenderCapability::DepthTest);
		drawRect(x, y, x + 16, y + 16, -2130706433);
		renderEnable(RenderCapability::Lighting);
		renderEnable(RenderCapability::DepthTest);
	}
	itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, stack, x, y);
	itemRenderer->renderItemOverlayIntoGUI(fontRenderer, mc->renderEngine, stack, x, y, customCountText);
	if (isDragSlot && carried != nullptr)
	{
		carried->stackSize = savedCarriedCount;
	}
}

void GuiContainer::updateDragSplitting()
{
	EntityPlayer *cp = getContainerPlayer();
	InventoryPlayer *inv = (cp != nullptr) ? cp->inventory : (mc != nullptr && mc->thePlayer != nullptr ? mc->thePlayer->inventory : nullptr);
	ItemStack *carried = inv != nullptr ? inv->getItemStack() : nullptr;
	if (carried == nullptr || !field_94076_q)
		return;

	const int_t savedCarriedCount = carried->stackSize;
	field_94069_F = savedCarriedCount;
	for (Slot *slot : field_94077_p)
	{
		if (slot == nullptr)
			continue;
		carried->stackSize = savedCarriedCount;
		const int_t baseCount = slot->getHasStack() ? slot->getStack()->stackSize : 0;
		Container::func_94525_a(field_94077_p, field_94067_D, carried, baseCount);
		if (carried->stackSize > carried->getMaxStackSize())
			carried->stackSize = carried->getMaxStackSize();
		if (carried->stackSize > slot->getSlotStackLimit())
			carried->stackSize = slot->getSlotStackLimit();
		field_94069_F -= carried->stackSize - baseCount;
	}
	carried->stackSize = savedCarriedCount;
}

void GuiContainer::finishDragSplitting(int_t button)
{
	if (field_94067_D < 0)
		return;

	const int_t dragLimit = field_94067_D;
	const bool wasDragging = field_94076_q && field_94077_p.size() > 1;
	Slot *singleSlot = (!field_94077_p.empty()) ? *field_94077_p.begin() : nullptr;
	std::set<Slot *> dragSlots = field_94077_p;

	field_94076_q = false;
	field_94077_p.clear();
	field_94067_D = -1;

	if (wasDragging)
	{
		handleMouseClickMode(nullptr, -999, Container::func_94534_d(0, dragLimit), 5);
		for (Slot *s : dragSlots)
		{
			if (s != nullptr)
				handleMouseClickMode(s, s->slotNumber, Container::func_94534_d(1, dragLimit), 5);
		}
		handleMouseClickMode(nullptr, -999, Container::func_94534_d(2, dragLimit), 5);
	}
	else if (singleSlot != nullptr)
	{
		const bool shift = (lwjgl::Keyboard::isKeyDown(42) || lwjgl::Keyboard::isKeyDown(54));
		if (field_94073_N && !shift && dragLimit == 0)
		{
			handleMouseClickMode(singleSlot, singleSlot->slotNumber, 0, 6);
		}
		else
		{
			handleMouseClick(singleSlot, singleSlot->slotNumber, button >= 0 ? button : dragLimit, shift);
		}
	}
	field_94073_N = false;
}

Slot *GuiContainer::getSlotAtPosition(int_t mouseX, int_t mouseY)
{
	for (int_t i = 0; i < (int_t)inventorySlots->slots.size(); i++)
	{
		Slot *slot = inventorySlots->slots[i];
		if (getIsMouseOverSlot(slot, mouseX, mouseY))
			return slot;
	}
	return nullptr;
}

Slot *GuiContainer::getControllerNavigationTarget(Slot *, int_t, int_t)
{
	return nullptr;
}

bool GuiContainer::getIsMouseOverSlot(Slot *slot, int_t mouseX, int_t mouseY)
{
	int_t guiX = guiLeft;
	int_t guiY = guiTop;
	int_t rx = mouseX - guiX;
	int_t ry = mouseY - guiY;
	return rx >= slot->xDisplayPosition - 1 && rx < slot->xDisplayPosition + 16 + 1
	    && ry >= slot->yDisplayPosition - 1 && ry < slot->yDisplayPosition + 16 + 1;
}

void GuiContainer::mouseClicked(int_t x, int_t y, int_t button)
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
	ContainerSlotNavigator &navigator = ContainerSlotNavigator::instance(getOwnerPlayerIndex());
#if PLATFORM_XBOX
	// A is also turned into a mouse click at the cursor, which the D-pad keeps
	// on the selected slot: with the D-pad in charge that second click put the
	// item straight back. The stick-driven pointer is not a hardware pointer.
	const bool pointerActive = false;
#else
	const bool pointerActive = platformMenuPointerActive();
#endif
	// Allow mod container panels outside the vanilla inventory bounds (e.g. TooManyItems)
	// to receive controller A/X clicks even when Legacy slot navigation is active.
	if ((x < guiLeft || x >= guiLeft + xSize || y < guiTop || y >= guiTop + ySize) &&
	    ModManager::getInstance().onContainerMouseClicked(this, x, y, button))
		return;

	// Console confirm buttons are exposed both as controller input and mouse
	// clicks. When D-pad selection owns the inventory (and the user is not pointing
	// with a hardware pointer like the Wii remote IR sensor), ignore the synthesized
	// mouse edge so the selected slot is activated exactly once.
	if (!pointerActive && !mc->isSplitScreenActive() && mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI
	    && navigator.controllerSelectionActive() && (button == 0 || button == 1))
		return;
	Slot *controllerSlot = nullptr;
	if (navigator.controllerSelectionActive() && !pointerActive)
		controllerSlot = navigator.selectedSlot();
	navigator.notePointerActivity();
#endif
	GuiScreen::mouseClicked(x, y, button);
	if (ModManager::getInstance().onContainerMouseClicked(this, x, y, button))
		return;

	if (button == 0 || button == 1)
	{
		Slot *slot = nullptr;
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
		if (controllerSlot != nullptr)
			slot = controllerSlot;
#endif
		if (slot == nullptr)
			slot = getSlotAtPosition(x, y);

		int_t guiX = guiLeft;
		int_t guiY = guiTop;
		bool outsideGui = x < guiX || y < guiY || x >= guiX + xSize || y >= guiY + ySize;
		int_t slotId = -1;
		if (slot != nullptr) slotId = slot->slotNumber;
		if (outsideGui && slot == nullptr) slotId = -999;
		if (slotId != -1)
		{
			const long_t now = static_cast<long_t>(consoleInputNowMs());
			field_94073_N = (field_94072_H == slot && (now - field_94070_G) < 250 && field_94071_C == button);
			field_94070_G = now;
			field_94072_H = slot;
			field_94071_C = button;

			bool shift = slotId != -999 && (lwjgl::Keyboard::isKeyDown(42) || lwjgl::Keyboard::isKeyDown(54));
			EntityPlayer *cp = getContainerPlayer();
			InventoryPlayer *inv = (cp != nullptr) ? cp->inventory : (mc != nullptr && mc->thePlayer != nullptr ? mc->thePlayer->inventory : nullptr);
			ItemStack *carried = inv != nullptr ? inv->getItemStack() : nullptr;

			if (!shift && slot != nullptr && carried != nullptr &&
			    Container::func_94527_a(slot, carried, true) &&
			    slot->isItemValid(carried) &&
			    inventorySlots->canDragIntoSlot(slot))
			{
				field_94077_p.clear();
				field_94077_p.insert(slot);
				field_94067_D = button;
				field_94076_q = true;
				updateDragSplitting();
			}
			else
			{
				handleMouseClick(slot, slotId, button, shift);
			}
		}
	}
}


void GuiContainer::handleMouseClick(Slot *slot, int_t slotId, int_t button, bool shift)
{
	if (slot != nullptr)
		slotId = slot->slotNumber;
	EntityPlayer *p = getContainerPlayer();
	if (p == nullptr && mc != nullptr) p = mc->thePlayer;
	const bool cursorEmptyBefore = (p != nullptr && p->inventory != nullptr && p->inventory->getItemStack() == nullptr);
	const bool slotHasStackBefore = (slot != nullptr && slot->getHasStack() && slot->getStack() != nullptr && slot->getStack()->stackSize > 0);
	const bool isResultSlot = (dynamic_cast<SlotCrafting *>(slot) != nullptr || dynamic_cast<SlotFurnace *>(slot) != nullptr ||
	                           dynamic_cast<SlotMerchantResult *>(slot) != nullptr);

	delete mc->playerController->windowClick(inventorySlots->windowId, slotId, button, shift, p);

	if (button == 0 && !shift && cursorEmptyBefore && slotHasStackBefore && !isResultSlot)
	{
		holdInitialPickedUp = true;
	}
}

void GuiContainer::handleMouseClickMode(Slot *slot, int_t slotId, int_t button, int_t mode)
{
	if (mode == 0 || mode == 1)
	{
		handleMouseClick(slot, slotId, button, mode == 1);
		return;
	}
	if (slot != nullptr)
		slotId = slot->slotNumber;
	EntityPlayer *p = getContainerPlayer();
	if (p == nullptr && mc != nullptr) p = mc->thePlayer;
	delete mc->playerController->windowClick(inventorySlots->windowId, slotId, button, mode, p);
}

void GuiContainer::mouseMovedOrUp(int_t x, int_t y, int_t button)
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
	// Button release is not pointer motion. Only actual movement should take
	// authority away from the controller-selected slot. Wheel and click events
	// have dx=0 and dy=0 and must not clear the controller slot selection.
	if (button < 0 && (lwjgl::Mouse::getEventDX() != 0 || lwjgl::Mouse::getEventDY() != 0))
		ContainerSlotNavigator::instance(getOwnerPlayerIndex()).notePointerActivity();
#endif
	if (button >= 0 && field_94067_D == button)
	{
		finishDragSplitting(button);
	}
	(void)x;
	(void)y;
}

void GuiContainer::keyTyped(char_t c, int_t key)
{
	if (ModManager::getInstance().onContainerKeyTyped(c, key))
		return;

	if (key == 1 || key == mc->gameSettings->keyBindInventory->keyCode)
	{
		EntityPlayer *p = getContainerPlayer();
		if (p != nullptr)
			p->closeScreen();
		else
			mc->thePlayer->closeScreen();
	}
}

void GuiContainer::onGuiClosed()
{
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM) || defined(XBOX_PLATFORM)
	// Before the thePlayer guard below: the navigator has to be released even on
	// the paths that return early here.
	ContainerSlotNavigator::instance(getOwnerPlayerIndex()).notifyClosed(this);
#endif

	EntityPlayer *p = getContainerPlayer();
	if (p == nullptr) p = mc->thePlayer;
	if (p == nullptr) return;
	inventorySlots->onCraftGuiClosed(p);
	mc->playerController->closeWindow(inventorySlots->windowId, p);
}

bool GuiContainer::doesGuiPauseGame()
{
	return false;
}

void GuiContainer::updateScreen()
{
	GuiScreen::updateScreen();
	EntityPlayer *p = getContainerPlayer();
	if (p == nullptr) p = mc->thePlayer;
	if (p != nullptr && (!p->isEntityAlive() || p->isDead))
	{
		p->closeScreen();
	}
}

void GuiContainer::tickHoldRepeat(Slot *target)
{
#if defined(XBOX_PLATFORM)
	const bool down = (XboxPad::playerSnapshot(getOwnerPlayerIndex() == 1 ? 1 : 0).held & XBOX_PAD_A) != 0;
	if (!down)
	{
		holdSawRelease = true;
		holdArmed = false;
		holdStopped = false;
		holdSlot = nullptr;
		holdInitialPickedUp = false;
		return;
	}
	if (!holdSawRelease || holdStopped)
		return;
	const int now = consoleInputNowMs();
	if (!holdArmed)
	{
		// The press itself already clicked once through the normal path; repeats start after a pause.
		holdArmed = true;
		holdSlot = target;
		holdNextMs = now + 350;
		holdIntervalMs = 80;
		return;
	}
	if (target == nullptr || target != holdSlot)
	{
		holdStopped = true;   // moved to another slot: wait for A to be released
		return;
	}
	if (now < holdNextMs)
		return;
	if (holdInitialPickedUp)
	{
		holdInitialPickedUp = false;
		returnInitialStack(target);
	}
	if (!holdRepeatTakeOne(target))
	{
		holdStopped = true;
		return;
	}
	holdIntervalMs = std::max(40, holdIntervalMs - 8);
	holdNextMs = now + holdIntervalMs;
#else
	(void)target;
#endif
}

void GuiContainer::returnInitialStack(Slot *slot)
{
	EntityPlayer *p = getContainerPlayer();
	if (p == nullptr && mc != nullptr) p = mc->thePlayer;
	if (slot == nullptr || p == nullptr || p->inventory == nullptr)
		return;
	if (slot->getHasStack())
		return;
	ItemStack *held = p->inventory->getItemStack();
	if (held == nullptr)
		return;

	// In Java, clicking the empty slot again returns the held stack back into the slot
	handleMouseClick(slot, slot->slotNumber, 0, false);
}

bool GuiContainer::holdRepeatTakeOne(Slot *slot)
{
	EntityPlayer *p = getContainerPlayer();
	if (p == nullptr && mc != nullptr) p = mc->thePlayer;
	if (slot == nullptr || p == nullptr || p->inventory == nullptr || inventorySlots == nullptr)
		return false;
	// Moving single items by hand would desync a server; result slots have their own pickup rules.
	if (mc == nullptr || mc->theWorld == nullptr || mc->theWorld->multiplayerWorld)
		return false;
	if (dynamic_cast<SlotCrafting *>(slot) != nullptr || dynamic_cast<SlotFurnace *>(slot) != nullptr ||
	    dynamic_cast<SlotMerchantResult *>(slot) != nullptr)
		return false;
	ItemStack *source = slot->getStack();
	if (source == nullptr || source->stackSize <= 0 || !slot->canTakeStack(p))
		return false;
	ItemStack *held = p->inventory->getItemStack();
	if (held != nullptr && (!held->isItemEqual(source) || !ItemStack::areItemStackTagsEqual(held, source) ||
	    held->stackSize >= held->getMaxStackSize()))
		return false;
	ItemStack *one = slot->decrStackSize(1);
	if (one == nullptr)
		return false;
	if (held != nullptr)
	{
		held->stackSize += one->stackSize;
		delete one;
	}
	else
	{
		p->inventory->setItemStack(one);
	}
	slot->onSlotChanged();
	inventorySlots->detectAndSendChanges();
	return true;
}
