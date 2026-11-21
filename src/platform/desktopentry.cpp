#include "desktopentry.h"
std::string desktopAutostartEntry(const std::string& executable)
{
    std::string quoted;
    for (char c : executable) {
        if (c == '%' ) quoted += "%%";
        else if (c == '\\' || c == '"' || c == '`' || c == '$') { quoted += '\\'; quoted += c; }
        else quoted += c;
    }
    std::string escaped;
    for (char c : quoted) {
        if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else escaped += c;
    }
    return "[Desktop Entry]\nType=Application\nName=Razer Mouse Lite\nExec=\"" + escaped + "\" --tray\nTerminal=false\nHidden=false\n";
}
