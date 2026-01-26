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

#include "gatt/service.hpp"
#include "util/log.hpp"

#include <gio/gio.h>
#include <stdexcept>
#include <string>

namespace {

// Introspection for org.bluez.GattService1
// We export:
//  - UUID (string)
//  - Primary (bool)
//  - Includes (array of object paths, empty for MVP)
const char* XML_SERVICE = R"XML(
<node>
  <interface name="org.bluez.GattService1">
    <property name="UUID" type="s" access="read"/>
    <property name="Primary" type="b" access="read"/>
    <property name="Includes" type="ao" access="read"/>
  </interface>
</node>
)XML";

/**
 * Property getter callback for org.bluez.GattService1.
 *
 * BlueZ reads these properties to understand the service definition.
 */
GVariant* on_get_property(GDBusConnection*,
                         const gchar*,
                         const gchar*,
                         const gchar*,
                         const gchar* property_name,
                         GError**,
                         gpointer)
{
    const std::string prop(property_name);

    if (prop == "UUID") {
        return g_variant_new_string(provision::gatt::SERVICE_UUID);
    }

    if (prop == "Primary") {
        return g_variant_new_boolean(TRUE);
    }

    if (prop == "Includes") {
        // Empty list of included services for MVP.
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("ao"));
        return g_variant_builder_end(&b);
    }

    // Unknown property (should not happen)
    return nullptr;
}

const GDBusInterfaceVTable SERVICE_VTABLE = {
    nullptr,        // method_call (none)
    on_get_property,
    nullptr,         // set_property (none)
    { 0 }
};

std::runtime_error make_error(const std::string& prefix, GError* err)
{
    std::string msg = prefix;
    msg += (err && err->message) ? err->message : "unknown error";
    if (err) g_error_free(err);
    return std::runtime_error(msg);
}

} // namespace

namespace provision::gatt {

void export_service(GDBusConnection* system_bus)
{
    if (!system_bus) {
        throw std::runtime_error("export_service: system_bus is null");
    }

    GError* err = nullptr;
    GDBusNodeInfo* node_info = g_dbus_node_info_new_for_xml(XML_SERVICE, &err);
    if (!node_info) {
        throw make_error("GattService XML parse failed: ", err);
    }

    // Export the service object on D-Bus at SERVICE_PATH.
    // BlueZ will inspect it as part of the registered application.
    guint reg_id = g_dbus_connection_register_object(
        system_bus,
        provision::gatt::SERVICE_PATH,
        node_info->interfaces[0],
        &SERVICE_VTABLE,
        nullptr,
        nullptr,
        &err
    );

    g_dbus_node_info_unref(node_info);

    if (reg_id == 0) {
        throw make_error("Failed to export GattService1 object: ", err);
    }

    provision::log::info(std::string("Exported GattService1 at ") + provision::gatt::SERVICE_PATH);
}

} // namespace provision::gatt
