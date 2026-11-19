#pragma once
#include <QByteArray>
#include <QColor>
#include <QJsonArray>
#include <QJsonObject>

class RazerProtocol
{
public:
    static QJsonArray models();
    static QList<QByteArray> lightingReports(const QJsonArray &steps, const QColor &color, int brightness);
    static QByteArray dpiReport(const QJsonObject &model, int dpi);
    static QByteArray report(int commandClass, int command, int size, int transaction = 0);
    static void checksum(QByteArray &report);
    static bool validResponse(const QByteArray &request, const QByteArray &response);
};
