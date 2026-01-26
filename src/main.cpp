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

#include <gio/gio.h>
#include <stdexcept>
#include <string>

#include "util/log.hpp"
#include "dbus/bluez_client.hpp"

#include "gatt/object_manager.hpp"
#include "gatt/service.hpp"
#include "gatt/device_info.hpp"
#include "gatt/state.hpp"
#include "gatt/command.hpp"

#include "adv/advertisement.hpp"
#include "wifi/ip_monitor.hpp"
#include "wifi/wifi_state_dispatcher.hpp"



namespace {

constexpr const char* ADV_PATH = "/org/bluez/provision/advertisement0";

struct RegistrationCtx {
    GDBusConnection* bus;
    std::string adapter_path;
};

// Helper to ensure we free ctx exactly once.
void free_ctx(RegistrationCtx* ctx)
{
    delete ctx;
}

gboolean start_async_registration(gpointer user_data)
{
    auto* ctx = static_cast<RegistrationCtx*>(user_data);

    provision::bluez::register_gatt_application_async(
        ctx->bus,
        ctx->adapter_path,
        provision::gatt::APP_PATH,
        [ctx](bool ok, const std::string& err) {
            if (!ok) {
                provision::log::error("RegisterApplication failed: " + err);
                free_ctx(ctx);
                return;
            }

            provision::log::info("GATT application registered");

            provision::bluez::register_advertisement_async(
                ctx->bus,
                ctx->adapter_path,
                ADV_PATH,
                [ctx](bool ok2, const std::string& err2) {
                    if (!ok2) {
                        provision::log::error("RegisterAdvertisement failed: " + err2);
                        free_ctx(ctx);
                        return;
                    }

                    provision::log::info("Advertisement registered");
                    free_ctx(ctx);
                }
            );
        }
    );

    // Run once; DO NOT free ctx here. It must outlive async callbacks.
    return G_SOURCE_REMOVE;
}

} // namespace

int main()
{
    provision::log::init("/var/log/provision/ble.log");
    provision::log::info("provision-ble starting (Milestone 4)");

    GError* err = nullptr;
    GDBusConnection* bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &err);
    if (!bus) {
        provision::log::error(std::string("Failed to connect to system D-Bus: ") +
                              (err ? err->message : "unknown error"));
        if (err) g_error_free(err);
        return 1;
    }
    // Set BLE pairing name BEFORE advertising
    provision::adv::set_ble_alias(bus, "PiDevelopDotcom");
    provision::wifi::start_ip_monitor();
    provision::wifi::init_wifi_state_dispatcher();
    try {
        // 1) Export objects
        provision::gatt::export_object_manager(bus);
        provision::gatt::export_service(bus);
        provision::gatt::export_device_info(bus);
        provision::gatt::export_state(bus);
        provision::gatt::export_command(bus);
        provision::adv::export_advertisement(bus);
        


        // 2) Find adapter
        auto adapter = provision::bluez::find_adapter(bus);

        // 3) Main loop first
        GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

        // IMPORTANT: ctx must survive until both async registrations finish.
        auto* reg_ctx = new RegistrationCtx{
            bus,
            adapter.adapter_path
        };

        // Schedule registration. No destroy-notify here.
        g_idle_add(start_async_registration, reg_ctx);

        provision::log::info("Entering main loop");
        g_main_loop_run(loop);

        g_main_loop_unref(loop);
    }
    catch (const std::exception& ex) {
        provision::log::error(std::string("Fatal error: ") + ex.what());
        g_object_unref(bus);
        return 1;
    }

    g_object_unref(bus);
    return 0;
}
