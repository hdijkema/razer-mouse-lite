#include "razermousewindow.h"
#include "translations.h"
#include "devicedatabase.h"
#include "files.h"
#include <FL/Fl_Image_Surface.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/filename.H>
#include <climits>
#include <cstdlib>
#include <iostream>

namespace {
std::string ui(const std::string& source) { return tr("RazerMouseWindow", source); }
Fl_Css_Box* label(FLTK_Css& css, int x, int y, int w, int h, const std::string& text, const std::vector<std::string>& classes = {})
{
    auto* result = new Fl_Css_Box(css, x, y, w, h);
    result->copy_label(text.c_str()); result->css_classes(classes);
    result->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP | FL_ALIGN_CLIP);
    return result;
}
Fl_Css_Button* button(FLTK_Css& css, int x, int y, int w, int h, const std::string& text)
{
    auto* result = new Fl_Css_Button(css, x, y, w, h);
    result->copy_label(text.c_str()); return result;
}
void add(CssChoice* choice, const std::string& text)
{
    std::string escaped;
    for (char c : text) { if (c == '\\' || c == '/' || c == '&') escaped += '\\'; escaped += c; }
    choice->add(escaped.c_str());
}
bool integerInput(const char* value, int& result)
{
    const std::string input = trim(value ? value : "");
    if (input.empty()) return false;
    char* end = nullptr; const auto number = std::strtol(input.c_str(), &end, 10);
    if (!end || *end || number < INT_MIN || number > INT_MAX) return false;
    result = static_cast<int>(number); return true;
}
template<class T> bool ready(std::future<T>& future)
{ return future.valid() && future.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }
class ProfileButton : public Fl_Css_Button
{
public:
    ProfileButton(FLTK_Css& css, int x, int y, int w, int h) : Fl_Css_Button(css, x, y, w, h) {}
    int index = 0;
    RazerMouseWindow* owner = nullptr;
    Color stripe;
protected:
    void draw() override {
        Fl_Css_Button::draw(); fl_color(fl_rgb_color(stripe.r, stripe.g, stripe.b));
        fl_rectf(x() + 3, y() + 16, 3, h() - 32);
    }
};
}

