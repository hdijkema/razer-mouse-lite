// SPDX-License-Identifier: GPL-2.0-or-later
#include "platform.h"
#include "razerprotocol.h"
#include "hiddescriptor.h"
#include "desktopentry.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <libudev.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {
QString systemError() { return QString::fromLocal8Bit(std::strerror(errno)); }
QString attr(udev_device *device, const char *name)
{
    const char *value = device ? udev_device_get_sysattr_value(device, name) : nullptr;
    return value ? QString::fromUtf8(value) : QString();
}
QString autostartFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/autostart/mouse-lite.desktop";
}
class LinuxChannel final : public FeatureChannel
{
public:
    explicit LinuxChannel(int descriptor) : descriptor(descriptor) {}
    ~LinuxChannel() override { ::close(descriptor); }
    bool send(const QByteArray &packet, QString *error) override
    {
        int count;
        do { count = ioctl(descriptor, HIDIOCSFEATURE(packet.size()), packet.constData()); } while (count < 0 && errno == EINTR);
        if (count == packet.size()) return true;
        if (error) *error = count < 0 ? systemError() : QCoreApplication::translate("Platform", "Incomplete HID transfer.");
        return false;
    }
    QByteArray receive(int length, QString *error) override
    {
        QByteArray packet(length, '\0');
        int count;
        do { count = ioctl(descriptor, HIDIOCGFEATURE(length), packet.data()); } while (count < 0 && errno == EINTR);
        if (count == length) return packet;
        if (error) *error = count < 0 ? systemError() : QCoreApplication::translate("Platform", "Incomplete HID transfer.");
        return {};
    }
private:
    int descriptor;
};
class DeviceWatcher final : public QObject
{
public:
    DeviceWatcher(QObject *owner, std::function<void()> changed) : QObject(owner)
    {
        context = udev_new();
        if (!context) return;
        monitor = udev_monitor_new_from_netlink(context, "udev");
        if (!monitor) return;
        udev_monitor_filter_add_match_subsystem_devtype(monitor, "hidraw", nullptr);
        if (udev_monitor_enable_receiving(monitor) < 0) return;
        auto notifier = new QSocketNotifier(udev_monitor_get_fd(monitor), QSocketNotifier::Read, this);
        connect(notifier, &QSocketNotifier::activated, this, [this, changed] {
            while (auto event = udev_monitor_receive_device(monitor)) {
                udev_device_unref(event);
                changed();
            }
        });
    }
    ~DeviceWatcher() override
    {
        // Disable Qt's notifier before releasing its file descriptor.
        for (auto notifier : findChildren<QSocketNotifier *>()) notifier->setEnabled(false);
        if (monitor) udev_monitor_unref(monitor);
        if (context) udev_unref(context);
    }
private:
    udev *context = nullptr;
    udev_monitor *monitor = nullptr;
};
bool xinput(const QStringList &arguments, QString *output = nullptr)
{
    QProcess process;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("LC_ALL", "C");
    process.setProcessEnvironment(environment);
    process.start(QStandardPaths::findExecutable("xinput"), arguments);
    if (!process.waitForStarted(1500)) return false;
    if (!process.waitForFinished(2000)) { process.kill(); process.waitForFinished(); return false; }
    if (output) *output = QString::fromUtf8(process.readAllStandardOutput());
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}
}

