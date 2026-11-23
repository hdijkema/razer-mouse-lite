#include "platform.h"
#include "files.h"
#include "../desktoptray.h"
#include <windows.h>

struct Platform::DesktopIntegration::Data
{
    HANDLE mutex = nullptr;
    HWND window = nullptr;
    std::unique_ptr<DesktopTray> tray;
    bool first = true, ready = true, isolated = false, reveal = false;
    static constexpr UINT revealMessage = WM_APP + 72;
    static LRESULT CALLBACK callback(HWND window, UINT message, WPARAM w, LPARAM l)
    {
        auto* self = reinterpret_cast<Data*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<Data*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self && message == revealMessage) self->reveal = true;
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
    if (data->ready) data->tray = std::make_unique<DesktopTray>();
}
Platform::DesktopIntegration::~DesktopIntegration()
{
    data->tray.reset();
    if (data->window) DestroyWindow(data->window);
    if (data->mutex) { if (data->first) ReleaseMutex(data->mutex); CloseHandle(data->mutex); }
}
bool Platform::DesktopIntegration::primary() const { return data->first; }
bool Platform::DesktopIntegration::valid() const { return data->ready; }
bool Platform::DesktopIntegration::trayAvailable() const { return data->tray && data->tray->available(); }
bool Platform::DesktopIntegration::revealRequested() { const bool value = data->reveal; data->reveal = false; return value; }
void Platform::DesktopIntegration::setTray(const std::vector<TrayItem>& items, Color color, const std::string& tooltip)
{
    if (data->tray) data->tray->set(items, color, tooltip);
}
void Platform::DesktopIntegration::poll(const std::function<void(int)>& action)
{
    MSG message;
    if (data->window) while (PeekMessageW(&message, data->window, 0, 0, PM_REMOVE)) DispatchMessageW(&message);
    if (data->tray) data->tray->poll(action);
}
