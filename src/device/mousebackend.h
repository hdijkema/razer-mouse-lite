#pragma once
#include "mousetypes.h"
#include "profile.h"

class MouseBackend
{
public:
    static QList<MouseDevice> enumerate();
    static ApplyResult apply(const MouseDevice &device, const Profile &profile);
    static ApplyResult readCobraStatus(const MouseDevice &device);
};
