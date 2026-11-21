#pragma once
#include "mousetypes.h"
#include <functional>
#include <memory>

class FeatureChannel
{
public:
    virtual ~FeatureChannel() = default;
    virtual bool send(const Bytes& packet, std::string* error) = 0;
    virtual Bytes receive(int length, std::string* error) = 0;
};
namespace Platform {
std::vector<MouseDevice> enumerate();
std::unique_ptr<FeatureChannel> openControl(const MouseDevice& device, std::string* error);
bool startupEnabled();
bool setStartupEnabled(bool enabled, std::string* error);
bool pointerSettingsSupported();
std::string pointerSettingsDescription();
bool applyPointerSettings(const MouseDevice& device, int speed, bool acceleration, std::string* error);
// The URL is fixed by the implementation; no redirects or automatic requests.
ApplyResult downloadDefinitions();
bool systemDark();
class DeviceWatcher
{
public:
    DeviceWatcher();
    ~DeviceWatcher();
    bool changed();
private:
    struct Data; std::unique_ptr<Data> data;
};
struct TrayItem { std::string label; bool checked = false; bool enabled = true; int action = 0; };
class DesktopIntegration
{
public:
    DesktopIntegration(bool isolated = false);
    ~DesktopIntegration();
    bool primary() const;
    bool valid() const;
    bool revealRequested();
    bool trayAvailable() const;
    void setTray(const std::vector<TrayItem>& items, Color color, const std::string& tooltip);
    // Profile indices >= 0, next=-1, reveal=-2, about=-3, quit=-4.
    void poll(const std::function<void(int)>& action);
private:
    struct Data; std::unique_ptr<Data> data;
};
}
