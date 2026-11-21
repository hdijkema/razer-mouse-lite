#include "selftest.h"
#include "mousebackend.h"
#include "platform.h"
#include "razerprotocol.h"
#include "profile.h"
#include "hiddescriptor.h"
#include "desktopentry.h"
#include "translations.h"
#include "devicedatabase.h"
#include "files.h"
#include "embedded.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>

namespace {
int checks = 0;
void check(bool condition, const std::string& description)
{
    ++checks; if (!condition) throw std::runtime_error(description);
}
std::vector<std::string> placeholders(const std::string& text)
{
    static const std::regex pattern("%[1-9][0-9]*|%n");
    std::vector<std::string> out;
    for (std::sregex_iterator it(text.begin(), text.end(), pattern), end; it != end; ++it) out.push_back(it->str());
    std::sort(out.begin(), out.end()); return out;
}
void checkTranslations(const std::string& directory)
{
    const auto manifest = Json::parse(Embedded::manifest);
    check(manifest.is_array() && manifest.size() >= 106, "Complete translation manifest");
    std::set<std::string> keys;
    for (const auto& message : manifest) keys.insert(text(message, "context") + '\4' + text(message, "source"));
    for (const auto& language : Translations::languages()) {
        const auto& catalogue = field(Translations::catalogues(), language);
        check(catalogue.is_object(), "Embedded catalogue: " + language);
        std::set<std::string> actual;
        for (auto context = catalogue.begin(); context != catalogue.end(); ++context) {
            check(context.value().is_object(), "Catalogue context is an object");
            for (auto entry = context.value().begin(); entry != context.value().end(); ++entry) actual.insert(context.key() + '\4' + entry.key());
        }
        check(actual == keys, "Catalogue and manifest contain the same messages: " + language);
        for (const auto& message : manifest) {
            const auto source = text(message, "source");
            const auto translated = text(field(catalogue, text(message, "context")), source);
            check(!translated.empty() && placeholders(source) == placeholders(translated), language + ": missing/invalid translation: " + source);
            check(source.empty() || source.front() != ' ' || translated.front() == ' ', "Leading spacing retained");
            check(source.empty() || source.back() != ' ' || translated.back() == ' ', "Trailing spacing retained");
        }
        if (!directory.empty()) {
            std::string bytes;
            check(Files::read(std::filesystem::u8path(directory) / (language + ".json"), bytes, 65536), "Translation source exists");
            check(Json::parse(bytes) == catalogue, "Source catalogue matches embedded data");
        }
    }
    check(Translations::resolveLanguage("system", "no_NO") == "nb", "Norwegian alias");
    check(Translations::resolveLanguage("system", "de_CH") == "de", "Regional fallback");
    check(Translations::resolveLanguage("pt-BR", "nl_NL") == "en", "Unknown language fallback");
    check(Translations::resolveLanguage("sv", "en_US") == "sv", "Explicit language");
    check(format("%1 + %2 = %3 / %10", {"%2", "b", "c"}) == "%2 + b = c / %10", "Placeholders do not recursively interpolate arguments");
}
struct Fixture
{
    std::filesystem::path root = std::filesystem::temp_directory_path() / ("MouseLite-contract-" + newId());
    Fixture() { std::filesystem::create_directories(root); }
    ~Fixture() {
        std::error_code ec;
        const auto resolved = std::filesystem::weakly_canonical(root, ec);
        const auto parent = std::filesystem::weakly_canonical(std::filesystem::temp_directory_path(), ec);
        if (!ec && resolved.parent_path() == parent && resolved.filename().u8string().rfind("MouseLite-contract-", 0) == 0)
            std::filesystem::remove_all(resolved, ec);
    }
};
}

