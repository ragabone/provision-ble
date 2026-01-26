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
#include "wifi/connect.hpp"
#include "util/log.hpp"
#include "gatt/state.hpp"

#include <NetworkManager.h>
#include <glib.h>

namespace {

// -----------------------------------------------------------------------------
// Context passed through async activation
// -----------------------------------------------------------------------------

struct ActivateCtx {
    std::string ssid;
};






} // namespace

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

namespace provision::wifi {

ConnectResult connect(const std::string& ssid,
                      const std::string& psk)
{
    provision::log::info("wifi_connect: starting ssid=" + ssid);

    GError* err = nullptr;

    NMClient* client = nm_client_new(nullptr, &err);
    if (!client) {
        provision::log::error("wifi_connect: NMClient init failed");
        if (err) g_error_free(err);
        return ConnectResult::FAILED;
    }

    // ------------------------------------------------------------
    // Find Wi-Fi device
    // ------------------------------------------------------------

    NMDeviceWifi* wifi = nullptr;
    const GPtrArray* devices = nm_client_get_devices(client);

    for (guint i = 0; devices && i < devices->len; ++i) {
        NMDevice* dev = NM_DEVICE(g_ptr_array_index(devices, i));
        if (NM_IS_DEVICE_WIFI(dev)) {
            wifi = NM_DEVICE_WIFI(dev);
            break;
        }
    }

    if (!wifi) {
        provision::log::error("wifi_connect: no Wi-Fi device found");
        g_object_unref(client);
        return ConnectResult::FAILED;
    }
    
    // ------------------------------------------------------------
    // Build connection profile
    // ------------------------------------------------------------

    NMConnection* connection = nm_simple_connection_new();

    NMSettingConnection* s_con =
        NM_SETTING_CONNECTION(nm_setting_connection_new());

    g_object_set(G_OBJECT(s_con),
                 NM_SETTING_CONNECTION_ID, ssid.c_str(),
                 NM_SETTING_CONNECTION_TYPE,
                 NM_SETTING_WIRELESS_SETTING_NAME,
                 NM_SETTING_CONNECTION_AUTOCONNECT, TRUE,
                 nullptr);

    nm_connection_add_setting(connection, NM_SETTING(s_con));

    NMSettingWireless* s_wifi =
        NM_SETTING_WIRELESS(nm_setting_wireless_new());

    GBytes* ssid_bytes = g_bytes_new(ssid.data(), ssid.size());

    g_object_set(G_OBJECT(s_wifi),
                 NM_SETTING_WIRELESS_SSID, ssid_bytes,
                 NM_SETTING_WIRELESS_MODE, "infrastructure",
                 nullptr);

    g_bytes_unref(ssid_bytes);
    nm_connection_add_setting(connection, NM_SETTING(s_wifi));

    NMSettingWirelessSecurity* s_sec =
        NM_SETTING_WIRELESS_SECURITY(nm_setting_wireless_security_new());

    g_object_set(G_OBJECT(s_sec),
                 NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk",
                 NM_SETTING_WIRELESS_SECURITY_PSK, psk.c_str(),
                 nullptr);

    nm_connection_add_setting(connection, NM_SETTING(s_sec));

    NMSettingIP4Config* s_ip4 =
        NM_SETTING_IP4_CONFIG(nm_setting_ip4_config_new());

    g_object_set(G_OBJECT(s_ip4),
                 NM_SETTING_IP_CONFIG_METHOD,
                 NM_SETTING_IP4_CONFIG_METHOD_AUTO,
                 nullptr);

    nm_connection_add_setting(connection, NM_SETTING(s_ip4));

    // ------------------------------------------------------------
    // Activate (async)
    // ------------------------------------------------------------

    auto* ctx = new ActivateCtx{ssid};

    nm_client_add_and_activate_connection2(
        client,
        connection,
        NM_DEVICE(wifi),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        ctx
    );

    g_object_unref(client);

    return ConnectResult::REQUESTED;
}

} // namespace provision::wifi
