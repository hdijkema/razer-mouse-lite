#include "platform.h"
#include "files.h"
#include "trayicon.h"
#include <FL/Fl_SVG_Image.H>
#include <windows.h>
#include <shellapi.h>
#include <chrono>

namespace {
std::wstring wide(const std::string& value) { return std::filesystem::u8path(value).wstring(); }
HICON profileIcon(Color color)
{
    constexpr int size = 32;
    const auto svg = Platform::profileIconSvg(color);
    Fl_SVG_Image image(nullptr, svg.c_str());
    if (image.fail()) return nullptr;
    image.resize(size, size); image.normalize();
    if (!image.array || image.d() != 4) return nullptr;
    BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth = size;
    info.bmiHeader.biHeight = -size; info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* memory = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &memory, nullptr, 0);
    if (!bitmap) return nullptr;
    auto* pixels = static_cast<DWORD*>(memory);
    for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) {
        const auto* rgba = image.array + (y * size + x) * 4;
        const DWORD alpha = rgba[3];
        // Windows HICON expects premultiplied BGRA, including antialiased edges.
        pixels[y * size + x] = (alpha << 24) | ((rgba[0] * alpha / 255) << 16) |
            ((rgba[1] * alpha / 255) << 8) | (rgba[2] * alpha / 255);
    }
    const unsigned char maskBits[size * size / 8]{};
    HBITMAP mask = CreateBitmap(size, size, 1, 1, maskBits);
    ICONINFO icon{}; icon.fIcon = TRUE; icon.hbmColor = bitmap; icon.hbmMask = mask;
    HICON result = CreateIconIndirect(&icon); DeleteObject(mask); DeleteObject(bitmap); return result;
}
}
struct Platform::DesktopIntegration::Data
{
    HANDLE mutex = nullptr;
    HWND window = nullptr;
    HICON icon = nullptr;
    NOTIFYICONDATAW tray{};
    std::vector<TrayItem> items;
    std::vector<int> actions;
    bool first = true, ready = true, isolated = false, reveal = false, trayShown = false, menu = false, suppressRelease = false;
    std::chrono::steady_clock::time_point click{};
    UINT taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    static constexpr UINT trayMessage = WM_APP + 71;
    static constexpr UINT revealMessage = WM_APP + 72;
    static LRESULT CALLBACK callback(HWND window, UINT message, WPARAM w, LPARAM l)
    {
        auto* self = reinterpret_cast<Data*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<Data*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self) {
            if (message == revealMessage) self->reveal = true;
            if (message == self->taskbarCreated && self->trayShown) self->trayShown = Shell_NotifyIconW(NIM_ADD, &self->tray) != FALSE;
            if (message == trayMessage) {
                if (l == WM_LBUTTONUP) {
                    if (self->suppressRelease) { self->suppressRelease = false; self->click = {}; }
                    else self->click = std::chrono::steady_clock::now() + std::chrono::milliseconds(GetDoubleClickTime());
                }
                if (l == WM_LBUTTONDBLCLK) { self->click = {}; self->suppressRelease = true; self->actions.push_back(-2); }
                if (l == WM_RBUTTONUP || l == WM_CONTEXTMENU) self->menu = true;
                if (l == NIN_KEYSELECT) self->actions.push_back(-2);
            }
        }
        return DefWindowProcW(window, message, w, l);
    }
};
Platform::DesktopIntegration::DesktopIntegration(bool isolated) : data(std::make_unique<Data>())
{
    data->isolated = isolated;
    if (isolated) return;
    std::error_code ec;
    std::filesystem::create_directories(Files::configDirectory(), ec);
    if (ec) { data->ready = false; return; }
    const auto identity = std::to_wstring(std::hash<std::wstring>{}(Files::configDirectory().wstring()));
    const auto className = L"MouseLite.Instance." + identity;
    data->mutex = CreateMutexW(nullptr, TRUE, (L"Local\\MouseLite-" + identity).c_str());
    if (!data->mutex) { data->ready = false; return; }
    data->first = GetLastError() != ERROR_ALREADY_EXISTS;
    if (!data->first) {
        for (int i = 0; i < 20; ++i) {
            const auto previous = FindWindowW(className.c_str(), L"MouseLite");
            if (previous) { PostMessageW(previous, Data::revealMessage, 0, 0); return; }
            Sleep(75);
        }
        data->ready = false; return;
    }
    WNDCLASSW cls{}; cls.lpfnWndProc = Data::callback; cls.hInstance = GetModuleHandleW(nullptr); cls.lpszClassName = className.c_str();
    RegisterClassW(&cls);
    data->window = CreateWindowExW(0, cls.lpszClassName, L"MouseLite", 0, 0, 0, 0, 0, nullptr, nullptr, cls.hInstance, data.get());
    data->ready = data->window != nullptr;
}
Platform::DesktopIntegration::~DesktopIntegration()
{
    if (data->trayShown) Shell_NotifyIconW(NIM_DELETE, &data->tray);
    if (data->icon) DestroyIcon(data->icon);
    if (data->window) DestroyWindow(data->window);
    if (data->mutex) { if (data->first) ReleaseMutex(data->mutex); CloseHandle(data->mutex); }
}
bool Platform::DesktopIntegration::primary() const { return data->first; }
bool Platform::DesktopIntegration::valid() const { return data->ready; }
bool Platform::DesktopIntegration::trayAvailable() const { return data->trayShown; }
bool Platform::DesktopIntegration::revealRequested() { const bool value = data->reveal; data->reveal = false; return value; }
void Platform::DesktopIntegration::setTray(const std::vector<TrayItem>& items, Color color, const std::string& tooltip)
{
    if (data->isolated || !data->first || !data->window) return;
    data->items = items;
    const auto icon = profileIcon(color);
    if (!icon) return;
    const auto oldIcon = data->icon; data->icon = icon;
    data->tray = {}; data->tray.cbSize = sizeof(data->tray); data->tray.hWnd = data->window; data->tray.uID = 1;
    data->tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; data->tray.uCallbackMessage = Data::trayMessage; data->tray.hIcon = icon;
    const auto tip = wide(tooltip); wcsncpy_s(data->tray.szTip, tip.c_str(), _TRUNCATE);
    data->trayShown = Shell_NotifyIconW(data->trayShown ? NIM_MODIFY : NIM_ADD, &data->tray) != FALSE;
    if (oldIcon) DestroyIcon(oldIcon);
}
void Platform::DesktopIntegration::poll(const std::function<void(int)>& action)
{
    MSG message;
    if (data->window) while (PeekMessageW(&message, data->window, 0, 0, PM_REMOVE)) DispatchMessageW(&message);
    if (data->click != std::chrono::steady_clock::time_point{} && std::chrono::steady_clock::now() >= data->click) {
        data->click = {}; data->actions.push_back(-1);
    }
    if (data->menu) {
        data->menu = false;
        HMENU menu = CreatePopupMenu();
        for (std::size_t i = 0; i < data->items.size(); ++i) {
            const auto& item = data->items[i];
            if (item.label.empty()) AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            else AppendMenuW(menu, MF_STRING | (item.checked ? MF_CHECKED : 0) | (item.enabled ? 0 : MF_GRAYED), 100 + i, wide(item.label).c_str());
        }
        POINT pointer; GetCursorPos(&pointer); SetForegroundWindow(data->window);
        const UINT selected = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, pointer.x, pointer.y, data->window, nullptr);
        PostMessageW(data->window, WM_NULL, 0, 0); DestroyMenu(menu);
        if (selected >= 100 && selected - 100 < data->items.size()) data->actions.push_back(data->items[selected - 100].action);
    }
    const auto actions = std::move(data->actions); data->actions.clear();
    for (int selected : actions) action(selected);
}
