#pragma once

#include "GuiScreen.h"
#include <string>

class Minecraft;
class NetClientHandler;
class ThreadConnectToServer;

// net.minecraft.src.GuiConnecting
class GuiConnecting : public GuiScreen
{
public:
	// protocolVersion: 0 = Auto, 29 = 1.2.5, 340 = 1.12.2 (translated).
	GuiConnecting(Minecraft *minecraft, const std::string &host, int_t port, int_t protocolVersion = 29);
	~GuiConnecting() override;

	void updateScreen() override;
	void initGui() override;
	void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
	static void setNetClientHandler(GuiConnecting *guiconnecting, NetClientHandler *netclienthandler);
	static NetClientHandler *getNetClientHandler(GuiConnecting *guiconnecting);
	static bool isCancelled(GuiConnecting *guiconnecting);

protected:
	void keyTyped(char_t c, int_t key) override;
	void actionPerformed(GuiButton *button) override;

	NetClientHandler *clientHandler;
	ThreadConnectToServer *connectThread;
	bool cancelled;
};
