#include "profile.h"
#include "files.h"
#include "i18n.h"
#include <set>

Json Profile::toJson() const
{
    return {{"id", id}, {"name", name}, {"dpi", dpi}, {"pollRate", pollRate}, {"speed", speed},
        {"acceleration", acceleration}, {"pointerSettings", pointerSettings},
        {"lighting", lighting}, {"color", color.hex()}, {"brightness", brightness}};
}
Profile Profile::fromJson(const Json& json)
{
    Profile p;
    p.id = text(json, "id");
    if (p.id.empty()) p.id = newId();
    p.name = text(json, "name", tr("Profiles", "Profile"));
    // Keep complete UTF-8 characters while limiting the editor to 80 characters.
    std::size_t end = 0, characters = 0;
    while (end < p.name.size()) {
        if ((static_cast<unsigned char>(p.name[end]) & 0xc0) != 0x80 && ++characters > 80) break;
        ++end;
    }
    p.name.resize(end);
    p.dpi = std::clamp(number(json, "dpi", 1600), 100, 45000);
    p.pollRate = number(json, "pollRate");
    if (p.pollRate != 0 && p.pollRate != 125 && p.pollRate != 500 && p.pollRate != 1000) p.pollRate = 0;
    p.speed = std::clamp(number(json, "speed", 10), 1, 20);
    p.acceleration = jsonBool(json, "acceleration");
    p.pointerSettings = jsonBool(json, "pointerSettings", jsonBool(json, "windowsSettings", true));
    p.lighting = text(json, "lighting", "static");
    if (p.lighting != "static" && p.lighting != "off" && p.lighting != "keep") p.lighting = "keep";
    p.color = Color::fromHex(text(json, "color", "#55d6a0"));
    p.brightness = std::clamp(number(json, "brightness", 65), 0, 100);
    return p;
}
std::filesystem::path ProfileStore::fileName() { return Files::configDirectory() / "profiles.json"; }

bool ProfileStore::load(std::string* error)
{
    const auto target = storagePath.empty() ? fileName() : storagePath;
    std::vector<Profile> candidate;
    std::string nextActive, nextDevice;
    bool nextRestore = false;
    std::error_code ec;
    const bool exists = std::filesystem::exists(target, ec);
    if (ec) { if (error) *error = ec.message(); loadFailed = true; return false; }
    if (exists) {
        std::string bytes;
        if (!Files::read(target, bytes, 4 * 1024 * 1024, error)) { loadFailed = true; return false; }
        const auto root = Json::parse(bytes, nullptr, false);
        if (!root.is_object() || number(root, "schema") != 1 || !field(root, "profiles").is_array()) {
            if (error) *error = tr("Profiles", "Invalid profile file. The file will not be overwritten.");
            loadFailed = true;
            return false;
        }
        std::set<std::string> ids;
        for (const auto& entry : array(root, "profiles")) {
            if (!entry.is_object()) continue;
            auto profile = Profile::fromJson(entry);
            if (!ids.insert(profile.id).second) { profile.id = newId(); ids.insert(profile.id); }
            candidate.push_back(std::move(profile));
        }
        nextActive = text(root, "activeId");
        nextDevice = text(root, "devicePath");
        nextRestore = jsonBool(root, "restoreOnStart");
    }
    if (candidate.empty()) {
        const char* names[] = {"Everyday", "Precision", "Fast"};
        const char* colors[] = {"#55d6a0", "#7ca9ff", "#f2b96a"};
        const int dpi[] = {1600, 800, 3200};
        for (int i = 0; i < 3; ++i) {
            Profile p;
            p.id = newId(); p.name = tr("Profiles", names[i]); p.dpi = dpi[i]; p.color = Color::fromHex(colors[i]);
            candidate.push_back(std::move(p));
        }
    }
    profiles = std::move(candidate); activeId = std::move(nextActive); devicePath = std::move(nextDevice); restoreOnStart = nextRestore;
    loadFailed = false;
    return true;
}
bool ProfileStore::save(std::string* error) const
{
    if (loadFailed) {
        if (error) *error = tr("Profiles", "Invalid profile file. The file will not be overwritten.");
        return false;
    }
    Json entries = Json::array();
    for (const auto& profile : profiles) entries.push_back(profile.toJson());
    return Files::atomicWrite(storagePath.empty() ? fileName() : storagePath,
        Json{{"schema", 1}, {"profiles", entries}, {"activeId", activeId}, {"devicePath", devicePath},
            {"restoreOnStart", restoreOnStart}}.dump(2), error);
}
