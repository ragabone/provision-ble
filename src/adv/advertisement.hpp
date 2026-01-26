/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   BLE LE Advertisement object for provisioning.
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
#include <string>
namespace provision::adv {

/**
 * Export the BLE advertisement object.
 *
 * Throws std::runtime_error on failure.
 */
void export_advertisement(GDBusConnection* system_bus);
void set_ble_alias(GDBusConnection* bus, const std::string& name);

} // namespace provision::adv
