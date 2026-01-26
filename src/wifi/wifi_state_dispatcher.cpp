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
 
#include "wifi/wifi_state_dispatcher.hpp"
#include "util/log.hpp"
#include "gatt/state.hpp"
#include <NetworkManager.h>
#include <glib.h>

namespace provision::wifi {

static gboolean on_ipv4_ready(gpointer)
{
    NMClient* client = nm_client_new(nullptr, nullptr);
    if (!client)
        return G_SOURCE_REMOVE;

    NMDevice* dev = nm_client_get_device_by_iface(client, "wlan0");
    if (!dev || !NM_IS_DEVICE_WIFI(dev)) {
        g_object_unref(client);
        return G_SOURCE_REMOVE;
    }

    /* SSID */
    std::string ssid = "unknown";
    NMAccessPoint* ap =
        nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI(dev));
    if (ap) {
        const GBytes* b = nm_access_point_get_ssid(ap);
        if (b) {
            gsize len = 0;
            const guint8* data =
                static_cast<const guint8*>(
                    g_bytes_get_data(
                        const_cast<GBytes*>(b), &len));
            ssid.assign(reinterpret_cast<const char*>(data), len);
        }
    }

    /* IPv4 */
    std::string ip;
    NMIPConfig* ip4 = nm_device_get_ip4_config(dev);
    if (ip4) {
        const GPtrArray* addrs = nm_ip_config_get_addresses(ip4);
        if (addrs && addrs->len > 0) {
            auto* addr =
                static_cast<NMIPAddress*>(
                    g_ptr_array_index(addrs, 0));
            ip = nm_ip_address_get_address(addr);
        }
    }

    if (!ip.empty()) {
        provision::log::info(
            "wifi connected ssid=" + ssid + " ip=" + ip
        );
        provision::gatt::notify_state_connected(ssid, ip);
        /* later: notify_state_connected(ssid, ip); */
    }

    g_object_unref(client);
    return G_SOURCE_REMOVE;
}

void notify_ipv4_ready()
{
    g_main_context_invoke(
        nullptr,          // default main context
        on_ipv4_ready,
        nullptr
    );
}

void init_wifi_state_dispatcher()
{
    /* intentionally empty for now */
}

} // namespace provision::wifi
