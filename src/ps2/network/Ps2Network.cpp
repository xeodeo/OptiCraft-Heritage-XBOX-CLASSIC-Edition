#ifdef PS2_PLATFORM

#include "ps2/network/Ps2Network.h"

#include "platform/Log.h"
#include "platform/Mutex.h"
#include "ps2/system/Ps2IrxLoader.h"

#include <delaythread.h>
#include <kernel.h>

extern "C" {
#ifdef PS2_REMOTE_DEBUG
#include <ps2ips.h>
#include <ps2ip_rpc.h>

// libps2ips keeps this low-level configuration RPC internal instead of
// exposing it through ps2ips.h. Read the ps2link-owned sm0 address so client
// sockets can bind to the existing interface without replacing its config.
int ps2ipc_ps2ip_getconfig(char *netif_name, t_ip_info *ip_info);
#else
#include <ps2ip.h>
#endif
}

#include <atomic>
#include <mutex>

namespace
{
PlatformMutex s_initMutex;
std::atomic_bool s_ready{false};
bool s_stackInitialized = false;
std::uint32_t s_localAddressNetworkOrder = 0;

void loadNetworkModules()
{
    // Modern EE ps2ip keeps lwIP on the EE. Only the Ethernet driver and
    // NETMAN bridge are required on the IOP; ps2ip-nm.irx is intentionally not
    // loaded because that is the alternative IOP-side TCP/IP stack.
    const int dev9 = Ps2IrxLoader::load("irx/ps2dev9.irx", "host:ps2dev9.irx");
    const int netman = Ps2IrxLoader::load("irx/netman.irx", "host:netman.irx");
    const int smap = Ps2IrxLoader::load("irx/smap.irx", "host:smap.irx");

    // A loader such as ps2link/OPL may already have one or more of these IRXs
    // resident, in which case a duplicate load can return an error. Do not fail
    // solely on the loader return values; the sm0 interface check below is the
    // definitive test that the Ethernet path came up.
    MC_LOG_INFO("network", "[PS2] IRX network load: dev9=%d netman=%d smap=%d\n",
                dev9, netman, smap);
}

bool startStackAndDhcp()
{
    if (s_stackInitialized)
        return true;

#ifdef PS2_REMOTE_DEBUG
    // ps2link already owns DEV9/SMAP and PS2IP-NM, but it does not load the
    // ps2ips RPC socket server. Load only that bridge module; reloading the
    // hardware/network stack would compete with ps2link and break remote I/O.
    MC_LOG_INFO("network", "[PS2] loading ps2ips RPC bridge for ps2link stack\n");
    McLog::flush();
    const int ps2ips = Ps2IrxLoader::load("irx/ps2ips.irx", "host:ps2ips.irx");
    if (ps2ips < 0)
    {
        // ps2ip_init() retries SIF RPC binding forever when no PS2IPS server is
        // present. Fail here instead of freezing the multiplayer worker/UI.
        MC_LOG_ERROR("network", "[PS2] ps2ips.irx unavailable; refusing blocking RPC bind\n");
        return false;
    }

    MC_LOG_INFO("network", "[PS2] ps2ips RPC bridge loaded id=%d; binding EE client\n", ps2ips);
    McLog::flush();
    if (ps2ip_init() < 0)
    {
        MC_LOG_ERROR("network", "[PS2] ps2ips RPC initialization failed\n");
        return false;
    }

    s_stackInitialized = true;
    MC_LOG_INFO("network", "[PS2] ps2ips RPC client ready; preserving ps2link network config\n");

    // Keep the exact network-order address returned by PS2IP-NM. Reusing the
    // binary value avoids reparsing it through the incompatible EE inet_aton()
    // path before the explicit client bind.
    t_ip_info info{};
    if (ps2ipc_ps2ip_getconfig(const_cast<char *>("sm0"), &info) > 0)
        s_localAddressNetworkOrder = info.ipaddr.s_addr;
    else
    {
        MC_LOG_WARN("network", "[PS2] failed to read ps2link sm0 configuration through ps2ips\n");
    }

    McLog::flush();
    return true;
#else
    loadNetworkModules();

    // ps2ipInit initializes NETMAN itself, then registers the EE-side lwIP
    // stack as sm0. Calling NetManInit a second time here is unnecessary and
    // can make ownership/deinit semantics ambiguous.
    ip4_addr ip{};
    ip4_addr mask{};
    ip4_addr gateway{};
    IP4_ADDR(&ip, 169, 254, 0, 1);
    IP4_ADDR(&mask, 255, 255, 0, 0);
    IP4_ADDR(&gateway, 0, 0, 0, 0);
    MC_LOG_INFO("network", "[PS2] calling ps2ipInit\n");
    McLog::flush();
    if (ps2ipInit(&ip, &mask, &gateway) < 0)
    {
        MC_LOG_ERROR("network", "[PS2] ps2ipInit failed\n");
        return false;
    }

    MC_LOG_INFO("network", "[PS2] ps2ipInit OK\n");
    McLog::flush();

    t_ip_info info{};
    if (!ps2ip_getconfig(const_cast<char *>("sm0"), &info))
    {
        MC_LOG_ERROR("network", "[PS2] network interface sm0 was not registered\n");
        ps2ipDeinit();
        return false;
    }

    MC_LOG_INFO("network", "[PS2] sm0 registered, enabling DHCP\n");
    McLog::flush();
    info.dhcp_enabled = 1;
    if (!ps2ip_setconfig(&info))
    {
        MC_LOG_ERROR("network", "[PS2] failed to enable DHCP on sm0\n");
        ps2ipDeinit();
        return false;
    }

    // Keep the stack resident even if the first DHCP wait times out. DHCP can
    // finish later (for example after a cable is connected), and the next
    // connection attempt can then succeed without reinitializing lwIP.
    s_stackInitialized = true;
    MC_LOG_INFO("network", "[PS2] DHCP client started\n");
    McLog::flush();
    return true;
#endif
}
}

