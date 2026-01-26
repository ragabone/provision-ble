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
namespace provision::gatt {

/**
 * Export the State characteristic.
 */
void export_state(GDBusConnection* system_bus);

/**
 * Trigger a Wi-Fi scan and notify results via State characteristic.
 */
void handle_wifi_scan_request();

/**
 * Trigger a Wi-Fi connect attempt.
 * (MVP: stubbed behaviour)
 */
void handle_wifi_connect_request(const std::string& ssid,
                                 const std::string& psk);

void notify_state_connected(const std::string& ssid,
                            const std::string& ip);


} // namespace provision::gatt
