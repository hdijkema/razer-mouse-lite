#include "files.h"
#include "common.h"
#include <cstdlib>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {
bool failed(std::string* error, const std::string& message)
{ if (error) *error = message; return false; }
#ifdef _WIN32
std::string utf8(const std::wstring& value)
{
    if (value.empty()) return {};
    const int length = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length, nullptr, nullptr);
    return result;
}
std::string systemError() { return "Windows error " + std::to_string(GetLastError()); }
#endif
}

std::filesystem::path Files::configDirectory()
{
#ifdef _WIN32
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
        std::filesystem::path result(path);
        CoTaskMemFree(path);
        return result / "MouseLite" / "MouseLite";
    }
    throw std::runtime_error("Could not locate the user settings folder.");
#else
    const auto xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && std::filesystem::path(xdg).is_absolute()) return std::filesystem::path(xdg) / "MouseLite/MouseLite";
    const auto home = std::getenv("HOME");
    if (!home || !*home) throw std::runtime_error("HOME is not set.");
    return std::filesystem::path(home) / ".config/MouseLite/MouseLite";
#endif
}
std::filesystem::path Files::executable()
{
#ifdef _WIN32
    std::wstring path(32768, L'\0');
    const auto size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!size || size == path.size()) throw std::runtime_error("Could not locate the executable.");
    path.resize(size);
    return path;
#else
    return std::filesystem::read_symlink("/proc/self/exe");
#endif
}
std::string Files::systemLocale()
{
#ifdef _WIN32
    wchar_t locale[LOCALE_NAME_MAX_LENGTH]{};
    return GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH) ? utf8(locale) : "en";
#else
    for (const char* key : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
        const char* value = std::getenv(key);
        if (value && *value) return value;
    }
    return "en";
#endif
}
std::string Files::legacyLanguage()
{
#ifdef _WIN32
    wchar_t value[64]{};
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\MouseLite\\MouseLite\\ui", L"language",
                    RRF_RT_REG_SZ, nullptr, value, &size) == ERROR_SUCCESS) return utf8(value);
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    const char* home = std::getenv("HOME");
    const auto base = xdg && std::filesystem::path(xdg).is_absolute() ? std::filesystem::path(xdg)
        : home ? std::filesystem::path(home) / ".config" : std::filesystem::path();
    std::ifstream file(base / "MouseLite/MouseLite.conf");
    std::string line, section;
    while (std::getline(file, line)) {
        line = trim(line);
        if (!line.empty() && line.front() == '[') section = line;
        if (section == "[ui]" && line.rfind("language=", 0) == 0) return line.substr(9);
    }
#endif
    return {};
}
bool Files::read(const std::filesystem::path& path, std::string& contents, std::size_t maximum, std::string* error)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return failed(error, "Cannot open " + path.u8string());
    std::string result;
    char buffer[16384];
    while (file) {
        file.read(buffer, sizeof(buffer));
        result.append(buffer, static_cast<std::size_t>(file.gcount()));
        if (result.size() > maximum) return failed(error, "File exceeds the size limit: " + path.u8string());
    }
    if (file.bad()) return failed(error, "Cannot read " + path.u8string());
    contents = std::move(result);
    return true;
}
bool Files::atomicWrite(const std::filesystem::path& path, const std::string& contents, std::string* error)
{
    std::error_code ec;
    const auto parent = path.has_parent_path() ? path.parent_path() : std::filesystem::current_path();
    std::filesystem::create_directories(parent, ec);
    if (ec) return failed(error, ec.message());
    const auto temporary = parent / (path.filename().u8string() + "." + newId() + ".tmp");
#ifdef _WIN32
    HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return failed(error, systemError());
    DWORD written = 0;
    const bool writtenOk = contents.size() <= MAXDWORD && WriteFile(file, contents.data(), static_cast<DWORD>(contents.size()), &written, nullptr)
        && written == contents.size() && FlushFileBuffers(file);
    std::string failure = writtenOk ? "" : systemError();
    CloseHandle(file);
    if (writtenOk && MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
    if (failure.empty()) failure = systemError();
    DeleteFileW(temporary.c_str());
    return failed(error, failure);
#else
    int fd = ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (fd < 0) return failed(error, std::strerror(errno));
    bool ok = true;
    std::size_t offset = 0;
    while (offset < contents.size()) {
        const auto count = ::write(fd, contents.data() + offset, contents.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = false; break; }
        offset += static_cast<std::size_t>(count);
    }
    if (ok && ::fsync(fd) != 0) ok = false;
    const std::string failure = std::strerror(errno);
    if (::close(fd) != 0) ok = false;
    if (ok && ::rename(temporary.c_str(), path.c_str()) == 0) {
        const int directory = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (directory >= 0) { ::fsync(directory); ::close(directory); }
        return true;
    }
    const auto message = ok ? std::string(std::strerror(errno)) : failure;
    ::unlink(temporary.c_str());
    return failed(error, message);
#endif
}
