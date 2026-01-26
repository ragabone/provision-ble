/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   Command GATT characteristic.
 *   Write-only control channel for provisioning commands.
 *
 * Milestone 5:
 *   - Accepts JSON commands via WriteValue
 * Skeleton only: logs payload, no dispatch yet.
 *
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
 * Export the Command characteristic (write-only).
 *
 * Throws std::runtime_error on failure.
 */
void export_command(GDBusConnection* system_bus);

} // namespace provision::gatt
