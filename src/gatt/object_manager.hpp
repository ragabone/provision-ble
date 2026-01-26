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

namespace provision::gatt {

/**
 * Export the D-Bus ObjectManager interface for our GATT application.
 *
 * BlueZ expects a GATT application to implement:
 *   org.freedesktop.DBus.ObjectManager.GetManagedObjects
 *
 * The managed objects map describes the full set of services/characteristics
 * under the application path.
 *
 * For this project, our application root is:
 *   /org/bluez/provision
 *
 * Throws std::runtime_error on failure.
 */
void export_object_manager(GDBusConnection* system_bus);

} // namespace provision::gatt
