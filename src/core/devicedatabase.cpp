#include "devicedatabase.h"
#include "razerprotocol.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QReadWriteLock>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <cmath>

namespace {
QReadWriteLock databaseLock;
QJsonArray installedModels;

bool integer(const QJsonValue &value, int minimum, int maximum)
{
    return value.isDouble() && std::isfinite(value.toDouble())
        && value.toDouble() == std::floor(value.toDouble())
        && value.toDouble() >= minimum && value.toDouble() <= maximum;
}

bool recipe(const QJsonValue &value)
{
    if (!value.isArray() || value.toArray().size() > 32) return false;
    const QMap<QString, int> functions{
        {"get_razer_report", 3},
        {"razer_chroma_standard_set_led_state", 3},
        {"razer_chroma_standard_set_led_effect", 3},
        {"razer_chroma_standard_set_led_brightness", 3},
        {"razer_chroma_standard_set_led_rgb", 3},
        {"razer_chroma_standard_matrix_effect_static", 1},
        {"razer_chroma_standard_matrix_effect_none", 0},
        {"razer_chroma_extended_matrix_brightness", 3},
        {"razer_chroma_extended_matrix_effect_static", 3},
        {"razer_chroma_extended_matrix_effect_none", 2},
        {"razer_chroma_mouse_extended_matrix_effect_static", 3},
        {"razer_chroma_mouse_extended_matrix_effect_none", 2},
        {"razer_naga_trinity_effect_static", 1},
        {"razer_naga_trinity_effect_none", 0},
        {"razer_basilisk_mobile_effect_static", 1}
    };
    bool built = false;
    for (const auto &entry : value.toArray()) {
        if (!entry.isObject()) return false;
        const auto step = entry.toObject();
        const auto op = step.value("op").toString();
        if (op == "build") {
            const auto fn = step.value("fn").toString();
            if (!functions.contains(fn) || !step.value("args").isArray()) return false;
            const auto args = step.value("args").toArray();
            if (args.size() != functions.value(fn)) return false;
            for (const auto &argument : args) {
                const auto arg = argument.toObject();
                const auto kind = arg.value("k").toString();
                if (kind == "const") { if (!integer(arg.value("value"), 0, 255)) return false; }
                else if (kind == "var") { if (arg.value("name") != "brightness") return false; }
                else if (kind == "rgb") { if (!integer(arg.value("offset"), 0, 0)) return false; }
                else return false;
            }
            if (fn == "get_razer_report" && (args.at(0).toObject().value("value") != 15
                || args.at(1).toObject().value("value") != 2 || args.at(2).toObject().value("value") != 6)) return false;
            built = true;
        } else if (op == "txid" || op == "set_arg") {
            if (!built || !integer(step.value("value").toObject().value("value"), 0, 255)) return false;
            if (op == "set_arg" && !integer(step.value("index"), 0, 79)) return false;
        } else if (op == "send") { if (!built) return false; }
        else if (op != "stop") return false;
    }
    try {
        RazerProtocol::lightingReports(value.toArray(), QColor("#123456"), 0);
        RazerProtocol::lightingReports(value.toArray(), QColor("#abcdef"), 100);
    } catch (...) { return false; }
    return true;
}
}

QJsonArray DeviceDatabase::bundled()
{
    static const auto database = [] {
        QFile file(":/data/mice.json");
        if (!file.open(QIODevice::ReadOnly)) return QJsonArray{};
        return QJsonDocument::fromJson(file.readAll()).array();
    }();
    return database;
}

QJsonArray DeviceDatabase::models()
{
    QReadLocker locker(&databaseLock);
    return installedModels.isEmpty() ? bundled() : installedModels;
}

bool DeviceDatabase::validate(const QByteArray &data, QJsonArray *models)
{
    if (data.isEmpty() || data.size() > maximumBytes) return false;
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !document.isArray()) return false;
    const auto array = document.array();
    if (array.isEmpty() || array.size() > 2048) return false;
    QSet<int> pids;
    const QSet<QString> fields{"pid", "name", "maxDpi", "dpiByte", "dpiTx", "interface", "waitUs", "zones", "pollTx"};
    for (const auto &value : array) {
        if (!value.isObject()) return false;
        const auto model = value.toObject();
        if (model.size() != fields.size()) return false;
        for (const auto &key : model.keys()) if (!fields.contains(key)) return false;
        if (!integer(model.value("pid"), 1, 65535) || pids.contains(model.value("pid").toInt())
            || !integer(model.value("maxDpi"), 100, 65535) || !model.value("dpiByte").isBool()
            || !integer(model.value("dpiTx"), 0, 255) || !integer(model.value("pollTx"), -1, 255)
            || !integer(model.value("interface"), 0, 15) || !integer(model.value("waitUs"), 0, 1000000)
            || model.value("name").toString().trimmed().isEmpty() || model.value("name").toString().size() > 160
            || !model.value("zones").isArray() || model.value("zones").toArray().size() > 5) return false;
        if (model.value("dpiByte").toBool() && model.value("maxDpi").toInt() > 25500) return false;
        pids.insert(model.value("pid").toInt());
        QSet<QString> zones;
        for (const auto &entry : model.value("zones").toArray()) {
            const auto zone = entry.toObject();
            const auto name = zone.value("name").toString();
            if (zone.size() != 4 || !QSet<QString>{"backlight", "logo", "scroll", "left", "right"}.contains(name)
                || zones.contains(name) || !recipe(zone.value("color")) || !recipe(zone.value("off"))
                || !recipe(zone.value("brightness"))) return false;
            zones.insert(name);
        }
    }
    if (models) *models = array;
    return true;
}

QString DeviceDatabase::cachePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/device-definitions.json";
}

bool DeviceDatabase::install(const QByteArray &data, const QString &path, QString *error)
{
    QJsonArray candidate;
    if (!validate(data, &candidate)) return false;
    const QJsonObject envelope{{"format", 1}, {"appVersion", QCoreApplication::applicationVersion()}, {"models", candidate}};
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) { if (error) *error = path; return false; }
    QSaveFile file(path);
    const auto bytes = QJsonDocument(envelope).toJson(QJsonDocument::Compact);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    QWriteLocker locker(&databaseLock);
    installedModels = candidate;
    return true;
}

bool DeviceDatabase::load(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > maximumBytes + 1024) return false;
    const auto envelope = QJsonDocument::fromJson(file.readAll()).object();
    if (envelope.value("format") != 1 || envelope.value("appVersion") != QCoreApplication::applicationVersion()) return false;
    QJsonArray candidate;
    if (!validate(QJsonDocument(envelope.value("models").toArray()).toJson(QJsonDocument::Compact), &candidate)) return false;
    QWriteLocker locker(&databaseLock);
    installedModels = candidate;
    return true;
}
