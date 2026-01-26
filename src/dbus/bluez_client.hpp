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

#pragma once

#include <gio/gio.h>
#include <string>
#include <functional>

namespace provision::bluez {

/**
 * AdapterPaths
 * ----------
 * adapter_path:
 *   The BlueZ object path of the adapter, typically /org/bluez/hci0
 */
struct AdapterPaths {
    std::string adapter_path;
};

/**
 * Find the first BlueZ adapter that exposes BOTH:
 *   - org.bluez.GattManager1
 *   - org.bluez.LEAdvertisingManager1
 *
 * Throws std::runtime_error on failure.
 */
AdapterPaths find_adapter(GDBusConnection* system_bus);

/**
 * Synchronous registration (legacy / unused for Milestone 4)
 */
void register_gatt_application(GDBusConnection* system_bus,
                               const std::string& adapter_path,
                               const std::string& app_path);

void register_advertisement(GDBusConnection* system_bus,
                            const std::string& adapter_path,
                            const std::string& adv_path);

/**
 * Asynchronous registration (Milestone 4 fix)
 */
using RegisterCallback = std::function<void(bool success, const std::string& error)>;

void register_gatt_application_async(GDBusConnection* system_bus,
                                     const std::string& adapter_path,
                                     const std::string& app_path,
                                     RegisterCallback cb);

void register_advertisement_async(GDBusConnection* system_bus,
                                  const std::string& adapter_path,
                                  const std::string& adv_path,
                                  RegisterCallback cb);

} // namespace provision::bluez
