#include "ThreadPollServers.h"

#include <chrono>
#include <thread>

#include "GuiMultiplayer.h"
#include "ServerNBTStorage.h"
#ifdef PS2_PLATFORM
#include "java/System.h"
#endif

void ThreadPollServers::start(const std::shared_ptr<ServerNBTStorage> &server)
{
#if defined(WII_PLATFORM) || defined(PS2_PLATFORM)
    run(server);
#else
    std::thread(&ThreadPollServers::run, server).detach();
#endif
}

void ThreadPollServers::run(std::shared_ptr<ServerNBTStorage> server)
{
    if (server == nullptr)
    {
        GuiMultiplayer::decrementThreadsPending();
        return;
    }

    try
    {
        const auto startTime = std::chrono::steady_clock::now();
        GuiMultiplayer::pollServer(server);
        const auto endTime = std::chrono::steady_clock::now();
        const long_t latency = (long_t)std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        std::lock_guard<std::mutex> guard(server->stateMutex);
#ifdef PS2_PLATFORM
        if (server->lag == -1)
        {
            if (server->pollRetryCount < 3)
            {
                ++server->pollRetryCount;
                server->nextPollTime = System::currentTimeMillis() + 5000;
            }
            else
                server->nextPollTime = 0;
        }
        else
        {
            server->lag = latency;
            server->nextPollTime = 0;
            server->pollRetryCount = 0;
        }
#else
        server->lag = latency;
#endif
    }
    catch (...)
    {
        std::lock_guard<std::mutex> guard(server->stateMutex);
        server->lag = -1;
        server->motd = "\xC2\xA7" "4Can't reach server";
        server->playerCount.clear();
#ifdef PS2_PLATFORM
        if (server->pollRetryCount < 3)
        {
            ++server->pollRetryCount;
            server->nextPollTime = System::currentTimeMillis() + 5000;
        }
        else
            server->nextPollTime = 0;
#endif
    }

    GuiMultiplayer::decrementThreadsPending();
}