int selfTest(const std::string& catalogueDirectory)
{
    try {
        checkTranslations(catalogueDirectory);
        const auto bundled = DeviceDatabase::bundled();
        const auto bytes = bundled.dump();
        for (const auto& model : bundled)
            check(DeviceDatabase::validate(Json::array({model}).dump()), "Validate bundled model: " + text(model, "name"));
        check(DeviceDatabase::validate(bytes), "Validate full bundled database");
        check(!DeviceDatabase::validate("[]") && !DeviceDatabase::validate("{broken"), "Reject empty/malformed database");
        check(!DeviceDatabase::validate(std::string(DeviceDatabase::maximumBytes + 1, ' ')), "Enforce database size limit");
        auto duplicate = bundled; duplicate.push_back(bundled.front());
        check(!DeviceDatabase::validate(duplicate.dump()), "Reject duplicate PIDs");
        auto invalid = bundled.front(); invalid["dpiTx"] = 256;
        check(!DeviceDatabase::validate(Json::array({invalid}).dump()), "Reject invalid protocol bytes");
        invalid = bundled.front(); invalid["newProtocol"] = true;
        check(!DeviceDatabase::validate(Json::array({invalid}).dump()), "Reject unknown model format");
        invalid = bundled.front();
        invalid["zones"] = Json::array({{{"name", "logo"},
            {"color", Json::array({{{"op", "build"}, {"fn", "unknown"}, {"args", Json::array()}}})},
            {"off", Json::array()}, {"brightness", Json::array()}}});
        check(!DeviceDatabase::validate(Json::array({invalid}).dump()), "Reject unknown protocol function");

        Fixture fixture;
        const auto databasePath = fixture.root / "definitions.json";
        std::string error, saved;
        check(DeviceDatabase::install(bytes, databasePath, &error), "Atomic database install");
        check(Files::read(databasePath, saved, DeviceDatabase::maximumBytes + 1024), "Read installed database");
        check(!DeviceDatabase::install("[broken", databasePath, &error), "Reject invalid database update");
        std::string after; check(Files::read(databasePath, after, DeviceDatabase::maximumBytes + 1024) && after == saved, "Rejected update preserves previous bytes");
        check(DeviceDatabase::load(databasePath) && DeviceDatabase::models() == bundled, "Reload database cache");
        auto expanded = bundled; auto synthetic = bundled.front(); synthetic["pid"] = 65534; synthetic["name"] = "Synthetic database test device"; expanded.push_back(synthetic);
        check(DeviceDatabase::install(expanded.dump(), databasePath, &error) && DeviceDatabase::models() == expanded, "New definitions become available");
        check(DeviceDatabase::load(databasePath) && DeviceDatabase::models() == expanded, "New definitions survive reload");
        check(DeviceDatabase::install(bytes, databasePath, &error), "Restore bundled test definitions");
        check(!DeviceDatabase::install(bytes, fixture.root, &error), "Reject database installation to directory");
        auto wrongVersion = Json::parse(saved); wrongVersion["appVersion"] = "incompatible";
        check(Files::atomicWrite(databasePath, wrongVersion.dump()), "Write incompatible cache fixture");
        check(!DeviceDatabase::load(databasePath), "Reject cache for different engine version");

        check(HidDescriptor::featureReportLength(fromHex("7508955ab102")) == 91, "Unnumbered HID feature report");
        check(HidDescriptor::featureReportLength(fromHex("7508952db102b102")) == 91, "Accumulated HID feature fields");
        check(HidDescriptor::featureReportLength(fromHex("7508955aa485019501b102b4b102")) == 91, "HID push/pop");
        check(HidDescriptor::featureReportLength(fromHex("85017508955ab102")) == 0, "Reject numbered HID report");
        check(HidDescriptor::featureReportLength(fromHex("750895")) == 0, "Reject truncated descriptor");
        check(HidDescriptor::featureReportLength(fromHex("77ffffffff97ffffffffb102")) == 0, "Reject descriptor integer overflow");
        check(desktopAutostartEntry("/home/user/My Mouse/50%/mouse-lite").find("Exec=\"/home/user/My Mouse/50%%/mouse-lite\" --tray\n") != std::string::npos, "XDG autostart quoting");
        check(desktopAutostartEntry("/tmp/a\nb").find("a\\nb") != std::string::npos, "XDG newline escaping");
        check(!Profile::fromJson({{"windowsSettings", false}}).pointerSettings, "Legacy profile migration");
        check(Profile::fromJson({{"pointerSettings", true}, {"windowsSettings", false}}).pointerSettings, "Portable key takes precedence");

        const auto models = RazerProtocol::models(); Json cobra; int commands = 0;
        check(models.size() >= 100, "Complete device database");
        for (const auto& model : models) {
            if (number(model, "pid") == 0x00a3) cobra = model;
            check(RazerProtocol::dpiReport(model, 800).size() == 90, "DPI packet length");
            for (const auto& zone : array(model, "zones")) for (const auto* effect : {"color", "off", "brightness"}) {
                const auto& steps = array(zone, effect);
                const auto packets = RazerProtocol::lightingReports(steps, Color::fromHex("#123456"), 100);
                if (steps.empty()) continue;
                check(!packets.empty(), "Nonempty lighting recipe creates packets");
                for (const auto& packet : packets) {
                    auto response = packet; response[0] = 2;
                    check(RazerProtocol::validResponse(packet, response), "Valid packet CRC");
                    response[10] ^= 1; check(!RazerProtocol::validResponse(packet, response), "Reject corrupt CRC"); ++commands;
                }
            }
        }
        check(!cobra.is_null() && number(cobra, "maxDpi") == 8500, "Cobra DPI capability");
        const auto dpi = RazerProtocol::dpiReport(cobra, 1850);
        check(hexBytes(Bytes(dpi.begin(), dpi.begin() + 13)) == "00ff00000007040501073a073a", "Cobra DPI golden packet");
        const auto zone = array(cobra, "zones").front();
        const auto rgb = RazerProtocol::lightingReports(array(zone, "color"), Color::fromHex("#123456"), 100).front();
        check(hexBytes(Bytes(rgb.begin(), rgb.begin() + 17)) == "001f000000090f02010401000001123456", "Cobra static color golden packet");
        const auto brightness = RazerProtocol::lightingReports(array(zone, "brightness"), Color{}, 100).front();
        check(hexBytes(Bytes(brightness.begin() + 5, brightness.begin() + 11)) == "030f040104ff", "Cobra brightness golden packet");
        bool rejected = false; try { RazerProtocol::dpiReport(cobra, 8501); } catch (...) { rejected = true; }
        check(rejected, "Reject unsupported DPI");
        auto mismatched = dpi; mismatched[6] = 7; RazerProtocol::checksum(mismatched);
        check(!RazerProtocol::validResponse(dpi, mismatched), "Reject mismatched command");

        Profile original; original.id = "test"; original.name = "Precisie · UTF-8"; original.dpi = 1850; original.pollRate = 500; original.color = Color::fromHex("#123456");
        check(Profile::fromJson(original.toJson()).toJson() == original.toJson(), "Profile roundtrip");
        const auto bounded = Profile::fromJson({{"dpi", -500}, {"speed", 80}, {"brightness", 999}, {"color", "invalid"}});
        check(bounded.dpi == 100 && bounded.speed == 20 && bounded.brightness == 100 && bounded.color.hex() == "#55d6a0", "Profile value bounds");
        const auto path = fixture.root / std::filesystem::u8path("profielen met spaties-µ.json");
        ProfileStore store(path); store.profiles.push_back(original); store.activeId = original.id; store.restoreOnStart = true;
        check(store.save(), "Atomic profile save to Unicode filename");
        ProfileStore loaded(path);
        check(loaded.load() && loaded.profiles.size() == 1 && loaded.activeId == original.id && loaded.restoreOnStart, "Load saved profiles");
        check(loaded.load() && loaded.profiles.size() == 1, "Repeated load does not duplicate profiles");
        check(Files::atomicWrite(path, "{broken"), "Write corrupt fixture");
        ProfileStore corrupt(path);
        check(!corrupt.load() && !corrupt.save(), "Corrupt profile is never overwritten");
        std::string broken; check(Files::read(path, broken, 100) && broken == "{broken", "Corrupt bytes preserved");
        check(!MouseBackend::apply({}, original).success, "Missing device does not report successful application");
        check(!MouseBackend::readCobraStatus({"", "", {}, 1, ""}).success, "Cobra diagnostic rejects other models");
        Platform::DesktopIntegration isolated(true);
        check(isolated.valid() && isolated.primary() && !isolated.trayAvailable(), "Tests do not create tray or instance locks");
        const auto devices = MouseBackend::enumerate();
        std::cout << "PASS: " << checks << " checks, " << models.size() << " models, " << commands
            << " lighting packets, 8 JSON catalogues, profile migration, atomic persistence and Cobra golden packets.\n"
            << "Supported USB mice detected (read only): " << devices.size() << "\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n'; return 1; }
}
