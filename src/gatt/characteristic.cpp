/*
 * Project: provision (BLE Provisioning for Raspberry Pi)
 *
 * Description:
 *   Base GATT characteristic implementation for BlueZ.
 *   Supports ReadValue, StartNotify/StopNotify, WriteValue, and emitting
 *   notifications via PropertiesChanged on the Value property.
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

#include "gatt/characteristic.hpp"
#include "util/log.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

struct CharContext {
    // Identity
    std::string uuid;
    std::string object_path;
    std::string service_path;
    const char* const* flags;

    // Callbacks
    provision::gatt::ReadCallback read_cb;
    provision::gatt::NotifyStateCallback notify_cb;
    provision::gatt::WriteCallback write_cb;

    // Runtime
    GDBusConnection* system_bus{nullptr}; // not owned
    bool notifying{false};

    // Cached Value property ("ay") used for notifications
    GVariant* value_ay{nullptr};
};

// Track characteristics so State can emit notifications by object_path
static std::unordered_map<std::string, CharContext*> g_chars;

/**
 * Introspection XML for org.bluez.GattCharacteristic1.
 *
 * We expose:
 *  - ReadValue (optional)
 *  - WriteValue (optional)
 *  - StartNotify / StopNotify
 *  - Properties UUID/Service/Flags/Value
 *
 * Note: Value property is included so BlueZ can observe it changing via
 * PropertiesChanged and forward it as an ATT notification.
 */
const char* XML_CHAR = R"XML(
<node>
  <interface name="org.bluez.GattCharacteristic1">
    <method name="ReadValue">
      <arg name="options" type="a{sv}" direction="in"/>
      <arg name="value" type="ay" direction="out"/>
    </method>
    <method name="WriteValue">
      <arg name="value" type="ay" direction="in"/>
      <arg name="options" type="a{sv}" direction="in"/>
    </method>
    <method name="StartNotify"/>
    <method name="StopNotify"/>
    <property name="UUID" type="s" access="read"/>
    <property name="Service" type="o" access="read"/>
    <property name="Flags" type="as" access="read"/>
    <property name="Value" type="ay" access="read"/>
  </interface>
</node>
)XML";

/**
 * Return an empty "ay".
 */
GVariant* empty_ay()
{
    static GVariant* cached = nullptr;

    if (!cached) {
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("ay"));
        cached = g_variant_builder_end(&b);
        cached = g_variant_ref_sink(cached);
    }

    return g_variant_ref(cached);
}


// Property getter
GVariant* on_get_property(GDBusConnection*,
                          const gchar*,
                          const gchar*,
                          const gchar*,
                          const gchar* prop,
                          GError**,
                          gpointer user_data)
{
    auto* ctx = static_cast<CharContext*>(user_data);

    if (std::string(prop) == "UUID")
        return g_variant_new_string(ctx->uuid.c_str());

    if (std::string(prop) == "Service")
        return g_variant_new_object_path(ctx->service_path.c_str());

    if (std::string(prop) == "Flags") {
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
        for (int i = 0; ctx->flags && ctx->flags[i]; ++i)
            g_variant_builder_add(&b, "s", ctx->flags[i]);
        return g_variant_builder_end(&b);
    }

    // Cached Value used for notifications. If never set, return empty.
    if (std::string(prop) == "Value") {
        if (ctx->value_ay)
            return g_variant_ref(ctx->value_ay);
        return empty_ay();
    }

    return nullptr;
}

/**
 * Emit PropertiesChanged for org.bluez.GattCharacteristic1 Value.
 *
 * BlueZ listens for this and converts it to an ATT Handle Value Notification
 * when notifications are enabled on the characteristic.
 */
void emit_value_changed(CharContext* ctx)
{
    if (!ctx || !ctx->system_bus) {
        provision::log::error("emit_value_changed: ctx or system_bus is null");
        return;
    }

    

    if (!ctx->value_ay) {
        provision::log::warn("emit_value_changed: value_ay is null, using empty_ay()");
        ctx->value_ay = empty_ay();
        g_variant_ref_sink(ctx->value_ay);
    }

    

    // -------------------------
    // Build changed (a{sv})
    // -------------------------
    GVariantBuilder changed;
    g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));

    
    g_variant_builder_add(
        &changed,
        "{sv}",
        "Value",
        g_variant_ref(ctx->value_ay)
    );

    GVariant* changed_v = g_variant_builder_end(&changed);
    if (!changed_v) {
        provision::log::error("emit_value_changed: changed_v is NULL");
        return;
    }

    

    // -------------------------
    // Build invalidated (as)
    // -------------------------
    GVariantBuilder invalidated;
    g_variant_builder_init(&invalidated, G_VARIANT_TYPE("as"));

    GVariant* invalidated_v = g_variant_builder_end(&invalidated);
    if (!invalidated_v) {
        provision::log::error("emit_value_changed: invalidated_v is NULL");
        g_variant_unref(changed_v);
        return;
    }

    

    // -------------------------
    // Emit signal
    // -------------------------
    

    if (!changed_v) {
    provision::log::error("emit_value_changed: changed_v is NULL");
    return;
}
if (!invalidated_v) {
    provision::log::error("emit_value_changed: invalidated_v is NULL");
    return;
}



