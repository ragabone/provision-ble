/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   Base interface for BlueZ GATT characteristics.
 *   Supports:
 *     - ReadValue (optional)
 *     - StartNotify / StopNotify (optional callback)
 *     - WriteValue (optional)
 *     - Emitting notifications by updating Value and emitting PropertiesChanged
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

namespace provision::gatt {

/**
 * ReadValue callback.
 *
 * Must return a GVariant of type "ay".
 */
using ReadCallback = GVariant* (*)();

/**
 * Notification state callback.
 *
 * Called when a client enables or disables notifications
 * via StartNotify / StopNotify.
 */
using NotifyStateCallback = void (*)(bool enabled);

/**
 * WriteValue callback.
 *
 * Called when a client writes to this characteristic via WriteValue.
 * The provided GVariant is the written value and is of type "ay".
 */
using WriteCallback = void (*)(GVariant* value_ay);

/**
 * Export a GATT characteristic object.
 *
 * uuid         - Characteristic UUID
 * object_path  - D-Bus object path
 * service_path - Parent service object path
 * flags        - NULL-terminated list ("read", "notify", "write", etc.)
 * read_cb      - Callback invoked on ReadValue (optional)
 * notify_cb    - Optional callback invoked on StartNotify / StopNotify
 * write_cb     - Callback invoked on WriteValue (optional)
 *
 * Throws std::runtime_error on failure.
 */
void export_characteristic(GDBusConnection* system_bus,
                           const std::string& uuid,
                           const std::string& object_path,
                           const std::string& service_path,
                           const char* const* flags,
                           ReadCallback read_cb,
                           NotifyStateCallback notify_cb = nullptr,
                           WriteCallback write_cb = nullptr);

/**
 * Emit a notification by updating the cached Value and emitting
 * org.freedesktop.DBus.Properties.PropertiesChanged for "Value".
 *
 * - object_path must match the characteristic object path used at export.
 * - value_ay must be a GVariant of type "ay".
 * - If notifications are not enabled (StartNotify not called), this is a no-op.
 */
void notify_characteristic_value(const std::string& object_path, GVariant* value_ay);

} // namespace provision::gatt
