#include "platform.h"
#include "files.h"
#include "trayicon.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>

namespace {
// GLib/GObject retain registered types and callbacks. Keep their modules loaded
// for the process lifetime; destroy every owned object before application exit.
struct GtkApi
{
    void* indicatorLib = nullptr;
    void* gtkLib = nullptr;
    void* objectLib = nullptr;
    void* glibLib = nullptr;
    void* (*indicatorNew)(const char*, const char*, int) = nullptr;
    void (*setStatus)(void*, int) = nullptr;
    void (*setMenu)(void*, void*) = nullptr;
    void (*setIcon)(void*, const char*, const char*) = nullptr;
    void (*setTitle)(void*, const char*) = nullptr;
    void (*setSecondary)(void*, void*) = nullptr;
    unsigned long (*indicatorType)() = nullptr;
    int (*gtkInit)(int*, char***) = nullptr;
    void* (*menuNew)() = nullptr;
    void* (*itemNew)(const char*) = nullptr;
    void* (*checkNew)(const char*) = nullptr;
    void* (*separatorNew)() = nullptr;
    void (*checkActive)(void*, int) = nullptr;
    void (*append)(void*, void*) = nullptr;
    void (*showAll)(void*) = nullptr;
    void (*sensitive)(void*, int) = nullptr;
    void (*destroy)(void*) = nullptr;
    void* (*refSink)(void*) = nullptr;
    void (*unref)(void*) = nullptr;
    unsigned long (*connect)(void*, const char*, void(*)(), void*, void(*)(void*, void*), unsigned) = nullptr;
    unsigned (*signalLookup)(const char*, unsigned long) = nullptr;
    unsigned (*disconnect)(void*, unsigned, unsigned, unsigned, void*, void*, void*) = nullptr;
    void (*objectGet)(void*, const char*, ...) = nullptr;
    int (*iterate)(void*, int) = nullptr;
    template<class T> bool symbol(void* module, T& target, const char* name) {
        target = module ? reinterpret_cast<T>(dlsym(module, name)) : nullptr; return target != nullptr;
    }
    bool load()
    {
        for (const char* name : {"libayatana-appindicator3.so.1", "libappindicator3.so.1"}) {
            indicatorLib = dlopen(name, RTLD_NOW | RTLD_LOCAL); if (indicatorLib) break;
        }
        if (!indicatorLib) return false;
        gtkLib = dlopen("libgtk-3.so.0", RTLD_NOW | RTLD_LOCAL);
        objectLib = dlopen("libgobject-2.0.so.0", RTLD_NOW | RTLD_LOCAL);
        glibLib = dlopen("libglib-2.0.so.0", RTLD_NOW | RTLD_LOCAL);
        const bool available =
            symbol(indicatorLib, indicatorNew, "app_indicator_new") &&
            symbol(indicatorLib, setStatus, "app_indicator_set_status") &&
            symbol(indicatorLib, setMenu, "app_indicator_set_menu") &&
            symbol(indicatorLib, setIcon, "app_indicator_set_icon_full") &&
            symbol(indicatorLib, indicatorType, "app_indicator_get_type") &&
            symbol(gtkLib, gtkInit, "gtk_init_check") &&
            symbol(gtkLib, menuNew, "gtk_menu_new") &&
            symbol(gtkLib, itemNew, "gtk_menu_item_new_with_label") &&
            symbol(gtkLib, checkNew, "gtk_check_menu_item_new_with_label") &&
            symbol(gtkLib, separatorNew, "gtk_separator_menu_item_new") &&
            symbol(gtkLib, checkActive, "gtk_check_menu_item_set_active") &&
            symbol(gtkLib, append, "gtk_menu_shell_append") &&
            symbol(gtkLib, showAll, "gtk_widget_show_all") &&
            symbol(gtkLib, sensitive, "gtk_widget_set_sensitive") &&
            symbol(gtkLib, destroy, "gtk_widget_destroy") &&
            symbol(objectLib, refSink, "g_object_ref_sink") &&
            symbol(objectLib, unref, "g_object_unref") &&
            symbol(objectLib, connect, "g_signal_connect_data") &&
            symbol(objectLib, signalLookup, "g_signal_lookup") &&
            symbol(objectLib, disconnect, "g_signal_handlers_disconnect_matched") &&
            symbol(objectLib, objectGet, "g_object_get") &&
            symbol(glibLib, iterate, "g_main_context_iteration");
        symbol(indicatorLib, setTitle, "app_indicator_set_title");
        symbol(indicatorLib, setSecondary, "app_indicator_set_secondary_activate_target");
        return available && gtkInit(nullptr, nullptr);
    }
};
GtkApi& gtk() { static GtkApi api; return api; }

struct GioApi
{
    void* module = dlopen("libgio-2.0.so.0", RTLD_NOW | RTLD_LOCAL);
    void* (*bus)(int, void*, void**) = nullptr;
    void* (*call)(void*, const char*, const char*, const char*, const char*, void*, const void*, int, int, void*, void**) = nullptr;
    void* (*variantNew)(const char*, ...) = nullptr;
    void* (*child)(void*, std::size_t) = nullptr;
    void* (*unbox)(void*) = nullptr;
    int (*classify)(void*) = nullptr;
    unsigned (*getUInt)(void*) = nullptr;
    void (*variantUnref)(void*) = nullptr;
    void (*unref)(void*) = nullptr;
    void (*freeError)(void*) = nullptr;
    bool loaded = false;
    GioApi() {
        if (!module) return;
        auto& api = gtk();
        loaded = api.symbol(module, bus, "g_bus_get_sync") && api.symbol(module, call, "g_dbus_connection_call_sync")
            && api.symbol(module, variantNew, "g_variant_new") && api.symbol(module, child, "g_variant_get_child_value")
            && api.symbol(module, unbox, "g_variant_get_variant") && api.symbol(module, classify, "g_variant_classify")
            && api.symbol(module, getUInt, "g_variant_get_uint32") && api.symbol(module, variantUnref, "g_variant_unref")
            && api.symbol(module, unref, "g_object_unref") && api.symbol(module, freeError, "g_error_free");
    }
};
}

