#pragma once
#include "platform.h"
#include "trayicon.h"
#include <fltk_trayicon.h>

// Application-specific action numbers and profile-coloured SVG stay here.
class DesktopTray {
    Fltk_TrayIcon tray_{{"razer-mouse-lite", "Razer Mouse Lite", true, false}};
    std::vector<int> actions_;
public:
    DesktopTray() {
        tray_.callback([this](Fltk_TrayIcon::Event event, int id) {
            actions_.push_back(event == Fltk_TrayIcon::Event::Activate ? -1 :
                event == Fltk_TrayIcon::Event::DoubleClick ? -2 : id);
        });
    }
    bool available() const { return tray_.available(); }
    void set(const std::vector<Platform::TrayItem>& items, Color color, const std::string& tooltip) {
        std::vector<Fltk_TrayIcon::MenuItem> menu;
        for (const auto& item : items) menu.push_back({item.label, item.action, item.enabled, item.action >= 0, item.checked});
        tray_.menu(menu, -2);
        tray_.tooltip(tooltip);
        if (tray_.icon_svg(Platform::profileIconSvg(color))) tray_.show();
    }
    void poll(const std::function<void(int)>& action) {
        tray_.poll();
        const auto actions = std::move(actions_); actions_.clear();
        for (int id : actions) action(id);
    }
};
