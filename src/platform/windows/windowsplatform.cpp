#include "platform.h"
#include "razerprotocol.h"
#include <QAbstractNativeEventFilter>
#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QSettings>
#include <windows.h>
#include <dbt.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#include <vector>

namespace {
class Handle
{
public:
    explicit Handle(HANDLE value) : value(value) {}
    ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle(const Handle &) = delete;
    HANDLE value;
};

QString winError()
{
    const DWORD code = GetLastError();
    wchar_t *buffer = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, 0, reinterpret_cast<wchar_t *>(&buffer), 0, nullptr);
    const QString message = buffer ? QString::fromWCharArray(buffer).trimmed() : QString::number(code);
    if (buffer) LocalFree(buffer);
    return message;
}

HANDLE openControl(const std::wstring &path)
{
    // Windows owns pointer read/write collections exclusively. Feature reports
    // also work on a zero-access handle (FILE_ANY_ACCESS), as in openrazer-win.
    for (DWORD access : {DWORD(GENERIC_READ | GENERIC_WRITE), DWORD(GENERIC_WRITE), DWORD(0)}) {
        const HANDLE handle = CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          nullptr, OPEN_EXISTING, 0, nullptr);
        if (handle != INVALID_HANDLE_VALUE) return handle;
    }
    return INVALID_HANDLE_VALUE;
}


}
QList<MouseDevice> Platform::enumerate()
{
    QList<MouseDevice> devices;
    GUID guid;
    HidD_GetHidGuid(&guid);
    const HDEVINFO info = SetupDiGetClassDevsW(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (info == INVALID_HANDLE_VALUE) return devices;
    const auto database = RazerProtocol::models();
    SP_DEVICE_INTERFACE_DATA entry{};
    entry.cbSize = sizeof(entry);
    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(info, nullptr, &guid, index, &entry); ++index) {
        DWORD size = 0;
        SetupDiGetDeviceInterfaceDetailW(info, &entry, nullptr, 0, &size, nullptr);
        if (!size) continue;
        std::vector<unsigned char> buffer(size);
        auto detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W *>(buffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(info, &entry, detail, size, nullptr, nullptr)) continue;
        const QString path = QString::fromWCharArray(detail->DevicePath);
        if (!path.contains("vid_1532", Qt::CaseInsensitive)) continue;
        Handle handle(CreateFileW(detail->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
        if (handle.value == INVALID_HANDLE_VALUE) continue;
        HIDD_ATTRIBUTES attributes{}; attributes.Size = sizeof(attributes);
        if (!HidD_GetAttributes(handle.value, &attributes) || attributes.VendorID != 0x1532) continue;
        QJsonObject model;
        for (const auto &item : database) if (item.toObject().value("pid").toInt() == attributes.ProductID) model = item.toObject();
        if (model.isEmpty()) continue;
        PHIDP_PREPARSED_DATA preparsed = nullptr;
        if (!HidD_GetPreparsedData(handle.value, &preparsed)) continue;
        HIDP_CAPS caps{};
        const auto result = HidP_GetCaps(preparsed, &caps);
        HidD_FreePreparsedData(preparsed);
        if (result != HIDP_STATUS_SUCCESS || caps.FeatureReportByteLength != 91) continue;
        const auto match = QRegularExpression("&mi_([0-9a-f]{2})", QRegularExpression::CaseInsensitiveOption).match(path);
        const int actualInterface = match.hasMatch() ? match.captured(1).toInt(nullptr, 16) : -1;
        // Only use the model's known USB control interface. Never write to the pointer input collection.
        if (actualInterface != model.value("interface").toInt()) continue;
        devices.append({path, model.value("name").toString(), model, attributes.ProductID});
    }
    SetupDiDestroyDeviceInfoList(info);
    return devices;
}


bool Platform::startupEnabled()
{
    QSettings run("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    return !run.value("RazerMouseLite").toString().isEmpty();
}

bool Platform::setStartupEnabled(bool enabled, QString *error)
{
    QSettings run("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (enabled) run.setValue("RazerMouseLite", "\"" + QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + "\" --tray");
    else run.remove("RazerMouseLite");
    run.sync();
    if (run.status() == QSettings::NoError) return true;
    if (error) *error = QCoreApplication::translate("Platform", "Could not update automatic startup.");
    return false;
}


namespace {
class WindowsChannel final : public FeatureChannel
{
public:
    explicit WindowsChannel(HANDLE value) : handle(value) {}
    bool send(const QByteArray &packet, QString *error) override
    {
        if (HidD_SetFeature(handle.value, const_cast<char *>(packet.constData()), ULONG(packet.size()))) return true;
        if (error) *error = winError();
        return false;
    }
    QByteArray receive(int length, QString *error) override
    {
        QByteArray packet(length, '\0');
        if (HidD_GetFeature(handle.value, packet.data(), ULONG(packet.size()))) return packet;
        if (error) *error = winError();
        return {};
    }
private:
    Handle handle;
};
class DeviceWatcher final : public QObject, public QAbstractNativeEventFilter
{
public:
    DeviceWatcher(QObject *owner, std::function<void()> callback) : QObject(owner), changed(std::move(callback))
    { QCoreApplication::instance()->installNativeEventFilter(this); }
    ~DeviceWatcher() override { QCoreApplication::instance()->removeNativeEventFilter(this); }
    bool nativeEventFilter(const QByteArray &, void *message, qintptr *) override
    {
        const auto msg = static_cast<MSG *>(message);
        if (msg->message == WM_DEVICECHANGE && (msg->wParam == DBT_DEVNODES_CHANGED
            || msg->wParam == DBT_DEVICEARRIVAL || msg->wParam == DBT_DEVICEREMOVECOMPLETE)) changed();
        return false;
    }
private:
    std::function<void()> changed;
};
}
std::unique_ptr<FeatureChannel> Platform::openControl(const MouseDevice &device, QString *error)
{
    const HANDLE handle = ::openControl(device.path.toStdWString());
    if (handle == INVALID_HANDLE_VALUE) { if (error) *error = winError(); return {}; }
    return std::make_unique<WindowsChannel>(handle);
}
bool Platform::pointerSettingsSupported() { return true; }
QString Platform::pointerSettingsDescription()
{
    return QCoreApplication::translate("Platform", "Applies to all mice. Games using raw input may ignore these settings.");
}
bool Platform::applyPointerSettings(const MouseDevice &, int speed, bool acceleration, QString *error)
{
    int oldMouse[3]{};
    if (!SystemParametersInfoW(SPI_GETMOUSE, 0, oldMouse, 0)) {
        if (error) *error = QCoreApplication::translate("Platform", "Could not read desktop pointer settings.");
        return false;
    }
    int mouse[3]{6, 10, acceleration ? 1 : 0};
    const UINT flags = SPIF_UPDATEINIFILE | SPIF_SENDCHANGE;
    if (!SystemParametersInfoW(SPI_SETMOUSE, 0, mouse, flags)) {
        if (error) *error = QCoreApplication::translate("Platform", "Could not change pointer acceleration.");
        return false;
    }
    if (!SystemParametersInfoW(SPI_SETMOUSESPEED, 0, reinterpret_cast<void *>(INT_PTR(qBound(1, speed, 20))), flags)) {
        SystemParametersInfoW(SPI_SETMOUSE, 0, oldMouse, flags);
        if (error) *error = QCoreApplication::translate("Platform", "Could not change pointer speed.");
        return false;
    }
    return true;
}
void Platform::watchDevices(QObject *owner, std::function<void()> changed)
{
    new DeviceWatcher(owner, std::move(changed));
}


