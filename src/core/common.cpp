#include "common.h"
#include <array>
#include <iomanip>
#include <random>
#include <sstream>

std::string Color::hex() const
{
    std::ostringstream out;
    out << '#' << std::hex << std::setfill('0') << std::setw(2) << int(r)
        << std::setw(2) << int(g) << std::setw(2) << int(b);
    return out.str();
}
Color Color::fromHex(const std::string& value, bool* valid)
{
    if (valid) *valid = false;
    if (value.size() != 7 || value[0] != '#') return {};
    const auto bytes = ::fromHex(value.substr(1));
    if (bytes.size() != 3) return {};
    if (valid) *valid = true;
    return {bytes[0], bytes[1], bytes[2]};
}
Bytes fromHex(const std::string& hex)
{
    if (hex.size() % 2) return {};
    auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    Bytes out;
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const int a = digit(hex[i]), b = digit(hex[i + 1]);
        if (a < 0 || b < 0) return {};
        out.push_back(static_cast<std::uint8_t>((a << 4) | b));
    }
    return out;
}
std::string hexBytes(const Bytes& bytes)
{
    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    for (auto byte : bytes) { out += digits[byte >> 4]; out += digits[byte & 15]; }
    return out;
}
std::string newId()
{
    std::random_device random;
    Bytes bytes(16);
    for (auto& byte : bytes) byte = static_cast<std::uint8_t>(random());
    bytes[6] = static_cast<std::uint8_t>((bytes[6] & 15) | 0x40);
    bytes[8] = static_cast<std::uint8_t>((bytes[8] & 63) | 0x80);
    const auto hex = hexBytes(bytes);
    return hex.substr(0, 8) + '-' + hex.substr(8, 4) + '-' + hex.substr(12, 4) + '-' + hex.substr(16, 4) + '-' + hex.substr(20);
}
