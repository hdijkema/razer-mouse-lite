#pragma once
#include <QJsonObject>
#include <QString>

struct MouseDevice
{
    QString path;
    QString name;
    QJsonObject model;
    int pid = 0;
    QString inputPath;
};

struct ApplyResult
{
    bool success = false;
    QString message;
};
