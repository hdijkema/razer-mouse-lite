#include "razermousewindow.h"
#include "selftest.h"
#include "translations.h"
#include "devicedatabase.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <clocale>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char* argv[])
{
    std::setlocale(LC_ALL, "");
    Fl::args_to_utf8(argc, argv);
    std::vector<std::string> arguments(argv + 1, argv + argc);
    auto has = [&](const std::string& option) { return std::find(arguments.begin(), arguments.end(), option) != arguments.end(); };
    auto value = [&](const std::string& option) -> std::string {
        const auto at = std::find(arguments.begin(), arguments.end(), option);
        return at == arguments.end() || at + 1 == arguments.end() ? "" : *(at + 1);
    };
    try {
        Translations translations;
        if (!translations.load(value("--language"))) { std::cerr << "Cannot load translations\n"; return 1; }
        const auto cancelLabel = tr("RazerMouseWindow", "Cancel");
        fl_cancel = cancelLabel.c_str();
        if (has("--version")) { std::cout << MOUSE_LITE_VERSION << '\n'; return 0; }
        if (has("--self-test")) return selfTest(value("--catalogues"));
        if (has("--probe")) {
            for (const auto& device : MouseBackend::enumerate()) if (device.pid == 0x00a3) {
                const auto result = MouseBackend::readCobraStatus(device); std::cout << result.message << '\n'; return result.success ? 0 : 1;
            }
            std::cout << "No wired Razer Cobra found.\n"; return 1;
        }
        const bool smoke = has("--smoke-test");
        if (!smoke) DeviceDatabase::load(DeviceDatabase::cachePath());
        Platform::DesktopIntegration desktop(smoke);
        if (!desktop.valid()) { std::cerr << "Could not initialize the application instance.\n"; return 1; }
        if (!desktop.primary()) return 0;
        Fl::lock();
        Fl::scheme("gtk+");
        RazerMouseWindow window(desktop, smoke, value("--theme"), value("--skin"));
        if (smoke) return window.smokeTest(value("--smoke-test")) ? 0 : 1;
        if (has("--tray")) {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(800);
            while (!desktop.trayAvailable() && std::chrono::steady_clock::now() < deadline) {
                desktop.poll([](int) {}); std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        if (!has("--tray") || !desktop.trayAvailable()) window.show();
        while (!window.finished()) {
            Fl::wait(0.2);
            if (!Fl::first_window()) std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
