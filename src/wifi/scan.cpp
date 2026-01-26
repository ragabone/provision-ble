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

#include "wifi/scan.hpp"
#include "util/log.hpp"

#include <NetworkManager.h>
#include <gio/gio.h>

#include <algorithm>
#include <atomic>
#include <map>

namespace provision::wifi {

namespace {

// -----------------------------------------------------------------------------
// Busy guard
// -----------------------------------------------------------------------------

static std::atomic_bool g_scan_busy{false};

struct ScanBusyGuard {
    bool acquired{false};

    ScanBusyGuard()
    {
        // acquire "busy" if currently false
        bool expected = false;
        acquired = g_scan_busy.compare_exchange_strong(expected, true);
    }

    ~ScanBusyGuard()
    {
        if (acquired)
            g_scan_busy.store(false);
    }
};

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static std::string ssid_to_string(GBytes* ssid_bytes)
{
    if (!ssid_bytes)
        return {};

    gsize len = 0;
    const guint8* data =
        static_cast<const guint8*>(g_bytes_get_data(ssid_bytes, &len));

    if (!data || len == 0)
        return {};

    return std::string(reinterpret_cast<const char*>(data), len);
}

} // namespace

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

std::vector<std::string> scan_ssids()
{
    ScanBusyGuard guard;
    if (!guard.acquired) {
        provision::log::warn("wifi_scan: ignored (busy)");
        return {};
    }

    provision::log::info("wifi_scan: starting scan");

    std::vector<std::string> result;
    GError* err = nullptr;

    NMClient* client = nm_client_new(nullptr, &err);
    if (!client) {
        provision::log::error("wifi_scan: NMClient init failed");
        if (err) g_error_free(err);
        return result;
    }

    const GPtrArray* devices = nm_client_get_devices(client);
    if (!devices || devices->len == 0) {
        provision::log::warn("wifi_scan: no NetworkManager devices present");
    }

    NMDeviceWifi* wifi = nullptr;

    for (guint i = 0; devices && i < devices->len; ++i) {
        NMDevice* dev = NM_DEVICE(g_ptr_array_index(devices, i));
        if (NM_IS_DEVICE_WIFI(dev)) {
            wifi = NM_DEVICE_WIFI(dev);
            break;
        }
    }

    if (!wifi) {
        provision::log::warn("wifi_scan: no Wi-Fi device found");
        g_object_unref(client);
        return result;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    nm_device_wifi_request_scan(wifi, nullptr, &err);
#pragma GCC diagnostic pop

    if (err) {
        provision::log::warn("wifi_scan: scan request failed, using cached results");
        g_error_free(err);
        err = nullptr;
    }

    // Allow scan results to populate
    g_usleep(700 * 1000);

    const GPtrArray* aps = nm_device_wifi_get_access_points(wifi);
    if (!aps) {
        provision::log::warn("wifi_scan: no access points returned");
        g_object_unref(client);
        return result;
    }

    std::map<std::string, int> best_strength;

    for (guint i = 0; i < aps->len; ++i) {
        NMAccessPoint* ap = NM_ACCESS_POINT(g_ptr_array_index(aps, i));
        if (!ap)
            continue;

        GBytes* ssid_bytes = nm_access_point_get_ssid(ap);
        std::string ssid = ssid_to_string(ssid_bytes);
        if (ssid.empty())
            continue;

        int strength = static_cast<int>(nm_access_point_get_strength(ap));

        auto it = best_strength.find(ssid);
        if (it == best_strength.end() || strength > it->second)
            best_strength[ssid] = strength;
    }

    std::vector<std::pair<std::string, int>> sorted;
    for (const auto& kv : best_strength)
        sorted.emplace_back(kv);

    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });

    for (const auto& kv : sorted)
        result.push_back(kv.first);

    provision::log::info(
        "wifi_scan: found " + std::to_string(result.size()) + " SSIDs");

    g_object_unref(client);
    return result;
}

} // namespace provision::wifi