RazerMouseWindow::RazerMouseWindow(Platform::DesktopIntegration& integration, bool sample, const std::string& mode, const std::string& skin)
    : appearance(css, sample), window(css, 80, 40, 1100, 828, "Razer Mouse Lite"), desktop(integration)
    , store(sample ? std::filesystem::temp_directory_path() / ("MouseLite-preview-" + newId()) / "profiles.json" : std::filesystem::path())
    , preview(sample)
{
    std::string error;
    if (!store.load(&error)) readOnly = true;
    const bool systemDark = preview ? true : Platform::systemDark();
    const bool themeOk = appearance.choose(mode.empty() ? appearance.mode() : mode, skin.empty() ? appearance.skin() : skin, systemDark);
    window.size_range(1000, 760);
    buildUi();
    window.callback([](Fl_Widget*, void* value) {
        auto* self = static_cast<RazerMouseWindow*>(value);
        if (self->desktop.trayAvailable()) self->window.hide();
        else { self->quitting = true; self->window.hide(); }
    }, this);
    restorePending = !preview && !readOnly && store.restoreOnStart && !store.activeId.empty();
    rebuildProfiles(); rebuildTray(); refreshAppearance();
    if (readOnly) setStatus(error, true);
    else if (!themeOk) setStatus(tr("Appearance", "Could not load the skin: %1", appearance.error()), true);
    if (preview) {
        for (const auto& model : DeviceDatabase::bundled()) if (number(model, "pid") == 0x00a3) {
            finishScan({{"preview:no-hardware", "Razer Cobra", model, 0x00a3, {}}}); break;
        }
    } else {
        watcher = std::make_unique<Platform::DeviceWatcher>();
        refreshDevices(); Fl::add_timeout(0.15, timer, this);
    }
    updateCapabilities();
}
RazerMouseWindow::~RazerMouseWindow()
{
    Fl::remove_timeout(timer, this);
    if (scan.valid()) scan.wait();
    if (application.valid()) application.wait();
    if (download.valid()) download.wait();
    if (systemTheme.valid()) systemTheme.wait();
}
void RazerMouseWindow::show() { window.show(); }
void RazerMouseWindow::reveal() { window.show(); window.take_focus(); }
void RazerMouseWindow::buildUi()
{
    window.begin();
    auto* rail = new Fl_Css_Group(css, 0, 0, 228, 828); rail->css_classes({"rail"});
    label(css, 24, 25, 184, 38, "RAZER / LITE", {"brand"});
    label(css, 24, 68, 184, 34, ui("Your mouse. Your settings. Done."), {"subtitle"});
    label(css, 24, 126, 180, 24, ui("PROFILES"), {"section"});
    profileScroll = new CssScroll(css, 14, 160, 206, 440); profileScroll->css_type("scroll"); profileScroll->css_classes({"profile-list"});
    profileScroll->type(Fl_Scroll::VERTICAL); profileScroll->scrollbar_size(9);
    profileItems = new Fl_Css_Group(css, 16, 162, 190, 420); profileItems->css_classes({"profile-list"}); profileItems->end(); profileScroll->end();
    label(css, 24, 618, 180, 48, ui("Click a profile to apply\nit immediately."), {"muted"});
    addButton = button(css, 16, 680, 92, 36, ui("+ New"));
    deleteButton = button(css, 116, 680, 96, 36, ui("Delete"));
    label(css, 24, 765, 180, 38, "FLTK / CSS\n" MOUSE_LITE_VERSION, {"muted"});
    rail->resizable(profileScroll); rail->end();
    label(css, 254, 22, 480, 38, "Razer Mouse Lite", {"brand"});
    label(css, 254, 64, 440, 28, ui("Your mouse. Your settings. Done."), {"subtitle"});
    activeLabel = label(css, 744, 25, 220, 34, ui("No profile applied yet"), {"badge"});
    activeLabel->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    aboutButton = button(css, 974, 25, 102, 34, ui("About"));
    const char* modes[] = {"Light", "Dark", "Follow system"};
    for (int i = 0; i < 3; ++i) {
        themeButtons[i] = button(css, 744 + i * 111, 66, 106, 29, tr("Appearance", modes[i]));
        themeButtons[i]->css_classes({"theme-mode"}); themeButtons[i]->type(FL_TOGGLE_BUTTON);
    }
    deviceBox = new CssChoice(css, 254, 106, 478, 36);
    refreshButton = button(css, 744, 106, 102, 36, ui("Refresh"));
    updateButton = button(css, 858, 106, 218, 36, ui("Update device definitions"));
    deviceLabel = label(css, 254, 148, 822, 30, ui("Looking for USB mice…"), {"muted"});
    auto* scroll = new CssScroll(css, 254, 184, 822, 424); scroll->css_type("scroll"); scroll->css_classes({"editor"});
    scroll->type(Fl_Scroll::BOTH); scroll->scrollbar_size(10);
    editor = new Fl_Css_Group(css, 254, 184, 822, 420); editor->css_classes({"editor"});
    label(css, 254, 186, 132, 36, ui("Profile name"));
    nameEdit = new Fl_Css_Input(css, 392, 186, 684, 36); nameEdit->maximum_size(320);
    auto* sensor = new Fl_Css_Group(css, 254, 242, 398, 174); sensor->css_classes({"card"});
    label(css, 274, 255, 354, 27, ui("Sensor"), {"card-title"});
    label(css, 274, 288, 354, 23, ui("Sensitivity"), {"muted"});
    dpiSlider = new CssSlider(css, 274, 315, 230, 32); dpiSlider->bounds(100, 8500); dpiSlider->step(50);
    dpiEdit = new Fl_Css_Input(css, 516, 315, 114, 32); dpiEdit->type(FL_INT_INPUT); dpiEdit->maximum_size(6); dpiEdit->when(FL_WHEN_CHANGED);
    label(css, 274, 365, 128, 34, ui("Polling rate"));
    pollBox = new CssChoice(css, 412, 365, 218, 34); add(pollBox, ui("Keep current"));
    for (int hz : {125, 500, 1000}) add(pollBox, std::to_string(hz) + " Hz");
    sensor->end();
    auto* lighting = new Fl_Css_Group(css, 670, 242, 406, 174); lighting->css_classes({"card"});
    label(css, 690, 255, 364, 27, ui("Lighting"), {"card-title"});
    label(css, 690, 288, 364, 23, ui("Effect / color"), {"muted"});
    lightingBox = new CssChoice(css, 690, 315, 206, 34);
    for (const auto& name : {ui("Static color"), ui("Off"), ui("Keep current")}) add(lightingBox, name);
    colorButton = new CssColorButton(css, 908, 315, 146, 34, "#55d6a0");
    label(css, 690, 365, 110, 34, ui("Brightness"));
    brightnessSlider = new CssSlider(css, 808, 365, 176, 34); brightnessSlider->bounds(0, 100);
    brightnessLabel = label(css, 990, 365, 64, 34, "65%", {"muted"});
    lighting->end();
    auto* pointer = new Fl_Css_Group(css, 254, 432, 822, 160); pointer->css_classes({"card"});
    label(css, 274, 446, 782, 26, ui("Desktop pointer"), {"card-title"});
    pointerBox = new CssCheck(css, 274, 478, 782, 32, nullptr); pointerBox->copy_label(ui("Include desktop pointer settings in this profile").c_str());
    label(css, 274, 518, 126, 32, ui("Speed"));
    speedSlider = new CssSlider(css, 410, 518, 340, 32); speedSlider->bounds(1, 20);
    speedLabel = label(css, 760, 518, 66, 32, "10 / 20", {"muted"});
    accelerationBox = new CssCheck(css, 840, 516, 214, 36, nullptr); accelerationBox->copy_label(ui("Enable pointer acceleration").c_str());
    label(css, 274, 560, 780, 28, Platform::pointerSettingsDescription(), {"muted"});
    pointer->end(); editor->end(); scroll->end();
    saveButton = button(css, 254, 622, 126, 38, ui("Save"));
    label(css, 396, 622, 454, 38, ui("Save changes before switching profiles."), {"muted"});
    applyButton = button(css, 872, 622, 204, 38, ui("Save and apply")); applyButton->css_classes({"primary"});
    startup = new CssCheck(css, 254, 676, 230, 34, nullptr); startup->copy_label(ui("Start at login").c_str()); startup->value(preview ? false : Platform::startupEnabled());
    restore = new CssCheck(css, 494, 676, 582, 34, nullptr); restore->copy_label(ui("Restore active profile on startup / reconnect").c_str()); restore->value(store.restoreOnStart);
    label(css, 254, 718, 230, 30, ui("Language (restart required)"), {"muted"});
    languageBox = new CssChoice(css, 490, 718, 216, 30); add(languageBox, ui("System language"));
    const auto languages = Translations::languages(); int selected = 0;
    const auto preference = preview ? "system" : Translations::selectedLanguage();
    for (std::size_t i = 0; i < languages.size(); ++i) {
        add(languageBox, Translations::languageName(languages[i]));
        if (languages[i] == preference) selected = static_cast<int>(i) + 1;
    }
    languageBox->value(selected);
    label(css, 724, 718, 62, 30, tr("Appearance", "Skin"), {"muted"});
    skinBox = new CssChoice(css, 790, 718, 150, 30);
    const auto skins = appearance.skins();
    for (std::size_t i = 0; i < skins.size(); ++i) { add(skinBox, skins[i]); if (skins[i] == appearance.skin()) skinBox->value(static_cast<int>(i)); }
    reloadCss = button(css, 950, 718, 126, 30, tr("Appearance", "Reload CSS"));
    statusLabel = label(css, 254, 764, 822, 48, ui("Choose a profile or edit its settings."), {"status"});
    window.resizable(scroll); window.end();
    const std::vector<Fl_Widget*> controls = {deviceBox, refreshButton, updateButton, aboutButton, addButton, deleteButton,
        nameEdit, dpiEdit, dpiSlider, speedSlider, pollBox, accelerationBox, pointerBox, lightingBox, colorButton, brightnessSlider,
        applyButton, saveButton, startup, restore, languageBox, skinBox, reloadCss, themeButtons[0], themeButtons[1], themeButtons[2]};
    for (auto* control : controls) control->callback(callback, this);
}
void RazerMouseWindow::callback(Fl_Widget* widget, void* value)
{
    auto& self = *static_cast<RazerMouseWindow*>(value);
    if (widget == self.refreshButton) self.refreshDevices();
    else if (widget == self.updateButton) self.updateDefinitions();
    else if (widget == self.aboutButton) self.showAbout();
    else if (widget == self.deviceBox) {
        const int index = self.deviceBox->value();
        if (index >= 0 && index < static_cast<int>(self.devices.size())) { self.store.devicePath = self.devices[index].path; self.persist(); }
        self.updateCapabilities();
    } else if (widget == self.dpiSlider) {
        self.dpiEdit->value(std::to_string(static_cast<int>(self.dpiSlider->value())).c_str());
    } else if (widget == self.dpiEdit) {
        int dpi; if (integerInput(self.dpiEdit->value(), dpi)) self.dpiSlider->value(std::clamp(dpi, static_cast<int>(self.dpiSlider->minimum()), static_cast<int>(self.dpiSlider->maximum())));
        self.dpiSlider->redraw();
    } else if (widget == self.speedSlider) self.speedLabel->copy_label((std::to_string(static_cast<int>(self.speedSlider->value())) + " / 20").c_str());
    else if (widget == self.brightnessSlider) self.brightnessLabel->copy_label((std::to_string(static_cast<int>(self.brightnessSlider->value())) + "%").c_str());
    else if (widget == self.pointerBox || widget == self.lightingBox) self.updateCapabilities();
    else if (widget == self.colorButton) {
        uchar r = self.editedColor.r, g = self.editedColor.g, b = self.editedColor.b;
        const auto title = ui("Profile color");
        if (fl_color_chooser(title.c_str(), r, g, b)) { self.editedColor = {r, g, b}; self.updateColorButton(); }
    } else if (widget == self.saveButton || widget == self.applyButton) {
        if (self.saveEditor()) {
            if (widget == self.applyButton) self.activateProfile(self.editing);
            else self.setStatus(ui("Profile saved. Click the profile to apply it."));
        }
    } else if (widget == self.addButton) {
        if (self.readOnly || self.busy()) return;
        Profile profile;
        if (self.editing >= 0 && self.editing < static_cast<int>(self.store.profiles.size())) profile = self.store.profiles[self.editing];
        profile.id = newId(); profile.name = ui("New profile");
        self.store.profiles.push_back(profile); self.editing = static_cast<int>(self.store.profiles.size()) - 1;
        if (self.persist()) { self.rebuildProfiles(); self.rebuildTray(); self.nameEdit->take_focus(); self.nameEdit->insert_position(0, static_cast<int>(profile.name.size())); }
    } else if (widget == self.deleteButton) {
        if (self.readOnly || self.busy() || self.editing < 0 || self.store.profiles.size() <= 1) return;
        const auto prompt = tr("RazerMouseWindow", "Delete profile '%1'?", self.store.profiles[self.editing].name);
        const auto cancel = ui("Cancel"), remove = ui("Delete"), title = ui("Delete profile");
        fl_message_title(title.c_str()); if (fl_choice("%s", cancel.c_str(), remove.c_str(), nullptr, prompt.c_str()) != 1) return;
        if (self.store.profiles[self.editing].id == self.store.activeId) self.store.activeId.clear();
        self.store.profiles.erase(self.store.profiles.begin() + self.editing);
        self.editing = std::min(self.editing, static_cast<int>(self.store.profiles.size()) - 1);
        self.persist(); self.rebuildProfiles(); self.rebuildTray();
    } else if (widget == self.startup) {
        if (self.preview) return;
        std::string error;
        if (!Platform::setStartupEnabled(self.startup->value(), &error)) { self.startup->value(!self.startup->value()); self.setStatus(error, true); }
    } else if (widget == self.restore) { self.store.restoreOnStart = self.restore->value(); self.persist(); }
    else if (widget == self.languageBox) {
        if (self.preview) return;
        const auto languages = Translations::languages(); const int index = self.languageBox->value();
        self.setStatus(Translations::saveLanguage(index <= 0 ? "system" : languages.at(index - 1))
            ? ui("Language saved. Restart Razer Mouse Lite to apply it.") : ui("Could not save the language preference."));
    } else if (widget == self.reloadCss) {
        if (!self.appearance.reload()) self.setStatus(tr("Appearance", "Could not load the skin: %1", self.appearance.error()), true);
        else { self.refreshAppearance(); self.setStatus(tr("Appearance", "Appearance saved.")); }
    } else if (widget == self.skinBox || widget == self.themeButtons[0] || widget == self.themeButtons[1] || widget == self.themeButtons[2]) {
        auto mode = self.appearance.mode(); const char* modes[] = {"light", "dark", "system"};
        for (int i = 0; i < 3; ++i) if (widget == self.themeButtons[i]) mode = modes[i];
        const auto skins = self.appearance.skins(); const int index = self.skinBox->value();
        const auto skin = index >= 0 && index < static_cast<int>(skins.size()) ? skins[index] : self.appearance.skin();
        if (!self.appearance.choose(mode, skin, self.appearance.systemDark())) self.setStatus(tr("Appearance", "Could not load the skin: %1", self.appearance.error()), true);
        else self.setStatus(tr("Appearance", "Appearance saved."));
        self.refreshAppearance(); self.themeCheck = {};
    }
    self.window.redraw();
}
void RazerMouseWindow::refreshAppearance()
{
    const char* modes[] = {"light", "dark", "system"};
    for (int i = 0; i < 3; ++i) themeButtons[i]->value(appearance.mode() == modes[i]);
    const auto skins = appearance.skins();
    for (std::size_t i = 0; i < skins.size(); ++i) if (skins[i] == appearance.skin()) skinBox->value(static_cast<int>(i));
    Fl::redraw(); window.redraw();
}
void RazerMouseWindow::rebuildProfiles()
{
    auto* previousGroup = Fl_Group::current();
    profileItems->clear(); profileItems->begin();
    const int count = static_cast<int>(store.profiles.size()), scrollY = profileScroll->yposition();
    profileItems->resize(profileItems->x(), profileItems->y(), 190, std::max(420, count * 84));
    for (int i = 0; i < count; ++i) {
        const auto& profile = store.profiles[i];
        auto* item = new ProfileButton(css, profileItems->x(), profileItems->y() + i * 84, 190, 72);
        item->index = i; item->owner = this; item->stripe = profile.color;
        item->css_classes({"profile"}); item->type(FL_TOGGLE_BUTTON);
        item->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
        item->copy_label((profile.name + (profile.id == store.activeId ? "  *" : "") + "\n" + std::to_string(profile.dpi) + " DPI").c_str());
        item->value(i == std::clamp(editing, 0, std::max(0, count - 1)));
        item->callback([](Fl_Widget* widget, void*) {
            auto* item = static_cast<ProfileButton*>(widget); auto* owner = item->owner; const int index = item->index;
            owner->loadEditor(index); owner->activateProfile(index);
        });
    }
    profileItems->end(); Fl_Group::current(previousGroup);
    profileScroll->scroll_to(0, scrollY); profileScroll->redraw();
    if (count) loadEditor(std::clamp(editing, 0, count - 1));
    updateCapabilities();
}
void RazerMouseWindow::loadEditor(int index)
{
    if (index < 0 || index >= static_cast<int>(store.profiles.size()) || busy()) return;
    editing = index; const auto& profile = store.profiles[index];
    nameEdit->value(profile.name.c_str()); dpiEdit->value(std::to_string(profile.dpi).c_str());
    dpiSlider->value(std::clamp(profile.dpi, 100, static_cast<int>(dpiSlider->maximum())));
    speedSlider->value(profile.speed); speedLabel->copy_label((std::to_string(profile.speed) + " / 20").c_str());
    accelerationBox->value(profile.acceleration); pointerBox->value(profile.pointerSettings && Platform::pointerSettingsSupported());
    const int rates[] = {0, 125, 500, 1000}; for (int i = 0; i < 4; ++i) if (profile.pollRate == rates[i]) pollBox->value(i);
    lightingBox->value(profile.lighting == "static" ? 0 : profile.lighting == "off" ? 1 : 2);
    brightnessSlider->value(profile.brightness); brightnessLabel->copy_label((std::to_string(profile.brightness) + "%").c_str());
    editedColor = profile.color; updateColorButton();
    for (int i = 0; i < profileItems->children(); ++i) if (auto* item = dynamic_cast<ProfileButton*>(profileItems->child(i))) item->value(item->index == index);
    updateCapabilities(); window.redraw();
}
void RazerMouseWindow::updateColorButton()
{
    colorButton->copy_label(editedColor.hex().c_str());
    colorButton->swatch(fl_rgb_color(editedColor.r, editedColor.g, editedColor.b));
}
bool RazerMouseWindow::persist()
{
    if (preview) return true;
    if (readOnly) return false;
    std::string error; if (store.save(&error)) return true;
    setStatus(tr("RazerMouseWindow", "Could not save: %1", error), true); return false;
}
bool RazerMouseWindow::saveEditor()
{
    if (readOnly || busy() || editing < 0 || editing >= static_cast<int>(store.profiles.size())) return false;
    const auto name = trim(nameEdit->value());
    if (name.empty()) { setStatus(ui("Enter a profile name."), true); return false; }
    int dpi = 0;
    if (!integerInput(dpiEdit->value(), dpi) || dpi < 100 || dpi > 45000) { setStatus(tr("Protocol", "DPI is outside this mouse's supported range."), true); return false; }
    auto next = store.profiles[editing]; next.name = name; next.dpi = dpi;
    const int rates[] = {0, 125, 500, 1000}; next.pollRate = rates[std::clamp(pollBox->value(), 0, 3)];
    next.speed = static_cast<int>(speedSlider->value()); next.acceleration = accelerationBox->value();
    if (Platform::pointerSettingsSupported()) next.pointerSettings = pointerBox->value();
    next.lighting = lightingBox->value() == 0 ? "static" : lightingBox->value() == 1 ? "off" : "keep";
    next.color = editedColor; next.brightness = static_cast<int>(brightnessSlider->value()); next = Profile::fromJson(next.toJson());
    const auto previous = store.profiles[editing]; const auto active = store.activeId;
    store.profiles[editing] = next; if (next.id == store.activeId) store.activeId.clear();
    if (!persist()) { store.profiles[editing] = previous; store.activeId = active; return false; }
    rebuildProfiles(); rebuildTray(); return true;
}
void RazerMouseWindow::setStatus(const std::string& text, bool error)
{
    statusLabel->copy_label(text.c_str()); statusLabel->css_classes(error ? std::vector<std::string>{"status", "error"} : std::vector<std::string>{"status"});
    statusLabel->redraw();
}
void RazerMouseWindow::activateProfile(int index)
{
    if (preview || readOnly || index < 0 || index >= static_cast<int>(store.profiles.size()) || busy() || scan.valid()) return;
    const int selected = deviceBox->value();
    if (selected < 0 || selected >= static_cast<int>(devices.size())) { setStatus(ui("No supported mouse connected. Connect the mouse using USB and click Refresh."), true); return; }
    auto profile = store.profiles[index]; const auto device = devices[selected];
    if (!Platform::pointerSettingsSupported()) profile.pointerSettings = false;
    applyingId = profile.id; setStatus(tr("RazerMouseWindow", "Applying profile '%1'…", profile.name));
    application = std::async(std::launch::async, [device, profile] { return MouseBackend::apply(device, profile); }); updateCapabilities(); rebuildTray();
}
void RazerMouseWindow::rebuildTray()
{
    std::vector<Platform::TrayItem> items; std::string name = ui("No profile applied yet"); Color color;
    for (std::size_t i = 0; i < store.profiles.size(); ++i) {
        const auto& profile = store.profiles[i]; const bool active = profile.id == store.activeId;
        if (active) { name = profile.name; color = profile.color; }
        items.push_back({profile.name + " · " + std::to_string(profile.dpi) + " DPI", active, !readOnly && !busy() && !scan.valid(), static_cast<int>(i)});
    }
    items.push_back({}); items.push_back({tr("Appearance", "Next profile"), false, !readOnly && !busy() && !scan.valid(), -1});
    items.push_back({ui("Open settings"), false, true, -2}); items.push_back({ui("About"), false, true, -3}); items.push_back({ui("Quit"), false, true, -4});
    activeLabel->copy_label(name.c_str());
    desktop.setTray(items, color, tr("RazerMouseWindow", "Razer Mouse Lite · %1\nClick: next profile · Double-click: settings", name));
}
void RazerMouseWindow::trayAction(int action)
{
    if (action == -4) {
        quitting = true;
        while (auto* open = Fl::first_window()) open->hide();
        window.hide();
    }
    else if (action == -3) showAbout(); else if (action == -2) reveal();
    else if (action == -1) {
        if (readOnly || busy() || scan.valid() || store.profiles.empty()) return;
        int index = -1; for (std::size_t i = 0; i < store.profiles.size(); ++i) if (store.profiles[i].id == store.activeId) index = static_cast<int>(i);
        index = (index + 1) % static_cast<int>(store.profiles.size()); loadEditor(index); activateProfile(index);
    } else if (action >= 0) { loadEditor(action); activateProfile(action); }
}
void RazerMouseWindow::refreshDevices()
{
    if (preview) return;
    if (scan.valid() || busy()) { refreshAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(800); return; }
    restoreAfterScan = !suppressRestoreOnce; suppressRestoreOnce = false;
    scan = std::async(std::launch::async, MouseBackend::enumerate); updateCapabilities(); rebuildTray();
}
void RazerMouseWindow::finishScan(std::vector<MouseDevice> result)
{
    bool wasConnected = false; for (const auto& device : devices) if (device.path == store.devicePath) wasConnected = true;
    devices = std::move(result); deviceBox->clear(); int selected = -1;
    for (std::size_t i = 0; i < devices.size(); ++i) {
        add(deviceBox, devices[i].name + (preview ? " · USB 1532:00A3" : ""));
        if (devices[i].path == store.devicePath) selected = static_cast<int>(i);
    }
    if (selected < 0 && !devices.empty()) selected = 0;
    if (devices.empty()) add(deviceBox, ui("No supported USB mouse found"));
    deviceBox->value(std::max(0, selected)); if (selected >= 0) store.devicePath = devices[selected].path;
    updateCapabilities(); rebuildTray();
    const bool reconnect = !preview && store.restoreOnStart && !wasConnected && selected >= 0 && !store.activeId.empty();
    if (restoreAfterScan && (restorePending || reconnect) && selected >= 0) {
        restorePending = false;
        for (std::size_t i = 0; i < store.profiles.size(); ++i) if (store.profiles[i].id == store.activeId) { activateProfile(static_cast<int>(i)); break; }
    }
}
void RazerMouseWindow::updateCapabilities()
{
    const int selected = deviceBox->value(); const bool connected = selected >= 0 && selected < static_cast<int>(devices.size());
    const bool unlocked = !readOnly && !busy();
    enable(editor, unlocked); enable(profileItems, unlocked); enable(addButton, unlocked); enable(deleteButton, unlocked && store.profiles.size() > 1);
    enable(saveButton, unlocked); enable(deviceBox, !busy() && !scan.valid()); enable(refreshButton, !busy() && !scan.valid());
    enable(updateButton, !preview && !download.valid()); enable(restore, !readOnly);
    bool canPoll = true;
    if (connected) {
        const auto& model = devices[selected].model; const int maxDpi = number(model, "maxDpi", 8500);
        dpiSlider->maximum(maxDpi); dpiSlider->step(jsonBool(model, "dpiByte") ? 100 : 50);
        int saved = 1600; integerInput(dpiEdit->value(), saved); dpiSlider->value(std::clamp(saved, 100, maxDpi));
        canPoll = number(model, "pollTx", -1) >= 0;
        deviceLabel->copy_label((tr("RazerMouseWindow", "Detected automatically · 100–%1 DPI · ", maxDpi)
            + (array(model, "zones").empty() ? ui("No RGB control for this model") : ui("DPI and lighting over USB"))).c_str());
    } else deviceLabel->copy_label(ui("Connect your Razer mouse using USB. You can save profiles in advance.").c_str());
    if (!canPoll) pollBox->value(0);
    enable(pollBox, canPoll); const bool staticColor = lightingBox->value() == 0;
    enable(colorButton, staticColor); enable(brightnessSlider, staticColor);
    const bool pointer = Platform::pointerSettingsSupported(); enable(pointerBox, pointer);
    enable(speedSlider, pointer && pointerBox->value()); enable(accelerationBox, pointer && pointerBox->value());
    enable(applyButton, connected && unlocked && !scan.valid()); window.redraw();
}
void RazerMouseWindow::updateDefinitions()
{
    if (preview || download.valid()) return;
    setStatus(ui("Checking device definitions…")); download = std::async(std::launch::async, Platform::downloadDefinitions); updateCapabilities();
}
void RazerMouseWindow::finishDownload(ApplyResult result)
{
    if (!result.success) { setStatus(ui("Could not update device definitions. Check your connection and try again."), true); return; }
    Json candidate;
    if (!DeviceDatabase::validate(result.message, &candidate)) { setStatus(ui("These device definitions are incompatible or damaged. The current definitions have been kept."), true); return; }
    if (candidate == DeviceDatabase::models()) { setStatus(ui("Device definitions are up to date.")); return; }
    std::string error;
    if (!DeviceDatabase::install(result.message, DeviceDatabase::cachePath(), &error)) { setStatus(tr("RazerMouseWindow", "Could not save device definitions: %1", error), true); return; }
    setStatus(ui("Device definitions updated. No mouse settings were changed.")); suppressRestoreOnce = true; refreshDevices();
}
void RazerMouseWindow::timer(void* value)
{
    auto* self = static_cast<RazerMouseWindow*>(value); self->poll();
    if (!self->quitting) Fl::repeat_timeout(0.15, timer, self);
}
void RazerMouseWindow::poll()
{
    desktop.poll([this](int action) { trayAction(action); });
    if (desktop.revealRequested()) reveal();
    if (!desktop.trayAvailable() && !window.visible() && !quitting) reveal();
    const auto now = std::chrono::steady_clock::now();
    if (watcher && watcher->changed()) refreshAt = now + std::chrono::milliseconds(800);
    if (refreshAt != std::chrono::steady_clock::time_point{} && now >= refreshAt) { refreshAt = {}; refreshDevices(); }
    try {
        if (ready(scan)) finishScan(scan.get());
        if (ready(application)) {
            const auto result = application.get(); store.activeId = result.success ? applyingId : "";
            const bool saved = persist(); rebuildProfiles(); rebuildTray();
            if (saved) setStatus(result.message, !result.success);
        }
        if (ready(download)) { finishDownload(download.get()); updateCapabilities(); }
        if (ready(systemTheme)) {
            const bool previous = appearance.dark();
            if (!appearance.systemChanged(systemTheme.get())) setStatus(tr("Appearance", "Could not load the skin: %1", appearance.error()), true);
            if (previous != appearance.dark()) refreshAppearance();
        }
        if (appearance.mode() == "system" && !systemTheme.valid() && now >= themeCheck) {
            themeCheck = now + std::chrono::seconds(2); systemTheme = std::async(std::launch::async, Platform::systemDark);
        }
    } catch (const std::exception& error) { setStatus(error.what(), true); updateCapabilities(); rebuildTray(); }
}
void RazerMouseWindow::showAbout()
{
    Fl_Css_Window about(css, window.x() + 120, window.y() + 100, 720, 360, nullptr);
    about.copy_label(ui("About Razer Mouse Lite").c_str()); about.set_modal();
    label(css, 24, 18, 672, 42, "Razer Mouse Lite " MOUSE_LITE_VERSION, {"brand"});
    label(css, 24, 76, 672, 100, ui("Huge thanks to the OpenRazer and openrazer-win communities for reverse-engineering the device protocols and sharing their work. This app builds on their efforts."));
    label(css, 24, 184, 672, 64, ui("Want to help support a new device? OpenRazer explains how to capture and analyse USB traffic."), {"muted"});
    const std::pair<const char*, std::string> links[] = {{"https://openrazer.github.io/", "OpenRazer"}, {"https://github.com/Ar4ikov/openrazer-win", "openrazer-win"},
        {"https://github.com/openrazer/openrazer/wiki/Reverse-Engineering-USB-Protocol", ui("USB protocol reverse-engineering guide")}};
    for (int i = 0; i < 3; ++i) {
        auto* link = button(css, 24 + i * 228, 276, 216, 52, links[i].second);
        link->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
        link->callback([](Fl_Widget*, void* uri) { char error[256]{}; fl_open_uri(static_cast<const char*>(uri), error, sizeof(error)); }, const_cast<char*>(links[i].first));
    }
    about.end(); about.show(); while (about.shown()) Fl::wait();
}
bool RazerMouseWindow::uiContracts()
{
    if (Fl_Group::current() != nullptr) return false;
    if (!preview || store.profiles.size() != 3 || editing != 0) return false;
    const auto originals = store.profiles; const auto originalMode = appearance.mode(), originalSkin = appearance.skin();
    nameEdit->value(""); if (saveEditor()) return false;
    nameEdit->value("Preview · UTF-8"); dpiEdit->value("1850"); store.activeId = store.profiles[0].id;
    if (!saveEditor() || store.profiles[0].dpi != 1850 || store.profiles[0].name != "Preview · UTF-8" || !store.activeId.empty()) return false;
    const auto saved = store.profiles[0].toJson(); dpiEdit->value("not a number");
    if (saveEditor() || store.profiles[0].toJson() != saved) return false;
    loadEditor(0); lightingBox->value(1); updateCapabilities(); if (colorButton->active_r()) return false;
    pointerBox->value(0); updateCapabilities(); if (speedSlider->active_r() || accelerationBox->active_r()) return false;
    readOnly = true; updateCapabilities(); if (saveEditor() || saveButton->active_r() || profileItems->active_r()) return false;
    readOnly = false;
    for (const auto& skin : {"mint", "graphite", "sand"}) {
        if (!appearance.choose("light", skin, false)) return false;
        const auto light = window.css_style().background;
        if (!appearance.choose("dark", skin, false) || window.css_style().background == light) return false;
        if (!appearance.choose("system", skin, false) || appearance.dark()) return false;
        if (!appearance.systemChanged(true) || !appearance.dark()) return false;
    }
    if (appearance.choose("dark", "../invalid", false)) return false;
    store.profiles = originals; store.activeId.clear(); loadEditor(0); rebuildProfiles(); rebuildTray();
    if (!appearance.choose(originalMode, originalSkin, true)) return false;
    refreshAppearance(); setStatus(ui("Choose a profile or edit its settings."));
    return true;
}
bool RazerMouseWindow::smokeTest(const std::string& filename)
{
    if (!uiContracts()) return false;
    window.set_visible(); window.clear_damage(FL_DAMAGE_ALL);
    Fl_Image_Surface surface(window.w(), window.h()); Fl_Surface_Device::push_current(&surface); surface.draw(&window);
    std::unique_ptr<Fl_RGB_Image> image(surface.image()); Fl_Surface_Device::pop_current();
    if (!image || image->w() != window.w() || image->h() != window.h()) return false;
    const int stride = image->ld() ? image->ld() : image->w() * image->d();
    const auto* pixel = reinterpret_cast<const uchar*>(image->data()[0] + 180 * stride + 240 * image->d());
    uchar r, g, b; Fl::get_color(window.css_style().background, r, g, b);
    if (pixel[0] != r || pixel[1] != g || pixel[2] != b) return false;
    return fl_write_png(filename.c_str(), image.get()) == 0;
}
