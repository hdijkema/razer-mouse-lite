#pragma once
#include "common.h"
#include <filesystem>

struct Profile
{
    std::string id;
    std::string name;
    int dpi = 1600;
    int pollRate = 0;
    int speed = 10;
    bool acceleration = false;
    bool pointerSettings = true;
    std::string lighting = "static";
    Color color;
    int brightness = 65;
    Json toJson() const;
    static Profile fromJson(const Json& json);
};

class ProfileStore
{
public:
    explicit ProfileStore(std::filesystem::path storagePath = {}) : storagePath(std::move(storagePath)) {}
    std::vector<Profile> profiles;
    std::string activeId;
    std::string devicePath;
    bool restoreOnStart = false;
    bool load(std::string* error = nullptr);
    bool save(std::string* error = nullptr) const;
    static std::filesystem::path fileName();
private:
    std::filesystem::path storagePath;
    bool loadFailed = false;
};
