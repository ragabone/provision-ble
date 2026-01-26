/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   Implementation of org.bluez.LEAdvertisement1 for provisioning.
 *
 * Notes:
 *   - Minimal advertisement for Milestone 4
 *   - Advertises local name + provisioning service UUID
 *   - Connectable advertisement
 *
 * Website:
 *   https://pidevelop.com
 *
 * Contact:
 *   james@pidevelop.com
 *
 * License:
 *   MIT License (see LICENSE file at repo root)
 */

#include "adv/advertisement.hpp"
#include "gatt/service.hpp"
#include "util/log.hpp"

#include <gio/gio.h>
#include <stdexcept>
#include <string>

namespace {

constexpr const char* ADV_PATH = "/org/bluez/provision/advertisement0";

// Introspection XML
const char* XML_ADV = R"XML(
<node>
  <interface name="org.bluez.LEAdvertisement1">
    <method name="Release"/>
    <property name="Type" type="s" access="read"/>
    <property name="ServiceUUIDs" type="as" access="read"/>
    <property name="LocalName" type="s" access="read"/>
    <property name="Includes" type="as" access="read"/>
    <property name="Flags" type="as" access="read"/>
  </interface>
</node>
)XML";

GVariant* on_get_property(GDBusConnection*,
                          const gchar*,
                          const gchar*,
                          const gchar*,
                          const gchar* prop,
                          GError**,
                          gpointer)
{
    const std::string p(prop);

    if (p == "Type")
        return g_variant_new_string("peripheral");

    // if (p == "LocalName")
    //     return g_variant_new_string("Provision-Setup");

    if (p == "ServiceUUIDs") {
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
        g_variant_builder_add(&b, "s", provision::gatt::SERVICE_UUID);
        return g_variant_builder_end(&b);
    }

    if (p == "Includes") {
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
        g_variant_builder_add(&b, "s", "tx-power");
        g_variant_builder_add(&b, "s", "local-name");
        return g_variant_builder_end(&b);
    }

    // This is the key addition for phone discoverability.
    // Most scanners expect Flags in the advertising payload.
    if (p == "Flags") {
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
        g_variant_builder_add(&b, "s", "general-discoverable");
        g_variant_builder_add(&b, "s", "le-only");
        return g_variant_builder_end(&b);
    }

    return nullptr;
}

void on_method_call(GDBusConnection*,
                    const gchar*,
                    const gchar*,
                    const gchar*,
                    const gchar* method,
                    GVariant*,
                    GDBusMethodInvocation* invocation,
                    gpointer)
{
    if (std::string(method) == "Release") {
        provision::log::info("Advertisement released by BlueZ");
        g_dbus_method_invocation_return_value(invocation, nullptr);
        return;
    }

    g_dbus_method_invocation_return_dbus_error(
        invocation,
        "org.freedesktop.DBus.Error.UnknownMethod",
        "Unknown method"
    );
}

const GDBusInterfaceVTable ADV_VTABLE = {
    on_method_call,
    on_get_property,
    nullptr,
    {0}
};

std::runtime_error make_error(const std::string& prefix, GError* err)
{
    std::string msg = prefix;
    msg += err && err->message ? err->message : "unknown error";
    if (err) g_error_free(err);
    return std::runtime_error(msg);
}

} // namespace

namespace provision::adv {

void set_ble_alias(GDBusConnection* bus,
                          const std::string& name)
{
    GError* err = nullptr;

    g_dbus_connection_call_sync(
        bus,
        "org.bluez",
        "/org/bluez/hci0",
        "org.freedesktop.DBus.Properties",
        "Set",
        g_variant_new(
            "(ssv)",
            "org.bluez.Adapter1",
            "Alias",
            g_variant_new_string(name.c_str())
        ),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &err
    );

    if (err) {
        provision::log::info(
            std::string("Failed to set BLE alias: ") + err->message);
        g_error_free(err);
    } else {
        provision::log::info(
            "BLE adapter alias set to '" + name + "'");
    }
}


void export_advertisement(GDBusConnection* system_bus)
{
    if (!system_bus)
        throw std::runtime_error("export_advertisement: system_bus is null");

    
    GError* err = nullptr;
    GDBusNodeInfo* node =
        g_dbus_node_info_new_for_xml(XML_ADV, &err);
    if (!node)
        throw make_error("Advertisement XML error: ", err);

    guint id = g_dbus_connection_register_object(
        system_bus,
        ADV_PATH,
        node->interfaces[0],
        &ADV_VTABLE,
        nullptr,
        nullptr,
        &err
    );

    g_dbus_node_info_unref(node);

    if (id == 0)
        throw make_error("Failed to export advertisement: ", err);

    provision::log::info("BLE advertisement exported");
}

} // namespace provision::adv
