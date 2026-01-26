# provision

BLE-based provisioning daemon for Raspberry Pi devices.

This project provides a reusable, appliance-style Bluetooth Low Energy (BLE)
provisioning mechanism that allows headless Raspberry Pi systems to be
configured on first boot without requiring a keyboard, display, or Ethernet
connection.

The provisioning process is designed to be:
- Browser-based (Web Bluetooth)
- Console-free for end users
- Reusable across multiple Raspberry Pi projects
- Disabled automatically after successful provisioning


## Project Information

- Website: **https://pidevelop.com**
- Hosted client: **https://piconnect.com/provision/web-ble-client**
- Contact: **james@pidevelop.com**
---

## Key Features (MVP)

- BLE GATT server exposed via BlueZ
- Read-only device information and provisioning state (Milestone 4)
- Wi-Fi scanning and credential provisioning via NetworkManager (future milestones)
- First-boot provisioning only (BLE disabled after configuration)
- Modular C++ implementation using GLib / GDBus

---

## Target Platform

- Raspberry Pi OS (Debian 13 / Trixie)
- Raspberry Pi 4 Model B
- Raspberry Pi Compute Module 4 (assumed compatible)

---

## Build Dependencies

- CMake
- GCC / G++
- GLib 2.0 (glib-2.0, gio-2.0)
- jsoncpp
- BlueZ

---

## Build (development)

```bash
mkdir -p build
cd build
cmake ..
make

---

## Run Program

sudo ./provision_ble
OR
Use as service -- see systemd directory in repo for setup instructions
---

License
MIT License — see the LICENSE file for details.

Contact
Website: https://pidevelop.com

Email: james@pidevelop.com
