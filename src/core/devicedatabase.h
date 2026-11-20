#pragma once
#include <QByteArray>
#include <QJsonArray>
#include <QString>

class DeviceDatabase
{
public:
    static constexpr int maximumBytes = 2 * 1024 * 1024;
    static QJsonArray bundled();
    static QJsonArray models();
    static bool validate(const QByteArray &data, QJsonArray *models = nullptr);
    static bool install(const QByteArray &data, const QString &path, QString *error);
    static bool load(const QString &path);
    static QString cachePath();
};
