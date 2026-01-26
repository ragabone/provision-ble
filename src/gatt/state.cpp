/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   Implementation of the State GATT characteristic.
 *
 * Notes:
 *   - Read + Notify characteristic
 *   - ReadValue returns current provisioning state
 *   - Wi-Fi scan progress and results are published via notifications
 *   - No NetworkManager logic lives here
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
#include "gatt/state.hpp"
#include "gatt/characteristic.hpp"
#include "gatt/service.hpp"
#include "wifi/scan.hpp"
#include "wifi/connect.hpp"
#include "util/log.hpp"
#include "wifi/wifi_state_dispatcher.hpp"

#include <string>
#include <vector>

namespace {

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------

static std::string g_state = "UNCONFIGURED";

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

GVariant* make_ay_from_string(const char* str)
{
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("ay"));

    for (const unsigned char* p = (const unsigned char*)str; *p; ++p)
        g_variant_builder_add(&b, "y", *p);

    return g_variant_builder_end(&b);
}

GVariant* make_state_payload(const std::string& state)
{
    std::string json = "{\"state\":\"" + state + "\"}";
    return make_ay_from_string(json.c_str());
}

void notify_state()
{
    GVariant* value = make_state_payload(g_state);

    provision::gatt::notify_characteristic_value(
        provision::gatt::CHR_STATE,
        value
    );

    g_variant_unref(value);
}

GVariant* on_read_state()
{
    provision::log::info("State ReadValue");
    return make_state_payload(g_state);
}

void on_state_notify(bool enabled)
{
    if (!enabled) {
        provision::log::info("State notify DISABLED by client");
        return;
    }

    provision::log::info("State notify ENABLED by client");

    /*
     * If Wi-Fi is already connected (e.g. provisioned via Imager or
     * a previous run), publish CONNECTED immediately so the Web BLE
     * client gets the truth without issuing any command.
     *
     * This is safe: the dispatcher runs on the GLib main context and
     * will no-op if not connected.
     */
    provision::wifi::notify_ipv4_ready();
}


// Flags: read + notify
static const char* FLAGS[] = {
    "read",
    "notify",
    nullptr
};

// Conservative single-chunk payload limit (bytes)
constexpr size_t MAX_NOTIFY_BYTES = 200;

static std::string json_escape(const std::string& in)
{
    std::string out;
    out.reserve(in.size());

    for (char c : in) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"':  out += "\\\""; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if ((unsigned char)c < 0x20) out += '?';
            else out += c;
        }
    }
    return out;
}

static std::string build_wifi_scan_payload(const std::vector<std::string>& ssids)
{
    std::string payload = "{\"op\":\"wifi_scan\",\"ssids\":[";
    bool first = true;

    for (const auto& ssid : ssids) {
        std::string entry =
            (first ? "" : ",") + std::string("\"") + json_escape(ssid) + "\"";

        // +2 for closing "]}"
        if (payload.size() + entry.size() + 2 > MAX_NOTIFY_BYTES)
            break;

        payload += entry;
        first = false;
    }

    payload += "]}";
    return payload;
}


} // namespace

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

namespace provision::gatt {

void handle_wifi_scan_request()
{
    provision::log::info("wifi_scan: request received");

    // 1. Notify SCANNING
    g_state = "SCANNING";
    notify_state();

    // 2. Perform scan
    std::vector<std::string> ssids = provision::wifi::scan_ssids();

    provision::log::info(
        "wifi_scan: completed, ssid_count=" + std::to_string(ssids.size()));

    // 3. Notify SSID payload
    std::string payload = build_wifi_scan_payload(ssids);
    GVariant* value = make_ay_from_string(payload.c_str());

    provision::log::info("wifi_scan: notifying SSID payload");
    notify_characteristic_value(CHR_STATE, value);
    g_variant_unref(value);

    // 4. Notify SCAN_COMPLETE
    g_state = "SCAN_COMPLETE";
    notify_state();
}

void notify_state_connected(const std::string& ssid,
                            const std::string& ip)
{
    provision::log::info(
        "notify_state_connected: ssid=" + ssid + " ip=" + ip);

    // Update global state
    g_state = "CONNECTED";

    // Build JSON payload
    std::string payload =
        "{"
        "\"state\":\"CONNECTED\","
        "\"ssid\":\"" + json_escape(ssid) + "\","
        "\"ip\":\"" + json_escape(ip) + "\""
        "}";

    GVariant* value = make_ay_from_string(payload.c_str());

    notify_characteristic_value(CHR_STATE, value);

    g_variant_unref(value);
}

void handle_wifi_connect_request(const std::string& ssid,
                                 const std::string& psk)
{
    provision::log::info("wifi_connect: request received");

    g_state = "CONNECTING";
    notify_state();

    auto result = provision::wifi::connect(ssid, psk);

    if (result != provision::wifi::ConnectResult::REQUESTED) {
        g_state = "UNCONFIGURED";
        notify_state();
    }
}

void export_state(GDBusConnection* system_bus)
{
    export_characteristic(
        system_bus,
        UUID_STATE,
        CHR_STATE,
        SERVICE_PATH,
        FLAGS,
        on_read_state,
        on_state_notify
    );

    provision::log::info("State characteristic exported");
}

} // namespace provision::gatt
