#include "razerprotocol.h"
#include "devicedatabase.h"
#include "i18n.h"
#include <stdexcept>

Json RazerProtocol::models() { return DeviceDatabase::models(); }
Bytes RazerProtocol::report(int commandClass, int command, int size, int transaction)
{
    Bytes result(90, 0);
    result[1] = static_cast<std::uint8_t>(transaction); result[5] = static_cast<std::uint8_t>(size);
    result[6] = static_cast<std::uint8_t>(commandClass); result[7] = static_cast<std::uint8_t>(command);
    return result;
}
void RazerProtocol::checksum(Bytes& packet)
{
    if (packet.size() != 90) throw std::runtime_error("Invalid report length");
    std::uint8_t crc = 0;
    for (int i = 2; i < 88; ++i) crc ^= packet[i];
    packet[88] = crc;
}
bool RazerProtocol::validResponse(const Bytes& request, const Bytes& response)
{
    if (request.size() != 90 || response.size() != 90) return false;
    auto checked = response; checksum(checked);
    return checked[88] == response[88] && std::equal(request.begin() + 2, request.begin() + 5, response.begin() + 2)
        && std::equal(request.begin() + 6, request.begin() + 8, response.begin() + 6);
}
Bytes RazerProtocol::dpiReport(const Json& model, int dpi)
{
    if (dpi < 100 || dpi > number(model, "maxDpi", 45000))
        throw std::runtime_error(tr("Protocol", "DPI is outside this mouse's supported range."));
    const bool byteDpi = jsonBool(model, "dpiByte");
    auto packet = report(4, byteDpi ? 1 : 5, byteDpi ? 3 : 7, number(model, "dpiTx"));
    if (byteDpi) {
        if (dpi % 100 != 0 || dpi > 25500) throw std::runtime_error(tr("Protocol", "This mouse supports DPI in steps of 100."));
        packet[8] = packet[9] = static_cast<std::uint8_t>(dpi / 100);
    } else {
        // Upstream misc_set_dpi_xy always sends VARSTORE, including NOSTORE recipes.
        packet[8] = 1;
        packet[9] = packet[11] = static_cast<std::uint8_t>(dpi >> 8);
        packet[10] = packet[12] = static_cast<std::uint8_t>(dpi & 255);
    }
    checksum(packet);
    return packet;
}
std::vector<Bytes> RazerProtocol::lightingReports(const Json& steps, const Color& color, int brightness)
{
    if (!steps.is_array()) throw std::runtime_error("Expected lighting recipe array");
    std::vector<Bytes> packets;
    Bytes current;
    auto rgb = [&](int offset) { current.at(8 + offset) = color.r; current.at(9 + offset) = color.g; current.at(10 + offset) = color.b; };
    for (const auto& step : steps) {
        const auto op = text(step, "op");
        if (op == "stop") break;
        if (op == "build") {
            const auto& args = array(step, "args");
            auto arg = [&](int i) {
                const auto& a = args.at(i);
                if (text(a, "k") == "const") return number(a, "value");
                if (text(a, "name") == "brightness") return std::clamp(brightness, 0, 100) * 255 / 100;
                throw std::runtime_error(tr("Protocol", "Unknown protocol argument."));
            };
            auto fn = text(step, "fn");
            if (fn.rfind("razer_chroma_", 0) == 0) fn.erase(0, 13);
            if (fn == "get_razer_report") current = report(arg(0), arg(1), arg(2));
            else if (fn == "standard_set_led_state" || fn == "standard_set_led_effect" || fn == "standard_set_led_brightness") {
                current = report(3, fn == "standard_set_led_state" ? 0 : fn == "standard_set_led_effect" ? 2 : 3, 3);
                for (int i = 0; i < 3; ++i) current[8 + i] = static_cast<std::uint8_t>(arg(i));
            } else if (fn == "standard_set_led_rgb") {
                current = report(3, 1, 5); current[8] = static_cast<std::uint8_t>(arg(0)); current[9] = static_cast<std::uint8_t>(arg(1)); rgb(2);
            } else if (fn == "standard_matrix_effect_static" || fn == "standard_matrix_effect_none") {
                const bool on = fn == "standard_matrix_effect_static";
                current = report(3, 10, on ? 4 : 1); current[8] = on ? 6 : 0; if (on) rgb(1);
            } else if (fn == "extended_matrix_brightness") {
                current = report(15, 4, 3);
                for (int i = 0; i < 3; ++i) current[8 + i] = static_cast<std::uint8_t>(arg(i));
            } else if (fn == "extended_matrix_effect_static" || fn == "extended_matrix_effect_none") {
                const bool on = fn == "extended_matrix_effect_static";
                current = report(15, 2, on ? 9 : 6);
                current[8] = static_cast<std::uint8_t>(arg(0)); current[9] = static_cast<std::uint8_t>(arg(1)); current[10] = on ? 1 : 0;
                if (on) { current[13] = 1; rgb(6); }
            } else if (fn == "mouse_extended_matrix_effect_static" || fn == "mouse_extended_matrix_effect_none") {
                const bool on = fn == "mouse_extended_matrix_effect_static";
                current = report(3, 13, on ? 6 : 3);
                current[8] = static_cast<std::uint8_t>(arg(0)); current[9] = static_cast<std::uint8_t>(arg(1)); current[10] = on ? 6 : 0;
                if (on) rgb(3);
            } else if (fn == "razer_naga_trinity_effect_static" || fn == "razer_basilisk_mobile_effect_static") {
                const bool naga = fn == "razer_naga_trinity_effect_static";
                current = report(15, 3, naga ? 14 : 11); current[12] = naga ? 2 : 1;
                rgb(5); rgb(8); if (naga) rgb(11);
            } else if (fn == "razer_naga_trinity_effect_none") current = report(15, 2, 6);
            else throw std::runtime_error(tr("Protocol", "Unknown protocol function: %1", fn));
        } else if (op == "txid" && current.size() == 90) current[1] = static_cast<std::uint8_t>(number(field(step, "value"), "value"));
        else if (op == "set_arg" && current.size() == 90) {
            const int index = number(step, "index", -1);
            if (index < 0 || index >= 80) throw std::runtime_error(tr("Protocol", "Invalid protocol index."));
            current[8 + index] = static_cast<std::uint8_t>(number(field(step, "value"), "value"));
        } else if (op == "send" && current.size() == 90) { checksum(current); packets.push_back(current); }
        else throw std::runtime_error(tr("Protocol", "Invalid protocol step."));
    }
    return packets;
}
