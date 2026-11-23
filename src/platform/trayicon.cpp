#include "trayicon.h"
#include "app-icon.h"

std::string Platform::profileIconSvg(Color color)
{
    auto svg = Embedded::appIcon;
    const std::string marker = "fill:#0f0";
    const auto position = svg.find(marker);
    if (position != std::string::npos) svg.replace(position, marker.size(), "fill:" + color.hex());
    return svg;
}
