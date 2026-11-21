#include "platform.h"
#include "razerprotocol.h"
#include "files.h"
#include "i18n.h"
#include <windows.h>
#include <dbt.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#include <winhttp.h>
#include <chrono>
#include <regex>

namespace {
std::string utf8(const std::wstring& s)
{
    const int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    std::string out(n, 0); WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr, nullptr); return out;
}
std::wstring wide(const std::string& s) { return std::filesystem::u8path(s).wstring(); }
std::string winError()
{
    const DWORD code = GetLastError(); wchar_t* buffer = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, 0, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    const auto result = buffer ? trim(utf8(buffer)) : std::to_string(code);
    if (buffer) LocalFree(buffer);
    return result;
}
class Handle
{
public:
    explicit Handle(HANDLE value) : value(value) {}
    ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle(const Handle&) = delete;
    HANDLE value;
};
HANDLE openHid(const std::wstring& path)
{
    // Feature reports work on zero-access handles owned by Windows for pointer input.
    for (DWORD access : {DWORD(GENERIC_READ | GENERIC_WRITE), DWORD(GENERIC_WRITE), DWORD(0)}) {
        const auto handle = CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (handle != INVALID_HANDLE_VALUE) return handle;
    }
    return INVALID_HANDLE_VALUE;
}
class WindowsChannel final : public FeatureChannel
{
public:
    explicit WindowsChannel(HANDLE value) : handle(value) {}
    bool send(const Bytes& packet, std::string* error) override
    {
        if (HidD_SetFeature(handle.value, const_cast<std::uint8_t*>(packet.data()), static_cast<ULONG>(packet.size()))) return true;
        if (error) *error = winError(); return false;
    }
    Bytes receive(int length, std::string* error) override
    {
        Bytes packet(length, 0);
        if (HidD_GetFeature(handle.value, packet.data(), static_cast<ULONG>(packet.size()))) return packet;
        if (error) *error = winError(); return {};
    }
private: Handle handle;
};
constexpr const wchar_t* runKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
class HttpHandle
{
public:
    explicit HttpHandle(HINTERNET h) : value(h) {}
    ~HttpHandle() { if (value) WinHttpCloseHandle(value); }
    HINTERNET value;
};
}
std::vector<MouseDevice> Platform::enumerate()
{
    std::vector<MouseDevice> devices;
    GUID guid; HidD_GetHidGuid(&guid);
    const auto info = SetupDiGetClassDevsW(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (info == INVALID_HANDLE_VALUE) return devices;
    const auto database = RazerProtocol::models();
    SP_DEVICE_INTERFACE_DATA entry{}; entry.cbSize = sizeof(entry);
    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(info, nullptr, &guid, index, &entry); ++index) {
        DWORD size = 0; SetupDiGetDeviceInterfaceDetailW(info, &entry, nullptr, 0, &size, nullptr);
        if (!size) continue;
        std::vector<unsigned char> buffer(size);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buffer.data()); detail->cbSize = sizeof(*detail);
        if (!SetupDiGetDeviceInterfaceDetailW(info, &entry, detail, size, nullptr, nullptr)) continue;
        Handle handle(CreateFileW(detail->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
        if (handle.value == INVALID_HANDLE_VALUE) continue;
        HIDD_ATTRIBUTES attributes{}; attributes.Size = sizeof(attributes);
        if (!HidD_GetAttributes(handle.value, &attributes) || attributes.VendorID != 0x1532) continue;
        Json model;
        for (const auto& item : database) if (number(item, "pid") == attributes.ProductID) { model = item; break; }
        if (model.is_null()) continue;
        PHIDP_PREPARSED_DATA preparsed = nullptr;
        if (!HidD_GetPreparsedData(handle.value, &preparsed)) continue;
        HIDP_CAPS caps{}; const auto result = HidP_GetCaps(preparsed, &caps); HidD_FreePreparsedData(preparsed);
        if (result != HIDP_STATUS_SUCCESS || caps.FeatureReportByteLength != 91) continue;
        const auto path = utf8(detail->DevicePath);
        std::smatch match;
        const std::regex expression("&mi_([0-9a-f]{2})", std::regex::icase);
        const int actualInterface = std::regex_search(path, match, expression) ? std::stoi(match[1].str(), nullptr, 16) : -1;
        // Never send settings through a pointer input collection or an unreviewed interface.
        if (actualInterface == number(model, "interface")) devices.push_back({path, text(model, "name"), model, attributes.ProductID, {}});
    }
    SetupDiDestroyDeviceInfoList(info); return devices;
}
std::unique_ptr<FeatureChannel> Platform::openControl(const MouseDevice& device, std::string* error)
{
    HANDLE handle = openHid(wide(device.path));
    if (handle == INVALID_HANDLE_VALUE) { if (error) *error = winError(); return {}; }
    HIDD_ATTRIBUTES attributes{}; attributes.Size = sizeof(attributes);
    if (!HidD_GetAttributes(handle, &attributes) || attributes.VendorID != 0x1532 || attributes.ProductID != device.pid) {
        CloseHandle(handle); if (error) *error = tr("Platform", "Mouse identity changed. Click Refresh and try again."); return {};
    }
    return std::make_unique<WindowsChannel>(handle);
}
bool Platform::startupEnabled()
{
    DWORD size = 0;
    return RegGetValueW(HKEY_CURRENT_USER, runKey, L"RazerMouseLite", RRF_RT_REG_SZ, nullptr, nullptr, &size) == ERROR_SUCCESS && size > sizeof(wchar_t);
}
bool Platform::setStartupEnabled(bool enabled, std::string* error)
{
    HKEY key = nullptr;
    LSTATUS status = RegCreateKeyExW(HKEY_CURRENT_USER, runKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (status == ERROR_SUCCESS) {
        if (enabled) {
            const auto command = L"\"" + Files::executable().wstring() + L"\" --tray";
            status = RegSetValueExW(key, L"RazerMouseLite", 0, REG_SZ, reinterpret_cast<const BYTE*>(command.c_str()), static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
        } else { status = RegDeleteValueW(key, L"RazerMouseLite"); if (status == ERROR_FILE_NOT_FOUND) status = ERROR_SUCCESS; }
        RegCloseKey(key);
    }
    if (status == ERROR_SUCCESS) return true;
    if (error) *error = tr("Platform", "Could not update automatic startup."); return false;
}
bool Platform::pointerSettingsSupported() { return true; }
bool Platform::systemDark()
{
    DWORD value = 1, size = sizeof(value);
    return RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS && value == 0;
}
std::string Platform::pointerSettingsDescription()
{ return tr("Platform", "Applies to all mice. Games using raw input may ignore these settings."); }
bool Platform::applyPointerSettings(const MouseDevice&, int speed, bool acceleration, std::string* error)
{
    int oldMouse[3]{};
    if (!SystemParametersInfoW(SPI_GETMOUSE, 0, oldMouse, 0)) { if (error) *error = tr("Platform", "Could not read desktop pointer settings."); return false; }
    int mouse[3]{6, 10, acceleration ? 1 : 0}; const UINT flags = SPIF_UPDATEINIFILE | SPIF_SENDCHANGE;
    if (!SystemParametersInfoW(SPI_SETMOUSE, 0, mouse, flags)) { if (error) *error = tr("Platform", "Could not change pointer acceleration."); return false; }
    if (!SystemParametersInfoW(SPI_SETMOUSESPEED, 0, reinterpret_cast<void*>(INT_PTR(std::clamp(speed, 1, 20))), flags)) {
        SystemParametersInfoW(SPI_SETMOUSE, 0, oldMouse, flags);
        if (error) *error = tr("Platform", "Could not change pointer speed."); return false;
    }
    return true;
}
struct Platform::DeviceWatcher::Data
{
    HWND window = nullptr; HDEVNOTIFY notification = nullptr; bool pending = false;
    static LRESULT CALLBACK callback(HWND window, UINT message, WPARAM w, LPARAM l)
    {
        auto* self = reinterpret_cast<Data*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<Data*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self && message == WM_DEVICECHANGE && (w == DBT_DEVNODES_CHANGED || w == DBT_DEVICEARRIVAL || w == DBT_DEVICEREMOVECOMPLETE)) self->pending = true;
        return DefWindowProcW(window, message, w, l);
    }
};
Platform::DeviceWatcher::DeviceWatcher() : data(std::make_unique<Data>())
{
    WNDCLASSW cls{}; cls.lpfnWndProc = Data::callback; cls.hInstance = GetModuleHandleW(nullptr); cls.lpszClassName = L"MouseLite.DeviceWatcher";
    RegisterClassW(&cls);
    data->window = CreateWindowExW(0, cls.lpszClassName, L"", 0, 0, 0, 0, 0, nullptr, nullptr, cls.hInstance, data.get());
    if (data->window) {
        DEV_BROADCAST_DEVICEINTERFACE_W filter{}; filter.dbcc_size = sizeof(filter); filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        HidD_GetHidGuid(&filter.dbcc_classguid);
        data->notification = RegisterDeviceNotificationW(data->window, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
    }
}
Platform::DeviceWatcher::~DeviceWatcher()
{
    if (data->notification) UnregisterDeviceNotification(data->notification);
    if (data->window) DestroyWindow(data->window);
}
bool Platform::DeviceWatcher::changed()
{
    MSG message;
    if (data->window) while (PeekMessageW(&message, data->window, 0, 0, PM_REMOVE)) DispatchMessageW(&message);
    const bool result = data->pending; data->pending = false; return result;
}
ApplyResult Platform::downloadDefinitions()
{
    const auto start = std::chrono::steady_clock::now();
    HttpHandle session(WinHttpOpen(L"RazerMouseLite/" MOUSE_LITE_VERSION, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session.value) return {false, winError()};
    WinHttpSetTimeouts(session.value, 3000, 5000, 5000, 5000);
    HttpHandle connection(WinHttpConnect(session.value, L"raw.githubusercontent.com", INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connection.value) return {false, winError()};
    HttpHandle request(WinHttpOpenRequest(connection.value, L"GET", L"/hdijkema/razer-mouse-lite/main/data/mice.json", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!request.value) return {false, winError()};
    DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (!WinHttpSetOption(request.value, WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof(redirects))
        || !WinHttpSendRequest(request.value, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        || !WinHttpReceiveResponse(request.value, nullptr)) return {false, winError()};
    DWORD status = 0, size = sizeof(status);
    if (!WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) || status != 200)
        return {false, "HTTPS response " + std::to_string(status)};
    std::string bytes;
    char buffer[16384];
    while (true) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(20)) return {false, "Download timed out"};
        DWORD count = 0;
        if (!WinHttpReadData(request.value, buffer, sizeof(buffer), &count)) return {false, winError()};
        if (!count) break;
        bytes.append(buffer, count);
        if (bytes.size() > 2 * 1024 * 1024) return {false, "Device definitions exceed 2 MiB"};
    }
    return {true, std::move(bytes)};
}