if (!g_variant_is_of_type(changed_v, G_VARIANT_TYPE("a{sv}"))) {
    provision::log::error("emit_value_changed: changed_v is not a{sv}");
    return;
}
if (!g_variant_is_of_type(invalidated_v, G_VARIANT_TYPE("as"))) {
    provision::log::error("emit_value_changed: invalidated_v is not as");
    return;
}


    g_dbus_connection_emit_signal(
        ctx->system_bus,
        nullptr,
        ctx->object_path.c_str(),
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        g_variant_new(
            "(s@a{sv}@as)",
            "org.bluez.GattCharacteristic1",
            changed_v,
            invalidated_v),
        nullptr
    );
}



std::runtime_error make_error(const std::string& prefix, GError* err)
{
    std::string msg = prefix;
    msg += err && err->message ? err->message : "unknown error";
    if (err) g_error_free(err);
    return std::runtime_error(msg);
}

// Method handler
void on_method_call(GDBusConnection*,
                    const gchar*,
                    const gchar*,
                    const gchar*,
                    const gchar* method,
                    GVariant* parameters,
                    GDBusMethodInvocation* invocation,
                    gpointer user_data)
{
    auto* ctx = static_cast<CharContext*>(user_data);

    if (std::string(method) == "ReadValue") {
        if (!ctx->read_cb) {
            g_dbus_method_invocation_return_dbus_error(
                invocation,
                "org.bluez.Error.Failed",
                "Read not supported"
            );
            return;
        }

        // ReadValue remains callback-driven (no behavior change).
        GVariant* value = ctx->read_cb();
        g_dbus_method_invocation_return_value(
            invocation,
            g_variant_new_tuple(&value, 1)
        );
        return;
    }

    if (std::string(method) == "WriteValue") {
        if (!ctx->write_cb) {
            g_dbus_method_invocation_return_dbus_error(
                invocation,
                "org.bluez.Error.Failed",
                "Write not supported"
            );
            return;
        }

        // Signature: WriteValue(ay value, a{sv} options)
        // Parameters arrive as tuple "(aya{sv})".
        GVariant* value_ay = nullptr;
        GVariant* options = nullptr;

        g_variant_get(parameters, "(@ay@a{sv})", &value_ay, &options);

        // We do not use options for MVP, but we must unref them.
        if (value_ay)
            ctx->write_cb(value_ay);

        if (value_ay)
            g_variant_unref(value_ay);
        if (options)
            g_variant_unref(options);

        g_dbus_method_invocation_return_value(invocation, nullptr);
        return;
    }

    if (std::string(method) == "StartNotify") {
        ctx->notifying = true;
       
        if (ctx->notify_cb)
            ctx->notify_cb(true);
        g_dbus_method_invocation_return_value(invocation, nullptr);
        return;
    }

    if (std::string(method) == "StopNotify") {
        ctx->notifying = false;
        
        if (ctx->notify_cb)
            ctx->notify_cb(false);
        g_dbus_method_invocation_return_value(invocation, nullptr);
        return;
    }

    g_dbus_method_invocation_return_dbus_error(
        invocation,
        "org.freedesktop.DBus.Error.UnknownMethod",
        "Unknown method"
    );
}

const GDBusInterfaceVTable CHAR_VTABLE = {
    on_method_call,
    on_get_property,
    nullptr,
    {0}
};

} // namespace

namespace provision::gatt {

void export_characteristic(GDBusConnection* system_bus,
                           const std::string& uuid,
                           const std::string& object_path,
                           const std::string& service_path,
                           const char* const* flags,
                           ReadCallback read_cb,
                           NotifyStateCallback notify_cb,
                           WriteCallback write_cb)
{
    GError* err = nullptr;

    auto* ctx = new CharContext{
        uuid,
        object_path,
        service_path,
        flags,
        read_cb,
        notify_cb,
        write_cb,
        system_bus
    };

    // Initialize Value cache to the current read value if available.
    // This helps keep Value property sensible even before first notify.
    if (ctx->read_cb) {
        ctx->value_ay = ctx->read_cb(); // expected "ay"
        g_variant_ref_sink(ctx->value_ay);
    }

    GDBusNodeInfo* node = g_dbus_node_info_new_for_xml(XML_CHAR, &err);
    if (!node)
        throw make_error("Characteristic XML error: ", err);

    guint id = g_dbus_connection_register_object(
        system_bus,
        object_path.c_str(),
        node->interfaces[0],
        &CHAR_VTABLE,
        ctx,
        nullptr,
        &err
    );

    g_dbus_node_info_unref(node);

    if (id == 0)
        throw make_error("Failed to export characteristic: ", err);

    // Register in lookup table for notifications by object_path
    g_chars[object_path] = ctx;

    
}

void notify_characteristic_value(const std::string& object_path, GVariant* value_ay)
{
    auto it = g_chars.find(object_path);
    if (it == g_chars.end()) {
        provision::log::warn("notify: characteristic not found for " + object_path);
        return;
    }

    CharContext* ctx = it->second;
    if (!ctx->notifying) {
        provision::log::info("notify: skipped (not notifying) for " + object_path);
        return;
    }

    if (!value_ay) {
        provision::log::warn("notify: null value for " + object_path);
        return;
    }

    if (ctx->value_ay)
        g_variant_unref(ctx->value_ay);

    // Sink the variant so lifetime is stable after caller returns
    ctx->value_ay = g_variant_ref_sink(value_ay);

    provision::log::info("notify: emitting Value change for " + object_path);
    emit_value_changed(ctx);
}

} // namespace provision::gatt
