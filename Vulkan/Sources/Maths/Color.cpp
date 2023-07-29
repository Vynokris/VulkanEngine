#include "Maths/Maths.h"
#include <algorithm>
#include <functional>
using namespace std;
using namespace Maths;


// ----- RGBA methods ----- //

RGBA::RGBA()                                                                   : r(0    ), g(0    ), b(0    ), a(1)   {}
RGBA::RGBA(const float& all)                                                   : r(all  ), g(all  ), b(all  ), a(all) {}
RGBA::RGBA(const float& _r, const float& _g, const float& _b, const float& _a) : r(_r   ), g(_g   ), b(_b   ), a(_a)  {}
RGBA::RGBA(const RGB& rgb, const float& _a)                                    : r(rgb.r), g(rgb.g), b(rgb.b), a(_a)  {}
RGBA::RGBA(const HSVA& hsva)                                                   : r(0), g(0), b(0), a(hsva.a)
{
    std::function compute = [](const HSV& hsv, const float & cst)
    {
        float k = fmodf((cst + hsv.h), 6);
        float t = 4.0f - k;
        k = (t < k) ? t : k;
        k = (k < 1) ? k : 1;
        k = (k > 0) ? k : 0;
        return hsv.v - hsv.v * hsv.s * k;
    };

    r = compute(hsva, 5);
    g = compute(hsva, 3);
    b = compute(hsva, 1);
}
float  RGBA::GetHue() const 
{
    const float minV = min(min(r, g), b);
    const float maxV = max(max(r, g), b);
    const float diff = maxV - minV;
    float hue;

    // If red is the maximum value.
    if (r > g && r > b)
        hue = (g - b) / diff;

    // If green is the maximum value.
    else if (g > r && g > b)
        hue = 2 + (b - r) / diff;

    // If blue is the maximum value.
    else
        hue = 4 + (r - g) / diff;

    // Keep the hue between 0 and 2pi;
    if (hue < 0)
        hue += TWOPI;
    else if (hue > TWOPI)
        hue -= TWOPI;

    return hue;
}
void RGBA::ShiftHue(const float& hue)
{
    *this = GetShiftedHue(hue);
}
RGBA RGBA::GetShiftedHue(const float& hue) const
{
    HSV hsv = HSVA(*this).ToHSV();
    
    hsv.h += hue;
    if (hsv.h >= TWOPI)
        hsv.h -= TWOPI;
    else if (hsv.h < 0)
        hsv.h += TWOPI;
    
    return RGBA(HSVA(hsv, a));
}
RGB    RGBA::ToRGB() const { return RGB(*this); }
float* RGBA::AsPtr()       { return &r; }


// ----- RGB methods ----- //

RGB::RGB()                                                  { r = 0;      g = 0;      b = 0;      }
RGB::RGB(const float& all)                                  { r = all;    g = all;    b = all;    }
RGB::RGB(const float& _r, const float& _g, const float& _b) { r = _r;     g = _g;     b = _b;     }
RGB::RGB(const RGBA& rgba)                                  { r = rgba.r; g = rgba.g; b = rgba.b; }
RGBA   RGB::ToRGBA(const float& a) const { return RGBA(*this, a); }
float* RGB::AsPtr ()                     { return &r; }


// ----- HSVA methods ----- //

HSVA::HSVA()                                                                   : h(0    ), s(0    ), v(0    ), a(0  ) {}
HSVA::HSVA(const float& all)                                                   : h(all  ), s(all  ), v(all  ), a(all) {}
HSVA::HSVA(const float& _h, const float& _s, const float& _v, const float& _a) : h(_h   ), s(_s   ), v(_v   ), a(_a ) {}
HSVA::HSVA(const HSV& hsv, const float& _a)                                    : h(hsv.h), s(hsv.s), v(hsv.v), a(_a ) {}
HSVA::HSVA(const RGBA& rgba)                                                   : h(0    ), s(0    ), v(0    ), a(rgba.a)
{
    const float minV = min(min(rgba.r, rgba.g), rgba.b);
    const float maxV = max(max(rgba.r, rgba.g), rgba.b);
    const float diff = maxV - minV;

    const float r = rgba.r;
    const float g = rgba.g;
    const float b = rgba.b;

    // Set Value.
    v = maxV;

    // If max and min are the same, return.
    if (diff < 0.00001f) {
        *this = { 0, 0, v, a };
        return;
    }

    // Set Saturation.
    if (maxV > 0) {
        s = diff / maxV;
    }
    else {
        *this = { 0, 0, v, a };
        return;
    }

    // Set Hue.
    if      (r >= maxV) h =        (g - b) / diff;
    else if (g >= maxV) h = 2.0f + (b - r) / diff;
    else if (b >= maxV) h = 4.0f + (r - g) / diff;

    // Keep Hue above 0.
    if (h < 0) h += TWOPI;
}
HSV HSVA::ToHSV() const { return HSV(*this); }
float* HSVA::AsPtr()    { return &h; }


// ----- HSV methods ----- //

HSV::HSV()                                                  { h = 0;      s = 0;      v = 0;      }
HSV::HSV(const float& all)                                  { h = all;    s = all;    v = all;    }
HSV::HSV(const float& _h, const float& _s, const float& _v) { h = _h;     s = _s;     v = _v;     }
HSV::HSV(const HSVA& hsva)                                  { h = hsva.h; s = hsva.s; v = hsva.v; }
HSVA HSV::ToHSVA(const float& a) const { return HSVA(*this, a); }
float* HSV::AsPtr()                    { return &h; }
