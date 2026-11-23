#include "ThreadConnectToServer.h"

#include "platform/Log.h"
#ifdef PS2_PLATFORM
#include "ps2/system/Ps2ThreadPriority.h"
#endif
#include <exception>
#include <iostream>
#include <stdexcept>

#include "GuiConnecting.h"
#include "GuiMultiplayer.h"
#include "ProtocolTranslator340IO.h"
#include "Minecraft.h"
#include "NetClientHandler.h"
#include "Packet2Handshake.h"
#include "Session.h"
#include "java/String.h"

ThreadConnectToServer::ThreadConnectToServer(GuiConnecting *guiconnecting, Minecraft *minecraft, const std::string &s, int_t i,
                                             int_t protocolVersion)
	: mc(minecraft)
	, hostName(s)
	, port(i)
	, protocol(protocolVersion)
{
	(void)guiconnecting;
}

ThreadConnectToServer::~ThreadConnectToServer()
{
	cancel();
	if (worker.joinable() && !worker.isCurrent())
		worker.join();
	std::lock_guard<PlatformMutex> guard(resultLock);
	if (resultHandler != nullptr)
	{
		resultHandler->disconnect();
		delete resultHandler;
		resultHandler = nullptr;
	}
}

void ThreadConnectToServer::start()
{
#ifdef PS2_PLATFORM
	constexpr int kConnectPriority = Ps2ThreadPriority::kNetwork;
#else
	constexpr int kConnectPriority = 64;
#endif
	if (!worker.start(&ThreadConnectToServer::wiiThreadEntry, this, 32 * 1024, kConnectPriority))
		throw std::runtime_error("Could not create connection thread");
}

void ThreadConnectToServer::cancel()
{
	cancelled.store(true);
}

NetClientHandler *ThreadConnectToServer::takeHandler()
{
	std::lock_guard<PlatformMutex> guard(resultLock);
	NetClientHandler *handler = resultHandler;
	resultHandler = nullptr;
	return handler;
}

bool ThreadConnectToServer::takeError(std::string &message)
{
	std::lock_guard<PlatformMutex> guard(resultLock);
	if (!errorPending)
		return false;
	message = resultError;
	errorPending = false;
	return true;
}

void *ThreadConnectToServer::wiiThreadEntry(void *argument)
{
	static_cast<ThreadConnectToServer *>(argument)->run();
	return nullptr;
}

void ThreadConnectToServer::run()
{
	try
	{
		MC_LOG_INFO("network", "[PS2] connect worker running for %s:%d\n", hostName.c_str(), static_cast<int>(port));
		McLog::flush();
		if (protocol != 29)
			GuiMultiplayer::resolveSrvIfDefault(hostName, port);
		if (protocol == 0)
		{
			// Auto: a server that answers the 1.7+ status ping is reached through
			// the 1.12.2 translator; anything else is spoken to natively.
			std::string motd;
			int online = 0, maximum = 0, serverProtocol = 0;
			long long lag = 0;
			protocol = Translator340Connection::pollStatus(hostName, port, motd, online, maximum, lag, serverProtocol)
				? translator340::kTargetProtocol : 29;
			MC_LOG_INFO("network", "auto version: server protocol %d -> using %d\n", serverProtocol,
				static_cast<int>(protocol));
			if (cancelled.load())
				return;
		}
		NetClientHandler *handler = new NetClientHandler(mc, hostName, port, static_cast<int>(protocol));
		if (cancelled.load())
		{
			handler->disconnect();
			delete handler;
			return;
		}
		handler->addToSendQueue(new Packet2Handshake(mc->session->username));
		std::lock_guard<PlatformMutex> guard(resultLock);
		resultHandler = handler;
	}
	catch (std::exception &exception)
	{
		if (cancelled.load())
			return;
		MC_LOG_ERROR("game", "%s\n", exception.what());
		std::lock_guard<PlatformMutex> guard(resultLock);
		resultError = exception.what();
		errorPending = true;
	}
}
