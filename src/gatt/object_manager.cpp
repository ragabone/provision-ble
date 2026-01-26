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

#include "gatt/object_manager.hpp"
#include "gatt/service.hpp"
#include "util/log.hpp"

#include <stdexcept>
#include <string>

namespace {

constexpr const char* OM_IFACE = "org.freedesktop.DBus.ObjectManager";

const char* XML_OM = R"XML(
<node>
  <interface name="org.freedesktop.DBus.ObjectManager">
    <method name="GetManagedObjects">
      <arg name="objects" type="a{oa{sa{sv}}}" direction="out"/>
    </method>
  </interface>
</node>
)XML";

/**
 * Helper: create a{sv} properties for org.bluez.GattService1.
 *
 * Returns: a{sv}
 */
GVariant* make_service_props()
{
    // a{sv}
    GVariantBuilder props;
    g_variant_builder_init(&props, G_VARIANT_TYPE_VARDICT);

    g_variant_builder_add(&props, "{sv}", "UUID",
                          g_variant_new_string(provision::gatt::SERVICE_UUID));

    g_variant_builder_add(&props, "{sv}", "Primary",
                          g_variant_new_boolean(TRUE));

    // Includes: ao (empty list)
    GVariantBuilder includes;
    g_variant_builder_init(&includes, G_VARIANT_TYPE("ao"));
    GVariant* includes_v = g_variant_builder_end(&includes);

    g_variant_builder_add(&props, "{sv}", "Includes", includes_v);

    return g_variant_builder_end(&props);
}

/**
 * Helper: create a{sv} properties for org.bluez.GattCharacteristic1.
 *
 * Returns: a{sv}
 */
GVariant* make_char_props(const char* uuid,
                          const char* service_path,
                          const char* const* flags)
{
    // a{sv}
    GVariantBuilder props;
    g_variant_builder_init(&props, G_VARIANT_TYPE_VARDICT);

    g_variant_builder_add(&props, "{sv}", "UUID",
                          g_variant_new_string(uuid));

    g_variant_builder_add(&props, "{sv}", "Service",
                          g_variant_new_object_path(service_path));

    // Flags: as
    GVariantBuilder flags_b;
    g_variant_builder_init(&flags_b, G_VARIANT_TYPE("as"));
    for (int i = 0; flags && flags[i]; i++) {
        g_variant_builder_add(&flags_b, "s", flags[i]);
    }
    GVariant* flags_v = g_variant_builder_end(&flags_b);
    g_variant_builder_add(&props, "{sv}", "Flags", flags_v);

    // Descriptors: ao (empty list)
    GVariantBuilder desc;
    g_variant_builder_init(&desc, G_VARIANT_TYPE("ao"));
    GVariant* desc_v = g_variant_builder_end(&desc);
    g_variant_builder_add(&props, "{sv}", "Descriptors", desc_v);

    return g_variant_builder_end(&props);
}

