#include "desktopentry.h"

QByteArray desktopAutostartEntry(const QString &executable)
{
    // First quote for Exec, then escape for the Desktop Entry string value.
    QString escaped = executable;
    escaped.replace("\\", "\\\\").replace("\"", "\\\"").replace("`", "\\`").replace("$", "\\$").replace("%", "%%");
    escaped.replace("\\", "\\\\").replace('\n', "\\n").replace('\r', "\\r").replace('\t', "\\t");
    return QString("[Desktop Entry]\nType=Application\nName=Razer Mouse Lite\nExec=\"%1\" --tray\nTerminal=false\nHidden=false\n").arg(escaped).toUtf8();
}
