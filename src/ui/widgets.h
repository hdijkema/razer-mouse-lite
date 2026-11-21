#pragma once
#include <fltk_css_widgets.h>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Slider.H>

class CssScroll : public Fl_Css_Widget<Fl_Scroll>
{
public:
    CssScroll(FLTK_Css& css, int x, int y, int w, int h) : Fl_Css_Widget<Fl_Scroll>(css, x, y, w, h)
    { box(fltk_css_detail::box_type(false)); }
};

class CssChoice : public Fl_Css_Widget<Fl_Choice>
{
public:
    CssChoice(FLTK_Css& css, int x, int y, int w, int h);
protected:
    void draw() override;
};
class CssColorButton : public Fl_Css_Button
{
public:
    using Fl_Css_Button::Fl_Css_Button;
    void swatch(Fl_Color color) { swatch_ = color; redraw(); }
protected:
    void draw() override;
private:
    Fl_Color swatch_ = FL_GREEN;
};
class CssCheck : public Fl_Css_Button
{
public:
    CssCheck(FLTK_Css& css, int x, int y, int w, int h, const char* label);
protected:
    void draw() override;
};
class CssSlider : public Fl_Css_Widget<Fl_Slider>
{
public:
    CssSlider(FLTK_Css& css, int x, int y, int w, int h);
protected:
    void draw() override;
};
