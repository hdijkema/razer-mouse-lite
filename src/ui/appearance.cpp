#include "appearance.h"
#include "files.h"
#include "preferences.h"
#include "embedded.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <set>

namespace {
bool validName(const std::string& name)
{
    return !name.empty() && name.size() <= 48 && name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") == std::string::npos;
}
std::vector<std::filesystem::path> skinDirectories(bool preview)
{
    std::vector<std::filesystem::path> result;
    if (!preview) result.push_back(Files::configDirectory() / "skins");
    result.push_back(Files::executable().parent_path() / "skins");
    result.push_back(Files::executable().parent_path() / "../share/razer-mouse-lite/skins");
    return result;
}
}
Appearance::Appearance(FLTK_Css& stylesheet, bool sample) : css(stylesheet), preview(sample)
{
    css.parse(Embedded::skin("mint", true));
    if (!preview) {
        Json settings;
        if (Preferences::load(settings)) {
            const auto appearance = field(settings, "appearance");
            mode_ = text(appearance, "mode", "system"); skin_ = text(appearance, "skin", "mint");
        }
    } else mode_ = "dark";
    if (mode_ != "light" && mode_ != "dark" && mode_ != "system") mode_ = "system";
    if (!validName(skin_)) skin_ = "mint";
}
std::vector<std::string> Appearance::skins() const
{
    std::set<std::string> values{"mint", "graphite", "sand"};
    for (const auto& directory : skinDirectories(preview)) {
        std::error_code ec;
        std::filesystem::directory_iterator entries(directory, ec), end;
        for (; !ec && entries != end && values.size() < 64; entries.increment(ec)) {
            const auto name = entries->path().filename().u8string();
            if (validName(name) && std::filesystem::is_regular_file(entries->path() / "light.css", ec)
                && std::filesystem::is_regular_file(entries->path() / "dark.css", ec)) values.insert(name);
        }
    }
    return {values.begin(), values.end()};
}
bool Appearance::apply(const std::string& mode, const std::string& skin, bool systemDark)
{
    error_.clear();
    const bool dark = mode == "dark" || (mode == "system" && systemDark);
    std::string source;
    for (const auto& directory : skinDirectories(preview)) {
        const auto file = directory / skin / (dark ? "dark.css" : "light.css");
        std::error_code ec;
        if (std::filesystem::exists(file, ec)) {
            if (!Files::read(file, source, 256 * 1024, &error_)) return false;
            break;
        }
    }
    if (source.empty()) source = Embedded::skin(skin, dark);
    if (source.empty()) { error_ = "Skin not found: " + skin; return false; }
    if (!css.parse(source)) { error_ = css.errors().front(); return false; }
    mode_ = mode; skin_ = skin; dark_ = dark; systemDark_ = systemDark;
    FLTK_Css_Node root; root.type = "window";
    const auto style = css.style(root);
    auto set = [](Fl_Color target, Fl_Color value) { uchar r, g, b; Fl::get_color(value, r, g, b); Fl::set_color(target, r, g, b); };
    uchar r, g, b;
    Fl::get_color(style.background, r, g, b); Fl::background(r, g, b);
    FLTK_Css_Node input; input.type = "input"; input.parent = &root;
    Fl::get_color(css.style(input).background, r, g, b); Fl::background2(r, g, b);
    Fl::get_color(style.color, r, g, b); Fl::foreground(r, g, b);
    set(FL_SELECTION_COLOR, css.style(input).selection_color);
    set(FL_INACTIVE_COLOR, fl_color_average(style.color, style.background, 0.45f));
    return true;
}
bool Appearance::choose(const std::string& mode, const std::string& skin, bool systemDark)
{
    if ((mode != "light" && mode != "dark" && mode != "system") || !validName(skin)) return false;
    if (!apply(mode, skin, systemDark)) return false;
    if (!preview && !Preferences::set("appearance", Json{{"mode", mode}, {"skin", skin}}, &error_)) return false;
    return true;
}
bool Appearance::systemChanged(bool dark)
{
    systemDark_ = dark;
    return mode_ != "system" || dark == dark_ ? true : apply(mode_, skin_, dark);
}
bool Appearance::reload() { return apply(mode_, skin_, systemDark_); }
