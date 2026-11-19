#include "profile.h"
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

QJsonObject Profile::toJson() const
{
    return {{"id", id}, {"name", name}, {"dpi", dpi}, {"pollRate", pollRate}, {"speed", speed},
            {"acceleration", acceleration}, {"pointerSettings", pointerSettings},
            {"lighting", lighting}, {"color", color.name()}, {"brightness", brightness}};
}

Profile Profile::fromJson(const QJsonObject &json)
{
    Profile p;
    p.id = json.value("id").toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
    p.name = json.value("name").toString(QCoreApplication::translate("Profiles", "Profile")).left(80);
    p.dpi = qBound(100, json.value("dpi").toInt(1600), 45000);
    p.pollRate = json.value("pollRate").toInt();
    if (!QList<int>{0, 125, 500, 1000}.contains(p.pollRate)) p.pollRate = 0;
    p.speed = qBound(1, json.value("speed").toInt(10), 20);
    p.acceleration = json.value("acceleration").toBool();
    p.pointerSettings = json.value("pointerSettings").toBool(json.value("windowsSettings").toBool(true));
    p.lighting = json.value("lighting").toString("static");
    if (!QStringList{"static", "off", "keep"}.contains(p.lighting)) p.lighting = "keep";
    p.color = QColor(json.value("color").toString("#55d6a0"));
    if (!p.color.isValid()) p.color = QColor("#55d6a0");
    p.brightness = qBound(0, json.value("brightness").toInt(65), 100);
    return p;
}

QString ProfileStore::fileName()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/profiles.json";
}

bool ProfileStore::load(QString *error)
{
    QFile file(storagePath.isEmpty() ? fileName() : storagePath);
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly)) { if (error) *error = file.errorString(); return false; }
        QJsonParseError parse;
        const auto document = QJsonDocument::fromJson(file.readAll(), &parse);
        if (parse.error != QJsonParseError::NoError || !document.isObject()
            || document.object().value("schema").toInt() != 1
            || !document.object().value("profiles").isArray()) {
            if (error) *error = QCoreApplication::translate("Profiles", "Invalid profile file. The file will not be overwritten.");
            return false;
        }
        const auto root = document.object();
        for (const auto &entry : root.value("profiles").toArray()) {
            if (!entry.isObject()) continue;
            auto p = Profile::fromJson(entry.toObject());
            bool duplicate = false;
            for (const auto &existing : profiles) if (existing.id == p.id) duplicate = true;
            if (duplicate) p.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            profiles.append(p);
        }
        activeId = root.value("activeId").toString();
        devicePath = root.value("devicePath").toString();
        restoreOnStart = root.value("restoreOnStart").toBool();
    }
    if (profiles.isEmpty()) {
        for (int i = 0; i < 3; ++i) {
            Profile p;
            p.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            p.name = QStringList{QCoreApplication::translate("Profiles", "Everyday"), QCoreApplication::translate("Profiles", "Precision"), QCoreApplication::translate("Profiles", "Fast")}.at(i);
            p.dpi = QList<int>{1600, 800, 3200}.at(i);
            p.color = QColor(QStringList{"#55d6a0", "#7ca9ff", "#f2b96a"}.at(i));
            profiles.append(p);
        }
    }
    return true;
}

bool ProfileStore::save(QString *error) const
{
    const QString target = storagePath.isEmpty() ? fileName() : storagePath;
    if (!QDir().mkpath(QFileInfo(target).absolutePath())) {
        if (error) *error = QCoreApplication::translate("Profiles", "Could not create the settings folder.");
        return false;
    }
    QJsonArray entries;
    for (const auto &profile : profiles) entries.append(profile.toJson());
    const QByteArray data = QJsonDocument(QJsonObject{{"schema", 1}, {"profiles", entries},
        {"activeId", activeId}, {"devicePath", devicePath}, {"restoreOnStart", restoreOnStart}}).toJson();
    QSaveFile file(target);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

