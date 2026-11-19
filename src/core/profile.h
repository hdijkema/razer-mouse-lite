#pragma once
#include <QColor>
#include <QJsonObject>
#include <QList>

struct Profile
{
    QString id;
    QString name;
    int dpi = 1600;
    int pollRate = 0;
    int speed = 10;
    bool acceleration = false;
    bool pointerSettings = true;
    QString lighting = "static";
    QColor color = QColor("#55d6a0");
    int brightness = 65;
    QJsonObject toJson() const;
    static Profile fromJson(const QJsonObject &json);
};

class ProfileStore
{
public:
    explicit ProfileStore(const QString &storagePath = QString()) : storagePath(storagePath) {}
    QList<Profile> profiles;
    QString activeId;
    QString devicePath;
    bool restoreOnStart = false;
    bool load(QString *error = nullptr);
    bool save(QString *error = nullptr) const;
    static QString fileName();
private:
    QString storagePath;
};
