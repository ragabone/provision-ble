# File: CMakeLists.txt
# Purpose:
#   Stepwise build configuration for the provision-ble daemon.
#
# Notes:
#   - We add source files incrementally as we implement each module.
#   - Current build compiles src/main.cpp and required components.

cmake_minimum_required(VERSION 3.16)

project(provision_ble LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

find_package(PkgConfig REQUIRED)

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

# GLib / GIO
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0)

# NetworkManager (libnm)
pkg_check_modules(NM REQUIRED libnm)

# ------------------------------------------------------------------------------
# Includes
# ------------------------------------------------------------------------------

include_directories(
    ${GLIB_INCLUDE_DIRS}
    ${NM_INCLUDE_DIRS}
    src
)

# ------------------------------------------------------------------------------
# Target
# ------------------------------------------------------------------------------

add_executable(provision-ble
    src/main.cpp

    # util
    src/util/log.cpp

    # dbus
    src/dbus/bluez_client.cpp

    # gatt
    src/gatt/service.cpp
    src/gatt/object_manager.cpp
    src/gatt/device_info.cpp
    src/gatt/state.cpp
    src/gatt/characteristic.cpp
    src/gatt/command.cpp

    # wifi
    src/wifi/scan.cpp
    src/wifi/connect.cpp
    src/wifi/ip_monitor.cpp
    src/wifi/wifi_state_dispatcher.cpp
    # advertising
    src/adv/advertisement.cpp 
    
)

# ------------------------------------------------------------------------------
# Link
# ------------------------------------------------------------------------------

target_link_libraries(provision-ble
    ${GLIB_LIBRARIES}
    ${NM_LIBRARIES}
)

target_compile_options(provision-ble PRIVATE
    ${GLIB_CFLAGS_OTHER}
    ${NM_CFLAGS_OTHER}
)

# ------------------------------------------------------------------------------
# Install
# ------------------------------------------------------------------------------

install(TARGETS provision-ble
        RUNTIME DESTINATION /usr/local/sbin)
