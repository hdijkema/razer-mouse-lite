#pragma once
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using Json = nlohmann::json;
using Bytes = std::vector<std::uint8_t>;

inline const Json& field(const Json& object, const std::string& name)
{
    static const Json missing;
    if (!object.is_object()) return missing;
    const auto it = object.find(name);
    return it == object.end() ? missing : *it;
}
inline int number(const Json& object, const std::string& name, int fallback = 0)
{
    const auto& value = field(object, name);
    if (!value.is_number()) return fallback;
    const double n = value.get<double>();
    return std::isfinite(n) && std::floor(n) == n && n >= std::numeric_limits<int>::min()
        && n <= std::numeric_limits<int>::max() ? static_cast<int>(n) : fallback;
}
inline std::string text(const Json& object, const std::string& name, const std::string& fallback = {})
{
    const auto& value = field(object, name);
    return value.is_string() ? value.get<std::string>() : fallback;
}
inline bool jsonBool(const Json& object, const std::string& name, bool fallback = false)
{
    const auto& value = field(object, name);
    return value.is_boolean() ? value.get<bool>() : fallback;
}
inline const Json& array(const Json& object, const std::string& name)
{
    static const Json empty = Json::array();
    const auto& value = field(object, name);
    return value.is_array() ? value : empty;
}
inline std::string trim(const std::string& value)
{
    const auto start = value.find_first_not_of(" \r\n\t");
    return start == std::string::npos ? "" : value.substr(start, value.find_last_not_of(" \r\n\t") - start + 1);
}
struct Color
{
    std::uint8_t r = 85, g = 214, b = 160;
    std::string hex() const;
    static Color fromHex(const std::string& value, bool* valid = nullptr);
};
std::string newId();
std::string hexBytes(const Bytes& bytes);
Bytes fromHex(const std::string& hex);
