#pragma once
#include <QByteArray>
namespace HidDescriptor {
// Size including the HID API's leading zero report-ID byte. Only report 0.
int featureReportLength(const QByteArray &descriptor);
}
