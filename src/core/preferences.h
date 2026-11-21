#pragma once
#include "common.h"
namespace Preferences {
bool load(Json& settings, std::string* error = nullptr);
bool set(const std::string& key, const Json& value, std::string* error = nullptr);
}
