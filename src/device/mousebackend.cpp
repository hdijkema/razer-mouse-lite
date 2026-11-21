// SPDX-License-Identifier: GPL-2.0-or-later
#include "mousebackend.h"
#include "platform.h"
#include "razerprotocol.h"
#include "i18n.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <stdexcept>

namespace {
[[noreturn]] void fail(const std::string& message) { throw std::runtime_error(message); }
std::string byteHex(int n) { std::ostringstream s; s << std::hex << std::setfill('0') << std::setw(2) << n; return s.str(); }
Bytes sendReport(FeatureChannel& channel, const Bytes& request, int waitUs)
{
    Bytes packet{0}; packet.insert(packet.end(), request.begin(), request.end());
    std::string error;
    if (!channel.send(packet, &error)) fail(tr("Backend", "HID write failed: %1", error));
    for (int poll = 0; poll < 8; ++poll) {
        std::this_thread::sleep_for(std::chrono::microseconds(poll == 0 ? std::max(1000, waitUs) : 10000));
        auto response = channel.receive(91, &error);
        if (response.empty()) fail(tr("Backend", "HID read failed: %1", error));
        response.erase(response.begin());
        if (!RazerProtocol::validResponse(request, response)) continue;
        const int status = response[0];
        if (status == 2) return response;
        if (status == 0 || status == 1) continue;
        fail(tr("Backend", "Mouse rejected command %1:%2 (status %3). Close other mouse software and try again.", byteHex(request[6]), byteHex(request[7]), status));
    }
    fail(tr("Backend", "No confirmation from the mouse. Close other mouse software and try again."));
}
}
std::vector<MouseDevice> MouseBackend::enumerate() { return Platform::enumerate(); }
ApplyResult MouseBackend::readCobraStatus(const MouseDevice& device)
{
    if (device.pid != 0x00a3) return {false, tr("Backend", "This diagnostic requires a wired Razer Cobra.")};
    try {
        std::string error;
        auto channel = Platform::openControl(device, &error);
        if (!channel) fail(error);
        auto query = RazerProtocol::report(4, 0x85, 7, 0xff); RazerProtocol::checksum(query);
        const auto response = sendReport(*channel, query, 600);
        const int x = (response[9] << 8) | response[10], y = (response[11] << 8) | response[12];
        return {true, tr("Backend", "Razer Cobra reports DPI X=%1, Y=%2. No settings changed.", x, y)};
    } catch (const std::exception& error) { return {false, error.what()}; }
}
ApplyResult MouseBackend::apply(const MouseDevice& device, const Profile& profile)
{
    bool hardwareStarted = false;
    try {
        if (device.path.empty()) return {false, tr("Backend", "No supported mouse connected. Connect the mouse using USB and click Refresh.")};
        if (profile.pointerSettings && !Platform::pointerSettingsSupported()) fail(Platform::pointerSettingsDescription());
        std::vector<Bytes> packets{RazerProtocol::dpiReport(device.model, profile.dpi)};
        if (profile.pollRate != 0) {
            if (number(device.model, "pollTx", -1) < 0 || (profile.pollRate != 125 && profile.pollRate != 500 && profile.pollRate != 1000))
                fail(tr("Backend", "Polling rate is not supported for this model. Select Keep current."));
            auto polling = RazerProtocol::report(0, 5, 1, number(device.model, "pollTx"));
            polling[8] = static_cast<std::uint8_t>(1000 / profile.pollRate); RazerProtocol::checksum(polling); packets.push_back(polling);
        }
        if (profile.lighting != "keep") for (const auto& zone : array(device.model, "zones")) {
            const auto commands = RazerProtocol::lightingReports(array(zone, profile.lighting == "off" ? "off" : "color"), profile.color, profile.brightness);
            if (commands.empty()) fail(tr("Backend", "This lighting mode is not supported."));
            packets.insert(packets.end(), commands.begin(), commands.end());
            if (profile.lighting == "static") {
                const auto brightness = RazerProtocol::lightingReports(array(zone, "brightness"), profile.color, profile.brightness);
                packets.insert(packets.end(), brightness.begin(), brightness.end());
            }
        }
        std::string error;
        auto channel = Platform::openControl(device, &error);
        if (!channel) fail(tr("Backend", "Could not open mouse: %1", error));
        for (const auto& packet : packets) { hardwareStarted = true; sendReport(*channel, packet, number(device.model, "waitUs", 600)); }
        if (profile.pointerSettings && !Platform::applyPointerSettings(device, profile.speed, profile.acceleration, &error)) fail(error);
        return {true, tr("Backend", "Profile '%1' applied ? %2 DPI", profile.name, profile.dpi)};
    } catch (const std::exception& error) {
        return {false, std::string(error.what()) + (hardwareStarted ? tr("Backend", " Earlier hardware commands have not been rolled back.") : "")};
    }
}
