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

#include <string>

namespace provision::wifi {

enum class ConnectResult {
    REQUESTED,
    FAILED
};

ConnectResult connect(const std::string& ssid,
                      const std::string& psk);

} // namespace provision::wifi