QList<MouseDevice> Platform::enumerate()
{
    QList<MouseDevice> result;
    auto context = udev_new();
    if (!context) return result;
    auto enumeration = udev_enumerate_new(context);
    if (!enumeration) { udev_unref(context); return result; }
    udev_enumerate_add_match_subsystem(enumeration, "hidraw");
    udev_enumerate_scan_devices(enumeration);
    const auto database = RazerProtocol::models();
    udev_list_entry *entry;
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(enumeration)) {
        auto raw = udev_device_new_from_syspath(context, udev_list_entry_get_name(entry));
        if (!raw) continue;
        const auto usb = udev_device_get_parent_with_subsystem_devtype(raw, "usb", "usb_device");
        const auto interface = udev_device_get_parent_with_subsystem_devtype(raw, "usb", "usb_interface");
        const auto hid = udev_device_get_parent_with_subsystem_devtype(raw, "hid", nullptr);
        const char *node = udev_device_get_devnode(raw);
        if (node && usb && interface && hid && attr(usb, "idVendor").toInt(nullptr, 16) == 0x1532) {
            const int pid = attr(usb, "idProduct").toInt(nullptr, 16);
            QJsonObject model;
            for (const auto &item : database) if (item.toObject().value("pid").toInt() == pid) model = item.toObject();
            if (!model.isEmpty() && attr(interface, "bInterfaceNumber").toInt(nullptr, 16) == model.value("interface").toInt()) {
                const QString hidPath = QString::fromUtf8(udev_device_get_syspath(hid));
                QFile descriptor(hidPath + "/report_descriptor");
                if (descriptor.open(QIODevice::ReadOnly) && HidDescriptor::featureReportLength(descriptor.readAll()) == 91) {
                    QString inputPath;
                    QDirIterator events(hidPath + "/input", {"event*"}, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
                    if (events.hasNext()) inputPath = "/dev/input/" + QFileInfo(events.next()).fileName();
                    result.append({QString::fromUtf8(node), model.value("name").toString(), model, pid, inputPath});
                }
            }
        }
        udev_device_unref(raw);
    }
    udev_enumerate_unref(enumeration);
    udev_unref(context);
    return result;
}
std::unique_ptr<FeatureChannel> Platform::openControl(const MouseDevice &device, QString *error)
{
    const int descriptor = ::open(QFile::encodeName(device.path).constData(), O_RDWR | O_CLOEXEC);
    if (descriptor < 0) {
        if (error) *error = QCoreApplication::translate("Platform", "Cannot access %1: %2. Install the supplied udev rule and reconnect the mouse.").arg(device.path, systemError());
        return {};
    }
    // Revalidate device identity: /dev/hidraw numbers can change after hotplug.
    hidraw_devinfo info{};
    if (ioctl(descriptor, HIDIOCGRAWINFO, &info) < 0 || quint16(info.vendor) != 0x1532 || quint16(info.product) != device.pid) {
        ::close(descriptor);
        if (error) *error = QCoreApplication::translate("Platform", "Mouse identity changed. Click Refresh and try again.");
        return {};
    }
    return std::make_unique<LinuxChannel>(descriptor);
}
bool Platform::startupEnabled()
{
    QSettings entry(autostartFile(), QSettings::IniFormat);
    return QFile::exists(autostartFile()) && !entry.value("Desktop Entry/Hidden", false).toBool();
}
bool Platform::setStartupEnabled(bool enabled, QString *error)
{
    const QString target = autostartFile();
    if (!enabled) {
        if (!QFile::exists(target) || QFile::remove(target)) return true;
        if (error) *error = QCoreApplication::translate("Platform", "Could not update automatic startup.");
        return false;
    }
    const QByteArray content = desktopAutostartEntry(QCoreApplication::applicationFilePath());
    QSaveFile file(target);
    if (!QDir().mkpath(QFileInfo(target).absolutePath()) || !file.open(QIODevice::WriteOnly)
        || file.write(content) != content.size() || !file.commit()) {
        if (error) *error = QCoreApplication::translate("Platform", "Could not update automatic startup.");
        return false;
    }
    return true;
}
bool Platform::pointerSettingsSupported()
{
    return qEnvironmentVariable("XDG_SESSION_TYPE") != "wayland" && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
        && !qEnvironmentVariableIsEmpty("DISPLAY") && !QStandardPaths::findExecutable("xinput").isEmpty();
}
QString Platform::pointerSettingsDescription()
{
    return pointerSettingsSupported()
        ? QCoreApplication::translate("Platform", "Applies to the selected X11/libinput mouse. Games using raw input may ignore these settings.")
        : QCoreApplication::translate("Platform", "Pointer speed and acceleration are managed by your Wayland desktop, or require xinput on X11. DPI and lighting still work.");
}
bool Platform::applyPointerSettings(const MouseDevice &device, int speed, bool acceleration, QString *error)
{
    if (!pointerSettingsSupported()) { if (error) *error = pointerSettingsDescription(); return false; }
    QString ids;
    if (device.inputPath.isEmpty() || !xinput({"--list", "--id-only"}, &ids)) {
        if (error) *error = QCoreApplication::translate("Platform", "Could not find the selected mouse in X11.");
        return false;
    }
    for (const auto &id : ids.split('\n', Qt::SkipEmptyParts)) {
        if (!QRegularExpression("^\\d+$").match(id.trimmed()).hasMatch()) continue;
        QString properties;
        if (!xinput({"--list-props", id.trimmed()}, &properties)) continue;
        const auto node = QRegularExpression("Device Node \\(\\d+\\):\\s*\"([^\"]+)\"").match(properties);
        if (!node.hasMatch() || node.captured(1) != device.inputPath) continue;
        const auto oldProfile = QRegularExpression("libinput Accel Profile Enabled \\(\\d+\\):\\s*([0-9, \\t]+)").match(properties);
        if (!oldProfile.hasMatch() || !properties.contains("libinput Accel Speed (")) break;
        const auto oldValues = oldProfile.captured(1).split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
        if (oldValues.size() < 2 || oldValues.size() > 3) break;
        QStringList profileArgs{"--set-prop", "--type=int", "--format=8", id.trimmed(), "libinput Accel Profile Enabled"};
        auto newProfile = profileArgs;
        newProfile << (acceleration ? "1" : "0") << (acceleration ? "0" : "1");
        if (oldValues.size() == 3) newProfile << "0";
        if (!xinput(newProfile)) break;
        const double normalized = (qBound(1, speed, 20) - 1) / 19.0 * 2.0 - 1.0;
        if (xinput({"--set-prop", "--type=float", id.trimmed(), "libinput Accel Speed", QString::number(normalized, 'f', 6)})) return true;
        profileArgs.append(oldValues);
        xinput(profileArgs);
        break;
    }
    if (error) *error = QCoreApplication::translate("Platform", "Could not change the selected mouse's X11/libinput pointer settings.");
    return false;
}
void Platform::watchDevices(QObject *owner, std::function<void()> changed)
{
    new DeviceWatcher(owner, std::move(changed));
}

