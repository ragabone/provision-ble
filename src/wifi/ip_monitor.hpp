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

namespace provision::wifi {

/**
 * Start a background thread that listens for kernel IPv4
 * address changes on wlan0 and logs events.
 *
 * One-shot init, intended to be called from main.cpp.
 */
void start_ip_monitor();

} // namespace provision::wifi
