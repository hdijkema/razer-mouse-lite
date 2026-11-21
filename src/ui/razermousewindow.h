#pragma once
#include "mousebackend.h"
#include "platform.h"
#include "widgets.h"
#include "appearance.h"
#include <chrono>
#include <future>

class RazerMouseWindow
{
public:
    explicit RazerMouseWindow(Platform::DesktopIntegration& desktop, bool preview = false, const std::string& mode = {}, const std::string& skin = {});
    ~RazerMouseWindow();
    void show();
    void reveal();
    bool finished() const { return quitting; }
    bool smokeTest(const std::string& image);
    bool uiContracts();
private:
    void buildUi();
    void showAbout();
    void updateDefinitions();
    void finishDownload(ApplyResult result);
    void refreshDevices();
    void finishScan(std::vector<MouseDevice> result);
    void rebuildProfiles();
    void rebuildTray();
    void loadEditor(int index);
    bool saveEditor();
    bool persist();
    void activateProfile(int index);
    void updateColorButton();
    void updateCapabilities();
    void setStatus(const std::string& text, bool error = false);
    void trayAction(int action);
    void poll();
    void refreshAppearance();
    static void timer(void* self);
    static void callback(Fl_Widget* widget, void* self);
    template<class T> void enable(T* widget, bool enabled) { if (enabled) widget->activate(); else widget->deactivate(); }
    bool busy() const { return application.valid(); }
    FLTK_Css css;
    Appearance appearance;
    Fl_Css_Window window;
    Platform::DesktopIntegration& desktop;
    ProfileStore store;
    std::vector<MouseDevice> devices;
    std::future<std::vector<MouseDevice>> scan;
    std::future<ApplyResult> application;
    std::future<ApplyResult> download;
    std::future<bool> systemTheme;
    std::chrono::steady_clock::time_point themeCheck{};
    std::unique_ptr<Platform::DeviceWatcher> watcher;
    std::chrono::steady_clock::time_point refreshAt{};
    CssScroll* profileScroll = nullptr;
    Fl_Css_Group* profileItems = nullptr;
    Fl_Css_Group* editor = nullptr;
    CssChoice* deviceBox = nullptr;
    Fl_Css_Input* nameEdit = nullptr;
    Fl_Css_Input* dpiEdit = nullptr;
    CssSlider* dpiSlider = nullptr;
    CssSlider* speedSlider = nullptr;
    Fl_Css_Box* speedLabel = nullptr;
    CssChoice* pollBox = nullptr;
    CssCheck* accelerationBox = nullptr;
    CssCheck* pointerBox = nullptr;
    CssChoice* lightingBox = nullptr;
    CssColorButton* colorButton = nullptr;
    CssSlider* brightnessSlider = nullptr;
    Fl_Css_Box* brightnessLabel = nullptr;
    Fl_Css_Box* statusLabel = nullptr;
    Fl_Css_Box* deviceLabel = nullptr;
    Fl_Css_Box* activeLabel = nullptr;
    Fl_Css_Button* applyButton = nullptr;
    Fl_Css_Button* saveButton = nullptr;
    Fl_Css_Button* refreshButton = nullptr;
    Fl_Css_Button* updateButton = nullptr;
    Fl_Css_Button* addButton = nullptr;
    Fl_Css_Button* deleteButton = nullptr;
    Fl_Css_Button* aboutButton = nullptr;
    CssCheck* startup = nullptr;
    CssCheck* restore = nullptr;
    CssChoice* languageBox = nullptr;
    CssChoice* skinBox = nullptr;
    Fl_Css_Button* themeButtons[3]{};
    Fl_Css_Button* reloadCss = nullptr;
    Color editedColor;
    int editing = -1;
    bool readOnly = false;
    bool preview = false;
    bool quitting = false;
    bool restorePending = false;
    bool suppressRestoreOnce = false;
    bool restoreAfterScan = true;
    std::string applyingId;
};
