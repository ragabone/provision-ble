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
 * Called from main.cpp once GLib is initialised.
 * Registers handlers for Wi-Fi state transitions.
 */
void init_wifi_state_dispatcher();

/**
 * Called from the netlink thread when wlan0 gains IPv4.
 * Safe to call from any thread.
 */
void notify_ipv4_ready();

}
