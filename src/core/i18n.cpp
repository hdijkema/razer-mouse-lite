#include "i18n.h"
#include "files.h"
#include "preferences.h"
#include "embedded.h"
#include <cctype>

namespace { std::string currentLanguage = "en"; }
const Json& Translations::catalogues()
{
    static const Json data = Json::parse(Embedded::translations);
    return data;
}
std::vector<std::string> Translations::languages() { return {"en", "nl", "de", "da", "nb", "sv", "fr", "es"}; }
std::string Translations::languageName(const std::string& language)
{
    static const std::vector<std::string> names = {"English", "Nederlands", "Deutsch", "Dansk", "Norsk (bokmål)", "Svenska", "Français", "Español"};
    const auto all = languages();
    const auto at = std::find(all.begin(), all.end(), language);
    return at == all.end() ? "" : names[static_cast<std::size_t>(at - all.begin())];
}
std::string Translations::resolveLanguage(const std::string& requested, const std::string& locale)
{
    std::string value = requested.empty() || requested == "system" ? locale : requested;
    value = value.substr(0, value.find_first_of("_- ."));
    for (auto& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (value == "no") value = "nb";
    const auto all = languages();
    return std::find(all.begin(), all.end(), value) == all.end() ? "en" : value;
}
std::string Translations::selectedLanguage()
{
    std::string data;
    if (Files::read(Files::configDirectory() / "settings.json", data, 65536)) {
        const auto settings = Json::parse(data, nullptr, false);
        return text(settings, "language", "system");
    }
    const auto legacy = Files::legacyLanguage();
    return legacy.empty() ? "system" : legacy;
}
bool Translations::saveLanguage(const std::string& language)
{
    const auto all = languages();
    if (language != "system" && std::find(all.begin(), all.end(), language) == all.end()) return false;
    return Preferences::set("language", language);
}
bool Translations::load(const std::string& overrideLanguage)
{
    currentLanguage = resolveLanguage(overrideLanguage.empty() ? selectedLanguage() : overrideLanguage, Files::systemLocale());
    return field(catalogues(), currentLanguage).is_object();
}
std::string Translations::translate(const std::string& context, const std::string& source)
{
    const auto& language = field(catalogues(), currentLanguage);
    return text(field(language, context), source, source);
}
std::string format(const std::string& source, const std::vector<std::string>& arguments)
{
    std::string out;
    for (std::size_t i = 0; i < source.size();) {
        if (source[i] != '%' || i + 1 == source.size() || source[i + 1] < '1' || source[i + 1] > '9') { out += source[i++]; continue; }
        std::size_t end = i + 1, number = 0;
        while (end < source.size() && std::isdigit(static_cast<unsigned char>(source[end]))) {
            number = std::min<std::size_t>(10000, number * 10 + static_cast<unsigned>(source[end++] - '0'));
        }
        out += number <= arguments.size() ? arguments[number - 1] : source.substr(i, end - i);
        i = end;
    }
    return out;
}
