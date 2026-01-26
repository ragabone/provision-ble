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

#include "dbus/bluez_client.hpp"
#include "util/log.hpp"

#include <stdexcept>
#include <string>
#include <memory>

namespace {

constexpr const char* BLUEZ_BUS = "org.bluez";
constexpr const char* OM_IFACE  = "org.freedesktop.DBus.ObjectManager";
constexpr const char* GATT_MGR_IFACE = "org.bluez.GattManager1";
constexpr const char* ADV_MGR_IFACE  = "org.bluez.LEAdvertisingManager1";

bool has_interface(GVariant* iface_dict, const char* iface_name)
{
    GVariantIter iter;
    const char* key = nullptr;
    GVariant* value = nullptr;

    g_variant_iter_init(&iter, iface_dict);
    while (g_variant_iter_next(&iter, "{&s@a{sv}}", &key, &value)) {
        bool match = (std::string(key) == iface_name);
        g_variant_unref(value);
        if (match) return true;
    }
    return false;
}

std::runtime_error make_error(const std::string& prefix, GError* err)
{
    std::string msg = prefix;
    msg += (err && err->message) ? err->message : "unknown error";
    if (err) g_error_free(err);
    return std::runtime_error(msg);
}

struct AsyncCtx {
    provision::bluez::RegisterCallback cb;
};

void on_async_call_finished(GObject* source_object,
                            GAsyncResult* res,
                            gpointer user_data)
{
    std::unique_ptr<AsyncCtx> ctx(static_cast<AsyncCtx*>(user_data));

    GError* err = nullptr;
    GVariant* reply = g_dbus_connection_call_finish(
        G_DBUS_CONNECTION(source_object),
        res,
        &err
    );

    if (reply) g_variant_unref(reply);

    if (err) {
        std::string msg = err->message ? err->message : "unknown error";
        g_error_free(err);
        ctx->cb(false, msg);
        return;
    }

    ctx->cb(true, {});
}

} // namespace

namespace provision::bluez {

AdapterPaths find_adapter(GDBusConnection* system_bus)
{
    if (!system_bus) {
        throw std::runtime_error("find_adapter: system_bus is null");
    }

    GError* err = nullptr;

    GVariant* reply = g_dbus_connection_call_sync(
        system_bus,
        BLUEZ_BUS,
        "/",
        OM_IFACE,
        "GetManagedObjects",
        nullptr,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &err
    );

    if (!reply) throw make_error("GetManagedObjects failed: ", err);

    GVariant* objects = nullptr;
    g_variant_get(reply, "(@a{oa{sa{sv}}})", &objects);

    AdapterPaths result{};
    GVariantIter outer;
    const char* obj_path = nullptr;
    GVariant* iface_dict = nullptr;

    g_variant_iter_init(&outer, objects);
    while (g_variant_iter_next(&outer, "{&o@a{sa{sv}}}", &obj_path, &iface_dict)) {
        bool has_gatt = has_interface(iface_dict, GATT_MGR_IFACE);
        bool has_adv  = has_interface(iface_dict, ADV_MGR_IFACE);
        g_variant_unref(iface_dict);

        if (has_gatt && has_adv) {
            result.adapter_path = obj_path;
            break;
        }
    }

    g_variant_unref(objects);
    g_variant_unref(reply);

    if (result.adapter_path.empty()) {
        throw std::runtime_error("No adapter found exposing GattManager1 and LEAdvertisingManager1");
    }

    provision::log::info("BlueZ adapter selected: " + result.adapter_path);
    return result;
}

/* ---------- Sync ---------- */

void register_gatt_application(GDBusConnection* system_bus,
                               const std::string& adapter_path,
                               const std::string& app_path)
{
    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);

    GVariant* args = g_variant_new(
        "(o@a{sv})",
        app_path.c_str(),
        g_variant_builder_end(&options)
    );

    GError* err = nullptr;
    GVariant* reply = g_dbus_connection_call_sync(
        system_bus,
        BLUEZ_BUS,
        adapter_path.c_str(),
        GATT_MGR_IFACE,
        "RegisterApplication",
        args,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &err
    );

    if (!reply) throw make_error("RegisterApplication failed: ", err);
    g_variant_unref(reply);
}

void register_advertisement(GDBusConnection* system_bus,
                            const std::string& adapter_path,
                            const std::string& adv_path)
{
    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);

    GVariant* args = g_variant_new(
        "(o@a{sv})",
        adv_path.c_str(),
        g_variant_builder_end(&options)
    );

    GError* err = nullptr;
    GVariant* reply = g_dbus_connection_call_sync(
        system_bus,
        BLUEZ_BUS,
        adapter_path.c_str(),
        ADV_MGR_IFACE,
        "RegisterAdvertisement",
        args,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &err
    );

    if (!reply) throw make_error("RegisterAdvertisement failed: ", err);
    g_variant_unref(reply);
}

/* ---------- Async ---------- */

void register_gatt_application_async(GDBusConnection* system_bus,
                                     const std::string& adapter_path,
                                     const std::string& app_path,
                                     RegisterCallback cb)
{
    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);

    GVariant* args = g_variant_new(
        "(o@a{sv})",
        app_path.c_str(),
        g_variant_builder_end(&options)
    );

    auto* ctx = new AsyncCtx{std::move(cb)};

    g_dbus_connection_call(
        system_bus,
        BLUEZ_BUS,
        adapter_path.c_str(),
        GATT_MGR_IFACE,
        "RegisterApplication",
        args,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        on_async_call_finished,
        ctx
    );
}

void register_advertisement_async(GDBusConnection* system_bus,
                                  const std::string& adapter_path,
                                  const std::string& adv_path,
                                  RegisterCallback cb)
{
    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);

    GVariant* args = g_variant_new(
        "(o@a{sv})",
        adv_path.c_str(),
        g_variant_builder_end(&options)
    );

    auto* ctx = new AsyncCtx{std::move(cb)};

    g_dbus_connection_call(
        system_bus,
        BLUEZ_BUS,
        adapter_path.c_str(),
        ADV_MGR_IFACE,
        "RegisterAdvertisement",
        args,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        on_async_call_finished,
        ctx
    );
}

} // namespace provision::bluez