void on_method_call(GDBusConnection*,
                    const gchar*,
                    const gchar*,
                    const gchar*,
                    const gchar* method_name,
                    GVariant*,
                    GDBusMethodInvocation* invocation,
                    gpointer)
{
    if (std::string(method_name) != "GetManagedObjects") {
        g_dbus_method_invocation_return_dbus_error(
            invocation,
            "org.freedesktop.DBus.Error.UnknownMethod",
            "Unknown method"
        );
        return;
    }

    provision::log::info("ObjectManager.GetManagedObjects called");

    // Build a{oa{sa{sv}}}
    GVariantBuilder objects;
    g_variant_builder_init(&objects, G_VARIANT_TYPE("a{oa{sa{sv}}}"));
    provision::log::info("OM: building object variant");

    // --- Service object ---
    {
        // a{sa{sv}}
        GVariantBuilder ifaces;
        g_variant_builder_init(&ifaces, G_VARIANT_TYPE("a{sa{sv}}"));

        g_variant_builder_add(&ifaces, "{s@a{sv}}",
                              "org.bluez.GattService1",
                              make_service_props());

        g_variant_builder_add(&objects, "{o@a{sa{sv}}}",
                              provision::gatt::SERVICE_PATH,
                              g_variant_builder_end(&ifaces));
    }

    // --- DeviceInfo characteristic (read) ---
    {
        static const char* flags[] = {"read", nullptr};

        GVariantBuilder ifaces;
        g_variant_builder_init(&ifaces, G_VARIANT_TYPE("a{sa{sv}}"));
        provision::log::info("OM: building ifaces variant");

        g_variant_builder_add(&ifaces, "{s@a{sv}}",
                              "org.bluez.GattCharacteristic1",
                              make_char_props(provision::gatt::UUID_DEVICEINFO,
                                              provision::gatt::SERVICE_PATH,
                                              flags));

        g_variant_builder_add(&objects, "{o@a{sa{sv}}}",
                              provision::gatt::CHR_DEVINFO,
                              g_variant_builder_end(&ifaces));
    }

    // --- State characteristic (read, notify) ---
    {
        static const char* flags[] = {"read", "notify", nullptr};

        GVariantBuilder ifaces;
        g_variant_builder_init(&ifaces, G_VARIANT_TYPE("a{sa{sv}}"));
        provision::log::info("OM: building ifaces notify variant");

        g_variant_builder_add(&ifaces, "{s@a{sv}}",
                              "org.bluez.GattCharacteristic1",
                              make_char_props(provision::gatt::UUID_STATE,
                                              provision::gatt::SERVICE_PATH,
                                              flags));

        g_variant_builder_add(&objects, "{o@a{sa{sv}}}",
                              provision::gatt::CHR_STATE,
                              g_variant_builder_end(&ifaces));
    }
    // --- Command characteristic (write) ---
    {
        static const char* flags[] = {"write", nullptr};

        GVariantBuilder ifaces;
        g_variant_builder_init(&ifaces, G_VARIANT_TYPE("a{sa{sv}}"));

        provision::log::info("OM: adding Command characteristic");

        g_variant_builder_add(&ifaces, "{s@a{sv}}",
                            "org.bluez.GattCharacteristic1",
                            make_char_props(provision::gatt::UUID_COMMAND,
                                            provision::gatt::SERVICE_PATH,
                                            flags));

        g_variant_builder_add(&objects, "{o@a{sa{sv}}}",
                            provision::gatt::CHR_COMMAND,
                            g_variant_builder_end(&ifaces));
    }

    provision::log::info("OM: building ifaces end variant");

    // Return as a single out arg in a tuple
    GVariant* out = g_variant_builder_end(&objects);
    g_dbus_method_invocation_return_value(invocation,
                                          g_variant_new_tuple(&out, 1));
}

const GDBusInterfaceVTable OM_VTABLE = {
    on_method_call,
    nullptr,
    nullptr,
    { 0 }
};

} // namespace

namespace provision::gatt {

void export_object_manager(GDBusConnection* system_bus)
{
    if (!system_bus) {
        throw std::runtime_error("export_object_manager: system_bus is null");
    }

    GError* err = nullptr;
    GDBusNodeInfo* node_info = g_dbus_node_info_new_for_xml(XML_OM, &err);
    if (!node_info) {
        const std::string msg = std::string("ObjectManager XML parse failed: ") +
                                (err ? err->message : "unknown error");
        if (err) g_error_free(err);
        throw std::runtime_error(msg);
    }

    guint reg_id = g_dbus_connection_register_object(
        system_bus,
        provision::gatt::APP_PATH,
        node_info->interfaces[0],
        &OM_VTABLE,
        nullptr,
        nullptr,
        &err
    );

    g_dbus_node_info_unref(node_info);

    if (reg_id == 0) {
        const std::string msg = std::string("Failed to export ObjectManager: ") +
                                (err ? err->message : "unknown error");
        if (err) g_error_free(err);
        throw std::runtime_error(msg);
    }

    provision::log::info(std::string("Exported ObjectManager at ") + provision::gatt::APP_PATH);
}

} // namespace provision::gatt
