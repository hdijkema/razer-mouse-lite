#pragma once
#include "common.h"
#include <filesystem>
class DeviceDatabase
{
public:
    static constexpr int maximumBytes = 2 * 1024 * 1024;
    static Json bundled();
    static Json models();
    static bool validate(const std::string& data, Json* models = nullptr);
    static bool install(const std::string& data, const std::filesystem::path& path, std::string* error);
    static bool load(const std::filesystem::path& path);
    static std::filesystem::path cachePath();
};