struct Platform::DesktopIntegration::Data
{
    int lock = -1, socket = -1;
    bool first = true, ready = true, isolated = false, reveal = false, bound = false, connected = false;
    std::filesystem::path socketPath;
    void* indicator = nullptr;
    void* menu = nullptr;
    std::vector<int> actions;
    struct Action { Data* owner; int id; };
    static void activate(void*, void* payload) {
        auto* action = static_cast<Action*>(payload); action->owner->actions.push_back(action->id);
    }
    static void freeAction(void* payload, void*) { delete static_cast<Action*>(payload); }
    static void activated(void*, int, int, void* payload) { static_cast<Data*>(payload)->actions.push_back(-1); }
    static void connection(void*, int connected, void* payload) { static_cast<Data*>(payload)->connected = connected != 0; }
    void pump() {
        if (!indicator) return;
        // FLTK owns the application loop. Never enter a second blocking gtk_main.
        for (int i = 0; i < 32 && gtk().iterate(nullptr, 0); ++i) {}
    }
};

Platform::DesktopIntegration::DesktopIntegration(bool isolated) : data(std::make_unique<Data>())
{
    data->isolated = isolated; if (isolated) return;
    std::error_code ec; const auto directory = Files::configDirectory(); std::filesystem::create_directories(directory, ec);
    if (ec) { data->ready = false; return; }
    data->lock = ::open((directory / "instance-native.lock").c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (data->lock < 0) { data->ready = false; return; }
    if (flock(data->lock, LOCK_EX | LOCK_NB) < 0) {
        if (errno != EWOULDBLOCK) { data->ready = false; return; }
        data->first = false;
    }
    data->socketPath = directory / "instance-native.sock";
    sockaddr_un address{}; address.sun_family = AF_UNIX; const auto path = data->socketPath.string();
    if (path.size() >= sizeof(address.sun_path)) { data->ready = false; return; }
    std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
    data->socket = ::socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (data->socket < 0) { data->ready = false; return; }
    if (!data->first) {
        bool sent = false;
        for (int i = 0; i < 15 && !sent; ++i) {
            sent = ::sendto(data->socket, "show", 4, 0, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 4;
            if (!sent) ::usleep(100000);
        }
        data->ready = sent; return;
    }
    ::unlink(data->socketPath.c_str());
    if (::bind(data->socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) { data->ready = false; return; }
    data->bound = true;
    if (::chmod(data->socketPath.c_str(), 0600) < 0) { data->ready = false; return; }
    if (const auto* disabled = std::getenv("MOUSE_LITE_DISABLE_TRAY"); disabled && *disabled) return;
    static const bool available = gtk().load();
    if (!available) return;
    auto& api = gtk();
    data->indicator = api.indicatorNew("razer-mouse-lite", "input-mouse", 3);
    if (!data->indicator) return;
    api.connect(data->indicator, "connection-changed", reinterpret_cast<void(*)()>(Data::connection), data.get(), nullptr, 0);
    int connected = 0;
    api.objectGet(data->indicator, "connected", &connected, nullptr);
    data->connected = connected != 0;
    if (api.signalLookup("activate", api.indicatorType()))
        api.connect(data->indicator, "activate", reinterpret_cast<void(*)()>(Data::activated), data.get(), nullptr, 0);
}
Platform::DesktopIntegration::~DesktopIntegration()
{
    if (data->indicator) {
        gtk().setStatus(data->indicator, 0);
        gtk().disconnect(data->indicator, 16, 0, 0, nullptr, nullptr, data.get());
        gtk().unref(data->indicator);
        if (data->menu) { gtk().destroy(data->menu); gtk().unref(data->menu); }
    }
    if (data->socket >= 0) ::close(data->socket);
    if (data->bound) ::unlink(data->socketPath.c_str());
    if (data->lock >= 0) ::close(data->lock);
}
bool Platform::DesktopIntegration::primary() const { return data->first; }
bool Platform::DesktopIntegration::valid() const { return data->ready; }
bool Platform::DesktopIntegration::trayAvailable() const { return data->indicator && data->connected; }
bool Platform::DesktopIntegration::revealRequested() { const bool result = data->reveal; data->reveal = false; return result; }
void Platform::DesktopIntegration::setTray(const std::vector<TrayItem>& items, Color color, const std::string& tooltip)
{
    if (!data->indicator) return;
    auto& api = gtk(); void* menu = api.menuNew(); api.refSink(menu);
    void* secondary = nullptr;
    for (const auto& item : items) {
        void* widget = item.label.empty() ? api.separatorNew() : item.action >= 0 ? api.checkNew(item.label.c_str()) : api.itemNew(item.label.c_str());
        if (!item.label.empty()) {
            if (item.action >= 0) api.checkActive(widget, item.checked);
            api.sensitive(widget, item.enabled);
            auto* action = new Data::Action{data.get(), item.action};
            api.connect(widget, "activate", reinterpret_cast<void(*)()>(Data::activate), action, Data::freeAction, 0);
        }
        api.append(menu, widget);
        if (item.action == -2) secondary = widget;
    }
    api.showAll(menu); api.setMenu(data->indicator, menu);
    if (secondary && api.setSecondary) api.setSecondary(data->indicator, secondary);
    if (data->menu) { api.destroy(data->menu); api.unref(data->menu); }
    data->menu = menu;
    const auto directory = Files::configDirectory() / "tray-icons";
    const auto icon = directory / ("badge-v1-" + color.hex().substr(1) + ".svg");
    std::error_code ec;
    bool available = std::filesystem::exists(icon, ec);
    if (!available) {
        const auto svg = profileIconSvg(color);
        available = Files::atomicWrite(icon, svg);
    }
    const auto description = tooltip.substr(0, tooltip.find('\n'));
    api.setIcon(data->indicator, available ? icon.c_str() : "input-mouse", description.c_str());
    if (api.setTitle) api.setTitle(data->indicator, "Razer Mouse Lite");
    api.setStatus(data->indicator, 1);
}
void Platform::DesktopIntegration::poll(const std::function<void(int)>& action)
{
    if (data->socket >= 0 && data->first) {
        char buffer[16]; ssize_t count;
        while ((count = ::recv(data->socket, buffer, sizeof(buffer), 0)) > 0)
            if (count == 4 && std::memcmp(buffer, "show", 4) == 0) data->reveal = true;
    }
    data->pump();
    const auto actions = std::move(data->actions); data->actions.clear();
    for (int selected : actions) action(selected);
}
bool Platform::systemDark()
{
    static GioApi gio;
    if (!gio.loaded) return false;
    void* error = nullptr;
    void* connection = gio.bus(2, nullptr, &error);
    if (!connection) { if (error) gio.freeError(error); return false; }
    void* result = gio.call(connection, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Settings", "Read", gio.variantNew("(ss)", "org.freedesktop.appearance", "color-scheme"),
        "(v)", 0, 1000, nullptr, &error);
    gio.unref(connection);
    if (!result) { if (error) gio.freeError(error); return false; }
    void* value = gio.child(result, 0); gio.variantUnref(result);
    while (gio.classify(value) == 'v') {
        void* next = gio.unbox(value); gio.variantUnref(value); value = next;
    }
    const bool dark = gio.classify(value) == 'u' && gio.getUInt(value) == 1;
    gio.variantUnref(value); return dark;
}
