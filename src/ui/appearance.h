#pragma once
#include <fltk_css.h>
#include <string>
#include <vector>

class Appearance
{
public:
    Appearance(FLTK_Css& css, bool preview);
    std::vector<std::string> skins() const;
    bool choose(const std::string& mode, const std::string& skin, bool systemDark);
    bool systemChanged(bool dark);
    bool reload();
    std::string mode() const { return mode_; }
    std::string skin() const { return skin_; }
    bool dark() const { return dark_; }
    bool systemDark() const { return systemDark_; }
    const std::string& error() const { return error_; }
private:
    bool apply(const std::string& mode, const std::string& skin, bool systemDark);
    FLTK_Css& css;
    bool preview;
    std::string mode_ = "system", skin_ = "mint", error_;
    bool dark_ = true, systemDark_ = true;
};
