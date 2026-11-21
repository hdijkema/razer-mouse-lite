#include "preferences.h"
#include "files.h"
bool Preferences::load(Json& settings, std::string* error)
{
    const auto path = Files::configDirectory() / "settings.json";
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) && !ec) { settings = Json::object(); return true; }
    if (ec) { if (error) *error = ec.message(); return false; }
    std::string bytes;
    if (!Files::read(path, bytes, 65536, error)) return false;
    settings = Json::parse(bytes, nullptr, false);
    if (settings.is_object()) return true;
    if (error) *error = "Invalid preferences file. The file will not be overwritten.";
    return false;
}
bool Preferences::set(const std::string& key, const Json& value, std::string* error)
{
    Json settings;
    if (!load(settings, error)) return false;
    settings[key] = value;
    return Files::atomicWrite(Files::configDirectory() / "settings.json", settings.dump(2), error);
}
