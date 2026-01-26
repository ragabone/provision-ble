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

// -----------------------------------------------------------------------------
// Frozen UUIDs (Milestone 3)
// -----------------------------------------------------------------------------

// Provisioning Service UUID
inline constexpr const char* SERVICE_UUID =
    "9a7d0000-7c2a-4f8e-9b32-9b3e6d4a0001";

// Characteristics (Milestone 4 subset)
inline constexpr const char* UUID_DEVICEINFO =
    "9a7d0000-7c2a-4f8e-9b32-9b3e6d4a0002";

inline constexpr const char* UUID_STATE =
    "9a7d0000-7c2a-4f8e-9b32-9b3e6d4a0003";

inline constexpr const char* UUID_COMMAND =
    "9a7d0000-7c2a-4f8e-9b32-9b3e6d4a0004";

// -----------------------------------------------------------------------------
// D-Bus object paths (our exported object tree)
// -----------------------------------------------------------------------------
//
// BlueZ expects an application object implementing ObjectManager at APP_PATH.
// Under that root, we export service(s) and characteristic(s) as objects.
// These paths are referenced in GetManagedObjects and in BlueZ registration.
//

inline constexpr const char* APP_PATH =
    "/org/bluez/provision";

inline constexpr const char* SERVICE_PATH =
    "/org/bluez/provision/service0";

inline constexpr const char* CHR_DEVINFO =
    "/org/bluez/provision/char0";

inline constexpr const char* CHR_STATE =
    "/org/bluez/provision/char1";

inline constexpr const char* CHR_COMMAND =
    "/org/bluez/provision/char2";


void export_service(GDBusConnection* system_bus);
} // namespace provision::gatt
