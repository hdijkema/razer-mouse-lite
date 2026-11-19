#pragma once
#include "mousetypes.h"
#include <QByteArray>
#include <QList>
#include <QObject>
#include <functional>
#include <memory>

// Native APIs are confined to the selected implementation of this interface.
class FeatureChannel
{
public:
    virtual ~FeatureChannel() = default;
    virtual bool send(const QByteArray &packet, QString *error) = 0;
    virtual QByteArray receive(int length, QString *error) = 0;
};

namespace Platform {
QList<MouseDevice> enumerate();
std::unique_ptr<FeatureChannel> openControl(const MouseDevice &device, QString *error);
bool startupEnabled();
bool setStartupEnabled(bool enabled, QString *error);
bool pointerSettingsSupported();
QString pointerSettingsDescription();
bool applyPointerSettings(const MouseDevice &device, int speed, bool acceleration, QString *error);
void watchDevices(QObject *owner, std::function<void()> changed);
}
