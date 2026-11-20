#include "razerprotocol.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <stdexcept>
#include "devicedatabase.h"
QJsonArray RazerProtocol::models()
{
    return DeviceDatabase::models();
}

QByteArray RazerProtocol::report(int commandClass, int command, int size, int transaction)
{
    QByteArray result(90, '\0');
    result[1] = char(transaction);
    result[5] = char(size);
    result[6] = char(commandClass);
    result[7] = char(command);
    return result;
}

void RazerProtocol::checksum(QByteArray &packet)
{
    unsigned char crc = 0;
    for (int i = 2; i < 88; ++i) crc ^= static_cast<unsigned char>(packet[i]);
    packet[88] = char(crc);
}

bool RazerProtocol::validResponse(const QByteArray &request, const QByteArray &response)
{
    if (request.size() != 90 || response.size() != 90) return false;
    auto checked = response;
    checksum(checked);
    return checked[88] == response[88] && request.mid(2, 3) == response.mid(2, 3)
        && request.mid(6, 2) == response.mid(6, 2);
}

QByteArray RazerProtocol::dpiReport(const QJsonObject &model, int dpi)
{
    if (dpi < 100 || dpi > model.value("maxDpi").toInt(45000))
        throw std::runtime_error(QCoreApplication::translate("Protocol", "DPI is outside this mouse's supported range.").toUtf8().constData());
    const bool byteDpi = model.value("dpiByte").toBool();
    auto packet = report(4, byteDpi ? 1 : 5, byteDpi ? 3 : 7, model.value("dpiTx").toInt());
    if (byteDpi) {
        if (dpi % 100 != 0 || dpi > 25500) throw std::runtime_error(QCoreApplication::translate("Protocol", "This mouse supports DPI in steps of 100.").toUtf8().constData());
        packet[8] = packet[9] = char(dpi / 100);
    } else {
        // Upstream misc_set_dpi_xy always sends VARSTORE, even for NOSTORE recipes.
        packet[8] = 1;
        packet[9] = packet[11] = char(dpi >> 8);
        packet[10] = packet[12] = char(dpi & 255);
    }
    checksum(packet);
    return packet;
}

QList<QByteArray> RazerProtocol::lightingReports(const QJsonArray &steps, const QColor &color, int brightness)
{
    QList<QByteArray> packets;
    QByteArray current;
    auto rgb = [&](int offset) {
        current[8 + offset] = char(color.red());
        current[9 + offset] = char(color.green());
        current[10 + offset] = char(color.blue());
    };
    for (const auto &value : steps) {
        const auto step = value.toObject();
        const QString op = step.value("op").toString();
        if (op == "stop") break;
        if (op == "build") {
            const auto args = step.value("args").toArray();
            auto arg = [&](int i) {
                const auto a = args.at(i).toObject();
                if (a.value("k") == "const") return a.value("value").toInt();
                if (a.value("name") == "brightness") return qBound(0, brightness, 100) * 255 / 100;
                throw std::runtime_error(QCoreApplication::translate("Protocol", "Unknown protocol argument.").toUtf8().constData());
            };
            QString fn = step.value("fn").toString();
            fn.remove("razer_chroma_");
            if (fn == "get_razer_report") current = report(arg(0), arg(1), arg(2));
            else if (fn == "standard_set_led_state" || fn == "standard_set_led_effect" || fn == "standard_set_led_brightness") {
                current = report(3, fn.endsWith("state") ? 0 : fn.endsWith("effect") ? 2 : 3, 3);
                for (int i = 0; i < 3; ++i) current[8 + i] = char(arg(i));
            } else if (fn == "standard_set_led_rgb") {
                current = report(3, 1, 5); current[8] = char(arg(0)); current[9] = char(arg(1)); rgb(2);
            } else if (fn == "standard_matrix_effect_static" || fn == "standard_matrix_effect_none") {
                const bool on = fn.endsWith("static");
                current = report(3, 10, on ? 4 : 1); current[8] = on ? 6 : 0;
                if (on) rgb(1);
            } else if (fn == "extended_matrix_brightness") {
                current = report(15, 4, 3);
                for (int i = 0; i < 3; ++i) current[8 + i] = char(arg(i));
            } else if (fn == "extended_matrix_effect_static" || fn == "extended_matrix_effect_none") {
                const bool on = fn.endsWith("static");
                current = report(15, 2, on ? 9 : 6);
                current[8] = char(arg(0)); current[9] = char(arg(1)); current[10] = on ? 1 : 0;
                if (on) { current[13] = 1; rgb(6); }
            } else if (fn == "mouse_extended_matrix_effect_static" || fn == "mouse_extended_matrix_effect_none") {
                const bool on = fn.endsWith("static");
                current = report(3, 13, on ? 6 : 3);
                current[8] = char(arg(0)); current[9] = char(arg(1)); current[10] = on ? 6 : 0;
                if (on) rgb(3);
            } else if (fn == "razer_naga_trinity_effect_static" || fn == "razer_basilisk_mobile_effect_static") {
                const bool naga = fn.contains("naga");
                current = report(15, 3, naga ? 14 : 11); current[12] = naga ? 2 : 1;
                rgb(5); rgb(8); if (naga) rgb(11);
            } else if (fn == "razer_naga_trinity_effect_none") {
                current = report(15, 2, 6);
            } else throw std::runtime_error(QCoreApplication::translate("Protocol", "Unknown protocol function: %1").arg(fn).toUtf8().constData());
        } else if (op == "txid" && current.size() == 90) {
            current[1] = char(step.value("value").toObject().value("value").toInt());
        } else if (op == "set_arg" && current.size() == 90) {
            const int index = step.value("index").toInt(-1);
            if (index < 0 || index >= 80) throw std::runtime_error(QCoreApplication::translate("Protocol", "Invalid protocol index.").toUtf8().constData());
            current[8 + index] = char(step.value("value").toObject().value("value").toInt());
        } else if (op == "send" && current.size() == 90) {
            checksum(current); packets.append(current);
        } else throw std::runtime_error(QCoreApplication::translate("Protocol", "Invalid protocol step.").toUtf8().constData());
    }
    return packets;
}