namespace Ps2Network
{
bool initialize()
{
#ifdef NO_NETWORK
    return false;
#else
    if (s_ready.load(std::memory_order_acquire))
        return true;

    MC_LOG_INFO("network", "[PS2] network initialize entered\n");
    McLog::flush();
    std::lock_guard<PlatformMutex> guard(s_initMutex);
    MC_LOG_INFO("network", "[PS2] network init lock acquired\n");
    McLog::flush();
    if (s_ready.load(std::memory_order_relaxed))
        return true;
    if (!startStackAndDhcp())
        return false;

#ifndef PS2_REMOTE_DEBUG
    // Leave DHCP alone while lwIP negotiates the lease. Repeated EE-side
    // ps2ip_getconfig()/NETMAN RPCs during this window have caused intermittent
    // IOP stalls on real hardware, including loss of PAD input. The network
    // workers can safely wait here without blocking the render/input thread.
    constexpr int kDhcpGracePeriodUs = 10000000;
    DelayThread(kDhcpGracePeriodUs);
#endif

    s_ready.store(true, std::memory_order_release);
#ifdef PS2_REMOTE_DEBUG
    MC_LOG_INFO("network", "[PS2] ps2link shared network ready; enabling socket attempts\n");
#else
    MC_LOG_INFO("network", "[PS2] DHCP grace period complete; enabling socket attempts\n");
#endif
    return true;
#endif
}

bool isReady()
{
    return s_ready.load(std::memory_order_acquire);
}

bool localAddressNetworkOrder(std::uint32_t &address)
{
    std::lock_guard<PlatformMutex> guard(s_initMutex);
    if (s_localAddressNetworkOrder == 0)
        return false;
    address = s_localAddressNetworkOrder;
    return true;
}
}

#endif
