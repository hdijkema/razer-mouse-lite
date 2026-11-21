#include "devicedatabase.h"
#include "razerprotocol.h"
#include "files.h"
#include "embedded.h"
#include <map>
#include <mutex>
#include <set>
#include <shared_mutex>

namespace {
std::shared_mutex databaseLock;
Json installedModels = Json::array();
bool integer(const Json& value, int minimum, int maximum)
{
    if (!value.is_number()) return false;
    const double n = value.get<double>();
    return std::isfinite(n) && n == std::floor(n) && n >= minimum && n <= maximum;
}
bool recipe(const Json& value)
{
    if (!value.is_array() || value.size() > 32) return false;
    static const std::map<std::string, int> functions = {
        {"get_razer_report", 3}, {"razer_chroma_standard_set_led_state", 3},
        {"razer_chroma_standard_set_led_effect", 3}, {"razer_chroma_standard_set_led_brightness", 3},
        {"razer_chroma_standard_set_led_rgb", 3}, {"razer_chroma_standard_matrix_effect_static", 1},
        {"razer_chroma_standard_matrix_effect_none", 0}, {"razer_chroma_extended_matrix_brightness", 3},
        {"razer_chroma_extended_matrix_effect_static", 3}, {"razer_chroma_extended_matrix_effect_none", 2},
        {"razer_chroma_mouse_extended_matrix_effect_static", 3}, {"razer_chroma_mouse_extended_matrix_effect_none", 2},
        {"razer_naga_trinity_effect_static", 1}, {"razer_naga_trinity_effect_none", 0}, {"razer_basilisk_mobile_effect_static", 1}};
    bool built = false;
    for (const auto& step : value) {
        if (!step.is_object()) return false;
        const auto op = text(step, "op");
        if (op == "build") {
            const auto fn = text(step, "fn");
            const auto found = functions.find(fn);
            if (found == functions.end() || !field(step, "args").is_array()) return false;
            const auto& args = array(step, "args");
            if (args.size() != static_cast<std::size_t>(found->second)) return false;
            for (const auto& arg : args) {
                const auto kind = text(arg, "k");
                if (kind == "const") { if (!integer(field(arg, "value"), 0, 255)) return false; }
                else if (kind == "var") { if (text(arg, "name") != "brightness") return false; }
                else if (kind == "rgb") { if (!integer(field(arg, "offset"), 0, 0)) return false; }
                else return false;
            }
            if (fn == "get_razer_report" && (field(args[0], "value") != 15 || field(args[1], "value") != 2 || field(args[2], "value") != 6)) return false;
            built = true;
        } else if (op == "txid" || op == "set_arg") {
            if (!built || !integer(field(field(step, "value"), "value"), 0, 255)) return false;
            if (op == "set_arg" && !integer(field(step, "index"), 0, 79)) return false;
        } else if (op == "send") { if (!built) return false; }
        else if (op != "stop") return false;
    }
    try {
        RazerProtocol::lightingReports(value, Color::fromHex("#123456"), 0);
        RazerProtocol::lightingReports(value, Color::fromHex("#abcdef"), 100);
    } catch (...) { return false; }
    return true;
}
}
Json DeviceDatabase::bundled()
{
    static const Json data = Json::parse(Embedded::mice);
    return data;
}
Json DeviceDatabase::models()
{
    std::shared_lock<std::shared_mutex> lock(databaseLock);
    return installedModels.empty() ? bundled() : installedModels;
}
bool DeviceDatabase::validate(const std::string& data, Json* models)
{
    if (data.empty() || data.size() > maximumBytes) return false;
    const auto candidate = Json::parse(data, nullptr, false);
    if (!candidate.is_array() || candidate.empty() || candidate.size() > 2048) return false;
    std::set<int> pids;
    static const std::set<std::string> fields = {"pid", "name", "maxDpi", "dpiByte", "dpiTx", "interface", "waitUs", "zones", "pollTx"};
    for (const auto& model : candidate) {
        if (!model.is_object() || model.size() != fields.size()) return false;
        for (auto it = model.begin(); it != model.end(); ++it) if (!fields.count(it.key())) return false;
        if (!integer(field(model, "pid"), 1, 65535) || !pids.insert(number(model, "pid")).second
            || !integer(field(model, "maxDpi"), 100, 65535) || !field(model, "dpiByte").is_boolean()
            || !integer(field(model, "dpiTx"), 0, 255) || !integer(field(model, "pollTx"), -1, 255)
            || !integer(field(model, "interface"), 0, 15) || !integer(field(model, "waitUs"), 0, 1000000)
            || trim(text(model, "name")).empty() || text(model, "name").size() > 640
            || !field(model, "zones").is_array() || array(model, "zones").size() > 5) return false;
        if (jsonBool(model, "dpiByte") && number(model, "maxDpi") > 25500) return false;
        std::set<std::string> zones;
        for (const auto& zone : array(model, "zones")) {
            const auto name = text(zone, "name");
            if (!zone.is_object() || zone.size() != 4 || !std::set<std::string>{"backlight", "logo", "scroll", "left", "right"}.count(name)
                || !zones.insert(name).second || !recipe(field(zone, "color")) || !recipe(field(zone, "off")) || !recipe(field(zone, "brightness"))) return false;
        }
    }
    if (models) *models = candidate;
    return true;
}
std::filesystem::path DeviceDatabase::cachePath() { return Files::configDirectory() / "device-definitions.json"; }
bool DeviceDatabase::install(const std::string& data, const std::filesystem::path& path, std::string* error)
{
    Json candidate;
    if (!validate(data, &candidate)) { if (error) *error = "Invalid device definitions"; return false; }
    const Json envelope = {{"format", 1}, {"appVersion", MOUSE_LITE_VERSION}, {"models", candidate}};
    if (!Files::atomicWrite(path, envelope.dump(), error)) return false;
    std::unique_lock<std::shared_mutex> lock(databaseLock);
    installedModels = std::move(candidate);
    return true;
}
bool DeviceDatabase::load(const std::filesystem::path& path)
{
    std::string bytes;
    if (!Files::read(path, bytes, maximumBytes + 1024)) return false;
    const auto envelope = Json::parse(bytes, nullptr, false);
    if (field(envelope, "format") != 1 || text(envelope, "appVersion") != MOUSE_LITE_VERSION || !field(envelope, "models").is_array()) return false;
    Json candidate;
    if (!validate(field(envelope, "models").dump(), &candidate)) return false;
    std::unique_lock<std::shared_mutex> lock(databaseLock);
    installedModels = std::move(candidate);
    return true;
}
