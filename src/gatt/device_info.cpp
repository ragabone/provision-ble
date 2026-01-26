/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   Implementation of the DeviceInfo GATT characteristic.
 *
 * Notes:
 *   - Returns a static JSON payload for Milestone 4.
 *   - Payload is encoded as UTF-8 bytes ("ay") as required by BlueZ.
 *   - File-backed metadata (/etc/provision/project.json) will be added later.
 *
 * Website:
 *   https://pidevelop.com
 *
 * Contact:
 *   james@pidevelop.com
 *
 * License:
 *   MIT License (see LICENSE file at repo root)
 */

#include "gatt/device_info.hpp"
#include "gatt/characteristic.hpp"
#include "gatt/service.hpp"
#include "util/log.hpp"

#include <string>

namespace {

// -----------------------------------------------------------------------------
// Temporary static payload (Milestone 4)
// -----------------------------------------------------------------------------

static const char* DEVICEINFO_JSON =
    "{"
    "\"Company\":\"PiDevelop.com\","
    "\"Developer\":\"james@pidevelop.com\","
    "\"project_name\":\"Provision BLE\""
    "}";

/**
 * Build a GVariant of type "ay" from a UTF-8 string.
 */
GVariant* make_ay_from_string(const char* str)
{
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("ay"));

    for (const unsigned char* p = (const unsigned char*)str; *p; ++p) {
        g_variant_builder_add(&b, "y", *p);
    }

    return g_variant_builder_end(&b); // ay
}

/**
 * ReadValue callback for DeviceInfo.
 */
GVariant* on_read_device_info()
{
    provision::log::info("DeviceInfo ReadValue");
    return make_ay_from_string(DEVICEINFO_JSON);
}

// Flags for this characteristic
static const char* FLAGS[] = {
    "read",
    nullptr
};

} // namespace

namespace provision::gatt {

void export_device_info(GDBusConnection* system_bus)
{
    export_characteristic(
        system_bus,
        UUID_DEVICEINFO,
        CHR_DEVINFO,
        SERVICE_PATH,
        FLAGS,
        on_read_device_info
    );

    provision::log::info("DeviceInfo characteristic exported");
}

} // namespace provision::gatt
