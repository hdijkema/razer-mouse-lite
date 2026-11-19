// SPDX-License-Identifier: GPL-2.0-or-later
#include "mousebackend.h"
#include "platform.h"
#include "razerprotocol.h"
#include <QCoreApplication>
#include <QThread>
#include <stdexcept>

namespace {
[[noreturn]] void fail(const QString &message) { throw std::runtime_error(message.toUtf8().constData()); }

QByteArray sendReport(FeatureChannel &channel, const QByteArray &request, int waitUs)
{
    QByteArray packet(1, '\0');
    packet += request;
    QString error;
    if (!channel.send(packet, &error)) fail(QCoreApplication::translate("Backend", "HID write failed: %1").arg(error));
    for (int poll = 0; poll < 8; ++poll) {
        QThread::usleep(static_cast<unsigned long>(poll == 0 ? qMax(1000, waitUs) : 10000));
        auto response = channel.receive(91, &error);
        if (response.isEmpty()) fail(QCoreApplication::translate("Backend", "HID read failed: %1").arg(error));
        response.remove(0, 1);
        if (!RazerProtocol::validResponse(request, response)) continue;
        const auto status = static_cast<unsigned char>(response[0]);
        if (status == 2) return response;
        if (status == 0 || status == 1) continue;
        fail(QCoreApplication::translate("Backend", "Mouse rejected command %1:%2 (status %3). Close other mouse software and try again.")
            .arg(static_cast<unsigned char>(request[6]), 2, 16, QChar('0'))
            .arg(static_cast<unsigned char>(request[7]), 2, 16, QChar('0')).arg(status));
    }
    fail(QCoreApplication::translate("Backend", "No confirmation from the mouse. Close other mouse software and try again."));
}
}

QList<MouseDevice> MouseBackend::enumerate() { return Platform::enumerate(); }

ApplyResult MouseBackend::readCobraStatus(const MouseDevice &device)
{
    if (device.pid != 0x00a3) return {false, QCoreApplication::translate("Backend", "This diagnostic requires a wired Razer Cobra.")};
    try {
        QString error;
        auto channel = Platform::openControl(device, &error);
        if (!channel) fail(error);
        auto query = RazerProtocol::report(4, 0x85, 7, 0xff);
        RazerProtocol::checksum(query);
        const auto response = sendReport(*channel, query, 600);
        const int x = (static_cast<unsigned char>(response[9]) << 8) | static_cast<unsigned char>(response[10]);
        const int y = (static_cast<unsigned char>(response[11]) << 8) | static_cast<unsigned char>(response[12]);
        return {true, QCoreApplication::translate("Backend", "Razer Cobra reports DPI X=%1, Y=%2. No settings changed.").arg(x).arg(y)};
    } catch (const std::exception &error) { return {false, QString::fromUtf8(error.what())}; }
}

ApplyResult MouseBackend::apply(const MouseDevice &device, const Profile &profile)
{
    bool hardwareStarted = false;
    try {
        if (device.path.isEmpty()) return {false, QCoreApplication::translate("Backend", "No supported mouse connected. Connect the mouse using USB and click Refresh.")};
        if (profile.pointerSettings && !Platform::pointerSettingsSupported()) fail(Platform::pointerSettingsDescription());
        QList<QByteArray> packets{RazerProtocol::dpiReport(device.model, profile.dpi)};
        if (profile.pollRate != 0) {
            if (device.model.value("pollTx").toInt(-1) < 0 || !QList<int>{125, 500, 1000}.contains(profile.pollRate))
                fail(QCoreApplication::translate("Backend", "Polling rate is not supported for this model. Select Keep current."));
            auto polling = RazerProtocol::report(0, 5, 1, device.model.value("pollTx").toInt());
            polling[8] = char(1000 / profile.pollRate);
            RazerProtocol::checksum(polling);
            packets.append(polling);
        }
        const auto zones = device.model.value("zones").toArray();
        if (profile.lighting != "keep") {
            for (const auto &value : zones) {
                const auto zone = value.toObject();
                const auto commands = RazerProtocol::lightingReports(zone.value(profile.lighting == "off" ? "off" : "color").toArray(), profile.color, profile.brightness);
                if (commands.isEmpty()) fail(QCoreApplication::translate("Backend", "This lighting mode is not supported."));
                packets.append(commands);
                if (profile.lighting == "static") packets.append(RazerProtocol::lightingReports(zone.value("brightness").toArray(), profile.color, profile.brightness));
            }
        }
        QString error;
        auto channel = Platform::openControl(device, &error);
        if (!channel) fail(QCoreApplication::translate("Backend", "Could not open mouse: %1").arg(error));
        for (const auto &packet : packets) {
            hardwareStarted = true;
            sendReport(*channel, packet, device.model.value("waitUs").toInt(600));
        }
        if (profile.pointerSettings && !Platform::applyPointerSettings(device, profile.speed, profile.acceleration, &error)) fail(error);
        return {true, QCoreApplication::translate("Backend", "Profile '%1' applied · %2 DPI").arg(profile.name).arg(profile.dpi)};
    } catch (const std::exception &error) {
        return {false, QString::fromUtf8(error.what()) + (hardwareStarted ? QCoreApplication::translate("Backend", " Earlier hardware commands have not been rolled back.") : QString())};
    }
}

