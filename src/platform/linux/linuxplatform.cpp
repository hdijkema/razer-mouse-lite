// SPDX-License-Identifier: GPL-2.0-or-later
#include "platform.h"
#include "razerprotocol.h"
#include "hiddescriptor.h"
#include "desktopentry.h"
#include "files.h"
#include "i18n.h"
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <libudev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
#include <spawn.h>
#include <signal.h>
#include <curl/curl.h>
#include <chrono>
#include <mutex>
#include <regex>
#include <sstream>
#include <thread>
extern char** environ;
namespace {
std::string systemError() { return std::strerror(errno); }
std::string attr(udev_device* device, const char* name)
{
    const char* value = device ? udev_device_get_sysattr_value(device, name) : nullptr;
    return value ? value : "";
}
int hex(const std::string& value) { try { return std::stoi(value, nullptr, 16); } catch (...) { return -1; } }
std::filesystem::path autostartFile()
{
    const auto* xdg = std::getenv("XDG_CONFIG_HOME"); const auto* home = std::getenv("HOME");
    const auto base = xdg && std::filesystem::path(xdg).is_absolute() ? std::filesystem::path(xdg)
        : std::filesystem::path(home ? home : "") / ".config";
    return base / "autostart/mouse-lite.desktop";
}
std::string findXinput()
{
    const auto* path = std::getenv("PATH"); std::istringstream paths(path ? path : ""); std::string folder;
    while (std::getline(paths, folder, ':')) {
        if (folder.empty()) continue;
        const auto file = std::filesystem::path(folder) / "xinput";
        if (::access(file.c_str(), X_OK) == 0) return file.string();
    }
    return {};
}
bool xinput(const std::vector<std::string>& arguments, std::string* output = nullptr)
{
    const auto executable = findXinput(); if (executable.empty()) return false;
    std::vector<std::string> strings{executable}; strings.insert(strings.end(), arguments.begin(), arguments.end());
    std::vector<char*> argv; for (auto& arg : strings) argv.push_back(arg.data()); argv.push_back(nullptr);
    std::vector<std::string> envStrings;
    for (auto** entry = environ; *entry; ++entry) if (std::strncmp(*entry, "LC_ALL=", 7) != 0) envStrings.emplace_back(*entry);
    envStrings.emplace_back("LC_ALL=C"); std::vector<char*> environment;
    for (auto& entry : envStrings) environment.push_back(entry.data()); environment.push_back(nullptr);
    int pipefd[2]; if (pipe2(pipefd, O_CLOEXEC) != 0) return false;
    posix_spawn_file_actions_t actions; posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addclose(&actions, pipefd[0]); posix_spawn_file_actions_addclose(&actions, pipefd[1]);
    pid_t child = 0;
    const int spawned = posix_spawn(&child, executable.c_str(), &actions, nullptr, argv.data(), environment.data());
    posix_spawn_file_actions_destroy(&actions); ::close(pipefd[1]);
    if (spawned != 0) { ::close(pipefd[0]); return false; }
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    std::string bytes; int status = 0; bool done = false, timedOut = false;
    while (!done) {
        char buffer[4096]; ssize_t count;
        while ((count = ::read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            bytes.append(buffer, static_cast<std::size_t>(count));
            if (bytes.size() > 1024 * 1024) { timedOut = true; break; }
        }
        if (waitpid(child, &status, WNOHANG) == child) done = true;
        else if (timedOut || std::chrono::steady_clock::now() >= deadline) {
            ::kill(child, SIGKILL); while (waitpid(child, &status, 0) < 0 && errno == EINTR) {} timedOut = true; done = true;
        } else { pollfd fd{pipefd[0], POLLIN, 0}; ::poll(&fd, 1, 20); }
    }
    char buffer[4096]; ssize_t count;
    while ((count = ::read(pipefd[0], buffer, sizeof(buffer))) > 0 && bytes.size() <= 1024 * 1024) bytes.append(buffer, static_cast<std::size_t>(count));
    ::close(pipefd[0]);
    if (output) *output = std::move(bytes);
    return !timedOut && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
class LinuxChannel final : public FeatureChannel
{
public:
    explicit LinuxChannel(int fd) : descriptor(fd) {}
    ~LinuxChannel() override { ::close(descriptor); }
    bool send(const Bytes& packet, std::string* error) override
    {
        int count;
        do { count = ioctl(descriptor, HIDIOCSFEATURE(packet.size()), packet.data()); } while (count < 0 && errno == EINTR);
        if (count == static_cast<int>(packet.size())) return true;
        if (error) *error = count < 0 ? systemError() : tr("Platform", "Incomplete HID transfer."); return false;
    }
    Bytes receive(int length, std::string* error) override
    {
        Bytes packet(length, 0); int count;
        do { count = ioctl(descriptor, HIDIOCGFEATURE(length), packet.data()); } while (count < 0 && errno == EINTR);
        if (count == length) return packet;
        if (error) *error = count < 0 ? systemError() : tr("Platform", "Incomplete HID transfer."); return {};
    }
private: int descriptor;
};
}
std::vector<MouseDevice> Platform::enumerate()
{
    std::vector<MouseDevice> result;
    auto* context = udev_new(); if (!context) return result;
    auto* enumeration = udev_enumerate_new(context); if (!enumeration) { udev_unref(context); return result; }
    udev_enumerate_add_match_subsystem(enumeration, "hidraw"); udev_enumerate_scan_devices(enumeration);
    const auto database = RazerProtocol::models(); udev_list_entry* entry;
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(enumeration)) {
        auto* raw = udev_device_new_from_syspath(context, udev_list_entry_get_name(entry)); if (!raw) continue;
        auto* usb = udev_device_get_parent_with_subsystem_devtype(raw, "usb", "usb_device");
        auto* interface = udev_device_get_parent_with_subsystem_devtype(raw, "usb", "usb_interface");
        auto* hid = udev_device_get_parent_with_subsystem_devtype(raw, "hid", nullptr);
        const char* node = udev_device_get_devnode(raw);
        if (node && usb && interface && hid && hex(attr(usb, "idVendor")) == 0x1532) {
            const int pid = hex(attr(usb, "idProduct")); Json model;
            for (const auto& item : database) if (number(item, "pid") == pid) { model = item; break; }
            if (!model.is_null() && hex(attr(interface, "bInterfaceNumber")) == number(model, "interface")) {
                const auto hidPath = std::filesystem::path(udev_device_get_syspath(hid)); std::string bytes;
                if (Files::read(hidPath / "report_descriptor", bytes, 16384)
                    && HidDescriptor::featureReportLength(Bytes(bytes.begin(), bytes.end())) == 91) {
                    std::string inputPath; std::error_code ec;
                    std::filesystem::recursive_directory_iterator files(hidPath / "input", std::filesystem::directory_options::skip_permission_denied, ec), end;
                    for (; !ec && files != end; files.increment(ec)) {
                        if (files->path().filename().string().rfind("event", 0) == 0) { inputPath = "/dev/input/" + files->path().filename().string(); break; }
                    }
                    result.push_back({node, text(model, "name"), model, pid, inputPath});
                }
            }
        }
        udev_device_unref(raw);
    }
    udev_enumerate_unref(enumeration); udev_unref(context); return result;
}
std::unique_ptr<FeatureChannel> Platform::openControl(const MouseDevice& device, std::string* error)
{
    const int fd = ::open(device.path.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0) { if (error) *error = tr("Platform", "Cannot access %1: %2. Install the supplied udev rule and reconnect the mouse.", device.path, systemError()); return {}; }
    hidraw_devinfo info{}; struct stat stat{}; bool valid = false;
    auto* context = udev_new();
    if (context && fstat(fd, &stat) == 0) {
        auto* raw = udev_device_new_from_devnum(context, 'c', stat.st_rdev);
        auto* interface = raw ? udev_device_get_parent_with_subsystem_devtype(raw, "usb", "usb_interface") : nullptr;
        valid = interface && hex(attr(interface, "bInterfaceNumber")) == number(device.model, "interface");
        if (raw) udev_device_unref(raw);
    }
    if (context) udev_unref(context);
    int descriptorSize = 0; hidraw_report_descriptor descriptor{};
    valid = valid && ioctl(fd, HIDIOCGRAWINFO, &info) == 0 && static_cast<std::uint16_t>(info.vendor) == 0x1532 && static_cast<std::uint16_t>(info.product) == device.pid
        && ioctl(fd, HIDIOCGRDESCSIZE, &descriptorSize) == 0 && descriptorSize > 0 && descriptorSize <= HID_MAX_DESCRIPTOR_SIZE;
    if (valid) {
        descriptor.size = static_cast<__u32>(descriptorSize);
        valid = ioctl(fd, HIDIOCGRDESC, &descriptor) == 0 && HidDescriptor::featureReportLength(Bytes(descriptor.value, descriptor.value + descriptorSize)) == 91;
    }
    if (!valid) { ::close(fd); if (error) *error = tr("Platform", "Mouse identity changed. Click Refresh and try again."); return {}; }
    return std::make_unique<LinuxChannel>(fd);
}
bool Platform::startupEnabled()
{
    std::string content;
    return Files::read(autostartFile(), content, 65536) && content.find("Hidden=true") == std::string::npos;
}
bool Platform::setStartupEnabled(bool enabled, std::string* error)
{
    if (!enabled) {
        std::error_code ec; std::filesystem::remove(autostartFile(), ec);
        if (!ec) return true; if (error) *error = ec.message(); return false;
    }
    return Files::atomicWrite(autostartFile(), desktopAutostartEntry(Files::executable().string()), error);
}
bool Platform::pointerSettingsSupported()
{
    const auto* wayland = std::getenv("WAYLAND_DISPLAY"); const auto* session = std::getenv("XDG_SESSION_TYPE"); const auto* display = std::getenv("DISPLAY");
    return !(wayland && *wayland) && !(session && std::string(session) == "wayland") && display && *display && !findXinput().empty();
}
std::string Platform::pointerSettingsDescription()
{
    return pointerSettingsSupported() ? tr("Platform", "Applies to the selected X11/libinput mouse. Games using raw input may ignore these settings.")
        : tr("Platform", "Pointer speed and acceleration are managed by your Wayland desktop, or require xinput on X11. DPI and lighting still work.");
}
bool Platform::applyPointerSettings(const MouseDevice& device, int speed, bool acceleration, std::string* error)
{
    if (!pointerSettingsSupported()) { if (error) *error = pointerSettingsDescription(); return false; }
    std::string ids;
    if (device.inputPath.empty() || !xinput({"--list", "--id-only"}, &ids)) { if (error) *error = tr("Platform", "Could not find the selected mouse in X11."); return false; }
    std::istringstream lines(ids); std::string id;
    while (std::getline(lines, id)) {
        id = trim(id); if (!std::regex_match(id, std::regex("[0-9]+"))) continue;
        std::string properties; if (!xinput({"--list-props", id}, &properties)) continue;
        std::smatch node, oldProfile;
        if (!std::regex_search(properties, node, std::regex(R"X(Device Node \(\d+\):\s*"([^"]+)")X")) || node[1] != device.inputPath) continue;
        if (!std::regex_search(properties, oldProfile, std::regex(R"(libinput Accel Profile Enabled \(\d+\):\s*([0-9, \t]+))")) || properties.find("libinput Accel Speed (") == std::string::npos) break;
        std::string old = oldProfile[1]; std::replace(old.begin(), old.end(), ',', ' '); std::istringstream values(old);
        std::vector<std::string> oldValues; std::string value; while (values >> value) oldValues.push_back(value);
        if (oldValues.size() < 2 || oldValues.size() > 3) break;
        std::vector<std::string> base{"--set-prop", "--type=int", "--format=8", id, "libinput Accel Profile Enabled"};
        auto next = base; next.push_back(acceleration ? "1" : "0"); next.push_back(acceleration ? "0" : "1");
        if (oldValues.size() == 3) next.push_back("0"); if (!xinput(next)) break;
        std::ostringstream normalized; normalized.imbue(std::locale::classic()); normalized << (std::clamp(speed, 1, 20) - 1) / 19.0 * 2.0 - 1.0;
        if (xinput({"--set-prop", "--type=float", id, "libinput Accel Speed", normalized.str()})) return true;
        base.insert(base.end(), oldValues.begin(), oldValues.end()); xinput(base); break;
    }
    if (error) *error = tr("Platform", "Could not change the selected mouse's X11/libinput pointer settings."); return false;
}
struct Platform::DeviceWatcher::Data { udev* context = nullptr; udev_monitor* monitor = nullptr; };
Platform::DeviceWatcher::DeviceWatcher() : data(std::make_unique<Data>())
{
    data->context = udev_new(); if (!data->context) return;
    data->monitor = udev_monitor_new_from_netlink(data->context, "udev"); if (!data->monitor) return;
    udev_monitor_filter_add_match_subsystem_devtype(data->monitor, "hidraw", nullptr);
    if (udev_monitor_enable_receiving(data->monitor) < 0) { udev_monitor_unref(data->monitor); data->monitor = nullptr; }
    else {
        const int fd = udev_monitor_get_fd(data->monitor);
        if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) < 0) { udev_monitor_unref(data->monitor); data->monitor = nullptr; }
    }
}
Platform::DeviceWatcher::~DeviceWatcher() { if (data->monitor) udev_monitor_unref(data->monitor); if (data->context) udev_unref(data->context); }
bool Platform::DeviceWatcher::changed()
{
    bool changed = false;
    if (data->monitor) while (auto* event = udev_monitor_receive_device(data->monitor)) { udev_device_unref(event); changed = true; }
    return changed;
}
ApplyResult Platform::downloadDefinitions()
{
    static std::once_flag initialized; static CURLcode init = CURLE_FAILED_INIT;
    std::call_once(initialized, [] { init = curl_global_init(CURL_GLOBAL_DEFAULT); });
    if (init != CURLE_OK) return {false, "HTTPS initialization failed"};
    auto* request = curl_easy_init(); if (!request) return {false, "HTTPS initialization failed"};
    std::string payload;
    curl_easy_setopt(request, CURLOPT_URL, "https://raw.githubusercontent.com/hdijkema/razer-mouse-lite/main/data/mice.json");
    curl_easy_setopt(request, CURLOPT_USERAGENT, "RazerMouseLite/" MOUSE_LITE_VERSION);
    curl_easy_setopt(request, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(request, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(request, CURLOPT_TIMEOUT, 20L); curl_easy_setopt(request, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(request, CURLOPT_NOSIGNAL, 1L); curl_easy_setopt(request, CURLOPT_WRITEDATA, &payload);
    curl_easy_setopt(request, CURLOPT_WRITEFUNCTION, +[](char* bytes, std::size_t size, std::size_t count, void* data) -> std::size_t {
        auto& out = *static_cast<std::string*>(data);
        if (count && size > (2 * 1024 * 1024 - out.size()) / count) return 0;
        const auto length = size * count;
        out.append(bytes, length); return length;
    });
    const auto result = curl_easy_perform(request); long status = 0;
    curl_easy_getinfo(request, CURLINFO_RESPONSE_CODE, &status); curl_easy_cleanup(request);
    return result == CURLE_OK && status == 200 ? ApplyResult{true, std::move(payload)} : ApplyResult{false, curl_easy_strerror(result)};
}
