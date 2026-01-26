/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   Wi-Fi scanning helpers using NetworkManager.
 *
 * Notes:
 *   - Stateless utility module
 *   - Returns SSIDs sorted by signal strength (descending)
 *   - No BLE knowledge, no side effects beyond logging
 *
 * License:
 *   MIT License
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
#include <vector>

namespace provision::wifi {

/**
 * Perform a one-shot Wi-Fi scan and return SSIDs sorted by strength.
 *
 * On failure, returns an empty vector.
 */
std::vector<std::string> scan_ssids();

} // namespace provision::wifi
