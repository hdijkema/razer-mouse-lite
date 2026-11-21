#include "widgets.h"
#include <FL/fl_draw.H>
#include <algorithm>
#include <cmath>

CssChoice::CssChoice(FLTK_Css& css, int x, int y, int w, int h)
    : Fl_Css_Widget<Fl_Choice>(css, x, y, w, h) { css_type("choice"); }
void CssChoice::draw()
{
    const auto style = css_style();
    fltk_css_detail::apply(*this, style);
    textfont(labelfont()); textsize(style.font_size); textcolor(style.color);
    fltk_css_detail::Paint_Scope scope(style);
    draw_box();
    fl_color(style.color);
    fl_font(labelfont(), style.font_size);
    if (mvalue()) fl_draw(mvalue()->label(), x() + 10, y(), w() - 38, h(), FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP, nullptr, 0);
    const int middle = y() + h() / 2;
    fl_polygon(x() + w() - 23, middle - 3, x() + w() - 13, middle - 3, x() + w() - 18, middle + 3);
}
CssCheck::CssCheck(FLTK_Css& css, int x, int y, int w, int h, const char* label)
    : Fl_Css_Button(css, x, y, w, h, label)
{
    css_type("check"); type(FL_TOGGLE_BUTTON); align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
}
void CssCheck::draw()
{
    const auto style = css_style();
    fltk_css_detail::apply(*this, style);
    fltk_css_detail::Paint_Scope scope(style);
    draw_box();
    const int top = y() + (h() - 17) / 2;
    fl_color(style.border_color); fl_rounded_rectf(x(), top, 18, 18, 4);
    fl_color(value() ? style.selection_color : style.background);
    fl_rounded_rectf(x() + 1, top + 1, 16, 16, 3);
    if (value()) {
        fl_color(fl_contrast(FL_WHITE, style.selection_color)); fl_line_style(FL_SOLID, 2);
        fl_line(x() + 4, top + 9, x() + 8, top + 13, x() + 14, top + 5); fl_line_style(0);
    }
    draw_label(x() + 27, y(), w() - 27, h());
    if (Fl::focus() == this) draw_focus(FL_FLAT_BOX, x() + 24, y(), w() - 24, h());
}
void CssColorButton::draw()
{
    Fl_Css_Button::draw(); fl_color(swatch_);
    fl_rounded_rectf(x() + 10, y() + h() / 2 - 7, 14, 14, 3);
}
CssSlider::CssSlider(FLTK_Css& css, int x, int y, int w, int h)
    : Fl_Css_Widget<Fl_Slider>(css, x, y, w, h)
{
    css_type("slider"); type(FL_HOR_SLIDER); slider_size(0.045f); step(1); when(FL_WHEN_CHANGED);
}
void CssSlider::draw()
{
    const auto style = css_style(); fltk_css_detail::apply(*this, style);
    fltk_css_detail::Paint_Scope scope(style); draw_box();
    const int left = x() + Fl::box_dx(box()), top = y() + Fl::box_dy(box());
    const int width = std::max(1, w() - Fl::box_dw(box())), height = std::max(1, h() - Fl::box_dh(box()));
    const int knob = std::max(static_cast<int>(slider_size() * width + 0.5), height / 2 + 1);
    const double fraction = minimum() == maximum() ? 0.5 : std::clamp((value() - minimum()) / (maximum() - minimum()), 0.0, 1.0);
    const int center = left + static_cast<int>(fraction * (width - knob) + 0.5) + knob / 2;
    const int middle = top + height / 2;
    fl_color(style.border_color); fl_rounded_rectf(left, middle - 2, width, 5, 2);
    fl_color(style.color); fl_rounded_rectf(left, middle - 2, std::max(1, center - left), 5, 2);
    fl_color(active_r() ? style.color : style.border_color); fl_pie(center - 7, middle - 7, 15, 15, 0, 360);
    if (Fl::focus() == this) draw_focus();
}
