#include "Maths/Maths.h"
#include <algorithm>
using namespace std;
using namespace Maths;


// ----- RGBA methods ----- //

RGBA::RGBA()                                                                   { r = 0;     g = 0;     b = 0;     a = 1;   }
RGBA::RGBA(const float& all)                                                   { r = all;   g = all;   b = all;   a = all; }
RGBA::RGBA(const float& _r, const float& _g, const float& _b, const float& _a) { r = _r;    g = _g;    b = _b;    a = _a;  }
RGBA::RGBA(const RGB& rgb, const float& _a)                                    { r = rgb.r; g = rgb.g; b = rgb.b; a = _a;  }
RGB    RGBA::toRGB() const { return RGB(*this); }
float* RGBA::ptr()       { return &r; }


// ----- RGB methods ----- //

RGB::RGB()                                                  { r = 0;      g = 0;      b = 0;      }
RGB::RGB(const float& all)                                  { r = all;    g = all;    b = all;    }
RGB::RGB(const float& _r, const float& _g, const float& _b) { r = _r;     g = _g;     b = _b;     }
RGB::RGB(const RGBA& rgba)                                  { r = rgba.r; g = rgba.g; b = rgba.b; }
RGBA   RGB::toRGBA() const { return RGBA(*this); }
float* RGB::ptr ()       { return &r; }


// ----- HSVA methods ----- //

HSVA::HSVA()                                                                   { h = 0;     s = 0;     v = 0;     a = 0;   }
HSVA::HSVA(const float& all)                                                   { h = all;   s = all;   v = all;   a = all; }
HSVA::HSVA(const float& _h, const float& _s, const float& _v, const float& _a) { h = _h;    s = _s;    v = _v;    a = _a;  }
HSVA::HSVA(const HSV& hsv, const float& _a)                                    { h = hsv.h; s = hsv.s; v = hsv.v; a = _a;  }
HSV HSVA::toHSV() const { return HSV(*this); }
float* HSVA::ptr() { return &h; }


// ----- HSV methods ----- //

HSV::HSV()                                                  { h = 0;      s = 0;      v = 0;      }
HSV::HSV(const float& all)                                  { h = all;    s = all;    v = all;    }
HSV::HSV(const float& _h, const float& _s, const float& _v) { h = _h;     s = _s;     v = _v;     }
HSV::HSV(const HSVA& hsva)                                  { h = hsva.h; s = hsva.s; v = hsva.v; }
HSVA HSV::toHSVA() const { return HSVA(*this); }
float* HSV::ptr() { return &h; }


// ----- Color arithmetics ----- //

// Returns the hue of an RGB color (0 <= rgba <= 1) (output in radians).
float Maths::ColorGetHue(const RGBA& rgb)
{
    const float r = rgb.r, g = rgb.g, b = rgb.b;

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
        hue += 2 * PI;
    else if (hue > 2 * PI)
        hue -= 2 * PI;

    return hue;
}

// Linear interpolation between two given colors.
RGBA Maths::ColorLerp(const float& val, const RGBA& start, const RGBA& end)
{
    return RGBA(start.r + val * (end.r - start.r),
                start.g + val * (end.g - start.g),
                start.b + val * (end.b - start.b),
                start.a + val * (end.a - start.a));
}

// Blend between two HSV colors.
HSV Maths::BlendHSV(const HSV& col0, const HSV& col1)
{
    const Vector2 totalVec = Vector2(col0.h, 1, true) + Vector2(col1.h, 1, true);
    const float avgHue = totalVec.GetAngle();
    const float avgSat = (col0.s + col1.s) / 2;
    const float avgVal = (col0.v + col1.v) / 2;
    return HSV{ avgHue, avgSat, avgVal };
}

// Convert an RGB color (0 <= rgba <= 1) to HSV.
HSVA Maths::RGBAtoHSVA(const RGBA& rgba)
{
    HSVA hsva = {};
    hsva.a = rgba.a;

    const float minV = min(min(rgba.r, rgba.g), rgba.b);
    const float maxV = max(max(rgba.r, rgba.g), rgba.b);
    const float diff = maxV - minV;

    const float r = rgba.r;
    const float g = rgba.g;
    const float b = rgba.b;

    // Set Value.
    hsva.v = maxV;

    // If max and min are the same, return.
    if (diff < 0.00001f)
        return { 0, 0, hsva.v, hsva.a };

    // Set Saturation.
    if (maxV > 0)
        hsva.s = diff / maxV;
    else
        return { 0, 0, hsva.v, hsva.a };

    // Set Hue.
    if (r >= maxV)
        hsva.h = (g - b) / diff;
    else if (g >= maxV)
        hsva.h = 2.0f + (b - r) / diff;
    else if (b >= maxV)
        hsva.h = 4.0f + (r - g) / diff;

    // Keep Hue above 0.
    if (hsva.h < 0)
        hsva.h += 2 * PI;

    return hsva;
}

// Convert an HSV color to RGB.
RGBA Maths::HSVAtoRGBA(const HSVA& hsva)
{
    RGBA rgba = { 0, 0, 0, hsva.a };

    // Red channel
    float k = fmodf((5.0f + hsva.h), 6);
    float t = 4.0f - k;
    k = (t < k) ? t : k;
    k = (k < 1) ? k : 1;
    k = (k > 0) ? k : 0;
    rgba.r = hsva.v - hsva.v * hsva.s * k;

    // Green channel
    k = fmodf((3.0f + hsva.h), 6);
    t = 4.0f - k;
    k = (t < k) ? t : k;
    k = (k < 1) ? k : 1;
    k = (k > 0) ? k : 0;
    rgba.g = hsva.v - hsva.v * hsva.s * k;

    // Blue channel
    k = fmodf((1.0f + hsva.h), 6);
    t = 4.0f - k;
    k = (t < k) ? t : k;
    k = (k < 1) ? k : 1;
    k = (k > 0) ? k : 0;
    rgba.b = hsva.v - hsva.v * hsva.s * k;

    return rgba;
}

// Shifts the hue of the given color.
RGBA Maths::ColorShift(const RGBA& color, const float& hue)
{
    HSV hsv = RGBAtoHSVA(color);
    
    hsv.h += hue;
    if (hsv.h >= 2 * PI)
        hsv.h -= 2 * PI;
    else if (hsv.h < 0)
        hsv.h += 2 * PI;
    
    return HSVAtoRGBA(hsv);
}
