/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   BLE-based provisioning daemon for Raspberry Pi devices.
 *
 * Website:
 *   https://pidevelop.com
 *
 * Contact:
 *   james@pidevelop.com
 *
 * License:
 *   MIT License (see LICENSE file at repo root)
 *
 * Copyright (c) 2026 PiDevelop
 */
#include "wifi/ip_monitor.hpp"
#include "util/log.hpp"

#include <thread>
#include <cstring>
#include <string>

#include <unistd.h>
#include <sys/socket.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <net/if.h>
#include "wifi/wifi_state_dispatcher.hpp"

namespace provision::wifi {


/* ---- Netlink monitor thread ------------------------------------------- */

static void ip_monitor_thread()
{
    int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
        provision::log::info("ip_monitor: failed to open netlink socket");
        return;
    }

    sockaddr_nl addr{};
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_IFADDR;

    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        provision::log::info("ip_monitor: netlink bind failed");
        close(fd);
        return;
    }
    provision::wifi::init_wifi_state_dispatcher();
    provision::log::info("wifi_connect: waiting for IPv4 config");

    char buffer[4096];

    while (true) {
        ssize_t len = recv(fd, buffer, sizeof(buffer), 0);
        if (len <= 0)
            continue;

        for (nlmsghdr* nh = reinterpret_cast<nlmsghdr*>(buffer);
             NLMSG_OK(nh, len);
             nh = NLMSG_NEXT(nh, len)) {

            if (nh->nlmsg_type != RTM_NEWADDR &&
                nh->nlmsg_type != RTM_DELADDR)
                continue;

            auto* ifa = reinterpret_cast<ifaddrmsg*>(NLMSG_DATA(nh));
            if (ifa->ifa_family != AF_INET)
                continue;

            char ifname[IF_NAMESIZE] = {};
            if (!if_indextoname(ifa->ifa_index, ifname))
                continue;

            if (std::strcmp(ifname, "wlan0") != 0)
                continue;

            if (nh->nlmsg_type == RTM_NEWADDR) {
                provision::wifi::notify_ipv4_ready();
            } else {
                provision::log::info(
                    "ip_monitor: wlan0 IPv4 removed"
                );
            }
        }
    }
}

/* ---- Public API -------------------------------------------------------- */

void start_ip_monitor()
{
    std::thread(ip_monitor_thread).detach();
}

} // namespace provision::wifi
