#pragma once
#include <filesystem>
#include <string>

namespace Files {
std::filesystem::path configDirectory();
std::filesystem::path executable();
std::string systemLocale();
std::string legacyLanguage();
bool read(const std::filesystem::path& path, std::string& contents, std::size_t maximum, std::string* error = nullptr);
bool atomicWrite(const std::filesystem::path& path, const std::string& contents, std::string* error = nullptr);
}
