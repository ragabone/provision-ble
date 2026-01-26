/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   Implementation of the Command GATT characteristic.
 *
 * Notes:
 *   - Write-only
 *   - Payload is expected to be small JSON
 *   - Dispatches explicit provisioning commands (e.g. wifi_scan)
 *
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

#include "gatt/command.hpp"
#include "gatt/characteristic.hpp"
#include "gatt/service.hpp"
#include "gatt/state.hpp"
#include "util/log.hpp"

#include <gio/gio.h>
#include <string>

namespace {

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

/**
 * Convert "ay" to std::string (best-effort UTF-8).
 */
std::string ay_to_string(GVariant* value)
{
    if (!value || !g_variant_is_of_type(value, G_VARIANT_TYPE("ay")))
        return {};

    gsize len = 0;
    const guint8* data =
        static_cast<const guint8*>(g_variant_get_fixed_array(value, &len, sizeof(guint8)));

    if (!data || len == 0)
        return {};

    return std::string(reinterpret_cast<const char*>(data), len);
}

/**
 * Very small JSON string extractor:
 *   Finds: "<key>" : "<value>"
 *
 * This is intentionally minimal to avoid pulling in a JSON dependency.
 * It is adequate for our controlled small payloads from the Web BLE client.
 */
std::string json_get_string(const std::string& payload, const std::string& key)
{
    // Find "key"
    const std::string needle = "\"" + key + "\"";
    size_t k = payload.find(needle);
    if (k == std::string::npos)
        return {};

    // Find ':'
    size_t colon = payload.find(':', k + needle.size());
    if (colon == std::string::npos)
        return {};

    // Find first quote after ':'
    size_t q1 = payload.find('"', colon + 1);
    if (q1 == std::string::npos)
        return {};

    // Find closing quote
    size_t q2 = payload.find('"', q1 + 1);
    if (q2 == std::string::npos || q2 <= q1 + 1)
        return {};

    return payload.substr(q1 + 1, q2 - (q1 + 1));
}

/**
 * WriteValue callback for Command characteristic.
 */
void on_write_command(GVariant* value)
{
    std::string payload = ay_to_string(value);

    if (payload.empty()) {
        provision::log::warn("Command WriteValue: empty payload");
        return;
    }

    provision::log::info("Command WriteValue: " + payload);

    // Primary op field
    std::string op = json_get_string(payload, "op");

    // Backward compatibility
    if (op.empty()) {
        std::string cmd = json_get_string(payload, "cmd");
        if (cmd == "wifi.scan")
            op = "wifi_scan";
        else if (cmd == "wifi.connect")
            op = "wifi_connect";
    }

    // ------------------------------------------------------------
    // wifi_scan
    // ------------------------------------------------------------
    if (op == "wifi_scan") {
        provision::log::info("Command dispatch: wifi_scan");
        provision::gatt::handle_wifi_scan_request();
        return;
    }

    // ------------------------------------------------------------
    // wifi_connect
    // Expected payload:
    // { "op":"wifi_connect", "ssid":"...", "psk":"..." }
    // ------------------------------------------------------------
    if (op == "wifi_connect") {
        std::string ssid = json_get_string(payload, "ssid");
        std::string psk  = json_get_string(payload, "psk");

        if (ssid.empty()) {
            provision::log::warn("wifi_connect: missing ssid");
            return;
        }

        provision::log::info("Command dispatch: wifi_connect");
        provision::gatt::handle_wifi_connect_request(ssid, psk);
        return;
    }

    // ------------------------------------------------------------
    // Unknown
    // ------------------------------------------------------------
    if (!op.empty()) {
        provision::log::warn("Command dispatch: unknown op=" + op);
        return;
    }

    provision::log::warn("Command dispatch: no op/cmd field");
}


// Flags: write (with response)
static const char* FLAGS[] = {
    "write",
    nullptr
};

} // namespace

namespace provision::gatt {

void export_command(GDBusConnection* system_bus)
{
    export_characteristic(
        system_bus,
        UUID_COMMAND,
        CHR_COMMAND,
        SERVICE_PATH,
        FLAGS,
        nullptr,           // no ReadValue
        nullptr,           // no notify callback
        on_write_command   // WriteValue handler
    );

    provision::log::info("Command characteristic exported");
}

} // namespace provision::gatt
