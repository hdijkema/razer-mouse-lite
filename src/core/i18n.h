#pragma once
#include "common.h"
#include <initializer_list>
#include <utility>

class Translations
{
public:
    bool load(const std::string& overrideLanguage = {});
    static std::vector<std::string> languages();
    static std::string languageName(const std::string& language);
    static std::string selectedLanguage();
    static std::string resolveLanguage(const std::string& requested, const std::string& systemLocale);
    static bool saveLanguage(const std::string& language);
    static std::string translate(const std::string& context, const std::string& source);
    static const Json& catalogues();
};

std::string format(const std::string& source, const std::vector<std::string>& arguments);
inline std::string asText(const std::string& s) { return s; }
inline std::string asText(const char* s) { return s; }
inline std::string asText(int n) { return std::to_string(n); }
template<typename... Args>
std::string tr(const std::string& context, const std::string& source, Args&&... args)
{
    return format(Translations::translate(context, source), {asText(std::forward<Args>(args))...});
}
