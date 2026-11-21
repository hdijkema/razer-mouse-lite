#pragma once
#include "common.h"
struct MouseDevice
{
    std::string path;
    std::string name;
    Json model;
    int pid = 0;
    std::string inputPath;
};
struct ApplyResult { bool success = false; std::string message; };
