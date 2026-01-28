#!/usr/bin/env bash
set -e

PROJECT_REPO="https://github.com/ragabone/provision-ble.git"
PROJECT_DIR="/opt/provision-ble"
BUILD_DIR="$PROJECT_DIR/build"
SERVICE_NAME="provision-ble.service"
# Install packages
sudo apt update
sudo apt install -y \
  git \
  cmake \
  build-essential \
  pkg-config \
  libglib2.0-dev \
  libnm-dev \
  bluez 

sudo rfkill unblock bluetooth
echo waiting for bluez ...
sleep 5
#Setup Bluetooth
sudo btmgmt power on
sudo btmgmt connectable on
sudo btmgmt discov on
#sudo rfkill unblock bluetooth
#sudo bluetoothctl power on
#sudo bluetoothctl discoverable on
#sudo bluetoothctl pairable on

# get project
sudo git clone "$PROJECT_REPO" "$PROJECT_DIR"
sudo chown -R "$USER:$USER" "$PROJECT_DIR"
#build
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake ..
make -j"$(nproc)"
#Setup Service
SERVICE_PATH="/etc/systemd/system/$SERVICE_NAME"
sudo tee "$SERVICE_PATH" >/dev/null <<EOF
[Unit]
Description=Provision BLE Service
After=bluetooth.service NetworkManager.service
Requires=bluetooth.service NetworkManager.service

[Service]
Type=simple
ExecStart=$BUILD_DIR/provision-ble
Restart=always
RestartSec=2
User=root

[Install]
WantedBy=multi-user.target
EOF
#Enable service
sudo systemctl daemon-reexec
sudo systemctl daemon-reload
sudo systemctl enable "$SERVICE_NAME"
sudo systemctl start "$SERVICE_NAME"
sudo systemctl status "$SERVICE_NAME" --no-pager
