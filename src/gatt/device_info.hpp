/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   DeviceInfo GATT characteristic.
 *   Exposes read-only device and project metadata over BLE.
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

#pragma once

#include <gio/gio.h>

namespace provision::gatt {

/**
 * Export the DeviceInfo characteristic.
 *
 * Throws std::runtime_error on failure.
 */
void export_device_info(GDBusConnection* system_bus);

} // namespace provision::gatt
